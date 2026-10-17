// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

// Panel value <-> component value conversions, built on the tables that tests/calibrate.cpp measured from the
// stage netlists. Knobs are calibrated in what the transcribed circuit actually does.

#include "Calibration.h"

#include <algorithm>
#include <cmath>

namespace bluechannel::model::controlmap
{
using namespace calibration;

// Pot position (0..1) where a monotone table (sampled at kPotSteps even positions) reaches value v.
template <int N>
inline double positionFor(const double (&t)[N], double v)
{
    const bool rising = t[N - 1] > t[0];
    if (rising ? v <= t[0] : v >= t[0]) return 0.0;
    if (rising ? v >= t[N - 1] : v <= t[N - 1]) return 1.0;
    for (int i = 1; i < N; ++i)
        if (rising ? t[i] >= v : t[i] <= v)
        {
            const double f = (v - t[i - 1]) / (t[i] - t[i - 1]);
            return (double(i - 1) + f) / double(N - 1);
        }
    return 1.0;
}

// Table value at pot position p (linear interpolation).
template <int N>
inline double valueAt(const double (&t)[N], double p)
{
    const double x = std::clamp(p, 0.0, 1.0) * double(N - 1);
    const int i = std::min(N - 2, int(x));
    return t[i] + (x - double(i)) * (t[i + 1] - t[i]);
}

// Frequency tables are interpolated in log frequency.
template <int N>
inline double positionForHz(const double (&t)[N], double hz)
{
    double lg[N];
    for (int i = 0; i < N; ++i) lg[i] = std::log(t[i]);
    return positionFor(lg, std::log(hz));
}
template <int N>
inline double hzAt(const double (&t)[N], double p)
{
    double lg[N];
    for (int i = 0; i < N; ++i) lg[i] = std::log(t[i]);
    return std::exp(valueAt(lg, p));
}

template <int N> inline double minOf(const double (&t)[N]) { return *std::min_element(t, t + N); }
template <int N> inline double maxOf(const double (&t)[N]) { return *std::max_element(t, t + N); }

// Input
inline int micCircuitPosition(int uiIndex) { return 11 - std::clamp(uiIndex, 0, 11); }   // UI ascending
inline double micGainDb(int uiIndex) { return kMicGainDb[micCircuitPosition(uiIndex)]; }
// Software unity detent: retain the twelve switch indices; replace the old near-unity (-2 dB) label.
// Nominal whole-channel calibration at 1 kHz, Classic impedance and 150-ohm source, trim/output at zero.
inline constexpr int kMicUnityIndex = 1;
inline constexpr double kMicUnityCorrectionDb = 2.754406;
inline double micUnityCorrectionDb(int uiIndex) { return uiIndex == kMicUnityIndex ? kMicUnityCorrectionDb : 0.0; }
inline double micUnityScale(int uiIndex) { return std::pow(10.0, micUnityCorrectionDb(uiIndex) / 20.0); }
inline double lineGainDb(int uiIndex) { return kLineGainDb[std::clamp(uiIndex, 0, 6)]; }
inline double trimPot(double db) { return positionFor(kTrimDb, db); }
inline double trimMaxDb() { return kTrimDb[kPotSteps - 1]; }

// Instrument stage gain at 1 kHz versus its trim gang (fitted from ngspice, tools/characterize.py inst).
// The gang tracks the mic/line trim: maximum instrument gain at maximum trim (see docs/CIRCUIT_MODEL.md).
inline constexpr double kInstGangPos[] = { 0, 0.05, 0.1, 0.15, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0 };
inline constexpr double kInstGainDb[] = { 19.5562, 17.1122, 15.4034, 14.1188, 13.1077, 11.6022, 10.5240,
                                          9.7080, 9.0664, 8.5473, 8.1180, 7.7566, 7.4479 };
inline double instrumentStageDb(double trimPotPos)
{
    const double g = 1.0 - std::clamp(trimPotPos, 0.0, 1.0);   // gang position in the reference model
    int i = 1;
    while (i < 12 && kInstGangPos[i] < g) ++i;
    const double t = (g - kInstGangPos[i - 1]) / (kInstGangPos[i] - kInstGangPos[i - 1]);
    return kInstGainDb[i - 1] + t * (kInstGainDb[i] - kInstGainDb[i - 1]);
}
inline double instrumentGangPosition(double trimPotPos) { return 1.0 - std::clamp(trimPotPos, 0.0, 1.0); }

// Mic input impedance settings scale the secondary load ladder (33k nominal = 1.32k reflected, "Classic").
inline double impedanceScale(int setting)
{
    static constexpr double ohms[] = { 600.0, 1400.0, 2400.0, 6800.0 };
    return ohms[std::clamp(setting, 0, 3)] / 1400.0;
}

// Filters: corner = constant / (R + 620), pot 0..47k.
inline constexpr double kFilterPotMax = 47000.0;
inline double lowPassOhms(double hz) { return std::clamp(kLowPassConstant / hz - 620.0, 0.0, kFilterPotMax); }
inline double highPassOhms(double hz) { return std::clamp(kHighPassConstant / hz - 620.0, 0.0, kFilterPotMax); }
inline double lowPassMinHz() { return kLowPassConstant / (kFilterPotMax + 620.0); }
inline double lowPassMaxHz() { return kLowPassConstant / 620.0; }
inline double highPassMinHz() { return kHighPassConstant / (kFilterPotMax + 620.0); }
inline double highPassMaxHz() { return kHighPassConstant / 620.0; }

// Shelves: 4-position switch + range button -> circuit position (see kShelves).
inline int lowShelfPosition(int sw, bool high)
{
    static constexpr int lo[] = { 6, 1, 3, 5 }, hi[] = { 0, 2, 4, 7 };
    return (high ? hi : lo)[std::clamp(sw, 0, 3)];
}
inline int highShelfPosition(int sw, bool high)
{
    static constexpr int lo[] = { 6, 0, 2, 4 }, hi[] = { 7, 1, 3, 5 };
    return (high ? hi : lo)[std::clamp(sw, 0, 3)];
}
inline double lowShelfPot(double db) { return positionFor(kLowShelfDb, db); }
inline double highShelfPot(double db) { return positionFor(kHighShelfDb, db); }

// Parametric bands: the frequency pot is shared by both ranges; the x1 legend is the control value.
inline double lmfFreqPot(double hz) { return positionForHz(kLmfFreqHz, hz); }
inline double hmfFreqPot(double hz) { return positionForHz(kHmfFreqHz, hz); }
inline double lmfHz(double pot, bool x3) { return x3 ? hzAt(kLmfFreqX3Hz, pot) : hzAt(kLmfFreqHz, pot); }
inline double hmfHz(double pot, bool x3) { return x3 ? hzAt(kHmfFreqX3Hz, pot) : hzAt(kHmfFreqHz, pot); }
inline double lmfGainPot(double db) { return positionFor(kLmfGainDb, db); }
inline double hmfGainPot(double db) { return positionFor(kHmfGainDb, db); }
inline double lmfQPot(double q) { return positionFor(kLmfQ, q); }
inline double hmfQPot(double q) { return positionFor(kHmfQ, q); }

// Compressor make-up, de-esser frequency, output fader.
inline double makeupPot(double db) { return positionFor(kMakeupDb, db); }
inline double deessFreqPot(double hz) { return positionForHz(kDeessFreqHz, hz); }

// Output control: panel dB is the overall line-in to line-out gain contributed by the output section
// (fader + output transformer + line receiver), so 0 dB is unity with the line input at 0 dB.
inline double outputOffsetDb() { return kOutputDb + kLineInputDb; }
inline double faderTrack(double panelDb)
{
    const double p = positionFor(kFaderDb, panelDb - outputOffsetDb());
    return p * p * p;
}
inline double outputMaxDb() { return kFaderDb[kPotSteps - 1] + outputOffsetDb(); }
} // namespace bluechannel::model::controlmap
