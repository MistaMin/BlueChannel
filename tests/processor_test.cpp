// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
//
// Host-level checks of the real plugin processor: bus layouts, state round trip and migration rules,
// malformed state, programs, editor lifetime, offline rendering and odd block sizes.

#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <juce_audio_utils/juce_audio_utils.h>

#include <cstdio>
#include <memory>

static int failures = 0;
static void check(bool ok, const char* what)
{
    std::printf("%-64s %s\n", what, ok ? "ok" : "FAIL");
    failures += !ok;
}

static void setParam(BlueChannelProcessor& p, const char* id, float value)
{
    auto* r = p.apvts.getParameter(id);
    r->setValueNotifyingHost(r->convertTo0to1(value));
}
static float getParam(BlueChannelProcessor& p, const char* id) { return p.apvts.getRawParameterValue(id)->load(); }

static void fillNoise(juce::AudioBuffer<float>& b, int seed)
{
    juce::Random r(seed);
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i) b.setSample(ch, i, (r.nextFloat() * 2.0f - 1.0f) * 0.2f);
}

static bool finite(const juce::AudioBuffer<float>& b)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i)
            if (!std::isfinite(b.getSample(ch, i))) return false;
    return true;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    namespace id = bluechannel::params::id;

    // Layouts.
    {
        BlueChannelProcessor p;
        juce::AudioProcessor::BusesLayout stereo, mono, bad;
        stereo.inputBuses.add(juce::AudioChannelSet::stereo());
        stereo.inputBuses.add(juce::AudioChannelSet::stereo());
        stereo.outputBuses.add(juce::AudioChannelSet::stereo());
        mono.inputBuses.add(juce::AudioChannelSet::mono());
        mono.inputBuses.add(juce::AudioChannelSet::disabled());
        mono.outputBuses.add(juce::AudioChannelSet::mono());
        bad.inputBuses.add(juce::AudioChannelSet::mono());
        bad.outputBuses.add(juce::AudioChannelSet::stereo());
        check(p.checkBusesLayoutSupported(stereo), "stereo with stereo sidechain supported");
        check(p.checkBusesLayoutSupported(mono), "mono without sidechain supported");
        check(!p.checkBusesLayoutSupported(bad), "mono in / stereo out rejected");
    }

    // State round trip, partial state and malformed state.
    {
        BlueChannelProcessor a;
        setParam(a, id::lmfGain, 7.5f);
        setParam(a, id::source, 0);
        setParam(a, id::compIn, 1);
        setParam(a, id::threshold, -12.0f);
        a.setSkin(1);
        juce::MemoryBlock blob;
        a.getStateInformation(blob);

        BlueChannelProcessor b;
        b.setStateInformation(blob.getData(), int(blob.getSize()));
        check(std::abs(getParam(b, id::lmfGain) - 7.5f) < 1e-3f && getParam(b, id::source) == 0.0f
                  && getParam(b, id::compIn) > 0.5f && std::abs(getParam(b, id::threshold) + 12.0f) < 1e-3f,
              "state round trip restores parameters");
        check(b.skin() == 1, "state round trip restores editor skin");

        // A state without the threshold entry restores that parameter to its default, even into a modified instance.
        auto xml = juce::AudioProcessor::getXmlFromBinary(blob.getData(), int(blob.getSize()));
        for (auto* child = xml->getFirstChildElement(); child != nullptr; child = child->getNextElement())
            if (child->getStringAttribute("id") == id::threshold) { xml->removeChildElement(child, true); break; }
        juce::MemoryBlock partial;
        juce::AudioProcessor::copyXmlToBinary(*xml, partial);
        BlueChannelProcessor c;
        setParam(c, id::threshold, 5.0f);
        c.setStateInformation(partial.getData(), int(partial.getSize()));
        check(std::abs(getParam(c, id::threshold) - 0.0f) < 1e-3f, "missing parameter restores to default");

        const char junk[] = "not a state";
        setParam(c, id::hmfGain, 3.0f);
        c.setStateInformation(junk, int(sizeof(junk)));
        check(std::abs(getParam(c, id::hmfGain) - 3.0f) < 1e-3f, "malformed state ignored, current state kept");
    }

    // Processing: stereo with sidechain, odd and oversized blocks, editor open and closed, programs.
    {
        BlueChannelProcessor p;
        p.setPlayConfigDetails(4, 2, 48000.0, 256);
        p.prepareToPlay(48000.0, 256);
        juce::MidiBuffer midi;
        bool ok = true;
        for (int blk : { 256, 1, 17, 129, 1024 })   // 1024 exceeds the announced size
        {
            juce::AudioBuffer<float> buf(4, blk);
            fillNoise(buf, blk);
            p.processBlock(buf, midi);
            ok = ok && finite(buf);
        }
        check(ok, "finite output for 1, 17, 129, 256 and oversized blocks");

        for (int prog = 0; prog < p.getNumPrograms(); ++prog)
        {
            p.setCurrentProgram(prog);
            juce::AudioBuffer<float> buf(4, 256);
            fillNoise(buf, prog);
            p.processBlock(buf, midi);
            ok = ok && finite(buf);
        }
        check(ok, "every factory program processes finite audio");

        std::unique_ptr<juce::AudioProcessorEditor> ed(p.createEditor());
        juce::AudioBuffer<float> buf(4, 256);
        fillNoise(buf, 3);
        p.processBlock(buf, midi);
        ed.reset();
        p.processBlock(buf, midi);
        check(finite(buf), "editor open and close around processing");
    }

    // Offline rendering at a different factor is deterministic across repeated bounces.
    {
        BlueChannelProcessor p;
        p.setPlayConfigDetails(2, 2, 44100.0, 512);
        setParam(p, id::offlineOversampling, 4);   // 8x
        setParam(p, id::compIn, 1);
        p.prepareToPlay(44100.0, 512);
        juce::MidiBuffer midi;
        juce::AudioBuffer<float> first, second;
        for (auto* out : { &first, &second })
        {
            juce::AudioBuffer<float> rt(2, 512);
            fillNoise(rt, 11);
            p.setNonRealtime(false);
            p.processBlock(rt, midi);
            p.setNonRealtime(true);
            juce::AudioBuffer<float> buf(2, 4096);
            fillNoise(buf, 5);
            for (int s = 0; s < 4096; s += 512)
            {
                juce::AudioBuffer<float> view(buf.getArrayOfWritePointers(), 2, s, 512);
                p.processBlock(view, midi);
            }
            *out = buf;
        }
        float d = 0;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 4096; ++i) d = std::max(d, std::abs(first.getSample(ch, i) - second.getSample(ch, i)));
        check(d == 0.0f, "repeated offline bounces are identical");
        check(p.telemetry().activeFactor.load() == 8, "offline render runs at the offline factor (8x)");
    }

    std::printf("%s (%d failures)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
