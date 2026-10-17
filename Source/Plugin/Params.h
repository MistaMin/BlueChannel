// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

// Parameter registry. IDs, order, ranges and choice lists are part of saved sessions and automation: never
// change the meaning of an existing ID, append new parameters instead (docs/PARAMETERS.md).

#include "Model/Controls.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace bluechannel::params
{
inline constexpr int kSchemaVersion = 1;

namespace id
{
// Input
inline constexpr const char* source = "source";
inline constexpr const char* micGain = "micgain";
inline constexpr const char* lineGain = "linegain";
inline constexpr const char* trim = "trim";
inline constexpr const char* phase = "phase";
inline constexpr const char* impedance = "impedance";
inline constexpr const char* air = "air";
// Equaliser
inline constexpr const char* allEq = "alleq";
inline constexpr const char* filterRoute = "filtroute";
inline constexpr const char* hpf = "hpf";
inline constexpr const char* lpf = "lpf";
inline constexpr const char* shelfRoute = "shelfroute";
inline constexpr const char* lfGain = "lfgain";
inline constexpr const char* lfFreq = "lffreq";
inline constexpr const char* lfHigh = "lfhigh";
inline constexpr const char* hfGain = "hfgain";
inline constexpr const char* hfFreq = "hffreq";
inline constexpr const char* hfHigh = "hfhigh";
inline constexpr const char* paramRoute = "paramroute";
inline constexpr const char* lmfGain = "lmfgain";
inline constexpr const char* lmfFreq = "lmffreq";
inline constexpr const char* lmfQ = "lmfq";
inline constexpr const char* lmfX3 = "lmfx3";
inline constexpr const char* hmfGain = "hmfgain";
inline constexpr const char* hmfFreq = "hmffreq";
inline constexpr const char* hmfQ = "hmfq";
inline constexpr const char* hmfX3 = "hmfx3";
// Compressor
inline constexpr const char* compIn = "compin";
inline constexpr const char* compType = "comptype";
inline constexpr const char* ratio = "ratio";
inline constexpr const char* threshold = "threshold";
inline constexpr const char* attack = "attack";
inline constexpr const char* release = "release";
inline constexpr const char* autoRelease = "autorel";
inline constexpr const char* makeup = "makeup";
inline constexpr const char* compKey = "compkey";
inline constexpr const char* blend = "blend";
inline constexpr const char* mix = "mix";
inline constexpr const char* compListen = "complisten";
// Gate
inline constexpr const char* gateIn = "gatein";
inline constexpr const char* expand = "expand";
inline constexpr const char* gateThreshold = "gatethresh";
inline constexpr const char* range = "range";
inline constexpr const char* hold = "hold";
inline constexpr const char* gateRelease = "gaterelease";
inline constexpr const char* fastAttack = "fastatt";
inline constexpr const char* hysteresis = "hyst";
inline constexpr const char* gateKey = "gatekey";
inline constexpr const char* gateListen = "gatelisten";
// De-esser
inline constexpr const char* deessIn = "deessin";
inline constexpr const char* deessThreshold = "deessthresh";
inline constexpr const char* deessFreq = "deessfreq";
inline constexpr const char* deessListen = "deesslisten";
// Routing and output
inline constexpr const char* dynPosition = "dynpos";
inline constexpr const char* bypass = "bypass";
inline constexpr const char* output = "output";
inline constexpr const char* softLimit = "softlimit";
inline constexpr const char* mute = "mute";
inline constexpr const char* link = "link";
// Processing
inline constexpr const char* oversampling = "oversampling";
inline constexpr const char* offlineOversampling = "offlineos";
inline constexpr const char* hq = "hq"; // appended: offline component solve
} // namespace id

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

// Cached raw parameter pointers for the audio thread.
class Reader
{
public:
    explicit Reader(juce::AudioProcessorValueTreeState& state);
    void read(model::Controls& c) const;
    bool hqEnabled() const;
    int playbackStages() const;
    int offlineStages() const;   // -1 = same as playback

private:
    juce::AudioProcessorValueTreeState& apvts;
    std::vector<std::pair<const char*, std::atomic<float>*>> cache;
    std::atomic<float>* get(const char* name) const;
};

// Text helpers shared by the editor.
juce::String micGainLabel(int index);
juce::String lineGainLabel(int index);
juce::String lowShelfLabel(int position, bool high);
juce::String highShelfLabel(int position, bool high);
juce::String hzText(double hz);
} // namespace bluechannel::params
