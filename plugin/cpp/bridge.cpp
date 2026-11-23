// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Marcos Quirino Deida

#include "bridge.h"

#include "Model/Engine.h"
#include "Plugin/HQRenderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <new>
#include <vector>

using namespace bluechannel::model;

namespace
{
// Table positions, in the order of src/params.rs (which is the order of Source/Plugin/Params.cpp).
enum P
{
    source, micGain, lineGain, trim, phase, impedance, air,
    allEq, filterRoute, hpf, lpf, shelfRoute, lfGain, lfFreq, lfHigh, hfGain, hfFreq, hfHigh,
    paramRoute, lmfGain, lmfFreq, lmfQ, lmfX3, hmfGain, hmfFreq, hmfQ, hmfX3,
    compIn, compType, ratio, threshold, attack, release, autoRelease, makeup, compKey, blend, mix, compListen,
    gateIn, expand, gateThreshold, range, hold, gateRelease, fastAttack, hysteresis, gateKey, gateListen,
    deessIn, deessThreshold, deessFreq, deessListen,
    dynPosition, bypass, output, softLimit, mute, link,
    oversampling, offlineOversampling, hq,
    kCount
};

// The same mapping as params::Reader::read in the JUCE build.
void read(const float* v, Controls& c)
{
    auto f = [&](P p) { return double(v[p]); };
    auto b = [&](P p) { return v[p] > 0.5f; };
    auto i = [&](P p) { return int(std::lround(v[p])); };
    c.source = Source(i(source));
    c.micGain = i(micGain);
    c.lineGain = i(lineGain);
    c.trimDb = f(trim);
    c.phase = b(phase);
    c.impedance = i(impedance);
    c.air = b(air);
    c.allEq = b(allEq);
    c.filterRoute = EqRoute(i(filterRoute));
    c.hpfHz = f(hpf);
    c.lpfHz = f(lpf);
    c.shelfRoute = EqRoute(i(shelfRoute));
    c.lfDb = f(lfGain);
    c.lfFreq = i(lfFreq);
    c.lfHigh = b(lfHigh);
    c.hfDb = f(hfGain);
    c.hfFreq = i(hfFreq);
    c.hfHigh = b(hfHigh);
    c.paramRoute = EqRoute(i(paramRoute));
    c.lmfDb = f(lmfGain);
    c.lmfHz = f(lmfFreq);
    c.lmfQ = f(lmfQ);
    c.lmfX3 = b(lmfX3);
    c.hmfDb = f(hmfGain);
    c.hmfHz = f(hmfFreq);
    c.hmfQ = f(hmfQ);
    c.hmfX3 = b(hmfX3);
    c.compIn = b(compIn);
    c.compType = CompType(i(compType));
    c.ratio = f(ratio);
    c.compThresholdDb = f(threshold);
    c.attackMs = f(attack);
    c.releaseS = f(release);
    c.autoRelease = b(autoRelease);
    c.makeupDb = f(makeup);
    c.compExtKey = b(compKey);
    c.blend = b(blend);
    c.mix = f(mix) / 100.0;
    c.compListen = b(compListen);
    c.gateIn = b(gateIn);
    c.expand = b(expand);
    c.gateThresholdDb = f(gateThreshold);
    c.rangeDb = f(range);
    c.holdS = f(hold);
    c.gateReleaseS = f(gateRelease);
    c.fastAttack = b(fastAttack);
    c.hysteresis = b(hysteresis);
    c.gateExtKey = b(gateKey);
    c.gateListen = b(gateListen);
    c.deessIn = b(deessIn);
    c.deessThresholdDb = f(deessThreshold);
    c.deessHz = f(deessFreq);
    c.deessListen = b(deessListen);
    c.dynPosition = DynPosition(i(dynPosition));
    c.bypass = b(bypass);
    c.outputDb = f(output);
    c.softLimit = b(softLimit);
    c.mute = b(mute);
    c.link = b(link);
}
} // namespace

// The same computation as DigitalEqGraph::refresh in the JUCE build.
struct BcEqView
{
    EqStage filters, shelves, lmf, hmf;
    Controls previous;
    bool dirty = true;
    float curve[kBcCurvePoints] {}, filterCurve[kBcCurvePoints] {};

    static double shelfHz(bool low, int index, bool high)
    {
        static const double lo[2][4] = { { 20, 56, 160, 460 }, { 33, 95, 270, 655 } };
        static const double hi[2][4] = { { 1500, 3300, 6800, 15000 }, { 2200, 4700, 10000, 18000 } };
        index = std::clamp(index, 0, 3);
        return low ? lo[high ? 1 : 0][index] : hi[high ? 1 : 0][index];
    }
};

struct BcEngine
{
    Engine engine;
    Controls controls;
    bluechannel::hq::Renderer hq;
    std::array<std::vector<double>, 2> hqAudio, hqKey;
    bool previousHQ = false, previousOffline = false;
    int maxBlock = 512;
};

extern "C" {

BcEngine* bc_create(void)
{
    try { return new BcEngine(); }
    catch (...) { return nullptr; }
}

void bc_destroy(BcEngine* e) { delete e; }

int bc_prepare(BcEngine* e, double sampleRate, int maxBlock, int channels)
{
    try
    {
        e->maxBlock = std::max(1, maxBlock);
        e->engine.prepare(sampleRate, e->maxBlock, channels);
        e->hq.prepare(sampleRate, e->maxBlock, std::clamp(channels, 1, 2));
        for (auto& b : e->hqAudio) b.assign(size_t(e->maxBlock), 0);
        for (auto& b : e->hqKey) b.assign(size_t(e->maxBlock), 0);
        e->previousHQ = e->previousOffline = false;
        return 1;
    }
    catch (...) { return 0; }
}

void bc_reset(BcEngine* e) { e->engine.reset(); e->hq.restart(); }

int bc_hq_compiled(void) { return bluechannel::hq::Renderer::compiled() ? 1 : 0; }

const char* bc_hq_error(BcEngine* e) { return e->hq.error(); }

int bc_param_count(void) { return kCount; }

void bc_process(BcEngine* e, float* const* io, int channels, int frames, const float* const* key, int keyChannels,
                const float* values, int offline, BcTelemetry* out)
{
    using H = bluechannel::hq::Renderer;
    read(values, e->controls);
    const int playback = int(std::lround(values[oversampling])), offlineStages = int(std::lround(values[offlineOversampling])) - 1;
    e->engine.setOversampling(playback, offlineStages);
    channels = std::min(channels, e->engine.numChannels());

    // The same policy as the JUCE processor: HQ arms on the parameter and solves only while the host
    // reports offline rendering; a block the solver cannot produce is muted, never passed through.
    const bool isOffline = offline != 0, wantHQ = values[hq] > 0.5f;
    if (wantHQ != e->previousHQ || (isOffline && !e->previousOffline)) e->hq.restart();
    e->previousHQ = wantHQ;
    e->previousOffline = isOffline;
    const bool solve = wantHQ && isOffline;
    int status = wantHQ ? (H::compiled() ? H::Armed : H::Unavailable) : H::Idle;

    // Hosts may call with more frames than announced: process in prepared-size chunks.
    for (int start = 0; start < frames; start += e->maxBlock)
    {
        const int len = std::min(e->maxBlock, frames - start);
        float* chunk[2] = { io[0] + start, channels > 1 ? io[1] + start : nullptr };
        const float* keys[2] = { keyChannels > 0 ? key[0] + start : nullptr, keyChannels > 1 ? key[1] + start : nullptr };
        if (!solve)
        {
            e->engine.process<float>(chunk, channels, len, keyChannels > 0 ? keys : nullptr, keyChannels, e->controls, isOffline);
            continue;
        }
        double* audio[2] = { e->hqAudio[0].data(), e->hqAudio[1].data() };
        const double* hkeys[2] = { e->hqKey[0].data(), e->hqKey[1].data() };
        for (int ch = 0; ch < channels; ++ch)
            for (int i = 0; i < len; ++i)
            {
                audio[ch][i] = double(chunk[ch][i]);
                e->hqKey[size_t(ch)][size_t(i)] = keyChannels > 0 ? double(keys[keyChannels > 1 ? ch : 0][i]) : 0;
            }
        const int stages = offlineStages < 0 ? playback : offlineStages;
        bool ok = false;
        try { ok = e->hq.render(audio, len, hkeys, keyChannels, e->controls, 1 << std::clamp(stages, 0, 4), e->engine.telemetry); }
        catch (...) { ok = false; }
        status = ok ? H::Solving : (H::compiled() ? H::Failed : H::Unavailable);
        for (int ch = 0; ch < channels; ++ch)
            for (int i = 0; i < len; ++i) chunk[ch][i] = ok ? float(audio[ch][i]) : 0.0f;
    }
    out->hqStatus = status;

    auto& t = e->engine.telemetry;
    const auto r = std::memory_order_relaxed;
    for (int ch = 0; ch < 2; ++ch)
    {
        out->inputPeak[ch] = t.inputPeak[ch].exchange(0.0f, r);
        out->outputPeak[ch] = t.outputPeak[ch].exchange(0.0f, r);
        out->inputSquare[ch] = t.inputSquare[ch].load(r);
        out->outputSquare[ch] = t.outputSquare[ch].load(r);
    }
    out->compGr = t.compGr.exchange(0.0f, r);
    out->gateGr = t.gateGr.exchange(0.0f, r);
    out->limiterGr = t.limiterGr.exchange(0.0f, r);
    out->deessLed = t.deessLed.exchange(0.0f, r);
    out->overload = t.overload.exchange(false, r) ? 1 : 0;
    out->activeFactor = t.activeFactor.load(r);
    out->rendering = t.rendering.load(r) ? 1 : 0;
}

BcEqView* bc_eq_create(void)
{
    try
    {
        auto* v = new BcEqView();
        v->filters.load(nl::kFilters); v->shelves.load(nl::kShelves);
        v->lmf.load(nl::kLmf); v->hmf.load(nl::kHmf);
        return v;
    }
    catch (...) { return nullptr; }
}

void bc_eq_destroy(BcEqView* v) { delete v; }

int bc_eq_update(BcEqView* v, const float* values, float* curve, float* filterCurve, float* handles)
{
    using namespace controlmap;
    Controls c;
    read(values, c);
    const double h[kBcHandles][2] = {
        { BcEqView::shelfHz(true, c.lfFreq, c.lfHigh), c.lfDb },
        { lmfHz(lmfFreqPot(c.lmfHz), c.lmfX3), c.lmfDb },
        { hmfHz(hmfFreqPot(c.hmfHz), c.hmfX3), c.hmfDb },
        { BcEqView::shelfHz(false, c.hfFreq, c.hfHigh), c.hfDb },
        { c.hpfHz, -3.0 },
        { c.lpfHz, -3.0 },
    };
    for (int i = 0; i < kBcHandles; ++i) { handles[2 * i] = float(h[i][0]); handles[2 * i + 1] = float(h[i][1]); }

    // Compressor, gate and input edits must not recompile the response networks.
    Controls view;
    view.allEq = c.allEq; view.bypass = c.bypass;
    view.filterRoute = c.filterRoute; view.hpfHz = c.hpfHz; view.lpfHz = c.lpfHz;
    view.shelfRoute = c.shelfRoute; view.lfDb = c.lfDb; view.hfDb = c.hfDb;
    view.lfFreq = c.lfFreq; view.hfFreq = c.hfFreq; view.lfHigh = c.lfHigh; view.hfHigh = c.hfHigh;
    view.paramRoute = c.paramRoute; view.lmfDb = c.lmfDb; view.lmfHz = c.lmfHz;
    view.lmfQ = c.lmfQ; view.lmfX3 = c.lmfX3; view.hmfDb = c.hmfDb;
    view.hmfHz = c.hmfHz; view.hmfQ = c.hmfQ; view.hmfX3 = c.hmfX3;
    int changed = 0;
    if (v->dirty || !(view == v->previous))
    {
        try
        {
            v->filters.set("hpr", highPassOhms(c.hpfHz)); v->filters.set("lpr", lowPassOhms(c.lpfHz));
            v->shelves.set("lfgain", lowShelfPot(c.lfDb)); v->shelves.set("hfgain", highShelfPot(c.hfDb));
            v->shelves.set("lfsel", lowShelfPosition(c.lfFreq, c.lfHigh)); v->shelves.set("hfsel", highShelfPosition(c.hfFreq, c.hfHigh));
            v->lmf.set("gain", lmfGainPot(c.lmfDb)); v->lmf.set("q", lmfQPot(c.lmfQ)); v->lmf.set("freq", lmfFreqPot(c.lmfHz)); v->lmf.set("x3", c.lmfX3 ? 1 : 0);
            v->hmf.set("gain", hmfGainPot(c.hmfDb)); v->hmf.set("q", hmfQPot(c.hmfQ)); v->hmf.set("freq", hmfFreqPot(c.hmfHz)); v->hmf.set("x3", c.hmfX3 ? 1 : 0);
            for (int i = 0; i < kBcCurvePoints; ++i)
            {
                const double f = 20 * std::pow(1000.0, double(i) / double(kBcCurvePoints - 1));
                std::complex<double> response(1, 0);
                const auto filt = v->filters.dk.analogResponse(0, 0, f);
                if (c.allEq && !c.bypass)
                {
                    if (c.filterRoute == EqRoute::In) response *= filt;
                    if (c.shelfRoute == EqRoute::In) response *= v->shelves.dk.analogResponse(0, 0, f);
                    if (c.paramRoute == EqRoute::In) response *= v->lmf.dk.analogResponse(0, 0, f) * v->hmf.dk.analogResponse(0, 0, f);
                }
                v->filterCurve[i] = float(20 * std::log10(std::max(1e-9, std::abs(filt))));
                v->curve[i] = float(20 * std::log10(std::max(1e-9, std::abs(response))));
            }
            v->previous = view; v->dirty = false; changed = 1;
        }
        catch (...) {}
    }
    for (int i = 0; i < kBcCurvePoints; ++i) { curve[i] = v->curve[i]; filterCurve[i] = v->filterCurve[i]; }
    return changed;
}

} // extern "C"
