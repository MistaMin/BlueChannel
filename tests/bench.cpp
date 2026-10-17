// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
//
// Engine timing: equal audio duration per case, warm-up excluded, callback wall time per block.
// Usage: BlueChannelBench [seconds=10]

#include "Model/Engine.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

using namespace bluechannel::model;

int main(int argc, char** argv)
{
    const double seconds = argc > 1 ? std::atof(argv[1]) : 10.0;
    std::printf("%-26s %6s %6s %9s %10s %10s %10s %8s\n", "case", "rate", "block", "factor", "cpu %core", "p99 ms", "max ms",
                "budget");
    struct Case { const char* name; bool full; };
    for (Case cs : { Case { "line, everything out", false }, Case { "all sections in", true } })
        for (double fs : { 48000.0, 96000.0 })
            for (int stages : { 0, 1, 2, 3, 4 })
            {
                const int block = 64;
                Engine e;
                e.prepare(fs, block, 2);
                e.setOversampling(stages, -1);
                Controls c;
                if (cs.full)
                {
                    c.filterRoute = c.shelfRoute = c.paramRoute = EqRoute::In;
                    c.hpfHz = 40; c.lpfHz = 18000; c.lfDb = 3; c.hfDb = 2; c.lmfDb = -3; c.hmfDb = 4;
                    c.compIn = c.gateIn = c.deessIn = c.softLimit = true;
                    c.compThresholdDb = -10; c.gateThresholdDb = -50;
                }
                std::mt19937 rng(7);
                std::normal_distribution<float> nd(0.0f, 0.1f);
                std::vector<float> l(static_cast<size_t>(block)), r(static_cast<size_t>(block));
                float* io[2] = { l.data(), r.data() };
                const int warm = int(0.5 * fs / block), blocks = int(seconds * fs / block);
                std::vector<double> times;
                times.reserve(size_t(blocks));
                for (int b = 0; b < warm + blocks; ++b)
                {
                    for (int i = 0; i < block; ++i) { l[size_t(i)] = nd(rng); r[size_t(i)] = nd(rng); }
                    const auto t0 = std::chrono::steady_clock::now();
                    e.process<float>(io, 2, block, nullptr, 0, c, false);
                    const auto t1 = std::chrono::steady_clock::now();
                    if (b >= warm) times.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
                }
                double sum = 0;
                for (double t : times) sum += t;
                std::sort(times.begin(), times.end());
                const double budget = 1000.0 * block / fs;
                const double cpu = 100.0 * sum / (seconds * 1000.0);
                std::printf("%-26s %6.0f %6d %8dx %9.1f%% %10.4f %10.4f %7.3fms\n", cs.name, fs, block, 1 << stages, cpu,
                            times[size_t(0.99 * double(times.size()))], times.back(), budget);
            }
    return 0;
}
