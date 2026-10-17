// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

// Host-independent engine: up to two channels, external key, oversampling, stereo link, control smoothing
// and telemetry. No JUCE dependency, so tests and benchmarks run it directly.
//
// Render policy (docs/OFFLINE_RENDER.md): a live bank processes playback, monitoring and realtime export.
// A separate render bank, with its own oversampling factor, is used only while the host reports offline
// processing and an offline factor other than "same as playback" is selected. Entering offline resets the
// render bank, so repeated bounces start from the same state; the live bank keeps its history meanwhile.
// Every oversampler is allocated in prepare(), so switching factors never allocates.

#include "Channel.h"
#include "DSP/Oversampler.h"

#include <array>
#include <atomic>
#include <cmath>
#include <vector>

namespace bluechannel::model
{
struct Telemetry
{
    // Peaks since the last read (UI takes them with exchange()).
    std::atomic<float> inputPeak[2] {}, outputPeak[2] {};
    std::atomic<float> inputSquare[2] {}, outputSquare[2] {};   // mean square over the last block
    std::atomic<float> compGr { 0 }, gateGr { 0 }, limiterGr { 0 }, deessLed { 0 };
    std::atomic<bool> overload { false };
    std::atomic<int> activeFactor { 1 };
    std::atomic<bool> rendering { false };

    static void raise(std::atomic<float>& a, float v)
    {
        float cur = a.load(std::memory_order_relaxed);
        while (v > cur && !a.compare_exchange_weak(cur, v, std::memory_order_relaxed)) {}
    }
};

class Engine
{
public:
    static constexpr int kMaxChannels = 2;
    static constexpr int kControlBlock = 16;      // base-rate samples between control updates
    static constexpr int kMaxStages = 4;          // 16x

    // Not real-time safe: allocates every bank and oversampler.
    void prepare(double baseRate, int maxBlock, int numChannels)
    {
        fs = baseRate;
        channels = std::clamp(numChannels, 1, kMaxChannels);
        maxBlockSize = std::max(1, maxBlock);
        for (auto* b : { &live, &render })
        {
            for (auto& c : b->ch)
                if (!c.isLoaded()) c.load();
            for (int s = 0; s <= kMaxStages; ++s)
                b->os[size_t(s)].prepare(fs, kControlBlock, 2 * kMaxChannels, s);
            b->stages = -1;
        }
        const size_t big = size_t(kControlBlock) << kMaxStages;
        for (auto& v : work) v.assign(big, 0.0);
        for (auto& v : keyWork) v.assign(big, 0.0);
        smoothedValid = false;
        selectStages(live, liveStages);
        selectStages(render, renderStagesResolved());
        wasOffline = false;
    }

    // Real-time safe.
    void reset()
    {
        for (auto* b : { &live, &render })
        {
            for (auto& c : b->ch) c.reset();
            for (auto& o : b->os) o.reset();
        }
        fadeLeft = 0;
        controlPhase = 0;
        smoothedValid = false;   // the next block starts on its targets, without a glide
    }

    // Oversampling: number of half-band stages (0 = 1x .. 4 = 16x). renderStages < 0 means "same as playback".
    void setOversampling(int playbackStages, int offlineStages)
    {
        liveStages = std::clamp(playbackStages, 0, kMaxStages);
        renderStages = offlineStages < 0 ? -1 : std::clamp(offlineStages, 0, kMaxStages);
    }

    // io: ioChannels buffers (at most the prepared count are processed), in place. key: 0..2 buffers (external
    // key), may be null.
    template <typename Sample>
    void process(Sample* const* io, int ioChannels, int numSamples, const Sample* const* key, int keyChannels,
                 const Controls& target, bool offline)
    {
        const int nch = std::min(channels, ioChannels);
        if (nch <= 0) return;
        const bool useRender = offline && renderStages >= 0 && renderStages != liveStages;
        if (offline && !wasOffline && useRender) startRender();
        wasOffline = offline;
        Bank& bank = useRender ? render : live;
        const int wantStages = useRender ? renderStages : liveStages;
        if (bank.stages != wantStages)
        {
            selectStages(bank, wantStages);
            if (!useRender) fadeLeft = int(0.01 * fs);
        }
        telemetry.rendering.store(useRender, std::memory_order_relaxed);
        telemetry.activeFactor.store(1 << bank.stages, std::memory_order_relaxed);

        Controls effective = target;
        if (keyChannels <= 0 || key == nullptr) { effective.compExtKey = false; effective.gateExtKey = false; }

        double inSq[2] = {}, outSq[2] = {};
        float inPk[2] = {}, outPk[2] = {};
        float cgr = 0, ggr = 0, lgr = 0, dl = 0;
        bool over = false;

        // Controls update on a fixed grid of kControlBlock samples of the stream, independent of how the host
        // splits it into blocks, so output does not depend on the buffer size.
        for (int start = 0, n = 0; start < numSamples; start += n)
        {
            n = std::min(kControlBlock - controlPhase, numSamples - start);
            if (controlPhase == 0) smoothControls(effective, kControlBlock, bank);
            controlPhase = (controlPhase + n) % kControlBlock;
            for (int ch = 0; ch < nch; ++ch)
            {
                double base[kControlBlock], kbase[kControlBlock];
                for (int i = 0; i < n; ++i)
                {
                    base[i] = double(io[ch][start + i]) * kFullScalePeak;
                    const int kc = keyChannels > 1 ? ch : 0;
                    kbase[i] = (keyChannels > 0 && key) ? double(key[kc][start + i]) * kFullScalePeak * kKeyReceiver : 0.0;
                }
                bank.os[size_t(bank.stages)].upsample(ch, base, n, work[size_t(ch)].data());
                bank.os[size_t(bank.stages)].upsample(kMaxChannels + ch, kbase, n, keyWork[size_t(ch)].data());
            }
            const int factor = 1 << bank.stages;
            const int m = n * factor;
            for (int i = 0; i < m; ++i)
            {
                double cl[2], gl[2];
                for (int ch = 0; ch < nch; ++ch)
                {
                    auto& c = bank.ch[size_t(ch)];
                    c.front(work[size_t(ch)][size_t(i)], keyWork[size_t(ch)][size_t(i)]);
                    cl[ch] = c.compLevel();
                    gl[ch] = c.gateLevel();
                }
                if (nch == 2 && smoothed.link)
                {
                    cl[0] = cl[1] = std::max(cl[0], cl[1]);
                    gl[0] = gl[1] = std::max(gl[0], gl[1]);
                }
                for (int ch = 0; ch < nch; ++ch)
                {
                    auto& c = bank.ch[size_t(ch)];
                    const double y = c.back(cl[ch], gl[ch]);
                    work[size_t(ch)][size_t(i)] = y;
                    const double vin = std::abs(c.inputTap) * kInternalToConnector, vout = std::abs(y);
                    inPk[ch] = std::max(inPk[ch], float(vin / kFullScalePeak));
                    outPk[ch] = std::max(outPk[ch], float(vout / kFullScalePeak));
                    inSq[ch] += vin * vin;
                    outSq[ch] += vout * vout;
                    over = over || std::abs(c.inputTap) > 0.5 * kSwingPositive || std::abs(c.dynTap) > 0.5 * kSwingPositive
                           || std::abs(c.outputTap) > 0.5 * kSwingPositive;
                }
                for (int ch = 0; ch < nch; ++ch)
                {
                    cgr = std::max(cgr, float(bank.ch[size_t(ch)].compGr));
                    ggr = std::max(ggr, float(bank.ch[size_t(ch)].gateGr));
                }
            }
            for (int ch = 0; ch < nch; ++ch)
            {
                double out[kControlBlock];
                bank.os[size_t(bank.stages)].downsample(ch, work[size_t(ch)].data(), n, out);
                for (int i = 0; i < n; ++i)
                {
                    double y = out[i] / kFullScalePeak;
                    if (fadeLeft - i > 0) y *= 1.0 - double(fadeLeft - i) / (0.01 * fs);
                    io[ch][start + i] = Sample(y);
                }
            }
            fadeLeft = std::max(0, fadeLeft - n);
            lgr = std::max(lgr, float(bank.ch[0].limiterGr()));
            dl = std::max(dl, float(bank.ch[0].deessIllumination()));
        }

        const int total = std::max(1, numSamples << bank.stages);
        for (int ch = 0; ch < nch; ++ch)
        {
            Telemetry::raise(telemetry.inputPeak[ch], inPk[ch]);
            Telemetry::raise(telemetry.outputPeak[ch], outPk[ch]);
            telemetry.inputSquare[ch].store(float(inSq[ch] / total / (kFullScalePeak * kFullScalePeak)), std::memory_order_relaxed);
            telemetry.outputSquare[ch].store(float(outSq[ch] / total / (kFullScalePeak * kFullScalePeak)), std::memory_order_relaxed);
        }
        Telemetry::raise(telemetry.compGr, cgr);
        Telemetry::raise(telemetry.gateGr, ggr);
        Telemetry::raise(telemetry.limiterGr, lgr);
        Telemetry::raise(telemetry.deessLed, dl);
        if (over) telemetry.overload.store(true, std::memory_order_relaxed);
    }

    int numChannels() const { return channels; }
    int latencySamples() const { return 0; }
    Telemetry telemetry;

private:
    // The external key enters through the same balanced receiver as the line input (half the differential
    // level), so an identical signal on the key and the main input produces the same detector level.
    static inline const double kKeyReceiver = std::pow(10.0, calibration::kLineInputDb / 20.0);

    struct Bank
    {
        std::array<Channel, kMaxChannels> ch;
        std::array<dsp::Oversampler, kMaxStages + 1> os;
        int stages = -1;
        bool configured = false;
    };

    double fs = 48000.0;
    int channels = 2, maxBlockSize = 512;
    int liveStages = 1, renderStages = -1;
    int fadeLeft = 0, controlPhase = 0;
    bool wasOffline = false;
    Bank live, render;
    std::array<std::vector<double>, kMaxChannels> work, keyWork;
    Controls smoothed;
    bool smoothedValid = false;

    int renderStagesResolved() const { return renderStages < 0 ? liveStages : renderStages; }

    void selectStages(Bank& b, int stages)
    {
        b.stages = stages;
        const double rate = fs * double(1 << stages);
        for (auto& c : b.ch) c.prepare(rate);
        b.os[size_t(stages)].reset();
        for (auto& c : b.ch) c.configure(smoothedValid ? smoothed : Controls {});
        b.configured = smoothedValid;
    }

    void startRender()
    {
        selectStages(render, renderStages);
        for (auto& c : render.ch) c.reset();
        for (auto& o : render.os) o.reset();
        controlPhase = 0;
    }

    // Continuous controls glide towards their targets (about 20 ms); switches apply at once.
    void smoothControls(const Controls& t, int n, Bank& bank)
    {
        if (!smoothedValid) { smoothed = t; smoothedValid = true; for (auto& c : bank.ch) c.configure(smoothed); bank.configured = true; return; }
        const double a = 1.0 - std::exp(-double(n) / (0.02 * fs));
        Controls s = t;
        auto glide = [&](double& cur, double tgt, double& out) { out = cur + (tgt - cur) * a; if (std::abs(out - tgt) < 1e-4 * (1.0 + std::abs(tgt))) out = tgt; };
        auto logGlide = [&](double& cur, double tgt, double& out) { out = std::exp(std::log(cur) + (std::log(tgt) - std::log(cur)) * a); if (std::abs(out / tgt - 1.0) < 1e-4) out = tgt; };
        glide(smoothed.trimDb, t.trimDb, s.trimDb);
        logGlide(smoothed.hpfHz, t.hpfHz, s.hpfHz);
        logGlide(smoothed.lpfHz, t.lpfHz, s.lpfHz);
        glide(smoothed.lfDb, t.lfDb, s.lfDb);
        glide(smoothed.hfDb, t.hfDb, s.hfDb);
        glide(smoothed.lmfDb, t.lmfDb, s.lmfDb);
        logGlide(smoothed.lmfHz, t.lmfHz, s.lmfHz);
        logGlide(smoothed.lmfQ, t.lmfQ, s.lmfQ);
        glide(smoothed.hmfDb, t.hmfDb, s.hmfDb);
        logGlide(smoothed.hmfHz, t.hmfHz, s.hmfHz);
        logGlide(smoothed.hmfQ, t.hmfQ, s.hmfQ);
        glide(smoothed.makeupDb, t.makeupDb, s.makeupDb);
        glide(smoothed.mix, t.mix, s.mix);
        logGlide(smoothed.deessHz, t.deessHz, s.deessHz);
        glide(smoothed.outputDb, t.outputDb, s.outputDb);
        if (s == smoothed && bank.configured) return;
        smoothed = s;
        for (auto& c : bank.ch) c.configure(smoothed);
    }
};
} // namespace bluechannel::model
