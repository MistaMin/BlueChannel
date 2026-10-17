#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 OpenGrid
"""Run ngspice on one subcircuit of the reference circuit model and write its AC response as CSV.

The reference model (a SPICE deck) is not part of this repository. Pass its path with --cir or the
BLUE_REFERENCE_CIR environment variable. The script copies every .param, .model and .subckt block from
the deck into a temporary library, wires the requested subcircuit to the model's settled supply rails,
drives one pin with a 1 V AC source and probes one pin (or the difference of two pins).

Output columns: frequency_hz, magnitude_db, phase_deg.

Examples:
  spice_reference.py --subckt LMF_EQ --params GAIN=1,QPOS=0.5,FREQ_POS=0.5,X3=0 --input in --output out
  spice_reference.py --subckt TRANSFORMER_OUTPUT --input in --output hot --minus cold
"""
import argparse
import cmath
import math
import os
import re
import subprocess
import sys
import tempfile

# Settled rails of the reference model's supply operating point.
RAILS = {"vp": 16.0718781, "vn": -14.7679257, "lp": 5.19033544, "p30": 32.4049685, "phantom": 48.2230288}
GROUNDS = {"ag", "ch", "dg", "0", "gnd", "cold"}  # "cold" inputs are driven single-ended


def library(cir_text):
    """Every .param, .model and .subckt block of the deck, with continuation lines."""
    out, block, in_sub = [], [], False
    lines = cir_text.splitlines()
    i = 0
    while i < len(lines):
        line = lines[i]
        low = line.strip().lower()
        if low.startswith(".subckt"):
            in_sub = True
        if in_sub:
            out.append(line)
            if low.startswith(".ends"):
                in_sub = False
        elif low.startswith(".model") or low.startswith(".param"):
            out.append(line)
            while i + 1 < len(lines) and lines[i + 1].startswith("+"):
                i += 1
                out.append(lines[i])
        i += 1
    return "\n".join(out) + "\n"


def subckt_pins(lib_text, name):
    for line in lib_text.splitlines():
        parts = line.split()
        if len(parts) > 1 and parts[0].lower() == ".subckt" and parts[1].lower() == name.lower():
            pins = []
            for p in parts[2:]:
                if p.lower().startswith("params:") or "=" in p:
                    break
                pins.append(p)
            return pins
    raise SystemExit(f"subcircuit {name} not found")


def run(args):
    cir = args.cir or os.environ.get("BLUE_REFERENCE_CIR")
    if not cir:
        raise SystemExit("pass --cir or set BLUE_REFERENCE_CIR")
    with open(cir, encoding="utf-8", errors="replace") as f:
        lib = library(f.read())
    pins = subckt_pins(lib, args.subckt)
    with tempfile.TemporaryDirectory() as tmp:
        libpath = os.path.join(tmp, "ref.lib")
        with open(libpath, "w") as f:
            f.write(lib)
        nodes = []
        for p in pins:
            pl = p.lower()
            if pl == args.input.lower():
                nodes.append("stim")
            elif pl in GROUNDS:
                nodes.append("0")
            elif pl in RAILS:
                nodes.append(pl)
            else:
                nodes.append("n_" + pl)
        params = " ".join(args.params.split(",")) if args.params else ""
        deck = [f"* reference AC run for {args.subckt}", f".include {libpath}"]
        for r, v in RAILS.items():
            deck.append(f"V{r} {r} 0 {v}")
        deck.append(f"VIN stim 0 DC {args.dc} AC 1")
        deck.append(f"X1 {' '.join(nodes)} {args.subckt}" + (f" params: {params}" if params else ""))
        for extra in args.extra or []:
            deck.append(extra)
        def node(n):
            n = n.lower()
            return f"v({n})" if n.startswith("x1.") else f"v(n_{n})"
        probe = node(args.output)
        if args.minus:
            probe += "-" + node(args.minus)
        outpath = os.path.join(tmp, "ac.txt")
        deck += [
            ".options gmin=1e-12 reltol=1e-6",
            ".control",
            "set wr_singlescale",
            f"ac dec {args.ppd} {args.fstart} {args.fstop}",
            f"let h = {probe}",
            f"wrdata {outpath} h",
            "quit",
            ".endc",
            ".end",
        ]
        deckpath = os.path.join(tmp, "deck.cir")
        with open(deckpath, "w") as f:
            f.write("\n".join(deck) + "\n")
        res = subprocess.run(["ngspice", "-b", deckpath], capture_output=True, text=True)
        if not os.path.exists(outpath):
            sys.stderr.write(res.stdout[-3000:] + res.stderr[-3000:])
            raise SystemExit("ngspice failed")
        rows = []
        with open(outpath) as f:
            for line in f:
                vals = line.split()
                try:
                    fr, re_, im = float(vals[0]), float(vals[1]), float(vals[2])
                except (ValueError, IndexError):
                    continue
                h = complex(re_, im)
                mag = 20 * math.log10(max(abs(h), 1e-30))
                rows.append((fr, mag, math.degrees(cmath.phase(h))))
    out = sys.stdout if not args.out else open(args.out, "w")
    out.write("frequency_hz,magnitude_db,phase_deg\n")
    for r in rows:
        out.write(f"{r[0]:.6g},{r[1]:.6f},{r[2]:.4f}\n")
    if args.out:
        out.close()


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--cir")
    ap.add_argument("--subckt", required=True)
    ap.add_argument("--params", default="")
    ap.add_argument("--input", required=True)
    ap.add_argument("--output", required=True)
    ap.add_argument("--minus")
    ap.add_argument("--dc", default="0")
    ap.add_argument("--extra", action="append", help="extra deck line (load, bias)")
    ap.add_argument("--fstart", type=float, default=10)
    ap.add_argument("--fstop", type=float, default=100e3)
    ap.add_argument("--ppd", type=int, default=24)
    ap.add_argument("--out")
    run(ap.parse_args())


if __name__ == "__main__":
    main()
