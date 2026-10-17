// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#include "HQRenderer.h"
#include "Model/Stages.h"
#include <juce_core/juce_core.h>
#include <map>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
#if BLUECHANNEL_HQ
#include <ngspice/sharedspice.h>
#include "Circuit/HQAudioCircuit.h"
#if JUCE_MAC || JUCE_LINUX
#include <dlfcn.h>
#endif
#endif

namespace bluechannel::hq
{
namespace
{
std::string number(double v) { std::ostringstream s; s << std::setprecision(17) << v; return s.str(); }
}
struct Renderer::Impl
{
    double fs = 48000, baseRate = 48000;
    int channels = 2, blockSize = 512;
    bool needsRestart = true, failed = false;
    std::string failure;
#if BLUECHANNEL_HQ
    juce::DynamicLibrary library;
    juce::File privateLibrary;
    decltype(&ngSpice_Init) init = nullptr;
    decltype(&ngSpice_Init_Sync) initSync = nullptr;
    decltype(&ngSpice_Command) command = nullptr;
    decltype(&ngSpice_Circ) circuitLoad = nullptr;
    decltype(&ngSpice_SetBkpt) breakpoint = nullptr;
    std::map<std::string, double> values, initialValues, fromValues;
    double controlsTime=0;
    static bool continuous(const std::string& name)
    {
        static const std::string names=" insttrim trim lpr hpr lfpot hfpot lmfgain lmfq lmffreq hmfgain hmfq hmffreq makeup mix deessfreq fader zscale ";
        return names.find(" "+name+" ")!=std::string::npos;
    }
    double controlValue(const std::string& name, double t) const
    {
        const double target=values.at(name);
        return continuous(name) ? target+(fromValues.at(name)-target)*std::exp(-std::max(0.0,t-controlsTime)/.02) : target;
    }
    model::Controls ctl;
    uint64_t position = 0;
    int frameCount = 0, emitted = 0, oversampling = 0;
    double endTime = 0, previousTime = 0;
    bool initialised = false, running = false;
    double* const* audio = nullptr;
    const double* const* keyAudio = nullptr;
    int keyCount = 0;
    model::Telemetry* telemetry = nullptr;
    double inputSquares[2] {};
    std::array<std::vector<double>, 2> input, keys, solved, baseInput, baseKey;
    dsp::Oversampler resampler;
    std::array<double, 2> lastInput {}, lastKey {};
    double lastPhysicalInput[2][3] {};
    // out+, out-, input tap, comp key, gate key, de-ess band, pre-limiter, optical state.
    static constexpr int signals = 9;
    int indices[2][signals] {};
    int timeIndex = -1;
    std::array<std::array<double, signals>, 2> previous {};
    std::array<double, 2> compCurrent {}, deessCurrent {}, limitCurrent {}, optoGain {1,1};
    std::array<dynamics::CompressorDetector, 2> comp;
    std::array<dynamics::GateDetector, 2> gate;
    std::array<dynamics::OptoCompressor, 2> opto;
    std::array<dynamics::DeessDetector, 2> deess;
    std::array<dynamics::LimiterDetector, 2> limiter;
    // Only the inverse LED calibration is used; no reduced audio stage is processed in HQ.
    model::DeesserStage deessCalibration;

    ~Impl()
    {
        if (command) send("quit");
        library.close();
        if (privateLibrary.getFullPathName().isNotEmpty()) privateLibrary.deleteFile();
    }
    void send(const std::string& s) { auto copy = s; command(copy.data()); }
    static int log(char* text, int, void* user)
    {
        auto& s = *static_cast<Impl*>(user);
        if (std::getenv("BLUECHANNEL_HQ_TRACE")) std::fprintf(stderr,"%s\n",text);
        std::string line(text);
        if (line.find("Error:") != std::string::npos || line.find("aborted") != std::string::npos
            || line.find("Timestep too small") != std::string::npos)
        { if (!s.failed) s.failure = line; s.failed = true; }
        return 0;
    }
    static int exit(int, bool, bool quit, int, void* user)
    {
        if (!quit) { auto& s = *static_cast<Impl*>(user); s.failed = true; s.failure = "Component solver stopped unexpectedly"; }
        return 0;
    }
    static int initData(pvecinfoall data, int, void* user)
    {
        auto& s = *static_cast<Impl*>(user);
        static constexpr const char* names[] = {"outp", "outn", "inputtap", "compkey", "gatekey",
                                              "xdeess.filter_out", "faded", "xlimit.xopt4.state", "xdeess.xopt2.state"};
        s.timeIndex = -1;
        for (auto& row : s.indices) std::fill(std::begin(row), std::end(row), -1);
        for (int i = 0; i < data->veccount; ++i)
        {
            std::string name = juce::String(data->vecs[i]->vecname).toLowerCase().toStdString();
            if (name == "time") s.timeIndex = i;
            for (int ch = 0; ch < s.channels; ++ch)
                for (int j = 0; j < signals; ++j)
                    if (name == "x" + std::to_string(ch) + "." + names[j]) s.indices[ch][j] = i;
        }
        if (s.timeIndex < 0) { s.failed = true; s.failure = "Component solver has no time vector"; }
        for (int ch = 0; ch < s.channels; ++ch)
            for (int j = 0; j < signals; ++j)
                if (s.indices[ch][j] < 0) { s.failed = true; s.failure = "Missing component solver probe: " + std::string(names[j]); }
        return 0;
    }
    static int voltage(double* value, double t, char* source, int, void* user)
    {
        auto& s = *static_cast<Impl*>(user);
        const std::string name(source);
        if (name.rfind("vctrl_", 0) == 0)
        {
            auto found = s.values.find(name.substr(6));
            *value = found == s.values.end() ? 0.0 : s.controlValue(found->first,t)-s.initialValues.at(found->first);
            return 0;
        }
        int ch = name.find("x1.") != std::string::npos ? 1 : 0;
        if (name.find("vopto") != std::string::npos) *value = s.optoGain[size_t(ch)];
        else if (s.frameCount <= 0) *value = 0;
        else
        {
            const bool key = name.find("vkey") != std::string::npos;
            const double x = std::clamp(t * s.fs - double(s.position), 0.0, double(s.frameCount));
            const int i = std::min(s.frameCount - 1, int(x));
            const auto& buffer = key ? s.keys[size_t(ch)] : s.input[size_t(ch)];
            const bool mic=name.find("vmic")!=std::string::npos;
            const bool inst=name.find("vinst")!=std::string::npos;
            const int role=mic ? 0 : inst ? 2 : 1;
            const bool active=key || int(s.ctl.source)==role;
            const double a = i == 0 ? (key ? s.lastKey[size_t(ch)] : s.lastPhysicalInput[ch][role]) : (active ? buffer[size_t(i-1)] : 0);
            const double b = active ? buffer[size_t(i)] : 0;
            *value = a + (b-a) * (x-double(i));
        }
        return 0;
    }
    static int current(double* value, double, char* source, int, void* user)
    {
        auto& s = *static_cast<Impl*>(user);
        std::string name(source);
        int ch = name.find("x1.") != std::string::npos ? 1 : 0;
        if (name.find("icomp") != std::string::npos) *value = s.compCurrent[size_t(ch)];
        else if (name.find("ideess") != std::string::npos) *value = s.deessCurrent[size_t(ch)];
        else *value = s.limitCurrent[size_t(ch)];
        return 0;
    }
    static int data(pvecvaluesall data, int, int, void* user)
    {
        auto& s = *static_cast<Impl*>(user);
        if (s.failed || s.timeIndex < 0) return 0;
        const double t = data->vecsa[s.timeIndex]->creal;
        std::array<std::array<double, signals>, 2> now {};
        for (int ch = 0; ch < s.channels; ++ch)
            for (int j = 0; j < signals; ++j)
            {
                const double v = data->vecsa[s.indices[ch][j]]->creal;
                if (!std::isfinite(v)) { s.failed = true; s.failure = "Non-finite component solution"; return 0; }
                now[size_t(ch)][size_t(j)] = v;
            }
        while (s.emitted < s.frameCount)
        {
            const double wanted = double(s.position + uint64_t(s.emitted) + 1) / s.fs;
            if (t < wanted - 1e-13) break;
            const double fraction = t > s.previousTime ? std::clamp((wanted-s.previousTime)/(t-s.previousTime), 0.0, 1.0) : 1;
            std::array<std::array<double, signals>, 2> sample {};
            for (int ch = 0; ch < s.channels; ++ch)
                for (int j = 0; j < signals; ++j)
                    sample[size_t(ch)][size_t(j)] = s.previous[size_t(ch)][size_t(j)] +
                        fraction*(now[size_t(ch)][size_t(j)]-s.previous[size_t(ch)][size_t(j)]);
            s.acceptSample(sample);
            ++s.emitted;
        }
        s.previous = now;
        s.previousTime = t;
        return 0;
    }
    void acceptSample(const std::array<std::array<double, signals>, 2>& sample)
    {
        double cl[2] {}, gl[2] {};
        for (int ch=0; ch<channels; ++ch)
        {
            cl[ch] = comp[size_t(ch)].level(sample[size_t(ch)][3]);
            gl[ch] = gate[size_t(ch)].level(sample[size_t(ch)][4]);
        }
        if (channels == 2 && ctl.link) { cl[0]=cl[1]=std::max(cl[0],cl[1]); gl[0]=gl[1]=std::max(gl[0],gl[1]); }
        for (int ch=0; ch<channels; ++ch)
        {
            const auto c = size_t(ch);
            double micro = 0, cgr = 0, ggr = 0;
            optoGain[c] = 1;
            if (!ctl.bypass)
            {
                if (ctl.compIn)
                {
                    if (ctl.compType == model::CompType::Vca)
                    { cgr = comp[c].gainReduction(cl[ch]); micro += model::vcaCurrentForReduction(cgr); }
                    else { optoGain[c] = opto[c].process(cl[ch]); cgr = opto[c].gainReductionDb(); }
                }
                if (ctl.gateIn) { ggr = gate[c].attenuation(gl[ch]); micro += model::vcaCurrentForReduction(ggr); }
            }
            compCurrent[c] = micro * 1e-6;
            deessCurrent[c] = 0;
            if (ctl.deessIn && !ctl.bypass)
            {
                const double gain = deess[c].gain(sample[c][5]);
                if (gain < 1) deessCurrent[c] = std::min(.02, deessCalibration.ledCurrentForResistance(deessCalibration.resistanceForGain(gain)));
            }
            limitCurrent[c] = 0;
            if (ctl.softLimit && !ctl.bypass)
            {
                const double gain = limiter[c].gain(sample[c][6]);
                if (gain < .9999)
                {
                    model::OpticalLaw law;
                    const double led = law.stateFor(model::LimiterStage::resistanceForGain(gain)) * 1e-3;
                    limitCurrent[c] = std::min(.02, led + 3*std::max(0.0,led-sample[c][7]*1e-3));
                }
            }
            const double y = (sample[c][0]-sample[c][1])/model::kFullScalePeak;
            audio[ch][emitted] = y;
            model::Telemetry::raise(telemetry->inputPeak[ch], float(std::abs(sample[c][2])*model::kInternalToConnector/model::kFullScalePeak));
            model::Telemetry::raise(telemetry->outputPeak[ch], float(std::abs(y)));
            model::Telemetry::raise(telemetry->compGr, float(cgr));
            model::Telemetry::raise(telemetry->gateGr, float(ggr));
            model::Telemetry::raise(telemetry->deessLed,float(std::max(0.0,sample[c][8])));
            model::OpticalLaw law;
            model::Telemetry::raise(telemetry->limiterGr,float(-20*std::log10(model::LimiterStage::bandGain(law.resistance(sample[c][7])))));
            const double metered=sample[c][2]*model::kInternalToConnector/model::kFullScalePeak;
            inputSquares[ch]+=metered*metered;
            if(std::abs(sample[c][2])>.5*model::kSwingPositive) telemetry->overload.store(true);
        }
    }
    void configure(const model::Controls& c)
    {
        using namespace model;
        using namespace model::controlmap;
        ctl = c;
        values = {{"source", double(c.source)}, {"micgain", double(micCircuitPosition(c.micGain))},
            {"linegain", double(c.lineGain)}, {"sourceohms", c.sourceOhms}, {"zscale", impedanceScale(c.impedance)},
            {"air", c.air}, {"insttrim", instrumentGangPosition(trimPot(c.trimDb))}, {"trim", trimPot(c.trimDb)},
            {"phase", c.phase}, {"pre", c.dynPosition == DynPosition::PreEq}, {"lpr", std::max(.001,lowPassOhms(c.lpfHz))},
            {"hpr", std::max(.001,highPassOhms(c.hpfHz))}, {"lfpot", lowShelfPot(c.lfDb)}, {"hfpot", highShelfPot(c.hfDb)},
            {"lfsel", lowShelfPosition(c.lfFreq,c.lfHigh)}, {"hfsel", highShelfPosition(c.hfFreq,c.hfHigh)},
            {"lmfgain", lmfGainPot(c.lmfDb)}, {"lmfq", lmfQPot(c.lmfQ)}, {"lmffreq", lmfFreqPot(c.lmfHz)}, {"lmfx3",c.lmfX3},
            {"hmfgain", hmfGainPot(c.hmfDb)}, {"hmfq", hmfQPot(c.hmfQ)}, {"hmffreq", hmfFreqPot(c.hmfHz)}, {"hmfx3",c.hmfX3},
            {"filterroute", double(c.filterRoute)}, {"shelfroute", double(c.shelfRoute)}, {"paramroute", double(c.paramRoute)},
            {"alleq",c.allEq}, {"bypass",c.bypass}, {"compkey",c.compExtKey && c.compType==CompType::Vca}, {"gatekey",c.gateExtKey},
            {"makeup",makeupPot(c.compIn ? c.makeupDb : 0)}, {"vcaactive",!c.bypass && (c.compIn||c.gateIn)},
            {"mix", c.compIn && c.blend ? c.mix : 1}, {"deessfreq",deessFreqPot(c.deessHz)}, {"deessin",c.deessIn && !c.bypass},
            {"complisten",c.compListen && !c.bypass}, {"gatelisten",c.gateListen && !c.bypass}, {"deesslisten",c.deessListen && !c.bypass},
            {"fader",faderTrack(c.outputDb)}, {"mute",c.mute}, {"limit",c.softLimit && !c.bypass}};
        const double offset = -20*std::log10(kInternalToConnector);
        deessCalibration.configure(c.deessHz);
        for (int ch=0; ch<channels; ++ch)
        {
            auto i = size_t(ch);
            comp[i].set({c.compThresholdDb+offset,c.ratio,c.attackMs*.001,c.releaseS,10,c.autoRelease});
            gate[i].set({c.gateThresholdDb+offset,c.rangeDb,c.holdS,c.gateReleaseS,c.fastAttack,c.hysteresis,c.expand});
            deess[i].set(c.deessThresholdDb+offset);
            bool limit = c.compType==CompType::OptoLimit;
            double ratio = limit ? 5+(c.ratio-1.5)/8.5*15 : 1.5+(c.ratio-1.5)/8.5*3.5;
            opto[i].set(c.compThresholdDb+offset,ratio,limit);
        }
    }
    bool openLibrary()
    {
        if (command) return true;
        juce::File source;
#if JUCE_MAC || JUCE_LINUX
        Dl_info info {};
        if (dladdr(reinterpret_cast<void*>(&Renderer::compiled), &info) && info.dli_fname)
        {
            auto local = juce::File(juce::String::fromUTF8(info.dli_fname)).getParentDirectory().getChildFile(BLUECHANNEL_HQ_LIBRARY_NAME);
            if (local.existsAsFile()) source = local;
        }
#endif
        if (!source.existsAsFile()) source = juce::File(BLUECHANNEL_HQ_LIBRARY);
        for (int i=0; i<16 && source.isSymbolicLink(); ++i) source=source.getLinkedTarget();
#if JUCE_MAC || JUCE_LINUX
        const juce::File temp(juce::SystemStats::getEnvironmentVariable("TMPDIR", "/tmp"));
#else
        const auto temp=juce::File::getSpecialLocation(juce::File::tempDirectory);
#endif
        privateLibrary = temp.getNonexistentChildFile("BlueChannel-HQ-"+juce::Uuid().toString(), source.getFileExtension());
        if (!source.copyFileTo(privateLibrary))
        { failure="Cannot copy HQ solver to temporary directory: "+privateLibrary.getFullPathName().toStdString();failed=true;return false; }
        if (!library.open(privateLibrary.getFullPathName()))
        {
            failure="Cannot load the isolated HQ component solver";
#if JUCE_MAC || JUCE_LINUX
            if (auto* detail=dlerror()) failure += ": "+std::string(detail);
#endif
            failed=true;return false;
        }
#define HQ_SYMBOL(member, name) member = reinterpret_cast<decltype(member)>(library.getFunction(name)); if (!member) { failed=true; failure="Missing solver entry point: " name; return false; }
        HQ_SYMBOL(init,"ngSpice_Init")
        HQ_SYMBOL(initSync,"ngSpice_Init_Sync")
        HQ_SYMBOL(command,"ngSpice_Command")
        HQ_SYMBOL(circuitLoad,"ngSpice_Circ")
        HQ_SYMBOL(breakpoint,"ngSpice_SetBkpt")
#undef HQ_SYMBOL
        init(log,nullptr,exit,data,initData,nullptr,this);
        int ident = 0;
        initSync(voltage,current,nullptr,&ident,this);
        send("set noaskquit");
        return !failed;
    }
    bool start(int factor)
    {
        if (!openLibrary()) return false;
        fs=baseRate*factor;
        send("delete all");
        send("destroy all");
        // Remove the prior circuit before loading another deck. ngspice 47's
        // source-history list is shared by loaded circuit records; retaining
        // old records makes quit free that list more than once after restarts.
        if (initialised) send("remcirc");
        failed = false; failure.clear();
        position=0; previousTime=0; previous={}; lastInput={}; lastKey={};
        for(auto& row:lastPhysicalInput) std::fill(std::begin(row),std::end(row),0);
        compCurrent={}; deessCurrent={}; limitCurrent={}; optoGain={1,1};
        for (int ch=0; ch<channels; ++ch)
        {
            auto i=size_t(ch);
            comp[i].prepare(fs); gate[i].prepare(fs); opto[i].prepare(fs); deess[i].prepare(fs); limiter[i].prepare(fs);
        }
        deessCalibration.prepare(fs);
        configure(ctl);
        initialValues=fromValues=values; controlsTime=0;
        std::string deck = "Blue Channel HQ component audio solver\n.param micunity=" + number(model::controlmap::micUnityScale(ctl.micGain)) + "\n" + std::string(circuit) + "\n";
        for (auto* name : controlNames)
            deck += ".global c_"+std::string(name)+"\nVctrl_"+name+" c_"+name+" 0 dc "+number(values[name])+" external\n";
        deck += "Vvp vp 0 16.0718781\nVvn vn 0 -14.7679257\nVp30 p30 0 32.4049685\nVlp lp 0 5.19033544\nVph phantom 0 48.2230288\n";
        for (int ch=0; ch<channels; ++ch)
        {
            const auto prefix="x"+std::to_string(ch);
            deck += prefix+" vp vn p30 lp phantom channel\n";
            for (auto* seed : biasSeeds)
            {
                std::string str(seed); auto equal=str.find('=');
                deck += ".nodeset v("+prefix+"."+str.substr(0,equal)+")="+str.substr(equal+1)+"\n";
            }
            deck += ".save v("+prefix+".outp) v("+prefix+".outn) v("+prefix+".inputtap) v("+prefix+".compkey) v("+prefix+".gatekey) v("+prefix+".xdeess.filter_out) v("+prefix+".faded) v("+prefix+".xlimit.xopt4.state) v("+prefix+".xdeess.xopt2.state)\n";
        }
        deck += ".options gmin=1e-12 rshunt=1e12 reltol=1e-5 abstol=1e-12 vntol=1e-6 method=gear maxord=2 itl1=1000 itl4=100\n.end\n";
        juce::StringArray lines;
        lines.addLines(juce::String(deck));
        std::vector<std::string> storage;
        for (const auto& line : lines) if (line.isNotEmpty()) storage.push_back(line.toStdString());
        std::vector<char*> ptrs;
        for (auto& line : storage) ptrs.push_back(line.data());
        ptrs.push_back(nullptr);
        circuitLoad(ptrs.data());
        int stages=0; while ((1<<stages)<factor) ++stages;
        resampler.prepare(baseRate,blockSize,4,stages);
        oversampling=factor;
        running=false; initialised=true; needsRestart=false;
        return !failed;
    }
#endif
};
Renderer::Renderer() : impl(std::make_unique<Impl>()) {}
Renderer::~Renderer() = default;
bool Renderer::compiled() { return BLUECHANNEL_HQ != 0; }
const char* Renderer::error() const { return impl->failure.c_str(); }
void Renderer::prepare(double fs, int block, int channels)
{
    impl->fs=impl->baseRate=fs; impl->blockSize=std::max(1,block); impl->channels=std::clamp(channels,1,2);
#if BLUECHANNEL_HQ
    for (auto* bank : {&impl->input,&impl->keys,&impl->solved})
        for (auto& b : *bank) b.assign(size_t(impl->blockSize)*16,0);
    for (auto* bank : {&impl->baseInput,&impl->baseKey})
        for (auto& b : *bank) b.assign(size_t(impl->blockSize),0);
#endif
    restart();
}
void Renderer::restart() { impl->needsRestart=true; }
bool Renderer::render(double* const* io, int n, const double* const* key, int kc,
                      const model::Controls& controls, int factor, model::Telemetry& telemetry)
{
#if BLUECHANNEL_HQ
    auto& s=*impl;
    if (n<=0 || n>s.blockSize) return false;
    factor=std::clamp(factor,4,16);
    // Source and mic gain-switch changes start a new operating point, including the fixed unity calibration.
    if (s.initialised && (s.ctl.source!=controls.source || s.ctl.micGain!=controls.micGain)) s.needsRestart=true;
    s.ctl=controls;
    if (kc<=0 || !key) { s.ctl.compExtKey=false; s.ctl.gateExtKey=false; }
    if (s.needsRestart || !s.initialised || factor!=s.oversampling)
        if (!s.start(factor)) return false;
    if (s.failed) return false;
    auto current=s.values;
    for(auto& [name,value]:current) value=s.controlValue(name,double(s.position)/s.fs);
    s.fromValues=std::move(current);
    s.controlsTime=double(s.position)/s.fs;
    s.configure(s.ctl);
    const int m=n*factor;
    s.frameCount=m; s.emitted=0; s.telemetry=&telemetry;
    s.inputSquares[0]=s.inputSquares[1]=0;
    double* solved[2]={s.solved[0].data(),s.solved[1].data()};
    s.audio=solved;
    for (int ch=0; ch<s.channels; ++ch)
        for (int i=0; i<n; ++i)
        {
            s.baseInput[size_t(ch)][size_t(i)] = io[ch][i]*model::kFullScalePeak;
            s.baseKey[size_t(ch)][size_t(i)] = key && kc>0 ? key[kc>1 ? ch:0][i]*model::kFullScalePeak*
                std::pow(10.0,model::calibration::kLineInputDb/20) : 0;
        }
    for (int ch=0; ch<s.channels; ++ch)
    {
        s.resampler.upsample(ch,s.baseInput[size_t(ch)].data(),n,s.input[size_t(ch)].data());
        s.resampler.upsample(2+ch,s.baseKey[size_t(ch)].data(),n,s.keys[size_t(ch)].data());
    }
    s.endTime=double(s.position+uint64_t(m))/s.fs;
    s.send("delete all");
    s.send("stop when time = "+number(s.endTime));
    for (int i=1; i<=m; ++i) s.breakpoint(double(s.position+uint64_t(i))/s.fs);
    if (!s.running)
    {
        // Minimum 4x integration; higher offline settings tighten the maximum adaptive step.
        const double dt=1/s.fs;
        s.send("tran "+number(dt)+" 3600 0 "+number(dt));
        s.running=true;
    }
    else s.send("resume");
    if (s.failed || s.emitted!=m)
    { s.failed=true; if(s.failure.empty()) s.failure="Component solve did not reach the requested block boundary"; return false; }
    for (int ch=0; ch<s.channels; ++ch)
    {
        s.lastInput[size_t(ch)]=s.input[size_t(ch)][size_t(m-1)]; s.lastKey[size_t(ch)]=s.keys[size_t(ch)][size_t(m-1)];
        for(int role=0;role<3;++role) s.lastPhysicalInput[ch][role]=int(s.ctl.source)==role ? s.lastInput[size_t(ch)] : 0;
    }
    for (int ch=0; ch<s.channels; ++ch)
    {
        s.resampler.downsample(ch,s.solved[size_t(ch)].data(),n,io[ch]);
        double outSquare=0;
        for (int i=0; i<n; ++i) outSquare+=io[ch][i]*io[ch][i];
        telemetry.inputSquare[ch].store(float(s.inputSquares[ch]/m));
        telemetry.outputSquare[ch].store(float(outSquare/n));
    }
    s.position+=uint64_t(m);
    telemetry.activeFactor.store(std::max(4,factor));
    telemetry.rendering.store(true);
    return true;
#else
    juce::ignoreUnused(io,n,key,kc,controls,factor,telemetry);
    impl->failure="HQ component solver is not included in this build";
    return false;
#endif
}
}
