// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

// Small text netlist format for the linear audio stages.
//
// The netlists in Netlists.h are transcriptions of the reference circuit model, one stage each. Supply rails
// and grounds are AC ground for the linear model. Element lines:
//
//   R  name a b value [keep]          resistor
//   C  name a b value [keep]          capacitor (simplification rules below unless "keep")
//   L  name a b value                 inductor
//   P  name a b total param           potentiometer leg, R = max(1m, total * param)
//   Q  name a b total param           complementary leg, R = max(1m, total * (1 - param))
//   K  name a b value param           scaled resistor, R = value * param
//   CS name a b value param [keep]    scaled capacitor, C = value * param
//   S  name a b param k               switch, closed when round(param) == k
//   N  name a b param k               switch, closed when round(param) != k
//   O  name plus minus out            ideal operational amplifier (nullor)
//   X  name p1 p2 s1 s2 ratio         ideal transformer, V(s1,s2) = ratio * V(p1,p2)
//   I  name node                      input: ideal voltage source from ground
//   J  name node                      input: current injected into the node (amperes)
//   Y  name node                      output probe (node voltage)
//   .option short=<C> open=<C>        per-stage capacitor simplification thresholds
//
// Values take SPICE suffixes (f p n u m k meg g). Comments start with '*' or ';'.
//
// Simplification: capacitors at or above `short` (default 10 uF) are treated as short circuits and those at
// or below `open` (default 470 pF) as open circuits, unless marked "keep". Coupling capacitors then lose
// their sub-audio poles and compensation capacitors their megahertz poles. The chain applies one explicit
// DC blocker instead. Every stage is compared against the full reference subcircuit in tests.

#include <cctype>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace bluechannel::circuit
{
enum class ElemType : std::uint8_t { R, C, L, OpAmp, Xfmr, Switch };
enum class ValueKind : std::uint8_t { Fixed, PotTop, PotBottom, Scaled };

struct Element
{
    ElemType type = ElemType::R;
    std::string name;
    int a = 0, b = 0, c = 0, d = 0;   // nodes (op-amp: plus, minus, out; transformer: p1, p2, s1, s2)
    double value = 0.0;               // ohm, farad, henry or turns ratio
    ValueKind kind = ValueKind::Fixed;
    int param = -1;                   // parameter index for pots, scaled values and switches
    int select = 0;                   // switch position
    bool invert = false;              // switch closed when param != select
    bool keep = false;                // exempt from capacitor simplification
};

struct Netlist
{
    std::vector<std::string> nodeNames { "0" };   // node 0 is ground
    std::vector<Element> elements;
    std::vector<std::string> paramNames;
    std::vector<int> inputs, outputs;
    std::vector<bool> inputIsCurrent;
    std::vector<std::string> inputNames, outputNames;
    double shortCap = 10e-6;
    double openCap = 470e-12;

    int node(const std::string& raw)
    {
        std::string n = lower(raw);
        static const char* grounds[] = { "0", "gnd", "ag", "vp", "vn", "ch", "dg", "lp", "p30" };
        for (auto* g : grounds)
            if (n == g) return 0;
        for (size_t i = 0; i < nodeNames.size(); ++i)
            if (nodeNames[i] == n) return int(i);
        nodeNames.push_back(n);
        return int(nodeNames.size() - 1);
    }

    int param(const std::string& raw)
    {
        std::string n = lower(raw);
        for (size_t i = 0; i < paramNames.size(); ++i)
            if (paramNames[i] == n) return int(i);
        paramNames.push_back(n);
        return int(paramNames.size() - 1);
    }

    int findParam(const std::string& raw) const
    {
        std::string n = lower(raw);
        for (size_t i = 0; i < paramNames.size(); ++i)
            if (paramNames[i] == n) return int(i);
        return -1;
    }

    int findInput(const std::string& name) const
    {
        for (size_t i = 0; i < inputNames.size(); ++i)
            if (inputNames[i] == lower(name)) return int(i);
        return -1;
    }

    int findOutput(const std::string& name) const
    {
        for (size_t i = 0; i < outputNames.size(); ++i)
            if (outputNames[i] == lower(name)) return int(i);
        return -1;
    }

    static std::string lower(std::string s)
    {
        for (auto& ch : s) ch = char(std::tolower((unsigned char) ch));
        return s;
    }
};

// Locale-independent number parser (a host may set a comma-decimal C locale, which std::stod would follow).
inline double parseNumber(const std::string& s, size_t& pos)
{
    pos = 0;
    double sign = 1.0;
    if (pos < s.size() && (s[pos] == '+' || s[pos] == '-')) sign = s[pos++] == '-' ? -1.0 : 1.0;
    double v = 0.0;
    bool digits = false;
    while (pos < s.size() && std::isdigit((unsigned char) s[pos])) { v = v * 10.0 + double(s[pos++] - '0'); digits = true; }
    if (pos < s.size() && s[pos] == '.')
    {
        ++pos;
        double scale = 0.1;
        while (pos < s.size() && std::isdigit((unsigned char) s[pos])) { v += double(s[pos++] - '0') * scale; scale *= 0.1; digits = true; }
    }
    if (!digits) throw std::runtime_error("netlist: bad number " + s);
    if (pos + 1 < s.size() && (s[pos] == 'e' || s[pos] == 'E') && (std::isdigit((unsigned char) s[pos + 1]) || s[pos + 1] == '-' || s[pos + 1] == '+'))
    {
        size_t q = pos + 1;
        int esign = 1;
        if (s[q] == '+' || s[q] == '-') esign = s[q++] == '-' ? -1 : 1;
        int e = 0;
        bool edigits = false;
        while (q < s.size() && std::isdigit((unsigned char) s[q])) { e = e * 10 + (s[q++] - '0'); edigits = true; }
        if (edigits) { v *= std::pow(10.0, double(esign * e)); pos = q; }
    }
    return sign * v;
}

inline double parseValue(const std::string& raw)
{
    const std::string s = Netlist::lower(raw);
    size_t pos = 0;
    double v = parseNumber(s, pos);
    const std::string suf = s.substr(pos);
    if (suf.rfind("meg", 0) == 0) return v * 1e6;
    if (suf.empty()) return v;
    switch (suf[0])
    {
        case 'f': return v * 1e-15;
        case 'p': return v * 1e-12;
        case 'n': return v * 1e-9;
        case 'u': return v * 1e-6;
        case 'm': return v * 1e-3;
        case 'k': return v * 1e3;
        case 'g': return v * 1e9;
        default: return v;
    }
}

inline Netlist parseNetlist(const char* text)
{
    Netlist net;
    std::string src(text);
    size_t start = 0;
    int lineNo = 0;
    while (start <= src.size())
    {
        size_t end = src.find('\n', start);
        if (end == std::string::npos) end = src.size();
        std::string line = src.substr(start, end - start);
        start = end + 1;
        ++lineNo;
        if (auto c = line.find_first_of(";*"); c != std::string::npos) line = line.substr(0, c);

        std::vector<std::string> tok;
        size_t p = 0;
        while (p < line.size())
        {
            while (p < line.size() && std::isspace((unsigned char) line[p])) ++p;
            size_t q = p;
            while (q < line.size() && !std::isspace((unsigned char) line[q])) ++q;
            if (q > p) tok.push_back(line.substr(p, q - p));
            p = q;
        }
        if (tok.empty()) continue;

        const std::string kw = Netlist::lower(tok[0]);
        auto need = [&](size_t n) {
            if (tok.size() < n) throw std::runtime_error("netlist line " + std::to_string(lineNo) + ": too few fields");
        };
        auto hasKeep = [&](size_t idx) { return tok.size() > idx && Netlist::lower(tok[idx]) == "keep"; };

        if (kw == ".option")
        {
            for (size_t i = 1; i < tok.size(); ++i)
            {
                const auto eq = tok[i].find('=');
                if (eq == std::string::npos) continue;
                const auto key = Netlist::lower(tok[i].substr(0, eq));
                const double v = parseValue(tok[i].substr(eq + 1));
                if (key == "short") net.shortCap = v;
                if (key == "open") net.openCap = v;
            }
            continue;
        }

        Element e;
        if (kw == "r" || kw == "c" || kw == "l")
        {
            need(5);
            e.type = kw == "r" ? ElemType::R : kw == "c" ? ElemType::C : ElemType::L;
            e.name = tok[1]; e.a = net.node(tok[2]); e.b = net.node(tok[3]);
            e.value = parseValue(tok[4]);
            e.keep = hasKeep(5);
        }
        else if (kw == "p" || kw == "q")
        {
            need(6);
            e.type = ElemType::R; e.name = tok[1]; e.a = net.node(tok[2]); e.b = net.node(tok[3]);
            e.value = parseValue(tok[4]); e.param = net.param(tok[5]);
            e.kind = kw == "p" ? ValueKind::PotTop : ValueKind::PotBottom;
        }
        else if (kw == "k" || kw == "cs")
        {
            need(6);
            e.type = kw == "k" ? ElemType::R : ElemType::C;
            e.name = tok[1]; e.a = net.node(tok[2]); e.b = net.node(tok[3]);
            e.value = parseValue(tok[4]); e.param = net.param(tok[5]);
            e.kind = ValueKind::Scaled;
            e.keep = hasKeep(6);
        }
        else if (kw == "s" || kw == "n")
        {
            need(6);
            e.type = ElemType::Switch; e.name = tok[1]; e.a = net.node(tok[2]); e.b = net.node(tok[3]);
            { size_t pp = 0; e.param = net.param(tok[4]); e.select = int(parseNumber(tok[5], pp)); e.invert = kw == "n"; }
        }
        else if (kw == "o")
        {
            need(5);
            e.type = ElemType::OpAmp; e.name = tok[1];
            e.a = net.node(tok[2]); e.b = net.node(tok[3]); e.c = net.node(tok[4]);
        }
        else if (kw == "x")
        {
            need(7);
            e.type = ElemType::Xfmr; e.name = tok[1];
            e.a = net.node(tok[2]); e.b = net.node(tok[3]); e.c = net.node(tok[4]); e.d = net.node(tok[5]);
            e.value = parseValue(tok[6]);
        }
        else if (kw == "i" || kw == "j")
        {
            need(3);
            net.inputNames.push_back(Netlist::lower(tok[1]));
            net.inputs.push_back(net.node(tok[2]));
            net.inputIsCurrent.push_back(kw == "j");
            continue;
        }
        else if (kw == "y")
        {
            need(3);
            net.outputNames.push_back(Netlist::lower(tok[1]));
            net.outputs.push_back(net.node(tok[2]));
            continue;
        }
        else
            throw std::runtime_error("netlist line " + std::to_string(lineNo) + ": unknown element " + tok[0]);
        net.elements.push_back(e);
    }
    return net;
}
} // namespace bluechannel::circuit
