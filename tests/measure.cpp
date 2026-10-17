// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
//
// Measurement fixtures for docs/MEASUREMENTS.md: stepped-sine magnitude response of the whole engine for
// named settings, and THD versus level for the three inputs. Writes CSV to stdout.
//
// Usage: BlueChannelMeasure response <case> [rate] [stages]
//        BlueChannelMeasure thd <mic|line|inst> [rate] [stages]
//        BlueChannelMeasure list

#include "Model/Engine.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

using namespace bluechannel::model;

static std::map<std::string, Controls> cases()
{
    std::map<std::string, Controls> m;
    Controls c; c.source = Source::Line;
    m["flat"] = c;
    Controls f = c; f.filterRoute = EqRoute::In; f.hpfHz = 100; f.lpfHz = 8000; m["filters-100-8k"] = f;
    Controls s = c; s.shelfRoute = EqRoute::In; s.lfDb = 12; s.hfDb = -12; s.lfFreq = 2; s.hfFreq = 2; m["shelves-+12-12"] = s;
    Controls p = c; p.paramRoute = EqRoute::In; p.lmfDb = 12; p.lmfHz = 100; p.hmfDb = -12; p.hmfHz = 3000; m["param-boost-cut"] = p;
    Controls q = p; q.lmfQ = 2.9; q.hmfQ = 2.9; m["param-narrow"] = q;
    Controls x = p; x.hmfX3 = true; x.hmfHz = 3000; x.hmfDb = 12; m["param-hmf-x3"] = x;
    Controls mic = c; mic.source = Source::Mic; mic.micGain = 0; m["mic-lowest-gain"] = mic;
    Controls air = mic; air.air = true; m["mic-air"] = air;
    Controls hi = mic; hi.impedance = 3; m["mic-high-z"] = hi;
    Controls lo = mic; lo.impedance = 0; m["mic-low-z"] = lo;
    Controls inst = c; inst.source = Source::Instrument; inst.trimDb = 20; m["inst-max-gain"] = inst;
    return m;
}

static double toneGain(Engine& e, const Controls& c, double f, double amp, double fs)
{
    const int settle = int(fs * std::max(0.25, 8.0 / f)), n = int(std::lround(std::ceil(20.0 * fs / f)));
    std::vector<double> buf(size_t(settle + n));
    for (size_t i = 0; i < buf.size(); ++i) buf[i] = amp * std::sin(2.0 * M_PI * f * double(i) / fs);
    for (size_t s = 0; s < buf.size(); s += 256)
    {
        double* io[1] = { buf.data() + s };
        e.process<double>(io, 1, int(std::min<size_t>(256, buf.size() - s)), nullptr, 0, c, false);
    }
    double re = 0, im = 0;
    for (int i = settle; i < settle + n; ++i)
    {
        re += buf[size_t(i)] * std::cos(2.0 * M_PI * f * i / fs);
        im += buf[size_t(i)] * std::sin(2.0 * M_PI * f * i / fs);
    }
    return 2.0 * std::hypot(re, im) / n / amp;
}

int main(int argc, char** argv)
{
    if (argc < 2 || !std::strcmp(argv[1], "list"))
    {
        for (auto& [k, v] : cases()) std::printf("%s\n", k.c_str());
        return 0;
    }
    const double fs = argc > 3 ? std::atof(argv[3]) : 48000.0;
    const int stages = argc > 4 ? std::atoi(argv[4]) : 1;
    Engine e;
    e.prepare(fs, 256, 1);
    e.setOversampling(stages, -1);
    if (!std::strcmp(argv[1], "response") && argc > 2)
    {
        auto all = cases();
        auto it = all.find(argv[2]);
        if (it == all.end()) return 2;
        std::printf("frequency_hz,gain_db\n");
        for (int i = 0; i <= 60; ++i)
        {
            const double f = 20.0 * std::pow(1000.0, i / 60.0);
            if (f > 0.45 * fs) break;
            e.reset();
            std::printf("%.2f,%.4f\n", f, 20.0 * std::log10(toneGain(e, it->second, f, 0.01, fs)));
        }
        return 0;
    }
    if (!std::strcmp(argv[1], "thd") && argc > 2)
    {
        Controls c; c.source = Source::Line;
        c.source = !std::strcmp(argv[2], "mic") ? Source::Mic : !std::strcmp(argv[2], "inst") ? Source::Instrument : Source::Line;
        c.micGain = 0;
        std::printf("input_dbfs,freq_hz,gain_db,thd_percent\n");
        for (double f : { 50.0, 1000.0 })
            for (double dbfs = -30.0; dbfs <= 6.0; dbfs += 3.0)
            {
                const double amp = std::pow(10.0, dbfs / 20.0);
                const int settle = int(fs * 0.5), n = int(std::lround(10.0 * fs / f));
                std::vector<double> buf(size_t(settle + n));
                for (size_t i = 0; i < buf.size(); ++i) buf[i] = amp * std::sin(2.0 * M_PI * f * double(i) / fs);
                e.reset();
                for (size_t s = 0; s < buf.size(); s += 256)
                {
                    double* io[1] = { buf.data() + s };
                    e.process<double>(io, 1, int(std::min<size_t>(256, buf.size() - s)), nullptr, 0, c, false);
                }
                double h[10] = {};
                for (int k = 1; k < 10; ++k)
                {
                    double re = 0, im = 0;
                    for (int i = settle; i < settle + n; ++i)
                    {
                        re += buf[size_t(i)] * std::cos(2.0 * M_PI * k * f * i / fs);
                        im += buf[size_t(i)] * std::sin(2.0 * M_PI * k * f * i / fs);
                    }
                    h[k] = 2.0 * std::hypot(re, im) / n;
                }
                double s2 = 0;
                for (int k = 2; k < 10; ++k) s2 += h[k] * h[k];
                std::printf("%.1f,%.0f,%.3f,%.5f\n", dbfs, f, 20.0 * std::log10(h[1] / amp), 100.0 * std::sqrt(s2) / h[1]);
            }
        return 0;
    }
    return 2;
}
