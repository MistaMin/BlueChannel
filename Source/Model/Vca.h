// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

// Discrete class-A VCA gain cell, reduced to its measured behaviour.
//
// The reference circuit is driven through its control port: a current sunk from the bender input. Gain at
// 1 kHz versus that current was measured with ngspice (tools/characterize.py vca-law, raw data in
// docs/measurements/characterisation/vca_law.csv). The cell is exponential: about -0.40 dB per uA from
// roughly 5 uA, with a soft start at zero current and a floor near -77 dB.
//
// Distortion of the cell stays below -73 dB up to 11 V peak input; the measurable part is a third harmonic
// that grows with level and control current (vca_thd.csv). It is modelled as y = g (x - k(I) x^3) with
// k(I) = 3e-9 * I^2 (I in uA, x in volts), which reproduces -84 dB at 25 uA and -73 dB at 50 uA, 11 V peak.
// Control feedthrough (a DC step with gain changes) is assumed trimmed out, as in a calibrated unit.

#include <algorithm>
#include <cmath>
#include <iterator>

namespace bluechannel::model
{
namespace vcalaw
{
inline constexpr double kMicroAmps[] = { 0, 2, 5, 10, 15, 20, 30, 40, 50, 60, 80, 100, 120, 150, 200, 250 };
inline constexpr double kGainDb[] = { 0.6655, -0.1052, -1.2714, -3.2256, -5.1945, -7.1702, -11.1423, -15.1246,
                                      -19.1165, -23.1169, -31.1670, -39.2851, -47.3951, -58.2816, -63.8288, -76.6740 };
inline constexpr int kPoints = int(std::size(kMicroAmps));
inline constexpr double kMaxMicroAmps = 250.0;
} // namespace vcalaw

// Cell gain (dB, includes the +0.67 dB the circuit has at zero control current) for a control current.
inline double vcaGainDb(double microAmps)
{
    using namespace vcalaw;
    microAmps = std::clamp(microAmps, 0.0, kMaxMicroAmps);
    int i = 1;
    while (i < kPoints - 1 && kMicroAmps[i] < microAmps) ++i;
    const double t = (microAmps - kMicroAmps[i - 1]) / (kMicroAmps[i] - kMicroAmps[i - 1]);
    return kGainDb[i - 1] + t * (kGainDb[i] - kGainDb[i - 1]);
}

// Control current that reduces the cell gain by grDb from its zero-current value.
inline double vcaCurrentForReduction(double grDb)
{
    using namespace vcalaw;
    const double target = kGainDb[0] - std::max(0.0, grDb);
    if (target >= kGainDb[0]) return 0.0;
    for (int i = 1; i < kPoints; ++i)
        if (kGainDb[i] <= target)
        {
            const double t = (kGainDb[i - 1] - target) / (kGainDb[i - 1] - kGainDb[i]);
            return kMicroAmps[i - 1] + t * (kMicroAmps[i] - kMicroAmps[i - 1]);
        }
    return kMaxMicroAmps;
}

inline double vcaCell(double x, double microAmps, double gainLinear)
{
    const double k = 3e-9 * microAmps * microAmps;
    return gainLinear * (x - k * x * x * x);
}
} // namespace bluechannel::model
