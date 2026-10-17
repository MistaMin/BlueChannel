// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

// One channel of the strip at the internal (oversampled) rate.
//
//   input (mic | line | instrument) -> trim/phase -> [EQ] -> dynamics -> [EQ] -> fader -> soft limiter
//   -> output transformer
//
// EQ sections routed to a key are removed from the audio path and filter the compressor or gate key instead
// (each section has its own key-path copy so its state is independent). The engine splits each sample into
// front() and back() so stereo-linked detectors can see both channels before either applies gain.

#include "Controls.h"
#include "Dynamics/Detectors.h"
#include "Stages.h"

namespace bluechannel::model
{
class Channel
{
public:
    // Allocates (loads every netlist). Call once from the preparation thread.
    void load()
    {
        mic.load();
        for (auto* f : { &filters, &filtersKey }) f->load(nl::kFilters);
        for (auto* s : { &shelves, &shelvesKey }) s->load(nl::kShelves);
        for (auto* s : { &lmf, &lmfKey }) s->load(nl::kLmf);
        for (auto* s : { &hmf, &hmfKey }) s->load(nl::kHmf);
        fader.load(nl::kFader);
        loaded = true;
    }

    // Real-time safe after load(): recomputes coefficients for a new internal rate and resets every history.
    void prepare(double fs)
    {
        sampleRate = fs;
        mic.prepare(fs);
        line.prepare(fs);
        inst.prepare(fs);
        trim.prepare(fs);
        for (auto* s : { &filters, &filtersKey, &shelves, &shelvesKey, &lmf, &lmfKey, &hmf, &hmfKey, &fader }) s->prepare(fs);
        vca.prepare(fs);
        deess.prepare(fs);
        limiter.prepare(fs);
        output.prepare(fs);
        compDet.prepare(fs);
        opto.prepare(fs);
        gateDet.prepare(fs);
        deessDet.prepare(fs);
        limDet.prepare(fs);
        inputBlock.setHighPass(2.0, fs);
        configured = false;
    }

    void reset()
    {
        mic.reset(); line.reset(); inst.reset(); trim.reset();
        for (auto* s : { &filters, &filtersKey, &shelves, &shelvesKey, &lmf, &lmfKey, &hmf, &hmfKey, &fader }) s->reset();
        deess.reset(); limiter.reset(); output.reset();
        compDet.reset(); opto.reset(); gateDet.reset(); deessDet.reset(); limDet.reset();
        inputBlock.reset();
    }

    // Control rate: maps panel values to component values. Only stages whose values changed recompile.
    void configure(const Controls& c)
    {
        using namespace controlmap;
        if (!configured || c.source != ctl.source) resetInputs(c.source);
        ctl = c;
        configured = true;
        const double trimPos = trimPot(c.trimDb);
        switch (c.source)
        {
            case Source::Mic: mic.configure(c.micGain, c.sourceOhms, c.impedance, c.air); break;
            case Source::Line: line.configure(c.lineGain); break;
            case Source::Instrument: inst.configure(trimPos); break;
        }
        trim.configure(trimPos, c.phase);

        // Key-path copies take the same values but only recompile while a key route uses them.
        const bool filtersK = isKey(c.filterRoute), shelvesK = isKey(c.shelfRoute), paramK = isKey(c.paramRoute);
        for (auto* f : { &filters, &filtersKey })
        {
            f->set("lpr", lowPassOhms(c.lpfHz));
            f->set("hpr", highPassOhms(c.hpfHz));
            f->setPrewarp(std::min(c.lpfHz, 0.4 * sampleRate));
            if (f == &filters || filtersK) f->update();
        }
        const int lfSel = lowShelfPosition(c.lfFreq, c.lfHigh), hfSel = highShelfPosition(c.hfFreq, c.hfHigh);
        static constexpr double hfLegend[] = { 3300, 4700, 6800, 10000, 15000, 18000, 1500, 2200 };
        for (auto* s : { &shelves, &shelvesKey })
        {
            s->set("lfgain", lowShelfPot(c.lfDb));
            s->set("hfgain", highShelfPot(c.hfDb));
            s->set("lfsel", lfSel);
            s->set("hfsel", hfSel);
            s->setPrewarp(hfLegend[hfSel]);
            if (s == &shelves || shelvesK) s->update();
        }
        const double lfp = lmfFreqPot(c.lmfHz), hfp = hmfFreqPot(c.hmfHz);
        for (auto* s : { &lmf, &lmfKey })
        {
            s->set("gain", lmfGainPot(c.lmfDb)); s->set("q", lmfQPot(c.lmfQ));
            s->set("freq", lfp); s->set("x3", c.lmfX3 ? 1 : 0);
            s->setPrewarp(lmfHz(lfp, c.lmfX3));
            if (s == &lmf || paramK) s->update();
        }
        for (auto* s : { &hmf, &hmfKey })
        {
            s->set("gain", hmfGainPot(c.hmfDb)); s->set("q", hmfQPot(c.hmfQ));
            s->set("freq", hfp); s->set("x3", c.hmfX3 ? 1 : 0);
            s->setPrewarp(std::min(hmfHz(hfp, c.hmfX3), 0.4 * sampleRate));
            if (s == &hmf || paramK) s->update();
        }
        vca.configure(c.compIn ? c.makeupDb : 0.0);
        deess.configure(c.deessHz);
        fader.set("fader", faderTrack(c.outputDb));
        fader.update();

        // Thresholds are set on the output (connector) scale; the detectors listen at internal nodes, which run
        // 6 dB lower (docs/GAIN_STAGING.md).
        const double toInternal = -20.0 * std::log10(kInternalToConnector);
        compDet.set({ c.compThresholdDb + toInternal, c.ratio, c.attackMs * 1e-3, c.releaseS, 10.0, c.autoRelease });
        const bool limitMode = c.compType == CompType::OptoLimit;
        const double optoRatio = limitMode ? 5.0 + (std::clamp(c.ratio, 1.5, 10.0) - 1.5) / 8.5 * 15.0
                                           : 1.5 + (std::clamp(c.ratio, 1.5, 10.0) - 1.5) / 8.5 * 3.5;
        opto.set(c.compThresholdDb + toInternal, optoRatio, limitMode);
        gateDet.set({ c.gateThresholdDb + toInternal, c.rangeDb, c.holdS, c.gateReleaseS, c.fastAttack, c.hysteresis, c.expand });
        deessDet.set(c.deessThresholdDb + toInternal);
    }

    // First half of a sample: input, trim, pre-dynamics EQ and the two keys. key = external key (volts).
    void front(double v, double extKey)
    {
        v = inputBlock.highPass(v);
        switch (ctl.source)
        {
            case Source::Mic: v = mic.process(v); break;
            case Source::Line: v = line.process(v); break;
            case Source::Instrument: v = inst.process(v); break;
        }
        v = trim.process(v);
        inputTap = v;
        if (!ctl.bypass && ctl.dynPosition == DynPosition::PostEq) v = audioEq(v);
        dynIn = v;
        const double compSrc = ctl.compExtKey && ctl.compType == CompType::Vca ? extKey : dynIn;
        const double gateSrc = ctl.gateExtKey ? extKey : dynIn;
        compKey = keyEq(compSrc, EqRoute::CompKey);
        gateKey = keyEq(gateSrc, EqRoute::GateKey);
    }

    double compLevel() { return compDet.level(compKey); }
    double gateLevel() { return gateDet.level(gateKey); }

    // Second half: dynamics with the detector levels the engine supplies (linked or not), then the output.
    double back(double compLevelDb, double gateLevelDb)
    {
        double v = dynIn;
        if (!ctl.bypass)
        {
            const bool vcaPath = ctl.compIn || ctl.gateIn;
            double micro = 0.0;
            compGr = 0.0;
            gateGr = 0.0;
            const double dry = v;
            if (ctl.compIn)
            {
                if (ctl.compType == CompType::Vca)
                {
                    compGr = compDet.gainReduction(compLevelDb);
                    micro += vcaCurrentForReduction(compGr);
                }
                else
                {
                    const double g = opto.process(compLevelDb);
                    compGr = opto.gainReductionDb();
                    v *= g;
                }
            }
            if (ctl.gateIn)
            {
                gateGr = gateDet.attenuation(gateLevelDb);
                micro += vcaCurrentForReduction(gateGr);
            }
            if (vcaPath)
            {
                v = vca.process(v, micro);
                if (ctl.compIn && ctl.blend) v = ctl.mix * v + (1.0 - ctl.mix) * dry;
            }

            const double band = deess.sidechain(v);
            if (ctl.deessIn)
            {
                const double g = deessDet.gain(band);
                const double led = g < 1.0 ? std::min(0.02, deess.ledCurrentForResistance(deess.resistanceForGain(g))) : 0.0;
                v = deess.process(v, led);
            }
            if (ctl.compListen) v = compKey;
            else if (ctl.gateListen) v = gateKey;
            else if (ctl.deessListen) v = band;
            if (ctl.dynPosition == DynPosition::PreEq) v = audioEq(v);
        }
        dynTap = v;
        v = fader.process(v);
        if (ctl.softLimit && !ctl.bypass)
        {
            const double g = limDet.gain(v);
            double led = g < 0.9999 ? limiter.ledCurrentForGain(g) : 0.0;
            // Drive the LEDs harder while the cell is still darker than wanted (shortens the optical attack).
            led = std::min(0.02, led + 3.0 * std::max(0.0, led - limiter.illuminationMa() * 1e-3));
            v = limiter.process(v, led);
        }
        if (ctl.mute) v = 0.0;
        outputTap = v;
        return output.process(v);
    }

    // Telemetry (internal node volts / dB), read by the engine after each sample.
    double inputTap = 0.0, dynTap = 0.0, outputTap = 0.0, compGr = 0.0, gateGr = 0.0;
    double deessIllumination() const { return deess.illumination(); }
    double limiterGr() const { return limiter.gainReductionDb(); }
    bool isLoaded() const { return loaded; }

private:
    Controls ctl;
    double sampleRate = 96000.0;
    bool loaded = false, configured = false;
    double dynIn = 0.0, compKey = 0.0, gateKey = 0.0;

    MicStage mic;
    LineStage line;
    InstrumentStage inst;
    TrimStage trim;
    EqStage filters, filtersKey, shelves, shelvesKey, lmf, lmfKey, hmf, hmfKey, fader;
    VcaStage vca;
    DeesserStage deess;
    LimiterStage limiter;
    OutputStage output;
    OnePole inputBlock;

    dynamics::CompressorDetector compDet;
    dynamics::OptoCompressor opto;
    dynamics::GateDetector gateDet;
    dynamics::DeessDetector deessDet;
    dynamics::LimiterDetector limDet;

    void resetInputs(Source s)
    {
        if (s == Source::Mic) mic.reset();
        if (s == Source::Line) line.reset();
        if (s == Source::Instrument) inst.reset();
    }

    bool inAudio(EqRoute r) const { return ctl.allEq && r == EqRoute::In; }
    static bool isKey(EqRoute r) { return r == EqRoute::CompKey || r == EqRoute::GateKey; }

    double audioEq(double v)
    {
        if (inAudio(ctl.filterRoute)) v = filters.process(v);
        if (inAudio(ctl.shelfRoute)) v = shelves.process(v);
        if (inAudio(ctl.paramRoute)) v = hmf.process(lmf.process(v));
        return v;
    }

    double keyEq(double v, EqRoute route)
    {
        if (ctl.filterRoute == route) v = filtersKey.process(v);
        if (ctl.shelfRoute == route) v = shelvesKey.process(v);
        if (ctl.paramRoute == route) v = hmfKey.process(lmfKey.process(v));
        return v;
    }
};
} // namespace bluechannel::model
