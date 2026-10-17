// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

// Digital side-chains. The circuit model keeps the analogue gain elements (VCA cell, optical cells) and
// exposes their control ports; these detectors decide the control current, as specified for the reference
// model. Design notes and the static curves are in docs/DYNAMICS.md.
//
// All detectors are causal, keep their state across blocks and are updated every internal sample. Levels are
// in dBu (sine-equivalent of the peak) at the node the detector listens to.

#include "Model/Levels.h"

#include <algorithm>
#include <cmath>

namespace bluechannel::dynamics
{
inline double coef(double seconds, double fs) { return 1.0 - std::exp(-1.0 / (std::max(1e-6, seconds) * fs)); }
inline double levelDbu(double peakVolts) { return model::peakVoltsToDbu(std::abs(peakVolts) + 1e-9); }

// Soft-knee downward compression curve: gain reduction (dB, >= 0) for an input level.
inline double compressionDb(double levelDb, double thresholdDb, double ratio, double kneeDb)
{
    const double over = levelDb - thresholdDb;
    const double slope = 1.0 - 1.0 / std::max(1.0, ratio);
    if (kneeDb > 0.0 && std::abs(over) <= kneeDb * 0.5)
    {
        const double t = over + kneeDb * 0.5;
        return slope * t * t / (2.0 * kneeDb);
    }
    return over > 0.0 ? slope * over : 0.0;
}

// VCA compressor: feed-forward, soft knee, log-domain ballistics. Auto release combines a fast release path
// with a slow one that only charges during sustained compression (program-dependent release).
class CompressorDetector
{
public:
    struct Settings { double thresholdDb = 0, ratio = 3, attackS = 0.01, releaseS = 0.3, kneeDb = 10; bool autoRelease = false; };

    void prepare(double fs) { sampleRate = fs; reset(); set(settings); }
    void reset() { gr = grSlow = 0.0; peak = 0.0; }
    void set(const Settings& s)
    {
        settings = s;
        att = coef(s.attackS, sampleRate);
        rel = coef(s.releaseS, sampleRate);
        relFast = coef(0.06, sampleRate);
        attSlow = coef(0.25, sampleRate);
        relSlow = coef(2.0, sampleRate);
        peakRel = coef(0.002, sampleRate);
    }
    // Rectified level of the key (short peak hold so the curve does not see every half cycle).
    double level(double key)
    {
        const double a = std::abs(key);
        peak = a > peak ? a : peak + (a - peak) * peakRel;
        return levelDbu(peak);
    }
    // Gain reduction (dB) for a level, advancing the ballistics.
    double gainReduction(double levelDb)
    {
        const double target = compressionDb(levelDb, settings.thresholdDb, settings.ratio, settings.kneeDb);
        if (!settings.autoRelease)
        {
            gr += (target - gr) * (target > gr ? att : rel);
            return gr;
        }
        gr += (target - gr) * (target > gr ? att : relFast);
        grSlow += (target - grSlow) * (target > grSlow ? attSlow : relSlow);
        return std::max(gr, grSlow);
    }
    double current() const { return settings.autoRelease ? std::max(gr, grSlow) : gr; }

private:
    Settings settings;
    double sampleRate = 96000.0, att = 0, rel = 0, relFast = 0, attSlow = 0, relSlow = 0, peakRel = 0;
    double gr = 0.0, grSlow = 0.0, peak = 0.0;
};

// Optical ("vintage") compressor: LED/LDR shunt attenuator with fixed timing. The cell has a fast attack, a
// two-stage release and a memory that lengthens release after sustained compression. Comp mode: soft knee,
// ratio 1.5..5; limit mode: hard knee, ratio 5..20 (behavioural extension, docs/DYNAMICS.md).
class OptoCompressor
{
public:
    static constexpr double kSeriesOhms = 10e3;

    void prepare(double fs)
    {
        sampleRate = fs;
        detAtt = coef(0.004, fs);
        detRel = coef(0.06, fs);
        cellAtt = coef(0.010, fs);
        memCharge = coef(2.5, fs);
        memDischarge = coef(6.0, fs);
        reset();
    }
    void reset() { det = 0.0; cell = 0.0; memory = 0.0; }
    void set(double thresholdDb, double newRatio, bool limitMode)
    {
        threshold = thresholdDb + (limitMode ? 4.0 : 0.0);   // limit mode raises the threshold point
        ratio = newRatio;
        knee = limitMode ? 2.0 : 12.0;
    }
    // Returns the attenuator gain (linear) for this sample from the key level (dBu).
    double process(double levelDb)
    {
        const double target = compressionDb(levelDb, threshold, ratio, knee);
        det += (target - det) * (target > det ? detAtt : detRel);
        // Cell: attenuation in dB follows the drive with its own attack and a release that slows with memory.
        const double releaseS = 0.08 + 1.6 * memory;
        const double relC = coef(releaseS, sampleRate);
        cell += (det - cell) * (det > cell ? cellAtt : relC);
        memory += ((cell > 1.0 ? 1.0 : 0.0) - memory) * (cell > 1.0 ? memCharge : memDischarge);
        return std::pow(10.0, -cell / 20.0);
    }
    double gainReductionDb() const { return cell; }

private:
    double sampleRate = 96000.0, detAtt = 0, detRel = 0, cellAtt = 0, memCharge = 0, memDischarge = 0;
    double det = 0, cell = 0, memory = 0;
    double threshold = 0, ratio = 3, knee = 12;
};

// Gate / downward expander driving the same VCA control port (its current adds to the compressor's).
class GateDetector
{
public:
    struct Settings { double thresholdDb = -40, rangeDb = 40, holdS = 0.1, releaseS = 0.5; bool fast = false, hysteresis = true, expand = false; };
    static constexpr double kExpansionRatio = 2.0;   // 1:2 below threshold in expander mode

    void prepare(double fs) { sampleRate = fs; set(settings); reset(); }
    void reset() { att = 0.0; peak = 0.0; holdLeft = 0; open = false; }
    void set(const Settings& s)
    {
        settings = s;
        attackC = coef(s.fast ? 50e-6 : 2e-3, sampleRate);
        peakRel = coef(0.005, sampleRate);
        holdSamples = int(s.holdS * sampleRate);
        releaseStep = std::max(1.0, s.rangeDb) / (std::max(0.01, s.releaseS) * sampleRate);
    }
    double level(double key)
    {
        const double a = std::abs(key);
        peak = a > peak ? a : peak + (a - peak) * peakRel;
        return levelDbu(peak);
    }
    // Attenuation in dB (>= 0).
    double attenuation(double levelDb)
    {
        const double closeAt = settings.thresholdDb - (settings.hysteresis ? 10.0 : 2.0);
        if (levelDb >= settings.thresholdDb) { open = true; holdLeft = holdSamples; }
        else if (levelDb < closeAt)
        {
            if (holdLeft > 0) --holdLeft;
            else open = false;
        }
        else if (holdLeft > 0) --holdLeft;

        double target = 0.0;
        if (!open)
            target = settings.expand
                         ? std::min(settings.rangeDb, std::max(0.0, settings.thresholdDb - levelDb) * (kExpansionRatio - 1.0))
                         : settings.rangeDb;
        if (target < att) att += (target - att) * attackC;
        else att = std::min(target, att + releaseStep);
        return att;
    }
    double current() const { return att; }

private:
    Settings settings;
    double sampleRate = 96000.0, attackC = 0, peakRel = 0, releaseStep = 0, att = 0, peak = 0;
    int holdSamples = 0, holdLeft = 0;
    bool open = false;
};

// De-esser: 2:1 at the tuned frequency above threshold, from the circuit's own band-pass output.
class DeessDetector
{
public:
    void prepare(double fs) { attC = coef(0.0005, fs); relC = coef(0.05, fs); reset(); }
    void reset() { env = 0.0; }
    void set(double thresholdDb) { threshold = thresholdDb; }
    // Wanted gain at the centre frequency (linear, <= 1).
    double gain(double band)
    {
        const double a = std::abs(band);
        env += (a - env) * (a > env ? attC : relC);
        const double over = levelDbu(env) - threshold;
        return over > 0.0 ? std::pow(10.0, -0.5 * over / 20.0) : 1.0;
    }

private:
    double attC = 0, relC = 0, env = 0, threshold = 0;
};

// Soft limiter: level-dependent ratio from -6 dBFS (1.5:1 to -4 dBFS, 2:1 to 0 dBFS, then infinite), on the
// connector-referred peak. Instant attack, 50 ms release of the peak detector.
class LimiterDetector
{
public:
    void prepare(double fs) { relC = coef(0.05, fs); reset(); }
    void reset() { peak = 0.0; }
    static double outputDb(double inDb)
    {
        if (inDb <= -6.0) return inDb;
        if (inDb <= -4.0) return -6.0 + (inDb + 6.0) / 1.5;
        if (inDb <= 0.0) return -6.0 + 2.0 / 1.5 + (inDb + 4.0) / 2.0;
        return -6.0 + 2.0 / 1.5 + 2.0;
    }
    // Wanted gain (linear) for this internal-node sample.
    double gain(double v)
    {
        const double a = std::abs(v) * model::kInternalToConnector;
        peak = a > peak ? a : peak + (a - peak) * relC;
        const double inDb = 20.0 * std::log10(peak / model::kFullScalePeak + 1e-12);
        return std::pow(10.0, (outputDb(inDb) - inDb) / 20.0);
    }

private:
    double relC = 0, peak = 0;
};
} // namespace bluechannel::dynamics
