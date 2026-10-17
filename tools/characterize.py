#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 OpenGrid
"""Nonlinear characterisation of reference subcircuits with ngspice (transient + Fourier analysis).

Measures what the reduced model is fitted to:
  vca-law   gain of the VCA gain cell at 1 kHz versus control sink current
  vca-thd   output fundamental and harmonics 2..5 versus input level, at several control currents
  inst      instrument stage gain and harmonics versus trim position and level

Writes CSV to stdout. Requires the reference deck via --cir or BLUE_REFERENCE_CIR (see spice_reference.py).
"""
import argparse
import math
import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(__file__))
from spice_reference import RAILS, library  # noqa: E402


def run_deck(lines, tmp):
    path = os.path.join(tmp, "deck.cir")
    with open(path, "w") as f:
        f.write("\n".join(lines) + "\n")
    res = subprocess.run(["ngspice", "-b", path], capture_output=True, text=True)
    return res.stdout + res.stderr


def harmonics(t, y, f0, cycles):
    """Amplitude of harmonics 1..5 over the last `cycles` periods (rectangular window, whole periods)."""
    period = 1.0 / f0
    t_end = t[-1]
    t0 = t_end - cycles * period
    sel = [(ti, yi) for ti, yi in zip(t, y) if ti >= t0]
    out = []
    for h in range(1, 6):
        re = im = 0.0
        for i in range(1, len(sel)):
            dt = sel[i][0] - sel[i - 1][0]
            ti, yi = sel[i][0], sel[i][1]
            re += yi * math.cos(2 * math.pi * h * f0 * ti) * dt
            im += yi * math.sin(2 * math.pi * h * f0 * ti) * dt
        T = sel[-1][0] - sel[0][0]
        out.append(2.0 * math.hypot(re, im) / T)
    dc = sum(yi for _, yi in sel) / len(sel)
    return out, dc


def transient(lib, tmp, inst, amp, f0, cycles=6, settle=8, extra=(), probe="v(out)"):
    libpath = os.path.join(tmp, "ref.lib")
    with open(libpath, "w") as f:
        f.write(lib)
    total = (cycles + settle) / f0
    step = 1.0 / (f0 * 400)
    lines = [f"* characterisation", f".include {libpath}"]
    for r, v in RAILS.items():
        lines.append(f"V{r} {r} 0 {v}")
    lines.append(f"VIN stim 0 DC 0 SIN(0 {amp} {f0})")
    lines += list(extra)
    lines.append(inst)
    out = os.path.join(tmp, "tr.txt")
    lines += [".options reltol=1e-5 abstol=1e-12 vntol=1e-7 method=gear maxord=2",
              ".control", "set wr_singlescale",
              f"tran {step} {total} 0 {step}", f"wrdata {out} {probe}", "quit", ".endc", ".end"]
    log = run_deck(lines, tmp)
    t, y = [], []
    if not os.path.exists(out):
        sys.stderr.write(log[-2000:])
        return None
    with open(out) as f:
        for line in f:
            v = line.split()
            try:
                t.append(float(v[0])); y.append(float(v[1]))
            except (ValueError, IndexError):
                pass
    os.remove(out)
    return harmonics(t, y, f0, cycles)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("what", choices=["vca-law", "vca-thd", "inst"])
    ap.add_argument("--cir")
    a = ap.parse_args()
    cir = a.cir or os.environ.get("BLUE_REFERENCE_CIR")
    if not cir:
        raise SystemExit("pass --cir or set BLUE_REFERENCE_CIR")
    lib = library(open(cir, encoding="utf-8", errors="replace").read())
    vca = "X1 stim out sink vp vn 0 0 VCA_AUDIO_CURRENT params: MAKEUP=1 OFFSET_SHIFT=0.5 MAKEUP_RELAY=1"
    with tempfile.TemporaryDirectory() as tmp:
        if a.what == "vca-law":
            print("sink_ua,gain_db,dc_out")
            for ua in [0, 2, 5, 10, 15, 20, 30, 40, 50, 60, 80, 100, 120, 150, 200, 250, 300]:
                r = transient(lib, tmp, vca, 0.05, 1000.0, extra=[f"ISINK sink vn {ua}u"])
                if r is None: continue
                h, dc = r
                print(f"{ua},{20*math.log10(h[0]/0.05):.4f},{dc:.5f}", flush=True)
        elif a.what == "vca-thd":
            print("sink_ua,in_vpk,gain_db,h2_db,h3_db,h4_db,h5_db")
            for ua in [0, 25, 50, 100]:
                for amp in [0.1, 0.3, 1.0, 2.0, 3.0, 4.0, 6.0, 8.0, 11.0]:
                    r = transient(lib, tmp, vca, amp, 1000.0, extra=[f"ISINK sink vn {ua}u"])
                    if r is None: continue
                    h, _ = r
                    rel = [20 * math.log10(max(x, 1e-15) / h[0]) for x in h[1:]]
                    print(f"{ua},{amp},{20*math.log10(h[0]/amp):.4f}," + ",".join(f"{x:.2f}" for x in rel), flush=True)
        else:
            inst = "X1 stim 0 out p30 0 0 INSTRUMENT_INPUT params: TRIM={trim} FRONT=1"
            print("trim,in_vpk,gain_db,h2_db,h3_db,h4_db,h5_db,dc")
            for trim in [0.0, 0.25, 0.5, 0.75, 1.0]:
                for amp in [0.01, 0.05, 0.2, 0.5, 1.0, 2.0]:
                    r = transient(lib, tmp, inst.format(trim=trim), amp, 1000.0, extra=["RL out 0 100k"])
                    if r is None: continue
                    h, dc = r
                    rel = [20 * math.log10(max(x, 1e-15) / h[0]) for x in h[1:]]
                    print(f"{trim},{amp},{20*math.log10(h[0]/amp):.4f}," + ",".join(f"{x:.2f}" for x in rel) + f",{dc:.4f}", flush=True)


if __name__ == "__main__":
    main()
