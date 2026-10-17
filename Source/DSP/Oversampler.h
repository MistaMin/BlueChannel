// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

// Explicit-factor oversampler (1x, 2x, 4x, 8x, 16x) built from minimum-phase IIR half-band stages, so the
// reported latency is zero at every factor. Each half-band is designed for a 120 dB stop band with the pass
// band kept to 20 kHz (or 0.4535 x the session rate when that is lower). Filters and scratch are allocated
// in prepare(); upsample()/downsample() are real-time safe.

#include "HalfBand.h"

#include <algorithm>
#include <memory>
#include <vector>

namespace bluechannel::dsp
{
class Oversampler
{
public:
    static constexpr double kStopbandDb = 120.0;
    static constexpr double kPassbandHz = 20000.0;

    // Not real-time safe.
    void prepare(double baseRate, int maxBlock, int channels, int numStages)
    {
        stagesN = std::clamp(numStages, 0, 4);
        factor_ = 1 << stagesN;
        maxBlockSize = maxBlock;
        up.clear();
        down.clear();
        const double fp = std::min(kPassbandHz, 0.4535 * baseRate);
        for (int s = 0; s < stagesN; ++s)
        {
            const double fLow = baseRate * double(1 << s);
            const double tbw = std::min(0.49, (fLow - 2.0 * fp) / (2.0 * fLow));
            const auto coefs = design::iirForSpec(kStopbandDb, std::max(0.01, tbw));
            up.push_back(std::make_unique<IirHalfband>(coefs));
            down.push_back(std::make_unique<IirHalfband>(coefs));
            up.back()->prepare(channels);
            down.back()->prepare(channels);
        }
        scratchA.assign(size_t(maxBlock) * size_t(factor_), 0.0);
        scratchB.assign(size_t(maxBlock) * size_t(factor_), 0.0);
    }

    void reset()
    {
        for (auto& s : up) s->reset();
        for (auto& s : down) s->reset();
    }

    int factor() const { return factor_; }

    // in: n base-rate samples; out: n * factor() samples.
    void upsample(int ch, const double* in, int n, double* out)
    {
        if (stagesN == 0) { std::copy(in, in + n, out); return; }
        const double* src = in;
        int len = n;
        for (int s = 0; s < stagesN; ++s)
        {
            double* dst = (s == stagesN - 1) ? out : (s % 2 == 0 ? scratchA.data() : scratchB.data());
            up[size_t(s)]->up(ch, src, len, dst);
            src = dst;
            len *= 2;
        }
    }

    // in: n * factor() samples (not modified); out: n base-rate samples.
    void downsample(int ch, const double* in, int n, double* out)
    {
        if (stagesN == 0) { std::copy(in, in + n, out); return; }
        const double* src = in;
        int len = n * factor_;
        for (int s = stagesN - 1; s >= 0; --s)
        {
            len /= 2;
            double* dst = (s == 0) ? out : (s % 2 == 0 ? scratchA.data() : scratchB.data());
            down[size_t(s)]->down(ch, src, len, dst);
            src = dst;
        }
    }

private:
    int stagesN = 0, factor_ = 1, maxBlockSize = 0;
    std::vector<std::unique_ptr<HalfbandStage>> up, down;
    std::vector<double> scratchA, scratchB;
};
} // namespace bluechannel::dsp
