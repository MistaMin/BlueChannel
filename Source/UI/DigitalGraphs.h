// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once
#include "PluginProcessor.h"
#include "Widgets.h"
#include <set>
#include <juce_dsp/juce_dsp.h>

namespace bluechannel::ui
{
// Graph edits use the same host gestures as the sliders. No audio-thread state is read or changed here.
class GraphParameters
{
public:
    explicit GraphParameters(juce::AudioProcessorValueTreeState& s) : state(s) {}
    ~GraphParameters() { release(); }
    double get(const char* id) const { return state.getRawParameterValue(id)->load(); }
    void edit(const char* id, double v)
    {
        auto* p = state.getParameter(id);
        if (touched.insert(p).second) p->beginChangeGesture();
        const auto& range=p->getNormalisableRange();
        p->setValueNotifyingHost(p->convertTo0to1(range.snapToLegalValue(juce::jlimit(range.start,range.end,float(v)))));
    }
    void release() { for(auto* p : touched) p->endChangeGesture(); touched.clear(); }
private:
    juce::AudioProcessorValueTreeState& state;
    std::set<juce::RangedAudioParameter*> touched;
};

class DigitalEqGraph : public juce::Component
{
    BlueChannelProcessor& proc;
    static constexpr int kOrder = 11, kFftSize = 1 << kOrder;
    juce::dsp::FFT fft { kOrder };
    juce::dsp::WindowingFunction<float> window { kFftSize, juce::dsp::WindowingFunction<float>::hann, false };
    std::array<float, kFftSize * 2> fftData {};
    std::array<float, kFftSize / 2> spectrumDb {};
    GraphParameters params;
    bluechannel::params::Reader reader;
    model::Controls controls, previous;
    model::EqStage filters, shelves, lmf, hmf;
    bool dirty = true;
    std::array<float,161> curve {}, filterCurve {};
    int active = -1;
    using P = juce::Point<float>;
    juce::Rectangle<float> area() const { return getLocalBounds().toFloat().reduced(32,24); }
    float x(double f) const { auto r=area(); return r.getX()+float(std::log(f/20)/std::log(1000.0))*r.getWidth(); }
    float y(double db) const { auto r=area(); return r.getCentreY()-float(db/24)*r.getHeight()*0.5f; }
    double freq(float px) const { auto r=area(); return 20*std::pow(1000.0,double((px-r.getX())/r.getWidth())); }
    double db(float py) const { auto r=area(); return double((r.getCentreY()-py)/r.getHeight())*48; }
    static juce::Colour colour(int i)
    {
        static const juce::Colour colours[] = {juce::Colour(0xffff6a5a),juce::Colour(0xff5ae07a),juce::Colour(0xff5ab4ff),juce::Colour(0xffffc04a)};
        return colours[i];
    }
    double shelfHz(bool low, int index) const
    {
        auto text= low ? bluechannel::params::lowShelfLabel(index,controls.lfHigh) : bluechannel::params::highShelfLabel(index,controls.hfHigh);
        return text.getDoubleValue()*(text.containsIgnoreCase("k") ? 1000 : 1);
    }
    P handle(int i) const
    {
        using namespace model::controlmap;
        if(i==4 || i==5) return {juce::jlimit(area().getX(),area().getRight(),x(i==4 ? controls.hpfHz : controls.lpfHz)),y(-3)};
        if(i==0) return {x(shelfHz(true,controls.lfFreq)),y(controls.lfDb)};
        if(i==3) return {x(shelfHz(false,controls.hfFreq)),y(controls.hfDb)};
        if(i==1) return {x(lmfHz(lmfFreqPot(controls.lmfHz),controls.lmfX3)),y(controls.lmfDb)};
        return {x(hmfHz(hmfFreqPot(controls.hmfHz),controls.hmfX3)),y(controls.hmfDb)};
    }
public:
    explicit DigitalEqGraph(BlueChannelProcessor& p) : proc(p), params(p.apvts), reader(p.apvts)
    {
        filters.load(model::nl::kFilters); shelves.load(model::nl::kShelves);
        lmf.load(model::nl::kLmf); hmf.load(model::nl::kHmf);
        setTitle("Equaliser response: drag frequency and gain; scroll a mid band for Q");
        spectrumDb.fill(-100);
        refresh();
    }
    void refresh()
    {
        const unsigned end = proc.spectrumWrite.load(std::memory_order_acquire);
        for (unsigned i=0;i<kFftSize;++i)
            fftData[i] = proc.spectrum[(end - kFftSize + i) & (BlueChannelProcessor::kSpectrumSize - 1)].load(std::memory_order_relaxed);
        std::fill(fftData.begin()+kFftSize, fftData.end(), 0);
        window.multiplyWithWindowingTable(fftData.data(), kFftSize);
        fft.performFrequencyOnlyForwardTransform(fftData.data());
        for(size_t i=0;i<spectrumDb.size();++i)
        {
            const float target=20*std::log10(std::max(1e-9f,fftData[i]*4/float(kFftSize)));
            spectrumDb[i] += (target-spectrumDb[i])*(target>spectrumDb[i] ? 0.8f : 0.2f);
        }
        repaint();
        reader.read(controls);
        // Compressor, gate and input edits must not recompile the response networks.
        model::Controls view;
        view.allEq=controls.allEq;view.bypass=controls.bypass;
        view.filterRoute=controls.filterRoute;view.hpfHz=controls.hpfHz;view.lpfHz=controls.lpfHz;
        view.shelfRoute=controls.shelfRoute;view.lfDb=controls.lfDb;view.hfDb=controls.hfDb;
        view.lfFreq=controls.lfFreq;view.hfFreq=controls.hfFreq;view.lfHigh=controls.lfHigh;view.hfHigh=controls.hfHigh;
        view.paramRoute=controls.paramRoute;view.lmfDb=controls.lmfDb;view.lmfHz=controls.lmfHz;
        view.lmfQ=controls.lmfQ;view.lmfX3=controls.lmfX3;view.hmfDb=controls.hmfDb;
        view.hmfHz=controls.hmfHz;view.hmfQ=controls.hmfQ;view.hmfX3=controls.hmfX3;
        if(!dirty && view==previous) return;
        previous=view; dirty=false;
        using namespace model::controlmap;
        filters.set("hpr",highPassOhms(controls.hpfHz)); filters.set("lpr",lowPassOhms(controls.lpfHz));
        shelves.set("lfgain",lowShelfPot(controls.lfDb)); shelves.set("hfgain",highShelfPot(controls.hfDb));
        shelves.set("lfsel",lowShelfPosition(controls.lfFreq,controls.lfHigh)); shelves.set("hfsel",highShelfPosition(controls.hfFreq,controls.hfHigh));
        lmf.set("gain",lmfGainPot(controls.lmfDb)); lmf.set("q",lmfQPot(controls.lmfQ)); lmf.set("freq",lmfFreqPot(controls.lmfHz)); lmf.set("x3",controls.lmfX3 ? 1 : 0);
        hmf.set("gain",hmfGainPot(controls.hmfDb)); hmf.set("q",hmfQPot(controls.hmfQ)); hmf.set("freq",hmfFreqPot(controls.hmfHz)); hmf.set("x3",controls.hmfX3 ? 1 : 0);
        for(size_t i=0;i<curve.size();++i)
        {
            const double f=20*std::pow(1000.0,double(i)/double(curve.size()-1));
            std::complex<double> response(1,0);
            if(controls.allEq && !controls.bypass)
            {
                if(controls.filterRoute==model::EqRoute::In) response*=filters.dk.analogResponse(0,0,f);
                if(controls.shelfRoute==model::EqRoute::In) response*=shelves.dk.analogResponse(0,0,f);
                if(controls.paramRoute==model::EqRoute::In) response*=lmf.dk.analogResponse(0,0,f)*hmf.dk.analogResponse(0,0,f);
            }
            filterCurve[i]=float(20*std::log10(std::max(1e-9,std::abs(filters.dk.analogResponse(0,0,f)))));
            curve[i]=float(20*std::log10(std::max(1e-9,std::abs(response))));
        }
        repaint();
    }
    void paint(juce::Graphics& g) override
    {
        auto r=getLocalBounds().toFloat(); auto a=area();
        g.setColour(juce::Colour(0xff050a12)); g.fillRoundedRectangle(r,4);
        for(double f : {50.,100.,200.,500.,1000.,2000.,5000.,10000.})
        {
            g.setColour(juce::Colour(0xff3a5679).withAlpha(0.3f)); g.drawVerticalLine(int(x(f)),a.getY(),a.getBottom());
            drawText(g,f>=1000 ? juce::String(f/1000,0)+"k" : juce::String(f,0),{x(f)-15,a.getBottom()+5,30,12},9,juce::Colour(0xff6f86a8),juce::Justification::centred,false);
        }
        for(int d : {-18,-12,-6,0,6,12,18})
        {
            g.setColour(juce::Colour(0xff3a5679).withAlpha(d==0 ? 0.7f : 0.25f)); g.drawHorizontalLine(int(y(d)),a.getX(),a.getRight());
            drawText(g,juce::String(d),{1,y(d)-6,27,12},9,juce::Colour(0xff6f86a8),juce::Justification::centred,false);
        }
        g.saveState(); g.reduceClipRegion(a.toNearestInt());
        juce::Path spectrumPath;
        spectrumPath.startNewSubPath(a.getX(),a.getBottom());
        const double rate=proc.spectrumRate.load(std::memory_order_relaxed);
        for(int px=0;px<=int(a.getWidth());++px)
        {
            const double f=freq(a.getX()+float(px));
            const int bin=std::clamp(int(f/rate*kFftSize),1,kFftSize/2-1);
            const float value=f<=rate*0.5 ? spectrumDb[size_t(bin)] : -100;
            spectrumPath.lineTo(a.getX()+float(px),a.getBottom()-juce::jlimit(0.0f,1.0f,(value+100)/100)*a.getHeight());
        }
        spectrumPath.lineTo(a.getRight(),a.getBottom());spectrumPath.closeSubPath();
        g.setGradientFill(juce::ColourGradient(juce::Colour(0x6639d0ff),0,a.getY(),juce::Colour(0x0639d0ff),0,a.getBottom(),false));
        g.fillPath(spectrumPath);
        g.setColour(juce::Colour(0x6639d0ff));g.strokePath(spectrumPath,juce::PathStrokeType(1));
        juce::Path p;
        for(size_t i=0;i<curve.size();++i)
        {
            const float px=a.getX()+float(i)/float(curve.size()-1)*a.getWidth();
            if(i==0) p.startNewSubPath(px,y(curve[i])); else p.lineTo(px,y(curve[i]));
        }
        g.setColour(juce::Colour(0xff39d0ff).withAlpha(0.13f)); g.strokePath(p,juce::PathStrokeType(7));
        g.setColour(juce::Colour(0xffdbe8ff)); g.strokePath(p,juce::PathStrokeType(1.7f));
        if(controls.filterRoute==model::EqRoute::CompKey || controls.filterRoute==model::EqRoute::GateKey)
        {
            juce::Path keyCurve;
            for(size_t i=0;i<filterCurve.size();++i)
            {
                const float px=a.getX()+float(i)/float(filterCurve.size()-1)*a.getWidth();
                if(i==0) keyCurve.startNewSubPath(px,y(filterCurve[i]));else keyCurve.lineTo(px,y(filterCurve[i]));
            }
            const float dash[]={4,4};juce::Path dashed;
            juce::PathStrokeType(1.3f).createDashedStroke(dashed,keyCurve,dash,2);
            g.setColour(juce::Colour(0xffdca757));g.fillPath(dashed);
        }
        g.restoreState();
        drawText(g,"OUTPUT L / FFT",{a.getRight()-110,5,110,12},8,juce::Colour(0xff6f86a8),juce::Justification::centredRight);
        const char* names[]={"LF","LMF","HMF","HF","HP","LP"};
        for(int i=0;i<6;++i)
        {
            const auto pos=handle(i);
            const auto route=i>=4 ? controls.filterRoute : i==0 || i==3 ? controls.shelfRoute : controls.paramRoute;
            const bool key=route==model::EqRoute::CompKey || route==model::EqRoute::GateKey;
            const bool on=key || (controls.allEq && !controls.bypass && route==model::EqRoute::In);
            auto c=(key ? juce::Colour(0xffdca757) : i>=4 ? juce::Colour(0xff39d0ff) : colour(i)).withAlpha(on ? 1.0f : 0.28f);
            g.setColour(c.withMultipliedAlpha(0.18f)); g.fillEllipse(pos.x-11,pos.y-11,22,22);
            g.setColour(c); g.drawEllipse(pos.x-5,pos.y-5,10,10,2);
            const auto title=juce::String(names[i])+(key ? (route==model::EqRoute::CompKey ? " / COMP" : " / GATE") : "");
            const float labelX=juce::jlimit(2.0f,float(getWidth())-84,pos.x-40);
            drawText(g,title,{labelX,pos.y+(i%2 ? 12 : -25),80,13},9,c);
        }
    }
    void mouseDown(const juce::MouseEvent& e) override
    {
        float best=22; active=-1;
        for(int i=0;i<6;++i) if(handle(i).getDistanceFrom(e.position)<best) {best=handle(i).getDistanceFrom(e.position);active=i;}
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        namespace id=bluechannel::params::id;
        if(active<0) return;
        const double f=std::clamp(freq(e.position.x),20.0,20000.0), gain=db(e.position.y);
        if(active>=4) {params.edit(active==4 ? id::hpf : id::lpf,f);refresh();return;}
        const char* gains[]={id::lfGain,id::lmfGain,id::hmfGain,id::hfGain};
        params.edit(gains[active],gain);
        if(active==1 || active==2) params.edit(active==1 ? id::lmfFreq : id::hmfFreq,f/((active==1 ? controls.lmfX3 : controls.hmfX3) ? 3 : 1));
        else
        {
            int best=0; double distance=1e10;
            for(int i=0;i<4;++i) {const double d=std::abs(std::log(f/shelfHz(active==0,i))); if(d<distance) {distance=d;best=i;}}
            params.edit(active==0 ? id::lfFreq : id::hfFreq,best);
        }
        refresh();
    }
    void mouseUp(const juce::MouseEvent&) override { params.release(); active=-1; }
    void mouseWheelMove(const juce::MouseEvent& e,const juce::MouseWheelDetails& w) override
    {
        mouseDown(e);
        if(active==1 || active==2) {auto id=active==1 ? bluechannel::params::id::lmfQ : bluechannel::params::id::hmfQ;params.edit(id,params.get(id)+w.deltaY);params.release();refresh();}
        active=-1;
    }
};

class DigitalCompGraph : public juce::Component
{
    GraphParameters params;
    int active=-1;
    juce::Rectangle<float> area() const {return getLocalBounds().toFloat().reduced(30,25);}
    float x(double d) const {auto r=area();return r.getX()+float((d+40)/60)*r.getWidth();}
    float y(double d) const {auto r=area();return r.getBottom()-float((d+40)/60)*r.getHeight();}
    double level(float py) const {auto r=area();return (r.getBottom()-py)/r.getHeight()*60-40;}
    double output(double in) const
    {
        namespace id=bluechannel::params::id;
        const int type=int(params.get(id::compType)); const bool limit=type==2;
        double ratio=params.get(id::ratio);
        if(type) ratio=limit ? 5+(ratio-1.5)/8.5*15 : 1.5+(ratio-1.5)/8.5*3.5;
        return in-dynamics::compressionDb(in,params.get(id::threshold)+(limit ? 4 : 0),ratio,type ? (limit ? 2 : 12) : 10)+params.get(id::makeup);
    }
    juce::Point<float> handle(int i) const
    {
        namespace id=bluechannel::params::id;
        const double t=params.get(id::threshold)+(int(params.get(id::compType))==2 ? 4 : 0);
        const double in=i==0 ? t : i==1 ? std::min(18.0,t+18) : std::max(-38.0,t-14);
        return {x(in),y(output(in))};
    }
public:
    explicit DigitalCompGraph(BlueChannelProcessor& p) : params(p.apvts) {setTitle("Compressor detector target: drag threshold and ratio");}
    void paint(juce::Graphics& g) override
    {
        auto a=area();g.setColour(juce::Colour(0xff050a12));g.fillRoundedRectangle(getLocalBounds().toFloat(),4);
        for(int d : {-40,-20,0,20})
        {
            g.setColour(juce::Colour(0xff3a5679).withAlpha(0.3f)); g.drawVerticalLine(int(x(d)),a.getY(),a.getBottom());g.drawHorizontalLine(int(y(d)),a.getX(),a.getRight());
            drawText(g,juce::String(d),{x(d)-15,a.getBottom()+5,30,12},9,juce::Colour(0xff6f86a8));
            drawText(g,juce::String(d),{0,y(d)-6,26,12},9,juce::Colour(0xff6f86a8));
        }
        g.saveState();g.reduceClipRegion(a.toNearestInt());
        g.setColour(juce::Colour(0xff3a5679));g.drawLine(x(-40),y(-40),x(20),y(20),1);
        juce::Path p; for(int i=-40;i<=20;++i) {if(i==-40)p.startNewSubPath(x(i),y(output(i)));else p.lineTo(x(i),y(output(i)));}
        g.setColour(juce::Colour(0xff39d0ff));g.strokePath(p,juce::PathStrokeType(2));g.restoreState();
        for(int i=0;i<3;++i)
        {
            auto h=handle(i);auto c=juce::Colour(i==0 ? 0xff39d0ff : i==1 ? 0xffffc04a : 0xff5ae07a);
            g.setColour(c);g.fillEllipse(h.x-5,h.y-5,10,10);
            const char* names[]={"THRESH","RATIO","MAKEUP"};
            drawText(g,names[i],{h.x-27,h.y+(i==1 ? -19.0f : 9.0f),54,12},8,c);
        }
        drawText(g,"DETECTOR TARGET / dBu",{30,5,a.getWidth(),12},8,juce::Colour(0xff6f86a8),juce::Justification::centredLeft);
    }
    void mouseDown(const juce::MouseEvent& e) override {active=-1;for(int i=0;i<3;++i) if(handle(i).getDistanceFrom(e.position)<20) active=i;}
    void mouseDrag(const juce::MouseEvent& e) override
    {
        namespace id=bluechannel::params::id;
        if(active==0) {auto a=area();params.edit(id::threshold,(e.position.x-a.getX())/a.getWidth()*60-40-(int(params.get(id::compType))==2 ? 4 : 0));}
        if(active==1)
        {
            const int type=int(params.get(id::compType));const double t=params.get(id::threshold)+(type==2 ? 4 : 0);
            const double run=std::min(18.0,t+18)-t, rise=level(e.position.y)-params.get(id::makeup)-t;
            double ratio=run/std::max(0.1,rise);
            if(type) ratio=type==2 ? 1.5+(ratio-5)/15*8.5 : 1.5+(ratio-1.5)/3.5*8.5;
            params.edit(id::ratio,ratio);
        }
        if(active==2)
        {
            const double t=params.get(id::threshold)+(int(params.get(id::compType))==2 ? 4 : 0);
            const double in=std::max(-38.0,t-14);
            params.edit(id::makeup,params.get(id::makeup)+level(e.position.y)-output(in));
        }
        repaint();
    }
    void mouseUp(const juce::MouseEvent&) override {params.release();active=-1;}
};
}
