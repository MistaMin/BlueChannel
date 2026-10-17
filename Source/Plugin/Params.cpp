// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid

#include "Params.h"

#include "Model/ControlMap.h"

#include <cstring>

namespace bluechannel::params
{
namespace cm = model::controlmap;
using APF = juce::AudioParameterFloat;
using APC = juce::AudioParameterChoice;
// Hosts may send fractional normalised values even to switches. Report their
// effective boolean value so a state round trip has the same host-visible value.
class SwitchParameter final : public juce::AudioParameterBool
{
public:
    using juce::AudioParameterBool::AudioParameterBool;
    float getValue() const override { return get() ? 1.0f : 0.0f; }
};
using APB = SwitchParameter;
using Attr = juce::AudioParameterFloatAttributes;

juce::String hzText(double hz)
{
    if (hz >= 10000.0) return juce::String(hz / 1000.0, 1) + " kHz";
    if (hz >= 1000.0) return juce::String(hz / 1000.0, 2) + " kHz";
    if (hz >= 100.0) return juce::String(juce::roundToInt(hz)) + " Hz";
    return juce::String(hz, 1) + " Hz";
}

static juce::String dbText(float v, int decimals = 1)
{
    return (v > 0.0f ? "+" : "") + juce::String(v, decimals) + " dB";
}

static juce::NormalisableRange<float> logRange(double lo, double hi)
{
    return { float(lo), float(hi),
             [=](float, float, float t) { return float(lo * std::pow(hi / lo, double(t))); },
             [=](float, float, float v) { return float(std::log(double(v) / lo) / std::log(hi / lo)); },
             [=](float, float, float v) { return juce::jlimit(float(lo), float(hi), v); } };
}

// Mic gain legends are input-to-output gain: the preamp gain plus the output transformer, less the line
// receiver loss that the Output control's 0 dB already compensates (see ControlMap::outputOffsetDb).
juce::String micGainLabel(int index)
{
    const double g = index == cm::kMicUnityIndex ? 0.0 : cm::micGainDb(index) - model::calibration::kLineInputDb;
    return (g > 0.05 ? "+" : "") + juce::String(g, 0) + " dB";
}
juce::String lineGainLabel(int index)
{
    const double g = cm::lineGainDb(index);
    return (g > 0.05 ? "+" : "") + juce::String(std::abs(g) < 0.05 ? 0.0 : g, 0) + " dB";
}
juce::String lowShelfLabel(int position, bool high)
{
    static const char* lo[] = { "20", "56", "160", "460" };
    static const char* hi[] = { "33", "95", "270", "655" };
    return juce::String(high ? hi[position] : lo[position]) + " Hz";
}
juce::String highShelfLabel(int position, bool high)
{
    static const char* lo[] = { "1.5k", "3.3k", "6.8k", "15k" };
    static const char* hi[] = { "2.2k", "4.7k", "10k", "18k" };
    return juce::String(high ? hi[position] : lo[position]) + "Hz";
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace model::calibration;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto pid = [](const char* s) { return juce::ParameterID { s, kSchemaVersion }; };
    auto dbAttr = [](int decimals = 1) {
        return Attr().withLabel("dB").withStringFromValueFunction([decimals](float v, int) { return dbText(v, decimals); });
    };
    auto hzAttr = Attr().withLabel("Hz").withStringFromValueFunction([](float v, int) { return hzText(v); });
    auto add = [&](auto p) { layout.add(std::move(p)); };
    const juce::StringArray routes { "Out", "In", "Comp key", "Gate key" };

    // Input
    add(std::make_unique<APC>(pid(id::source), "Input", juce::StringArray { "Mic", "Line", "Inst" }, 0));
    juce::StringArray mic, line;
    for (int i = 0; i < 12; ++i) mic.add(micGainLabel(i));
    for (int i = 0; i < 7; ++i) line.add(lineGainLabel(i));
    add(std::make_unique<APC>(pid(id::micGain), "Mic Gain", mic, model::controlmap::kMicUnityIndex));
    add(std::make_unique<APC>(pid(id::lineGain), "Line Gain", line, 3));
    add(std::make_unique<APF>(pid(id::trim), "Trim", juce::NormalisableRange<float>(0.0f, float(cm::trimMaxDb()), 0.01f), 0.0f, dbAttr()));
    add(std::make_unique<APB>(pid(id::phase), "Phase", false));
    add(std::make_unique<APC>(pid(id::impedance), "Impedance",
                              juce::StringArray { "Low 600", "Classic 1.4k", "Medium 2.4k", "High 6.8k" }, 1));
    add(std::make_unique<APB>(pid(id::air), "Mic Air", false));

    // Equaliser
    add(std::make_unique<APB>(pid(id::allEq), "All EQ", true));
    add(std::make_unique<APC>(pid(id::filterRoute), "Filters", routes, 0));
    add(std::make_unique<APF>(pid(id::hpf), "High Pass", logRange(cm::highPassMinHz(), cm::highPassMaxHz()), float(cm::highPassMinHz()), hzAttr));
    add(std::make_unique<APF>(pid(id::lpf), "Low Pass", logRange(cm::lowPassMinHz(), cm::lowPassMaxHz()), float(cm::lowPassMaxHz()), hzAttr));
    add(std::make_unique<APC>(pid(id::shelfRoute), "Shelving EQ", routes, 0));
    add(std::make_unique<APF>(pid(id::lfGain), "LF Gain", juce::NormalisableRange<float>(-17.5f, 17.5f, 0.01f), 0.0f, dbAttr()));
    add(std::make_unique<APC>(pid(id::lfFreq), "LF Frequency", juce::StringArray { "20/33 Hz", "56/95 Hz", "160/270 Hz", "460/655 Hz" }, 1));
    add(std::make_unique<APB>(pid(id::lfHigh), "LF Hi Range", false));
    add(std::make_unique<APF>(pid(id::hfGain), "HF Gain", juce::NormalisableRange<float>(-17.5f, 17.5f, 0.01f), 0.0f, dbAttr()));
    add(std::make_unique<APC>(pid(id::hfFreq), "HF Frequency", juce::StringArray { "1.5/2.2 kHz", "3.3/4.7 kHz", "6.8/10 kHz", "15/18 kHz" }, 1));
    add(std::make_unique<APB>(pid(id::hfHigh), "HF Hi Range", false));
    add(std::make_unique<APC>(pid(id::paramRoute), "Parametric EQ", routes, 0));
    const float pg = float(std::floor(10.0 * std::min(kLmfGainDb[0], -kLmfGainDb[kPotSteps - 1])) / 10.0);
    add(std::make_unique<APF>(pid(id::lmfGain), "LMF Gain", juce::NormalisableRange<float>(-pg, pg, 0.01f), 0.0f, dbAttr()));
    add(std::make_unique<APF>(pid(id::lmfFreq), "LMF Frequency", logRange(cm::minOf(kLmfFreqHz), cm::maxOf(kLmfFreqHz)), 120.0f, hzAttr));
    add(std::make_unique<APF>(pid(id::lmfQ), "LMF Q", logRange(cm::minOf(kLmfQ), cm::maxOf(kLmfQ)), 1.2f,
                              Attr().withStringFromValueFunction([](float v, int) { return juce::String(v, 2); })));
    add(std::make_unique<APB>(pid(id::lmfX3), "LMF x3", false));
    add(std::make_unique<APF>(pid(id::hmfGain), "HMF Gain", juce::NormalisableRange<float>(-pg, pg, 0.01f), 0.0f, dbAttr()));
    add(std::make_unique<APF>(pid(id::hmfFreq), "HMF Frequency", logRange(cm::minOf(kHmfFreqHz), cm::maxOf(kHmfFreqHz)), 1800.0f, hzAttr));
    add(std::make_unique<APF>(pid(id::hmfQ), "HMF Q", logRange(cm::minOf(kHmfQ), cm::maxOf(kHmfQ)), 1.2f,
                              Attr().withStringFromValueFunction([](float v, int) { return juce::String(v, 2); })));
    add(std::make_unique<APB>(pid(id::hmfX3), "HMF x3", false));

    // Compressor
    add(std::make_unique<APB>(pid(id::compIn), "Comp In", false));
    add(std::make_unique<APC>(pid(id::compType), "Comp Type", juce::StringArray { "VCA", "Vintage Comp", "Vintage Lim" }, 0));
    add(std::make_unique<APF>(pid(id::ratio), "Ratio", logRange(1.5, 10.0), 3.0f,
                              Attr().withStringFromValueFunction([](float v, int) { return juce::String(v, 1) + ":1"; })));
    add(std::make_unique<APF>(pid(id::threshold), "Comp Threshold", juce::NormalisableRange<float>(-28.0f, 12.0f, 0.01f), 0.0f, dbAttr()));
    add(std::make_unique<APF>(pid(id::attack), "Attack", logRange(0.1, 100.0), 10.0f,
                              Attr().withLabel("ms").withStringFromValueFunction([](float v, int) { return juce::String(v, v < 1 ? 2 : 1) + " ms"; })));
    add(std::make_unique<APF>(pid(id::release), "Release", logRange(0.1, 7.0), 0.3f,
                              Attr().withLabel("s").withStringFromValueFunction([](float v, int) { return juce::String(v, 2) + " s"; })));
    add(std::make_unique<APB>(pid(id::autoRelease), "Auto Release", false));
    add(std::make_unique<APF>(pid(id::makeup), "Makeup", juce::NormalisableRange<float>(0.0f, float(kMakeupDb[0]), 0.01f), 0.0f, dbAttr()));
    add(std::make_unique<APB>(pid(id::compKey), "Comp Ext Key", false));
    add(std::make_unique<APB>(pid(id::blend), "Blend", false));
    add(std::make_unique<APF>(pid(id::mix), "Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
                              Attr().withLabel("%").withStringFromValueFunction([](float v, int) { return juce::String(juce::roundToInt(v)) + " %"; })));
    add(std::make_unique<APB>(pid(id::compListen), "Comp Listen", false));

    // Gate
    add(std::make_unique<APB>(pid(id::gateIn), "Gate In", false));
    add(std::make_unique<APB>(pid(id::expand), "Expand", false));
    add(std::make_unique<APF>(pid(id::gateThreshold), "Gate Threshold", juce::NormalisableRange<float>(-40.0f, 10.0f, 0.01f), -40.0f, dbAttr()));
    add(std::make_unique<APF>(pid(id::range), "Range", juce::NormalisableRange<float>(0.0f, 80.0f, 0.1f), 20.0f, dbAttr(0)));
    add(std::make_unique<APF>(pid(id::hold), "Hold", logRange(0.02, 4.0), 0.1f,
                              Attr().withLabel("s").withStringFromValueFunction([](float v, int) { return v < 1 ? juce::String(juce::roundToInt(v * 1000)) + " ms" : juce::String(v, 2) + " s"; })));
    add(std::make_unique<APF>(pid(id::gateRelease), "Gate Release", logRange(0.1, 5.0), 0.5f,
                              Attr().withLabel("s").withStringFromValueFunction([](float v, int) { return juce::String(v, 2) + " s"; })));
    add(std::make_unique<APB>(pid(id::fastAttack), "Fast Attack", false));
    add(std::make_unique<APB>(pid(id::hysteresis), "Hysteresis", true));
    add(std::make_unique<APB>(pid(id::gateKey), "Gate Ext Key", false));
    add(std::make_unique<APB>(pid(id::gateListen), "Gate Listen", false));

    // De-esser
    add(std::make_unique<APB>(pid(id::deessIn), "De-Ess In", false));
    add(std::make_unique<APF>(pid(id::deessThreshold), "De-Ess Threshold", juce::NormalisableRange<float>(-30.0f, 10.0f, 0.01f), 0.0f, dbAttr()));
    add(std::make_unique<APF>(pid(id::deessFreq), "De-Ess Frequency", logRange(cm::minOf(kDeessFreqHz), cm::maxOf(kDeessFreqHz)), 5000.0f, hzAttr));
    add(std::make_unique<APB>(pid(id::deessListen), "De-Ess Listen", false));

    // Routing and output
    add(std::make_unique<APC>(pid(id::dynPosition), "Dynamics Position", juce::StringArray { "Post-EQ", "Pre-EQ" }, 0));
    add(std::make_unique<APB>(pid(id::bypass), "Bypass", false));
    add(std::make_unique<APF>(pid(id::output), "Output", juce::NormalisableRange<float>(-60.0f, float(cm::outputMaxDb()), 0.01f, 2.5f), 0.0f, dbAttr()));
    add(std::make_unique<APB>(pid(id::softLimit), "Soft Limiter", false));
    add(std::make_unique<APB>(pid(id::mute), "Mute", false));
    add(std::make_unique<APB>(pid(id::link), "Stereo Link", true));

    // Processing
    add(std::make_unique<APC>(pid(id::oversampling), "Oversampling", juce::StringArray { "1x", "2x", "4x", "8x", "16x Derp mode" }, 1));
    add(std::make_unique<APC>(pid(id::offlineOversampling), "Offline Oversampling",
                              juce::StringArray { "Same as playback", "1x", "2x", "4x", "8x", "16x Derp mode" }, 0));
    add(std::make_unique<APB>(pid(id::hq), "HQ Mode (Offline)", false));
    return layout;
}

Reader::Reader(juce::AudioProcessorValueTreeState& state) : apvts(state)
{
    // Every id constant, resolved once so the audio thread never looks parameters up by string.
    const char* all[] = { id::source, id::micGain, id::lineGain, id::trim, id::phase, id::impedance, id::air, id::allEq,
                          id::filterRoute, id::hpf, id::lpf, id::shelfRoute, id::lfGain, id::lfFreq, id::lfHigh, id::hfGain,
                          id::hfFreq, id::hfHigh, id::paramRoute, id::lmfGain, id::lmfFreq, id::lmfQ, id::lmfX3, id::hmfGain,
                          id::hmfFreq, id::hmfQ, id::hmfX3, id::compIn, id::compType, id::ratio, id::threshold, id::attack,
                          id::release, id::autoRelease, id::makeup, id::compKey, id::blend, id::mix, id::compListen, id::gateIn,
                          id::expand, id::gateThreshold, id::range, id::hold, id::gateRelease, id::fastAttack, id::hysteresis,
                          id::gateKey, id::gateListen, id::deessIn, id::deessThreshold, id::deessFreq, id::deessListen,
                          id::dynPosition, id::bypass, id::output, id::softLimit, id::mute, id::link, id::oversampling,
                          id::offlineOversampling, id::hq };
    for (auto* n : all)
    {
        auto* v = state.getRawParameterValue(n);
        jassert(v != nullptr);
        cache.emplace_back(n, v);
    }
}

std::atomic<float>* Reader::get(const char* name) const
{
    for (auto& [n, v] : cache)
        if (n == name) return v;
    for (auto& [n, v] : cache)
        if (std::strcmp(n, name) == 0) return v;
    jassertfalse;
    return cache.front().second;
}

void Reader::read(model::Controls& c) const
{
    using namespace model;
    auto f = [&](const char* n) { return double(get(n)->load(std::memory_order_relaxed)); };
    auto b = [&](const char* n) { return get(n)->load(std::memory_order_relaxed) > 0.5f; };
    auto i = [&](const char* n) { return juce::roundToInt(get(n)->load(std::memory_order_relaxed)); };
    c.source = Source(i(id::source));
    c.micGain = i(id::micGain);
    c.lineGain = i(id::lineGain);
    c.trimDb = f(id::trim);
    c.phase = b(id::phase);
    c.impedance = i(id::impedance);
    c.air = b(id::air);
    c.allEq = b(id::allEq);
    c.filterRoute = EqRoute(i(id::filterRoute));
    c.hpfHz = f(id::hpf);
    c.lpfHz = f(id::lpf);
    c.shelfRoute = EqRoute(i(id::shelfRoute));
    c.lfDb = f(id::lfGain);
    c.lfFreq = i(id::lfFreq);
    c.lfHigh = b(id::lfHigh);
    c.hfDb = f(id::hfGain);
    c.hfFreq = i(id::hfFreq);
    c.hfHigh = b(id::hfHigh);
    c.paramRoute = EqRoute(i(id::paramRoute));
    c.lmfDb = f(id::lmfGain);
    c.lmfHz = f(id::lmfFreq);
    c.lmfQ = f(id::lmfQ);
    c.lmfX3 = b(id::lmfX3);
    c.hmfDb = f(id::hmfGain);
    c.hmfHz = f(id::hmfFreq);
    c.hmfQ = f(id::hmfQ);
    c.hmfX3 = b(id::hmfX3);
    c.compIn = b(id::compIn);
    c.compType = CompType(i(id::compType));
    c.ratio = f(id::ratio);
    c.compThresholdDb = f(id::threshold);
    c.attackMs = f(id::attack);
    c.releaseS = f(id::release);
    c.autoRelease = b(id::autoRelease);
    c.makeupDb = f(id::makeup);
    c.compExtKey = b(id::compKey);
    c.blend = b(id::blend);
    c.mix = f(id::mix) / 100.0;
    c.compListen = b(id::compListen);
    c.gateIn = b(id::gateIn);
    c.expand = b(id::expand);
    c.gateThresholdDb = f(id::gateThreshold);
    c.rangeDb = f(id::range);
    c.holdS = f(id::hold);
    c.gateReleaseS = f(id::gateRelease);
    c.fastAttack = b(id::fastAttack);
    c.hysteresis = b(id::hysteresis);
    c.gateExtKey = b(id::gateKey);
    c.gateListen = b(id::gateListen);
    c.deessIn = b(id::deessIn);
    c.deessThresholdDb = f(id::deessThreshold);
    c.deessHz = f(id::deessFreq);
    c.deessListen = b(id::deessListen);
    c.dynPosition = DynPosition(i(id::dynPosition));
    c.bypass = b(id::bypass);
    c.outputDb = f(id::output);
    c.softLimit = b(id::softLimit);
    c.mute = b(id::mute);
    c.link = b(id::link);
}


bool Reader::hqEnabled() const { return get(id::hq)->load(std::memory_order_relaxed) > .5f; }

int Reader::playbackStages() const { return juce::roundToInt(get(id::oversampling)->load()); }
int Reader::offlineStages() const { return juce::roundToInt(get(id::offlineOversampling)->load()) - 1; }
} // namespace bluechannel::params
