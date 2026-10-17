// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

// Audio stages of one channel. Each wraps a linear DK stage built from Netlists.h and adds the large-signal
// behaviour that the linear netlist leaves out (op-amp swing, transformer core, measured device curves).
// All signals are volts at the circuit nodes. process() is real-time safe; configure() may recompile the
// DK coefficients (bounded work, no allocation) and should be called at control rate.

#include "Circuit/DkStage.h"
#include "Circuit/Netlists.h"
#include "ControlMap.h"
#include "Optical.h"
#include "Shapers.h"
#include "Vca.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace bluechannel::model
{
using circuit::DkStage;
namespace nl = circuit::netlists;

// Transformer core: the magnetising current is linear in the netlist; its non-linear excess is injected as
// a current at the core node from the flux of the previous sample (a small, delayed perturbation).
// i_x = (flux / Lm) * |flux / fluxRef|^p. For the input transformer p = 1.4 and fluxRef are fitted to the
// part's published distortion (0.2 % at 50 Hz, 0 dBu and 1 % at +10 dBu, 200 ohm source) by
// tests/model_test.cpp. The output transformer value is an estimate: its winding data is not published.
struct CoreFlux
{
    double lm = 5.0, fluxRef = 1.0, power = 1.4;
    double flux = 0.0, leak = 1.0, dt = 0.0;
    void prepare(double fs)
    {
        dt = 1.0 / fs;
        leak = std::exp(-2.0 * 3.14159265358979 * 0.5 / fs);   // 0.5 Hz leak keeps the integrator bounded
        flux = 0.0;
    }
    double current() const
    {
        const double r = std::abs(flux) / fluxRef;
        return -(flux / lm) * std::pow(r, power);
    }
    void advance(double coreVolts) { flux = flux * leak + coreVolts * dt; }
};

class MicStage
{
public:
    static constexpr double kFluxRef = 0.0183;   // fitted: BlueChannelModelTest fit

    void prepare(double fs)
    {
        if (dk.numInputs() == 0) load();
        sampleRate = fs;
        dk.setSampleRate(fs);
        core.lm = 5.0;
        core.fluxRef = kFluxRef;
        core.prepare(fs);
        reset();
    }
    void load()
    {
        dk.load(circuit::parseNetlist(nl::kMicInput));
        pGain = dk.param("gain"); pRsrc = dk.param("rsrc"); pZ = dk.param("zscale"); pAir = dk.param("air");
        dk.setParam(pZ, 1.0);
    }
    void configure(int uiGain, double sourceOhms, int impedance, bool air)
    {
        const int pos = controlmap::micCircuitPosition(uiGain);
        dk.setParam(pGain, pos);
        dk.setParam(pRsrc, sourceOhms);
        dk.setParam(pZ, controlmap::impedanceScale(impedance));
        dk.setParam(pAir, air ? 1.0 : 0.0);
        // Closed-loop bandwidth of the amplifier: gain-bandwidth over its noise gain (positions 7..11 use
        // the minimum-gain feedback tap).
        const double ng = std::max(1.0, std::pow(10.0, (calibration::kMicGainDb[std::min(pos, 6)] - 14.0) / 20.0));
        bandwidth.setLowPass(10e6 / ng, sampleRate);
        dk.update();
    }
    void reset() { dk.reset(); core.flux = 0.0; bandwidth.reset(); }
    void setCoreReference(double fluxRef) { core.fluxRef = fluxRef; }   // for fitting tools
    double process(double v)
    {
        const double u[2] = { v, core.current() };
        double y[2];
        dk.process(u, y);
        core.advance(y[1]);
        return railClip(bandwidth.lowPass(y[0]));
    }
    DkStage dk;

private:
    CoreFlux core;
    OnePole bandwidth;
    double sampleRate = 96000.0;
    int pGain = -1, pRsrc = -1, pZ = -1, pAir = -1;
};

class LineStage
{
public:
    void prepare(double fs)
    {
        if (receiver.numInputs() == 0)
        {
            receiver.load(circuit::parseNetlist(nl::kLineInput));
            gain.load(circuit::parseNetlist(nl::kLineGain));
            pGain = gain.param("gain");
        }
        receiver.setSampleRate(fs);
        gain.setSampleRate(fs);
        receiver.update();
        reset();
    }
    void configure(int uiGain)
    {
        gain.setParam(pGain, std::clamp(uiGain, 0, 6));
        gain.update();
    }
    void reset() { receiver.reset(); gain.reset(); }
    double process(double v) { return railClip(gain.process1(railClip(receiver.process1(v)))); }

private:
    DkStage receiver, gain;
    int pGain = -1;
};

// Discrete JFET / PNP instrument stage, reduced to curves fitted from ngspice (docs/CIRCUIT_MODEL.md):
// gain versus trim gang, the emitter-bypass bass shelf, a second-order term growing with gain squared, and
// an asymmetric output limit.
class InstrumentStage
{
public:
    void prepare(double fs)
    {
        sampleRate = fs;
        coupling.setHighPass(1.0, fs);
        reset();
    }
    void configure(double trimPotPos)
    {
        gainHigh = std::pow(10.0, controlmap::instrumentStageDb(trimPotPos) / 20.0);
        const double r = 120.0 + 1000.0 * controlmap::instrumentGangPosition(trimPotPos);
        bypass.setHighPass(1.0 / (2.0 * 3.14159265358979 * 47e-6 * r), sampleRate);
        a2 = 3.4e-4 * gainHigh * gainHigh;
    }
    void reset() { bypass.reset(); coupling.reset(); }
    double process(double v)
    {
        const double x = v + a2 * v * v;
        const double y = kGainUnbypassed * x + (gainHigh - kGainUnbypassed) * bypass.highPass(x);
        return railClip(coupling.highPass(softAsym(y, 6.0, 10.0)));
    }

private:
    static constexpr double kGainUnbypassed = 2.2;
    OnePole bypass, coupling;
    double sampleRate = 96000.0, gainHigh = 1.0, a2 = 0.0;
};

class TrimStage
{
public:
    void prepare(double fs)
    {
        if (dk.numInputs() == 0)
        {
            dk.load(circuit::parseNetlist(nl::kTrim));
            pTrim = dk.param("trim"); pPhase = dk.param("phase");
        }
        dk.setSampleRate(fs);
        dk.update();
        reset();
    }
    void configure(double trimPot, bool phase)
    {
        dk.setParam(pTrim, trimPot);
        dk.setParam(pPhase, phase ? 1.0 : 0.0);
        dk.update();
    }
    void reset() { dk.reset(); }
    double process(double v) { return railClip(dk.process1(v)); }

private:
    DkStage dk;
    int pTrim = -1, pPhase = -1;
};

// A plain DK stage with an output swing limit (filters, shelves, parametric bands, fader).
class EqStage
{
public:
    void load(const char* netlist) { dk.load(circuit::parseNetlist(netlist)); }
    void prepare(double fs) { dk.setSampleRate(fs); dk.update(); dk.reset(); }
    void set(const char* name, double v) { dk.setParam(dk.param(name), v); }
    void setPrewarp(double hz) { dk.setPrewarp(hz); }
    void update() { dk.update(); }
    void reset() { dk.reset(); }
    double process(double v) { return railClip(dk.process1(v)); }
    DkStage dk;
};

// Compressor gain cell and make-up amplifier.
class VcaStage
{
public:
    void prepare(double fs)
    {
        if (makeup.numInputs() == 0)
        {
            makeup.load(circuit::parseNetlist(nl::kMakeup));
            pMakeup = makeup.param("makeup");
        }
        makeup.setSampleRate(fs);
        makeup.update();
    }
    void configure(double makeupDb)
    {
        makeup.setParam(pMakeup, controlmap::makeupPot(makeupDb));
        makeup.update();
    }
    double process(double v, double microAmps)
    {
        const double g = std::pow(10.0, vcaGainDb(microAmps) / 20.0);
        return railClip(makeup.process1(railClip(vcaCell(v, microAmps, g))));
    }

private:
    DkStage makeup;
    int pMakeup = -1;
};

// Opto de-esser: tuned band-pass and the phase-cancelling optical network (equations in docs/CIRCUIT_MODEL.md).
class DeesserStage
{
public:
    static constexpr double kA = 9.1e3 / 8.2e3;   // band feed into the output summer
    static constexpr double kB = 9.1e3 / 4.3e3;   // optical feed into the output summer

    void prepare(double fs)
    {
        if (dk.numInputs() == 0)
        {
            dk.load(circuit::parseNetlist(nl::kDeesser));
            pFreq = dk.param("freq");
            OpticalLaw law;
            law.kind = OpticalLaw::Kind::Deesser;
            cell.setLaw(law);
        }
        dk.setSampleRate(fs);
        cell.prepare(fs);
        dk.update();
        reset();
    }
    void configure(double hz)
    {
        const double pot = controlmap::deessFreqPot(hz);
        dk.setParam(pFreq, pot);
        dk.setPrewarp(std::min(hz, 20000.0));
        dk.update();
        const double mag = std::pow(10.0, controlmap::valueAt(calibration::kDeessBandDb, pot) / 20.0);
        const double ph = controlmap::valueAt(calibration::kDeessBandPhase, pot);
        bandRe = mag * std::cos(ph);
        bandIm = mag * std::sin(ph);
    }
    void reset() { dk.reset(); cell.reset(); band = 0.0; }

    // Divider ratio of the optical node for a cell resistance.
    static double divider(double r) { return (1.0 / 3.9e3) / (1.0 / 3.9e3 + 1.0 / r + 1.0 / 4.3e3); }

    // Resistance that gives centre-frequency gain g (0 < g <= 1). With the band response H at the centre and
    // w = kA - kB * divider(R), the gain is |1 + H w|; solve the quadratic for w, then invert the divider.
    double resistanceForGain(double g) const
    {
        if (g >= 0.9999) return 1e12;
        const double h2 = bandRe * bandRe + bandIm * bandIm;
        const double disc = bandRe * bandRe - h2 * (1.0 - g * g);
        double w = disc >= 0.0 ? (-bandRe - std::sqrt(disc)) / h2 : -bandRe / h2;   // deepest reachable if below
        w = std::clamp(w, 0.0, kA);
        const double d = (kA - w) / kB;
        if (d >= divider(1e12)) return 1e12;
        if (d <= 1e-6) return 1.0;
        const double invR = (1.0 / 3.9e3) / d - 1.0 / 3.9e3 - 1.0 / 4.3e3;
        return invR <= 0.0 ? 1e12 : 1.0 / invR;
    }
    double ledCurrentForResistance(double r) const { return cell.getLaw().stateFor(r) * 1e-3; }

    // Band-pass output for the detector, computed before process() in each sample.
    double sidechain(double v)
    {
        band = dk.process1(v);
        return band;
    }
    double process(double v, double ledAmps)
    {
        const double r = cell.step(ledAmps);
        return railClip(v + kA * band - kB * divider(r) * band);
    }
    double lastBand() const { return band; }
    double illumination() const { return cell.illumination(); }

private:
    DkStage dk;
    OpticalCell cell;
    int pFreq = -1;
    double band = 0.0, bandRe = -1.0, bandIm = 0.0;
};

// Three-band opto soft limiter: complementary split, one LDR shunt per band, inverting summer.
class LimiterStage
{
public:
    void prepare(double fs)
    {
        if (dk.numInputs() == 0) dk.load(circuit::parseNetlist(nl::kLimiterSplit));
        dk.setSampleRate(fs);
        dk.update();
        for (auto& c : cells)
        {
            OpticalLaw law;
            law.kind = OpticalLaw::Kind::Limiter;
            c.setLaw(law);
            c.prepare(fs);
        }
        reset();
    }
    void reset()
    {
        dk.reset();
        for (auto& c : cells) c.reset();
    }
    // Band gain relative to the dark state for a cell resistance, and its inverse.
    static double bandGain(double r) { return 2.0 / (2.0 + 5.1e3 / r); }
    static double resistanceForGain(double g)
    {
        g = std::clamp(g, 1e-4, 0.999999);
        return 5.1e3 * g / (2.0 * (1.0 - g));
    }
    double ledCurrentForGain(double g) const { return cells[0].getLaw().stateFor(resistanceForGain(g)) * 1e-3; }

    double process(double v, double ledAmps)
    {
        double y[3];
        dk.process(&v, y);
        double sum = 0.0;
        for (int b = 0; b < 3; ++b)
        {
            const double r = cells[size_t(b)].step(ledAmps);
            const double node = y[b] * (1.0 / 5.1e3) / (2.0 / 5.1e3 + 1.0 / r);
            sum += node / 5.1e3;
        }
        return railClip(-10e3 * sum);
    }
    double gainReductionDb() const { return -20.0 * std::log10(bandGain(cells[1].resistance())); }
    double illuminationMa() const { return cells[1].illumination(); }

private:
    DkStage dk;
    std::array<OpticalCell, 3> cells;
};

class OutputStage
{
public:
    static constexpr double kFluxRef = 0.35;   // estimate (unpublished winding data), see docs/CIRCUIT_MODEL.md

    void prepare(double fs)
    {
        if (dk.numInputs() == 0) dk.load(circuit::parseNetlist(nl::kOutput));
        dk.setSampleRate(fs);
        dk.update();
        core.lm = 2.0;
        core.fluxRef = kFluxRef;
        core.prepare(fs);
        reset();
    }
    void reset() { dk.reset(); core.flux = 0.0; }
    double process(double v)
    {
        const double u[2] = { railClip(v), core.current() };
        double y[2];
        dk.process(u, y);
        // Core voltage follows the drive (tertiary feedback): use the input as its estimate.
        core.advance(u[0]);
        return y[0] - y[1];
    }

private:
    DkStage dk;
    CoreFlux core;
};
} // namespace bluechannel::model
