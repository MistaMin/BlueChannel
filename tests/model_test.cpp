// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
//
// Native checks of the channel model and engine (no JUCE).
//
//   BlueChannelModelTest            run the checks
//   BlueChannelModelTest fit        print the fitted transformer core constant

#include "Model/Engine.h"

#include <atomic>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <string>
#include <vector>

using namespace bluechannel;
using namespace bluechannel::model;

// ---- allocation counter (only counts inside a marked scope on this thread) ----------------------------------
static thread_local bool gCount = false;
static std::atomic<long> gAllocs { 0 };
void* operator new(std::size_t n)
{
    if (gCount) ++gAllocs;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept
{
    if (gCount && p) ++gAllocs;
    std::free(p);
}
void operator delete(void* p, std::size_t) noexcept
{
    if (gCount && p) ++gAllocs;
    std::free(p);
}

static int failures = 0;
static void check(bool ok, const char* what, double value = NAN)
{
    std::printf("%-58s %12.4f  %s\n", what, value, ok ? "ok" : "FAIL");
    failures += !ok;
}

struct Harm { double h1, thd, dc; };

// Fundamental and THD (harmonics 2..9) over the last `cycles` periods.
static Harm analyse(const std::vector<double>& y, double f, double fs, int cycles)
{
    const int n = int(std::lround(cycles * fs / f));   // choose cycles so this is a whole number of periods
    const int start = int(y.size()) - n;
    double h[10] = {}, dc = 0;
    for (int k = 1; k < 10; ++k)
    {
        double re = 0, im = 0;
        for (int i = 0; i < n; ++i)
        {
            const double ph = 2.0 * M_PI * k * f * double(i) / fs;
            re += y[size_t(start + i)] * std::cos(ph);
            im += y[size_t(start + i)] * std::sin(ph);
        }
        h[k] = 2.0 * std::hypot(re, im) / n;
    }
    for (int i = 0; i < n; ++i) dc += y[size_t(start + i)];
    double s = 0;
    for (int k = 2; k < 10; ++k) s += h[k] * h[k];
    return { h[1], std::sqrt(s) / h[1], dc / n };
}

// Runs the engine on a mono sine and returns the output (host units).
static std::vector<double> runSine(Engine& e, const Controls& c, double f, double amp, double seconds, double fs, int block = 256)
{
    const int n = int(seconds * fs);
    std::vector<double> buf(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) buf[size_t(i)] = amp * std::sin(2.0 * M_PI * f * i / fs);
    for (int s = 0; s < n; s += block)
    {
        double* p = buf.data() + s;
        double* io[1] = { p };
        e.process<double>(io, 1, std::min(block, n - s), nullptr, 0, c, false);
    }
    return buf;
}

static double db(double x) { return 20.0 * std::log10(std::max(1e-30, x)); }

// THD of the mic stage alone at 50 Hz with a 200 ohm source, primary level in dBu.
static double micThd(double fluxRef, double dbu)
{
    MicStage m;
    m.load();
    const double fs = 48000.0 * 4;
    m.prepare(fs);
    m.configure(0, 200.0, 1, false);    // lowest gain (attenuator position): keeps the amplifier linear
    m.setCoreReference(fluxRef);
    const double amp = dbuToPeakVolts(dbu);
    std::vector<double> y;
    const int n = int(fs * 0.6);
    for (int i = 0; i < n; ++i) y.push_back(m.process(amp * std::sin(2.0 * M_PI * 50.0 * i / fs)));
    return analyse(y, 50.0, fs, 10).thd;
}

int main(int argc, char** argv)
{
    if (argc > 1 && std::string(argv[1]) == "fit")
    {
        double lo = 1e-4, hi = 10.0;   // THD falls as the reference flux rises
        for (int it = 0; it < 50; ++it)
        {
            const double mid = std::sqrt(lo * hi);
            (micThd(mid, 0.0) > 0.002 ? lo : hi) = mid;
        }
        const double ref = std::sqrt(lo * hi);
        std::printf("fitted kFluxRef = %.6g\n", ref);
        for (double dbu : { -10.0, 0.0, 10.0 })
            std::printf("THD at 50 Hz, %+.0f dBu, 200 ohm source: %.3f %%\n", dbu, 100.0 * micThd(ref, dbu));
        return 0;
    }

    const double fs = 48000.0;
    Engine e;
    e.prepare(fs, 512, 1);
    e.setOversampling(1, -1);
    Controls c;   // line input, 0 dB, everything out

    // 1. Unity path: -18 dBFS at 1 kHz through line input / trim / fader / output transformer.
    {
        e.reset();
        auto y = runSine(e, c, 1000.0, 0.125893, 0.5, fs);
        const auto h = analyse(y, 1000.0, fs, 20);
        check(std::abs(db(h.h1 / 0.125893)) < 0.5, "unity gain at 1 kHz (dB)", db(h.h1 / 0.125893));
        check(h.thd < 1e-3, "THD at -18 dBFS line path (fraction)", h.thd);
        // Polarity: correlation with the input over the last cycles.
        double corr = 0;
        for (int i = int(y.size()) - 960; i < int(y.size()); ++i) corr += y[size_t(i)] * std::sin(2.0 * M_PI * 1000.0 * i / fs);
        check(corr > 0, "overall polarity non-inverting", corr);
    }

    // 2. Transformer core fit (mic stage alone).
    {
        const double t0 = micThd(MicStage::kFluxRef, 0.0), t10 = micThd(MicStage::kFluxRef, 10.0);
        check(t0 > 0.0015 && t0 < 0.0025, "input transformer THD 50 Hz 0 dBu (target 0.2 %)", 100 * t0);
        check(t10 > 0.007 && t10 < 0.014, "input transformer THD 50 Hz +10 dBu (target 1 %)", 100 * t10);
    }

    // 3. EQ: low-mid boost at its centre, high-pass removes 20 Hz.
    {
        Controls q = c;
        q.paramRoute = EqRoute::In;
        q.lmfDb = 12.0; q.lmfHz = 200.0; q.lmfQ = 1.5;
        e.reset();
        auto y = runSine(e, q, 200.0, 0.05, 0.6, fs);
        const double g = db(analyse(y, 200.0, fs, 20).h1 / 0.05);
        check(std::abs(g - 12.0) < 1.0, "LMF +12 dB at 200 Hz (dB)", g);
        Controls f = c;
        f.filterRoute = EqRoute::In;
        f.hpfHz = 200.0;
        e.reset();
        auto z = runSine(e, f, 50.0, 0.05, 1.0, fs);
        const double gz = db(analyse(z, 50.0, fs, 10).h1 / 0.05);
        check(gz < -30.0, "HPF 200 Hz: 50 Hz attenuated (dB)", gz);
    }

    // 4. Compressor static curve: 1 kHz tone 20 dB over threshold at ratio 4 -> about 15 dB reduction.
    {
        Controls k = c;
        k.compIn = true; k.ratio = 4.0; k.compThresholdDb = -20.0; k.attackMs = 1.0; k.releaseS = 0.1;
        e.reset();
        // Internal level of a -18 dBFS line input is about -2.2 dBu; feed 0 dBFS (+15.8 dBu internal).
        const double amp = std::pow(10.0, (-20.0 + 20.0 + 6.19 - 22.0) / 20.0);   // 0 dBu internal
        auto y = runSine(e, k, 1000.0, amp, 1.0, fs);
        const double g = db(analyse(y, 1000.0, fs, 20).h1 / amp);
        // Internal 0 dBu is +6 dBu on the (connector-referred) threshold scale.
        const double expected = 0.6655 - dynamics::compressionDb(6.02, -20.0, 4.0, 10.0);
        check(std::abs(g - expected) < 1.0, "VCA compressor gain at 26 dB over threshold (dB)", g);
    }

    // 5. De-esser: the band output is inverted at its centre (assumption behind the control law).
    {
        DeesserStage d;
        d.prepare(fs * 2);
        d.configure(5000.0);
        std::vector<double> band;
        for (int i = 0; i < 9600; ++i) band.push_back(d.sidechain(std::sin(2.0 * M_PI * 5000.0 * i / (fs * 2))));
        double corr = 0;
        for (int i = 4800; i < 9600; ++i) corr += band[size_t(i)] * std::sin(2.0 * M_PI * 5000.0 * i / (fs * 2));
        check(corr < 0, "de-esser band inverted at centre", corr);
        Controls s = c;
        s.deessIn = true; s.deessHz = 5000.0; s.deessThresholdDb = -20.0;
        e.reset();
        auto y = runSine(e, s, 5000.0, 0.125893, 0.5, fs);
        const double g = db(analyse(y, 5000.0, fs, 50).h1 / 0.125893);
        check(g < -3.0, "de-esser reduces a loud 5 kHz tone (dB)", g);
    }

    // 6. Soft limiter: +6 dBFS input stays near the ceiling.
    {
        Controls l = c;
        l.softLimit = true;
        e.reset();
        auto y = runSine(e, l, 1000.0, 2.0, 1.0, fs);
        double pk = 0;
        for (size_t i = y.size() / 2; i < y.size(); ++i) pk = std::max(pk, std::abs(y[i]));
        check(db(pk) < 1.0, "soft limiter steady-state peak at +6 dBFS input (dBFS)", db(pk));
    }

    // 7. Block-size independence (exact).
    {
        Controls b = c;
        b.compIn = true; b.ratio = 6; b.compThresholdDb = -10; b.paramRoute = EqRoute::In; b.hmfDb = 6;
        std::vector<double> ref;
        for (int blk : { 512, 1, 17, 64 })
        {
            e.reset();
            auto y = runSine(e, b, 440.0, 0.5, 0.25, fs, blk);
            if (ref.empty()) { ref = y; continue; }
            double d = 0;
            for (size_t i = 0; i < y.size(); ++i) d = std::max(d, std::abs(y[i] - ref[i]));
            check(d < 1e-9, ("block size " + std::to_string(blk) + " matches 512 (max diff)").c_str(), d);
        }
    }

    // 8. No allocation or free inside process(), including an oversampling change and a switch change.
    {
        Controls a = c;
        a.shelfRoute = EqRoute::In; a.lfFreq = 2; a.compIn = true;
        std::vector<double> buf(512, 0.1);
        double* io[1] = { buf.data() };
        e.process<double>(io, 1, 512, nullptr, 0, a, false);
        gAllocs = 0;
        gCount = true;
        for (int i = 0; i < 20; ++i)
        {
            a.lfFreq = i % 4;
            a.lmfDb = double(i % 7);
            e.setOversampling(i % 3, -1);
            e.process<double>(io, 1, 512, nullptr, 0, a, false);
        }
        gCount = false;
        check(gAllocs == 0, "allocations + frees inside process()", double(gAllocs.load()));
        e.setOversampling(1, -1);
    }

    // 9. Overload recovery: +20 dBFS burst, then silence returns to a finite, decaying output.
    {
        Controls o = c;
        o.compIn = true; o.gateIn = true; o.deessIn = true; o.softLimit = true; o.shelfRoute = EqRoute::In; o.lfDb = 15;
        e.reset();
        auto y = runSine(e, o, 60.0, 10.0, 0.3, fs);
        std::vector<double> z(size_t(3 * fs), 0.0);   // the output coupling network clears clipping DC in ~2 s
        double* io[1] = { z.data() };
        for (int s = 0; s < int(z.size()); s += 256) { io[0] = z.data() + s; e.process<double>(io, 1, 256, nullptr, 0, o, false); }
        bool finite = true;
        for (double v : y) finite = finite && std::isfinite(v);
        for (double v : z) finite = finite && std::isfinite(v);
        double tail = 0;
        for (size_t i = z.size() - 4800; i < z.size(); ++i) tail = std::max(tail, std::abs(z[i]));
        check(finite, "finite output after +20 dBFS burst", finite);
        check(tail < 1e-3, "silence after burst decays (peak, last 0.1 s)", tail);
    }

    // 10. Offline render bank: deterministic repeated bounce.
    {
        Engine r;
        r.prepare(fs, 256, 1);
        r.setOversampling(1, 3);
        Controls k = c;
        k.compIn = true;
        std::vector<double> first, second;
        for (auto* out : { &first, &second })
        {
            std::vector<double> buf(4800);
            for (size_t i = 0; i < buf.size(); ++i) buf[i] = 0.3 * std::sin(2.0 * M_PI * 300.0 * double(i) / fs);
            // A few realtime blocks in between, then an offline pass.
            std::vector<double> rt(256, 0.2);
            double* rio[1] = { rt.data() };
            r.process<double>(rio, 1, 256, nullptr, 0, k, false);
            for (size_t s = 0; s < buf.size(); s += 256)
            {
                double* io[1] = { buf.data() + s };
                r.process<double>(io, 1, int(std::min<size_t>(256, buf.size() - s)), nullptr, 0, k, true);
            }
            *out = buf;
        }
        double d = 0;
        for (size_t i = 0; i < first.size(); ++i) d = std::max(d, std::abs(first[i] - second[i]));
        check(d < 1e-12, "repeated offline renders identical (max diff)", d);
    }

    // 11. Netlist values parse the same under a comma-decimal C locale (hosts may set one).
    {
        const char* prev = std::setlocale(LC_NUMERIC, nullptr);
        const std::string saved = prev ? prev : "C";
        const bool switched = std::setlocale(LC_NUMERIC, "de_DE.UTF-8") || std::setlocale(LC_NUMERIC, "de_DE") || std::setlocale(LC_NUMERIC, "fr_FR.UTF-8");
        const double v = circuit::parseValue("5.1k"), e = circuit::parseValue("1.5e-3"), m = circuit::parseValue("2.2meg");
        std::setlocale(LC_NUMERIC, saved.c_str());
        check(v == 5100.0 && std::abs(e - 1.5e-3) < 1e-15 && m == 2.2e6, switched ? "netlist numbers under comma-decimal locale" : "netlist numbers (locale not available)", v);
    }

    std::printf("%s (%d failures)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
