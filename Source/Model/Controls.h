// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

// Front-panel state in panel units. The plugin wrapper fills this from host parameters once per block; the
// engine never reads host parameter objects. Ranges are documented with the parameter registry
// (Source/Plugin/Params.cpp, docs/PARAMETERS.md).

namespace bluechannel::model
{
enum class Source { Mic = 0, Line = 1, Instrument = 2 };
enum class EqRoute { Out = 0, In = 1, CompKey = 2, GateKey = 3 };
enum class CompType { Vca = 0, OptoComp = 1, OptoLimit = 2 };
enum class DynPosition { PostEq = 0, PreEq = 1 };

struct Controls
{
    // Input
    Source source = Source::Line;
    int micGain = 8;            // 0..11, ascending gain (-14 .. +54 dB)
    int lineGain = 3;           // 0..6, ascending gain (-18 .. +18 dB)
    double trimDb = 0.0;        // 0 .. 20.8 dB (mic/line); the same pot sets instrument gain
    bool phase = false;
    int impedance = 1;          // 0 Low, 1 Classic, 2 Medium, 3 High
    bool air = false;
    double sourceOhms = 150.0;  // microphone source impedance assumed by the input model

    // Equaliser
    bool allEq = true;
    EqRoute filterRoute = EqRoute::Out;
    double hpfHz = 14.3, lpfHz = 37000.0;
    EqRoute shelfRoute = EqRoute::Out;
    double lfDb = 0.0, hfDb = 0.0;
    int lfFreq = 1, hfFreq = 1;  // 0..3 switch position
    bool lfHigh = false, hfHigh = false;
    EqRoute paramRoute = EqRoute::Out;
    double lmfDb = 0.0, lmfHz = 120.0, lmfQ = 1.2;
    bool lmfX3 = false;
    double hmfDb = 0.0, hmfHz = 1800.0, hmfQ = 1.2;
    bool hmfX3 = false;

    // Compressor
    bool compIn = false;
    CompType compType = CompType::Vca;
    double ratio = 3.0, compThresholdDb = 0.0, attackMs = 10.0, releaseS = 0.3;
    bool autoRelease = false;
    double makeupDb = 0.0;
    bool compExtKey = false, blend = false;
    double mix = 1.0;           // 0 = dry, 1 = compressed (active with blend)
    bool compListen = false;

    // Gate / expander
    bool gateIn = false, expand = false;
    double gateThresholdDb = -40.0, rangeDb = 40.0, holdS = 0.1, gateReleaseS = 0.5;
    bool fastAttack = false, hysteresis = true, gateExtKey = false, gateListen = false;

    // De-esser
    bool deessIn = false;
    double deessThresholdDb = 0.0, deessHz = 5000.0;
    bool deessListen = false;

    // Routing and output
    DynPosition dynPosition = DynPosition::PostEq;
    bool bypass = false;
    double outputDb = 0.0;      // -60 .. +6.6 dB
    bool softLimit = false;
    bool mute = false;
    bool link = true;           // stereo: compressor and gate respond to the louder channel

    bool operator==(const Controls&) const = default;
};
} // namespace bluechannel::model
