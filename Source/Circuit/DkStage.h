// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

// Linear circuit stage discretised with the nodal DK method (trapezoidal companion models).
//
// Every capacitor and inductor becomes a conductance G plus a history current x (the state). With the
// modified nodal analysis matrix M of the companion network:
//
//     v = M^-1 (Nx' x + Nu' u)          node voltages and auxiliary currents
//     y = No v                          output node voltages
//     x[n+1] = c (Nx v) + s x[n]        c = 2G, s = -1 for capacitors; c = -2G, s = +1 for inductors
//
// which gives the state-space form y = C x + D u, x' = A x + B u. Compilation (topology, assembly, LU and
// the A/B/C/D products) runs on the audio thread when a control moves; it uses only buffers sized in
// load() and costs O(N^3) for N unknowns (about 20 to 40 here). Switches and simplified capacitors are
// applied by merging nodes, and nodes left dangling by an open switch are pruned so they add no states.
//
// The operational amplifiers are ideal; output-swing limits are applied by the model that owns the stage.

#include "Netlist.h"

#include <algorithm>
#include <complex>

namespace bluechannel::circuit
{
class DkStage
{
public:
    DkStage() = default;
    explicit DkStage(const char* netlistText) { load(parseNetlist(netlistText)); }

    // Allocates. Call at preparation time only.
    void load(Netlist n)
    {
        net = std::move(n);
        const int nodes = int(net.nodeNames.size());
        int nOp = 0, nX = 0, nReact = 0;
        for (auto& e : net.elements)
        {
            nOp += e.type == ElemType::OpAmp;
            nX += e.type == ElemType::Xfmr;
            nReact += e.type == ElemType::C || e.type == ElemType::L;
        }
        nIn = int(net.inputs.size());
        nOut = int(net.outputs.size());
        nVin = 0;
        for (bool cur : net.inputIsCurrent) nVin += !cur;
        maxN = nodes + nOp + nX + nVin;
        maxS = nReact;
        params.assign(net.paramNames.size(), 0.0);
        parent.assign(size_t(nodes), 0);
        unk.assign(size_t(nodes), -1);
        nb1.assign(size_t(nodes), -2);
        nbMulti.assign(size_t(nodes), 0);
        pinned.assign(size_t(nodes), 0);
        active.assign(net.elements.size(), 0);
        stateByElem.assign(net.elements.size(), 0.0);
        M.assign(size_t(maxN * maxN), 0.0);
        piv.assign(size_t(maxN), 0);
        Z.assign(size_t(maxN * (maxS + nIn)), 0.0);
        stateElem.assign(size_t(maxS), 0);
        stateC.assign(size_t(maxS), 0.0);
        stateSign.assign(size_t(maxS), 0.0);
        stateA.assign(size_t(maxS), 0);
        stateB.assign(size_t(maxS), 0);
        A.assign(size_t(maxS * maxS), 0.0);
        B.assign(size_t(maxS * std::max(1, nIn)), 0.0);
        C.assign(size_t(std::max(1, nOut) * maxS), 0.0);
        D.assign(size_t(std::max(1, nOut) * std::max(1, nIn)), 0.0);
        x.assign(size_t(maxS), 0.0);
        xn.assign(size_t(maxS), 0.0);
        S = 0;
        dirty = true;
        topologyDirty = true;
    }

    const Netlist& netlist() const { return net; }
    int param(const char* name) const { return net.findParam(name); }
    int input(const char* name) const { return net.findInput(name); }
    int output(const char* name) const { return net.findOutput(name); }
    int numInputs() const { return nIn; }
    int numOutputs() const { return nOut; }
    int numStates() const { return S; }
    int numUnknowns() const { return N; }

    void setSampleRate(double fs)
    {
        if (fs != sampleRate) { sampleRate = fs; dirty = true; }
    }

    // Bilinear frequency prewarping: the response is exact at this frequency (0 disables).
    void setPrewarp(double hz)
    {
        if (hz != prewarpHz) { prewarpHz = hz; dirty = true; }
    }

    void setParam(int index, double value)
    {
        if (index < 0 || index >= int(params.size())) return;
        if (params[size_t(index)] == value) return;
        // A switch that changes position changes the topology.
        for (auto& e : net.elements)
            if (e.param == index && (e.type == ElemType::Switch))
                if (std::lround(params[size_t(index)]) != std::lround(value)) topologyDirty = true;
        params[size_t(index)] = value;
        dirty = true;
    }
    double getParam(int index) const { return params[size_t(index)]; }

    bool needsUpdate() const { return dirty || topologyDirty; }

    // Real-time safe: no allocation. Returns false if the network is singular (keeps old coefficients).
    bool update()
    {
        if (!dirty && !topologyDirty) return true;
        if (topologyDirty)
        {
            for (int k = 0; k < S; ++k) stateByElem[size_t(stateElem[size_t(k)])] = x[size_t(k)];
            compileTopology();
        }
        const bool ok = assembleAndSolve();
        if (topologyDirty)
            for (int k = 0; k < S; ++k) x[size_t(k)] = stateByElem[size_t(stateElem[size_t(k)])];
        dirty = false;
        topologyDirty = false;
        return ok;
    }

    void reset()
    {
        std::fill(x.begin(), x.end(), 0.0);
        std::fill(stateByElem.begin(), stateByElem.end(), 0.0);
    }

    // One sample: u has numInputs() values, y receives numOutputs() values.
    inline void process(const double* u, double* y) noexcept
    {
        const double* cx = x.data();
        for (int o = 0; o < nOut; ++o)
        {
            double acc = 0.0;
            const double* cr = C.data() + o * maxS;
            for (int k = 0; k < S; ++k) acc += cr[k] * cx[k];
            const double* dr = D.data() + o * std::max(1, nIn);
            for (int i = 0; i < nIn; ++i) acc += dr[i] * u[i];
            y[o] = acc;
        }
        for (int k = 0; k < S; ++k)
        {
            double acc = 0.0;
            const double* ar = A.data() + k * maxS;
            for (int j = 0; j < S; ++j) acc += ar[j] * cx[j];
            const double* br = B.data() + k * std::max(1, nIn);
            for (int i = 0; i < nIn; ++i) acc += br[i] * u[i];
            xn[size_t(k)] = acc;
        }
        std::swap(x, xn);
    }

    // Single input / single output convenience.
    inline double process1(double u) noexcept
    {
        double y[8];
        process(&u, y);
        return y[0];
    }

    // Continuous-time (analogue) response of the compiled network, for validation tools. Allocates.
    std::complex<double> analogResponse(int out, int in, double hz)
    {
        if (topologyDirty) { compileTopology(); topologyDirty = false; dirty = true; }
        using cd = std::complex<double>;
        const double w = 2.0 * 3.14159265358979323846 * hz;
        std::vector<cd> m(size_t(N * N), cd(0.0));
        std::vector<cd> r(size_t(N), cd(0.0));
        auto at = [&](int i, int j) -> cd& { return m[size_t(i * N + j)]; };
        auto stamp = [&](int a, int b, cd y) {
            const int ia = unk[size_t(find(a))], ib = unk[size_t(find(b))];
            if (ia >= 0) at(ia, ia) += y;
            if (ib >= 0) at(ib, ib) += y;
            if (ia >= 0 && ib >= 0) { at(ia, ib) -= y; at(ib, ia) -= y; }
        };
        stampStructure([&](int i, int j, double v) { at(i, j) += v; });
        for (size_t ei = 0; ei < net.elements.size(); ++ei)
        {
            if (!active[ei]) continue;
            const auto& e = net.elements[ei];
            if (e.type == ElemType::R) stamp(e.a, e.b, cd(1.0 / resistance(e)));
            else if (e.type == ElemType::C) stamp(e.a, e.b, cd(0.0, w * capacitance(e)));
            else if (e.type == ElemType::L) stamp(e.a, e.b, cd(1.0, 0.0) / cd(0.0, w * e.value));
        }
        if (const int row = inputRow(in); row >= 0) r[size_t(row)] = 1.0;
        // Gaussian elimination with partial pivoting.
        for (int k = 0; k < N; ++k)
        {
            int p = k;
            for (int i = k + 1; i < N; ++i)
                if (std::abs(at(i, k)) > std::abs(at(p, k))) p = i;
            if (p != k) { for (int j = 0; j < N; ++j) std::swap(at(k, j), at(p, j)); std::swap(r[size_t(k)], r[size_t(p)]); }
            const cd d = at(k, k);
            if (std::abs(d) < 1e-300) return cd(0.0);
            for (int i = k + 1; i < N; ++i)
            {
                const cd f = at(i, k) / d;
                if (f == cd(0.0)) continue;
                for (int j = k; j < N; ++j) at(i, j) -= f * at(k, j);
                r[size_t(i)] -= f * r[size_t(k)];
            }
        }
        for (int i = N - 1; i >= 0; --i)
        {
            cd s = r[size_t(i)];
            for (int j = i + 1; j < N; ++j) s -= at(i, j) * r[size_t(j)];
            r[size_t(i)] = s / at(i, i);
        }
        const int row = unk[size_t(find(net.outputs[size_t(out)]))];
        return row >= 0 ? r[size_t(row)] : cd(0.0);
    }

private:
    Netlist net;
    std::vector<double> params;
    double sampleRate = 48000.0, prewarpHz = 0.0;
    bool dirty = true, topologyDirty = true;

    int maxN = 0, maxS = 0, nIn = 0, nOut = 0, nVin = 0;
    int N = 0, S = 0, nodeUnknowns = 0;

    std::vector<int> parent, unk, nb1;
    std::vector<unsigned char> nbMulti, pinned, active;
    std::vector<double> stateByElem;
    std::vector<double> M, Z;
    std::vector<int> piv;
    std::vector<int> stateElem, stateA, stateB;
    std::vector<double> stateC, stateSign;
    std::vector<double> A, B, C, D, x, xn;

    int find(int i) const
    {
        while (parent[size_t(i)] != i) i = parent[size_t(i)];
        return i;
    }
    void unite(int a, int b)
    {
        a = find(a); b = find(b);
        if (a == b) return;
        if (b == 0) std::swap(a, b);        // keep ground as the root
        parent[size_t(b)] = a;
    }

    double paramValue(const Element& e) const { return e.param >= 0 ? params[size_t(e.param)] : 0.0; }

    double resistance(const Element& e) const
    {
        switch (e.kind)
        {
            case ValueKind::PotTop: return std::max(1e-3, e.value * paramValue(e));
            case ValueKind::PotBottom: return std::max(1e-3, e.value * (1.0 - paramValue(e)));
            case ValueKind::Scaled: return std::max(1e-3, e.value * paramValue(e));
            case ValueKind::Fixed: break;
        }
        return e.value;
    }
    double capacitance(const Element& e) const
    {
        return e.kind == ValueKind::Scaled ? e.value * paramValue(e) : e.value;
    }
    bool switchClosed(const Element& e) const
    {
        const bool eq = std::lround(paramValue(e)) == e.select;
        return e.invert ? !eq : eq;
    }

    void compileTopology()
    {
        const int nodes = int(net.nodeNames.size());
        for (int i = 0; i < nodes; ++i) { parent[size_t(i)] = i; unk[size_t(i)] = -1; pinned[size_t(i)] = 0; }
        for (size_t ei = 0; ei < net.elements.size(); ++ei)
        {
            const auto& e = net.elements[ei];
            active[ei] = 0;
            switch (e.type)
            {
                case ElemType::Switch:
                    if (switchClosed(e)) unite(e.a, e.b);
                    break;
                case ElemType::C:
                {
                    const double c = capacitance(e);
                    if (!e.keep && c >= net.shortCap) unite(e.a, e.b);
                    else if (!e.keep && c <= net.openCap) {}
                    else active[ei] = 1;
                    break;
                }
                case ElemType::R:
                case ElemType::L:
                case ElemType::OpAmp:
                case ElemType::Xfmr: active[ei] = 1; break;
            }
        }
        for (auto& e : net.elements)
        {
            if (e.type == ElemType::OpAmp) { pinned[size_t(find(e.a))] = pinned[size_t(find(e.b))] = pinned[size_t(find(e.c))] = 1; }
            if (e.type == ElemType::Xfmr) { pinned[size_t(find(e.a))] = pinned[size_t(find(e.b))] = pinned[size_t(find(e.c))] = pinned[size_t(find(e.d))] = 1; }
        }
        for (int n : net.inputs) pinned[size_t(find(n))] = 1;
        for (int n : net.outputs) pinned[size_t(find(n))] = 1;

        // Prune dangling nodes: a node whose two-terminal elements all lead to one other node carries no current.
        for (int pass = 0; pass < nodes; ++pass)
        {
            for (int i = 0; i < nodes; ++i) { nb1[size_t(i)] = -2; nbMulti[size_t(i)] = 0; }
            auto note = [&](int r, int other) {
                if (nb1[size_t(r)] == -2) nb1[size_t(r)] = other;
                else if (nb1[size_t(r)] != other) nbMulti[size_t(r)] = 1;
            };
            for (size_t ei = 0; ei < net.elements.size(); ++ei)
            {
                const auto& e = net.elements[ei];
                if (!active[ei] || (e.type != ElemType::R && e.type != ElemType::C && e.type != ElemType::L)) continue;
                const int ra = find(e.a), rb = find(e.b);
                if (ra == rb) { active[ei] = 0; continue; }
                note(ra, rb);
                note(rb, ra);
            }
            bool changed = false;
            for (size_t ei = 0; ei < net.elements.size(); ++ei)
            {
                const auto& e = net.elements[ei];
                if (!active[ei] || (e.type != ElemType::R && e.type != ElemType::C && e.type != ElemType::L)) continue;
                for (int r : { find(e.a), find(e.b) })
                    if (r != 0 && !pinned[size_t(r)] && !nbMulti[size_t(r)]) { active[ei] = 0; changed = true; break; }
            }
            if (!changed) break;
        }

        nodeUnknowns = 0;
        auto use = [&](int n) {
            const int r = find(n);
            if (r != 0 && unk[size_t(r)] < 0) unk[size_t(r)] = nodeUnknowns++;
        };
        for (size_t ei = 0; ei < net.elements.size(); ++ei)
        {
            const auto& e = net.elements[ei];
            if (e.type == ElemType::Switch) continue;
            if (e.type == ElemType::R || e.type == ElemType::C || e.type == ElemType::L)
            {
                if (!active[ei]) continue;
                use(e.a); use(e.b);
            }
            else if (e.type == ElemType::OpAmp) { use(e.a); use(e.b); use(e.c); }
            else if (e.type == ElemType::Xfmr) { use(e.a); use(e.b); use(e.c); use(e.d); }
        }
        for (int n : net.inputs) use(n);
        for (int n : net.outputs) use(n);
        // Path from node index to unknown through the root.
        for (int i = 0; i < nodes; ++i) unk[size_t(i)] = unk[size_t(find(i))];

        N = nodeUnknowns;
        for (auto& e : net.elements) N += (e.type == ElemType::OpAmp) + (e.type == ElemType::Xfmr);
        N += nVin;

        S = 0;
        for (size_t ei = 0; ei < net.elements.size(); ++ei)
        {
            const auto& e = net.elements[ei];
            if (!active[ei] || (e.type != ElemType::C && e.type != ElemType::L)) continue;
            stateElem[size_t(S)] = int(ei);
            stateA[size_t(S)] = unk[size_t(e.a)];
            stateB[size_t(S)] = unk[size_t(e.b)];
            ++S;
        }
    }

    // Row of the right-hand side that input i drives: the source row of a voltage input, or the node row of a
    // current input (-1 when that node is ground).
    int inputRow(int i) const
    {
        if (net.inputIsCurrent[size_t(i)]) return unk[size_t(net.inputs[size_t(i)])];
        int row = nodeUnknowns;
        for (auto& e : net.elements) row += (e.type == ElemType::OpAmp) + (e.type == ElemType::Xfmr);
        for (int j = 0; j < i; ++j) row += !net.inputIsCurrent[size_t(j)];
        return row;
    }

    // Stamps for op-amps, transformers and sources (frequency independent), plus gmin.
    template <typename Add>
    void stampStructure(Add add) const
    {
        for (int i = 0; i < nodeUnknowns; ++i) add(i, i, 1e-12);
        int q = nodeUnknowns;
        for (auto& e : net.elements)
        {
            if (e.type == ElemType::OpAmp)
            {
                const int p = unk[size_t(e.a)], m = unk[size_t(e.b)], o = unk[size_t(e.c)];
                if (o >= 0) add(o, q, -1.0);           // output current injected into the output node
                if (p >= 0) add(q, p, 1.0);            // V(plus) - V(minus) = 0
                if (m >= 0) add(q, m, -1.0);
                ++q;
            }
            else if (e.type == ElemType::Xfmr)
            {
                const int p1 = unk[size_t(e.a)], p2 = unk[size_t(e.b)], s1 = unk[size_t(e.c)], s2 = unk[size_t(e.d)];
                const double n = e.value;
                if (s1 >= 0) { add(s1, q, 1.0); add(q, s1, 1.0); }
                if (s2 >= 0) { add(s2, q, -1.0); add(q, s2, -1.0); }
                if (p1 >= 0) { add(p1, q, -n); add(q, p1, -n); }
                if (p2 >= 0) { add(p2, q, n); add(q, p2, n); }
                ++q;
            }
        }
        for (int i = 0; i < nIn; ++i)
        {
            if (net.inputIsCurrent[size_t(i)]) continue;
            const int nd = unk[size_t(net.inputs[size_t(i)])];
            if (nd >= 0) { add(nd, q, 1.0); add(q, nd, 1.0); }
            ++q;
        }
    }

    bool assembleAndSolve()
    {
        const double fs = sampleRate;
        double K = 2.0 * fs;
        if (prewarpHz > 0.0 && prewarpHz < 0.45 * fs)
        {
            const double w0 = 2.0 * 3.14159265358979323846 * prewarpHz;
            K = w0 / std::tan(w0 / (2.0 * fs));
        }
        std::fill(M.begin(), M.begin() + N * N, 0.0);
        auto at = [&](int i, int j) -> double& { return M[size_t(i * N + j)]; };
        auto stamp = [&](int ia, int ib, double g) {
            if (ia >= 0) at(ia, ia) += g;
            if (ib >= 0) at(ib, ib) += g;
            if (ia >= 0 && ib >= 0) { at(ia, ib) -= g; at(ib, ia) -= g; }
        };
        stampStructure([&](int i, int j, double v) { at(i, j) += v; });
        for (size_t ei = 0; ei < net.elements.size(); ++ei)
            if (active[ei] && net.elements[ei].type == ElemType::R)
                stamp(unk[size_t(net.elements[ei].a)], unk[size_t(net.elements[ei].b)], 1.0 / resistance(net.elements[ei]));
        for (int k = 0; k < S; ++k)
        {
            const auto& e = net.elements[size_t(stateElem[size_t(k)])];
            const bool cap = e.type == ElemType::C;
            const double g = cap ? K * capacitance(e) : 1.0 / (K * e.value);
            stamp(stateA[size_t(k)], stateB[size_t(k)], g);
            stateC[size_t(k)] = cap ? 2.0 * g : -2.0 * g;
            stateSign[size_t(k)] = cap ? -1.0 : 1.0;
        }

        // LU with partial pivoting, in place.
        for (int k = 0; k < N; ++k)
        {
            int p = k;
            double best = std::abs(at(k, k));
            for (int i = k + 1; i < N; ++i)
                if (std::abs(at(i, k)) > best) { best = std::abs(at(i, k)); p = i; }
            piv[size_t(k)] = p;
            if (best < 1e-300) return false;
            if (p != k)
                for (int j = 0; j < N; ++j) std::swap(at(k, j), at(p, j));
            const double inv = 1.0 / at(k, k);
            for (int i = k + 1; i < N; ++i)
            {
                double& f = at(i, k);
                if (f == 0.0) continue;
                f *= inv;
                for (int j = k + 1; j < N; ++j) at(i, j) -= f * at(k, j);
            }
        }

        // Right-hand sides: one column per state (unit history current) and per input (unit source).
        const int cols = S + nIn;
        std::fill(Z.begin(), Z.begin() + N * cols, 0.0);
        for (int k = 0; k < S; ++k)
        {
            double* z = Z.data() + k * N;
            if (stateA[size_t(k)] >= 0) z[stateA[size_t(k)]] += 1.0;
            if (stateB[size_t(k)] >= 0) z[stateB[size_t(k)]] -= 1.0;
        }
        for (int i = 0; i < nIn; ++i)
            if (const int row = inputRow(i); row >= 0) Z[size_t((S + i) * N + row)] = 1.0;
        for (int c = 0; c < cols; ++c)
        {
            double* z = Z.data() + c * N;
            for (int k = 0; k < N; ++k)
                if (piv[size_t(k)] != k) std::swap(z[k], z[piv[size_t(k)]]);
            for (int i = 1; i < N; ++i)
            {
                double s = z[i];
                for (int j = 0; j < i; ++j) s -= at(i, j) * z[j];
                z[i] = s;
            }
            for (int i = N - 1; i >= 0; --i)
            {
                double s = z[i];
                for (int j = i + 1; j < N; ++j) s -= at(i, j) * z[j];
                z[i] = s / at(i, i);
            }
        }

        auto zv = [&](int col, int row) { return row >= 0 ? Z[size_t(col * N + row)] : 0.0; };
        const int nInS = std::max(1, nIn);
        for (int k = 0; k < S; ++k)
        {
            for (int j = 0; j < S; ++j)
                A[size_t(k * maxS + j)] = stateC[size_t(k)] * (zv(j, stateA[size_t(k)]) - zv(j, stateB[size_t(k)]))
                                          + (k == j ? stateSign[size_t(k)] : 0.0);
            for (int i = 0; i < nIn; ++i)
                B[size_t(k * nInS + i)] = stateC[size_t(k)] * (zv(S + i, stateA[size_t(k)]) - zv(S + i, stateB[size_t(k)]));
        }
        for (int o = 0; o < nOut; ++o)
        {
            const int row = unk[size_t(net.outputs[size_t(o)])];
            for (int k = 0; k < S; ++k) C[size_t(o * maxS + k)] = zv(k, row);
            for (int i = 0; i < nIn; ++i) D[size_t(o * nInS + i)] = zv(S + i, row);
        }
        return true;
    }
};
} // namespace bluechannel::circuit
