// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#include "Plugin/HQRenderer.h"
#include "Plugin/PluginProcessor.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <iostream>
#include <chrono>
#include <numeric>

static int hostStartupCheck()
{
    using namespace bluechannel;
    // Exercise the actual host wrapper, including float/double buffers and the non-realtime policy.
    {
        BlueChannelProcessor processor;
        processor.setPlayConfigDetails(1,1,48000,8);
        auto* hqParameter=processor.apvts.getParameter(params::id::hq);
        hqParameter->setValueNotifyingHost(1);
        processor.prepareToPlay(48000,8);
        processor.setNonRealtime(true);
        juce::AudioBuffer<float> buffer(1,8);
        for(int i=0;i<8;++i) buffer.setSample(0,i,float(.02*std::sin(2*3.14159265358979*1000*(i+1)/48000)));
        juce::MidiBuffer midi;
        processor.processBlock(buffer,midi);
        if(processor.hqStatus.load()!=hq::Renderer::Solving)
        {std::cerr<<processor.hqLastError()<<'\n';return 6;}
        juce::AudioBuffer<double> doubles(1,8);
        for(int i=0;i<8;++i) doubles.setSample(0,i,.02*std::sin(2*3.14159265358979*1000*(i+9)/48000));
        processor.processBlock(doubles,midi);
        if(processor.hqStatus.load()!=hq::Renderer::Solving)
        {std::cerr<<processor.hqLastError()<<'\n';return 7;}
        for(int i=0;i<8;++i) if(!std::isfinite(doubles.getSample(0,i))) return 8;
    }
    return 0;
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    using namespace bluechannel;
    if (!hq::Renderer::compiled()) { std::cout << "HQ not compiled\n"; return 0; }
    if(argc>1 && std::string(argv[1])=="--startup")return hostStartupCheck();
    const auto start=std::chrono::steady_clock::now();
    constexpr int n=16;
    model::Controls controls; controls.source=model::Source::Line;
    model::Telemetry telemetry;
    hq::Renderer whole, split;
    whole.prepare(48000,n,1); split.prepare(48000,n,1);
    std::array<double,n> input {}, a {}, b {};
    for(int i=0;i<n;++i) input[size_t(i)]=.02*std::sin(2*3.14159265358979*1000*(i+1)/48000);
    a=input; b=input;
    double* ap[]={a.data()};
    if (!whole.render(ap,n,nullptr,0,controls,4,telemetry)) {std::cerr<<whole.error()<<'\n';return 1;}
    for(int offset : {0,8})
    {
        double* bp[]={b.data()+offset};
        if (!split.render(bp,8,nullptr,0,controls,4,telemetry)) {std::cerr<<split.error()<<'\n';return 1;}
    }
    double residual=0, peak=0;
    for(int i=0;i<n;++i)
    {
        if (!std::isfinite(a[size_t(i)]) || !std::isfinite(b[size_t(i)])) return 2;
        residual=std::max(residual,std::abs(a[size_t(i)]-b[size_t(i)]));
        peak=std::max(peak,std::abs(a[size_t(i)]));
    }
    std::cout<<"peak="<<peak<<" block_residual="<<residual<<'\n';
    if (peak<1e-7 || peak>1 || residual>1e-4) return 3;
    // Restart must reproduce the same operating point and transient, without retaining the previous bounce.
    whole.restart(); a=input;
    if (!whole.render(ap,n,nullptr,0,controls,4,telemetry)) {std::cerr<<whole.error()<<'\n';return 1;}
    for(int i=0;i<n;++i) if(std::abs(a[size_t(i)]-b[size_t(i)])>1e-4) return 4;
    // Host controls change within the ongoing solve (no reload); the output must remain finite.
    std::cout<<"input source restart check"<<std::endl;
    controls.source=model::Source::Instrument; controls.trimDb=6; controls.phase=true;
    controls.allEq=true; controls.filterRoute=model::EqRoute::CompKey;
    controls.paramRoute=model::EqRoute::In; controls.lmfDb=3;
    a=input;
    if (!whole.render(ap,n,nullptr,0,controls,4,telemetry)) {std::cerr<<whole.error()<<'\n';return 1;}
    for(auto v:a) if(!std::isfinite(v)) return 5;
    if(argc>1 && std::string(argv[1])=="--extended")
    {
        // Every audio nonlinear family is present even when switched out. Exercise the gain-control ports.
        controls.compIn=controls.gateIn=controls.deessIn=controls.softLimit=true;
        controls.compThresholdDb=-28; controls.gateThresholdDb=-20; controls.deessThresholdDb=-30;
        std::cout<<"dynamics control check"<<std::endl;
        for(int block=0;block<8;++block)
        {
            a=input;
            if (!whole.render(ap,n,nullptr,0,controls,4,telemetry)) {std::cerr<<whole.error()<<'\n';return 1;}
        }
        std::cout<<"microphone source check"<<std::endl;
        controls.source=model::Source::Mic;
        a=input;
        if (!whole.render(ap,n,nullptr,0,controls,4,telemetry)) {std::cerr<<whole.error()<<'\n';return 1;}
    }
    if(auto result=hostStartupCheck())return result;
    std::cout<<"HQ solve, isolated instances, continuation, automation and restart passed; seconds="
        <<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<'\n';
}
