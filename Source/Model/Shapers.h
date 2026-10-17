// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

// Memoryless large-signal limits.

#include "Levels.h"

#include <algorithm>
#include <cmath>

namespace bluechannel::model
{
// Op-amp output swing: linear inside the rails, with a short quadratic knee (C1 continuous) of width w volts
// into a hard limit at hi / lo. The rails are asymmetric, so clipping adds even harmonics.
inline double railClip(double v, double hi = kSwingPositive, double lo = kSwingNegative, double w = 0.35)
{
    const double up = hi - w;
    if (v > up)
    {
        const double d = v - up;
        return d >= 2.0 * w ? hi : up + d - d * d / (4.0 * w);
    }
    const double dn = lo + w;
    if (v < dn)
    {
        const double d = dn - v;
        return d >= 2.0 * w ? lo : dn - d + d * d / (4.0 * w);
    }
    return v;
}

// Smooth saturation towards +pos / -neg (tanh shape on each side, unity slope at zero).
inline double softAsym(double v, double pos, double neg)
{
    return v >= 0.0 ? pos * std::tanh(v / pos) : -neg * std::tanh(-v / neg);
}

// One-pole high-pass / low-pass used for coupling networks that were simplified out of the netlists.
struct OnePole
{
    double a = 0.0, z = 0.0, x1 = 0.0;
    void setHighPass(double hz, double fs) { a = std::exp(-2.0 * 3.14159265358979 * hz / fs); }
    void setLowPass(double hz, double fs) { a = std::exp(-2.0 * 3.14159265358979 * hz / fs); }
    void reset() { z = x1 = 0.0; }
    double highPass(double x)
    {
        // y[n] = a (y[n-1] + x[n] - x[n-1])
        z = a * (z + x - x1);
        x1 = x;
        return z;
    }
    double lowPass(double x)
    {
        z = x + a * (z - x);
        return z;
    }
};
} // namespace bluechannel::model
