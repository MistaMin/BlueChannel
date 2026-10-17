// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

// LED / light-dependent-resistor cells.
//
// The reference model gives estimated laws (no measured part data was available): LED current is normalised
// to 1 mA, the illumination state follows it with first-order attack and release, and the resistance is a
// function of the state. Both laws are inverted here so a detector can ask for the current that produces a
// wanted resistance.
//
//   Limiter cell:  R = Rmin + (Rdark - Rmin) / (1 + (s * 1e5)^1.08)        458 ohm at 1 mA, 76 ohm at 20 mA
//   De-esser cell: R = Rmin + (Rdark - Rmin) / (1 + (Rdark - Rmin) / R1mA * s^0.9)
//
// with Rdark = 100 Mohm, Rmin = 60 ohm, R1mA = 500 ohm, attack 2 ms and release 20 ms.

#include <algorithm>
#include <cmath>

namespace bluechannel::model
{
struct OpticalLaw
{
    enum class Kind { Limiter, Deesser } kind = Kind::Limiter;
    double rdark = 100e6, rmin = 60.0, r1ma = 500.0;

    double resistance(double s) const
    {
        s = std::max(0.0, s);
        if (kind == Kind::Limiter) return rmin + (rdark - rmin) / (1.0 + std::pow(s * 1e5, 1.08));
        return rmin + (rdark - rmin) / (1.0 + (rdark - rmin) / r1ma * std::pow(s, 0.9));
    }

    // Illumination state (LED current in mA, steady state) that gives resistance r.
    double stateFor(double r) const
    {
        if (r >= rdark) return 0.0;
        r = std::max(r, rmin * 1.0001);
        const double q = (rdark - rmin) / (r - rmin) - 1.0;
        if (q <= 0.0) return 0.0;
        if (kind == Kind::Limiter) return std::pow(q, 1.0 / 1.08) / 1e5;
        return std::pow(q * r1ma / (rdark - rmin), 1.0 / 0.9);
    }
};

class OpticalCell
{
public:
    void setLaw(OpticalLaw l) { law = l; }
    const OpticalLaw& getLaw() const { return law; }

    void prepare(double fs, double attackS = 2e-3, double releaseS = 20e-3)
    {
        att = 1.0 - std::exp(-1.0 / (attackS * fs));
        rel = 1.0 - std::exp(-1.0 / (releaseS * fs));
    }
    void reset() { state = 0.0; }

    // LED current in amperes; returns the resistance after this sample.
    double step(double ledAmps)
    {
        const double illum = std::max(0.0, ledAmps) * 1000.0;
        state += (illum - state) * (illum > state ? att : rel);
        return law.resistance(state);
    }
    double resistance() const { return law.resistance(state); }
    double illumination() const { return state; }

private:
    OpticalLaw law;
    double state = 0.0, att = 0.0, rel = 0.0;
};
} // namespace bluechannel::model
