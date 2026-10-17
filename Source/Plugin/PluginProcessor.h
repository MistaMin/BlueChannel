// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

#include "Model/Engine.h"
#include "Params.h"

#include <juce_audio_processors/juce_audio_processors.h>

class BlueChannelProcessor : public juce::AudioProcessor
{
public:
    BlueChannelProcessor();
    ~BlueChannelProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock(juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    bool supportsDoublePrecisionProcessing() const override { return true; }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Blue Channel"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    // Editor-side state (not automatable, saved with the session).
    int skin() const { return int(apvts.state.getProperty("skin", 0)); }
    void setSkin(int s) { apvts.state.setProperty("skin", s, nullptr); }
    int vuSource() const { return int(apvts.state.getProperty("vusource", 0)); }
    void setVuSource(int s) { apvts.state.setProperty("vusource", s, nullptr); }
    bool vuHighCal() const { return bool(apvts.state.getProperty("vucal", false)); }
    void setVuHighCal(bool b) { apvts.state.setProperty("vucal", b, nullptr); }

    bluechannel::model::Telemetry& telemetry() { return engine.telemetry; }
    int activeChannels() const { return channelsActive.load(std::memory_order_relaxed); }
    bool sidechainConnected() const { return sidechainActive.load(std::memory_order_relaxed); }

    juce::UndoManager undo;
    juce::AudioProcessorValueTreeState apvts;

private:
    template <typename Sample>
    void process(juce::AudioBuffer<Sample>& buffer);

    bluechannel::params::Reader reader;
    bluechannel::model::Engine engine;
    bluechannel::model::Controls controls;
    std::atomic<int> channelsActive { 2 };
    std::atomic<bool> sidechainActive { false };
    int currentProgram = 0;
    double sampleRate_ = 48000.0;
    int maxBlock_ = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BlueChannelProcessor)
};
