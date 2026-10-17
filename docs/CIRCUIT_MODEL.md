# Circuit model

How Blue Channel turns the reference circuit model into a real-time engine, what is transcribed, what is
reduced to measured behaviour, what was corrected or added, and how each part is verified.

## Reference

The reference is a component-level SPICE model of a classic British channel strip (a deck for ngspice 47,
with a JSON record of components, device provenance and validation). It covers the microphone, line and
instrument inputs, trim and phase, filters, shelving and parametric EQ, the discrete class-A VCA with its
bender, the optical de-esser and three-band optical limiter, routing, fader and the transformer output,
powered by its regulated supply. Its original analogue detectors were removed by design; control ports
(VCA sink current, LED currents) are exposed for external, digital detectors. The model is validated against
itself (numerical checks), not against hardware; transformer, optical and some transistor parameters are
estimates. The reference files are not part of this repository.

The reference unit's user documentation was used for panel ranges and behaviour. It describes a later
revision, so a few features exist only there; they are listed under *Extensions*.

## Engine: nodal DK state-space

`Source/Circuit/DkStage.h` compiles a netlist into a state-space model for the current control values:

- Modified nodal analysis with ideal op-amps (nullors), ideal transformers, voltage and current inputs.
- Capacitors and inductors become trapezoidal companion models (conductance plus history current). The
  history currents are the states: `y = C x + D u`, `x' = A x + B u`.
- Switch positions merge nodes; nodes left dangling by an open switch are pruned so they add no states.
- Each recompile is an LU factorisation of 20 to 40 unknowns and runs on the audio thread when a control
  moves, using buffers sized in advance (no allocation). Bilinear warping is corrected at one frequency per
  stage (the band's centre or corner), and the whole chain runs at the oversampled rate.

Simplification: capacitors of 10 uF and above are shorted and 470 pF and below are opened unless marked
`keep`. In the shelving and parametric stages every capacitor is kept: shorting coupling capacitors there
cost up to 0.68 dB near 20 Hz. Capacitors whose RC corner is in band (for example 220 pF across 47 kohm,
a 15 kHz pole) are kept everywhere. A 2 Hz DC blocker at the input stands in for the coupling network.

## Stages

| Stage | Netlist | Behavioural additions |
|---|---|---|
| Microphone input | 1:5 input transformer, switched feedback ladder, attenuator positions | core distortion, amplifier bandwidth, rail clip |
| Line input | balanced receiver + 7-position gain switch | rail clip |
| Instrument input | none (discrete JFET/PNP stage) | gain, bass shelf and distortion fitted to ngspice |
| Trim and phase | non-inverting 0 to +20.8 dB, inverter | rail clip |
| Filters | 3rd-order low-pass and high-pass | rail clip |
| Shelving EQ | full network, 8 positions per band | rail clip |
| Parametric LMF, HMF | full networks with x3 range contacts | rail clip |
| VCA and make-up | make-up amplifier | gain cell law and distortion measured in ngspice |
| De-esser | tuned band-pass and inverter | optical cell and summing network (exact algebra) |
| Soft limiter | complementary 3-band split | optical cells and summer (exact algebra) |
| Output level | buffer, loaded fader, x2.19 amplifier | rail clip |
| Output stage | drive amp, follower, 1:2:1 transformer with tertiary feedback | estimated core-current term; terminal effect currently cancels under ideal feedback |

Op-amp swing is 2 V from each rail of the reference supply's settled operating point (+16.07 V / -14.77 V),
so clipping is asymmetric. It is applied at each stage output, not at every internal op-amp.

### Input transformer core

The netlist's magnetising inductance is linear. The non-linear excess of the magnetising current is injected
at the core node, computed from the flux of the previous sample: `i = (flux / Lm) |flux / ref|^1.4`. The
exponent and reference flux are fitted to the part's published distortion (0.2 % at 50 Hz, 0 dBu and 1 % at
+10 dBu with a 200 ohm source); the fit gives 0.200 % and 0.99 %. The output transformer uses the same form
with an estimated reference (its winding data is not published). An isolated audit on 2026-10-17 found
that its output is bit-identical with core-current injection enabled and disabled at 20 Hz, 50 Hz and
1 kHz, at connector-equivalent levels of +4, +16 and +22 dBu. The ideal tertiary-feedback loop fixes the
core voltage and the drive stage does not limit its correction, so this term currently contributes no
terminal distortion in those tests. Output-transformer saturation is therefore an open modelling gap,
not an established feature. See `measurements/evidence-0.1.0/transformer-audit/`.

The line input is the balanced electronic receiver in `kLineInput`, followed by `kLineGain`. It bypasses
the microphone transformer, and its implemented nonlinearity is rail clipping. All three source paths
pass through the output stage. The microphone core model uses a leaky flux accumulator and a single-valued
nonlinear current law; it is not a full hysteresis model.

### Microphone amplifier bandwidth

The ideal op-amp in the netlist has no bandwidth limit. The model adds the closed-loop pole (10 MHz
gain-bandwidth over the noise gain of the selected position). The reference op-amp macro shows a further
lag of about 8 degrees at 17.8 kHz at the highest gains (about 1.2 us), which the model does not reproduce;
magnitude agrees within 0.35 dB.

### VCA gain cell

Measured with ngspice through the control port (`tools/characterize.py`, raw data in
`docs/measurements/characterisation`). Gain falls about 0.40 dB per uA from roughly 5 uA, starts at +0.67 dB
at zero current and floors near -77 dB at 250 uA. Distortion stays below -73 dB up to 11 V peak; the
measurable third harmonic grows with level and control current and is modelled as `g (x - 3e-9 I^2 x^3)`.
Control feedthrough is assumed trimmed out. Compressor and gate currents add at the same port, as in the
reference.

### Instrument stage

The discrete JFET/PNP stage is reduced to curves fitted from ngspice: gain at 1 kHz for 13 trim positions,
the emitter-bypass bass shelf (corner from 47 uF and the bypass resistance), a second-order term growing with
gain squared (0.032 /V at maximum gain), an asymmetric output limit and a 1 Hz output coupling.

### Optical cells

The reference gives estimated LED/LDR laws (no measured part data was available): dark 100 Mohm, about
460 ohm at 1 mA and 76 ohm at 20 mA for the limiter cells, a different curve for the de-esser cell, with 2 ms
attack and 20 ms release. Both laws are inverted so the detectors can ask for the LED current that gives a
wanted resistance. The de-esser and limiter networks around the cells are evaluated exactly: for the
de-esser `y = x + (9.1/8.2) b - (9.1/4.3) D(R) b`, which cancels exactly when the cell is dark.

## Corrections to the transcription

Both are documented in the netlists and covered by tests.

1. **De-esser frequency pot**: transcribed as 20 kohm, giving 3.6 to 9.1 kHz. A 47 kohm pot gives 2.1 to
   8.7 kHz, close to the documented 2.2 to 9.2 kHz, so the 20 kohm value is treated as a misread.
2. **Instrument trim gang**: as transcribed, the instrument stage's gang runs opposite to the trim stage's,
   giving 19.6 to 28 dB. With the gangs tracking, the total is 7.4 to 40.4 dB (13.4 to 46.4 dB input to
   output), consistent with the documented 10 to 40 dB.

## Extensions (not in the reference circuit)

| Feature | Implementation |
|---|---|
| Mic impedance Low / Classic / Medium / High | Secondary load ladder scaled so the reflected load is 600, 1400, 2400 or 6800 ohm; ratios (and attenuator steps) unchanged. Classic is the transcribed circuit. |
| Mic source impedance | 150 ohm series source, so impedance and Air change level and response as with a real microphone. |
| Mic Air | 15 kohm + 0.4 H branch across the secondary: heavier loading below a few kHz, about +2 dB relative lift above 10 kHz. |
| Shelf positions 20 Hz, 655 Hz, 1.5 kHz, 2.2 kHz | Extra switch positions with capacitors chosen so their half-boost frequencies keep the ratio of the neighbouring legends. |
| Vintage Comp / Lim | Optical shunt attenuator before the VCA with fixed timing (fast attack, two-stage release with memory). Comp: soft knee, ratio 1.5 to 5; Lim: hard knee, ratio 5 to 20, threshold 4 dB higher. |
| Blend / Mix | Dry (gain cell input) and compressed signals mixed after the make-up amplifier. |

## Differences from the reference unit's documentation

These follow the circuit model rather than the printed ranges.

| Control | Documentation | Blue Channel (circuit) |
|---|---|---|
| Mic gain | 0 to +60 dB, 10 dB steps, with a range switch | 12 positions, about 6 dB apart, -8 to +60 dB input to output |
| Line gain | -20 to +10 dB | 7 positions, -18 to +18 dB |
| Instrument gain | +10 to +40 dB | +13.4 to +46.4 dB input to output |
| Parametric gain | +/-18 dB | +/-14.8 dB |
| High-pass | 20 Hz to 1.6 kHz | 19.6 Hz to 1.5 kHz |
| Low-pass | 400 Hz to 22 kHz | 307 Hz to 23.6 kHz |
| De-esser frequency | 2.2 to 9.2 kHz | 2.1 to 8.7 kHz (after correction 1) |

Left out: inserts and split modes, external sum, post-mic output, phantom power, A/D converter, and the
analogue detectors (replaced by the digital side-chains in `docs/DYNAMICS.md`).

## Verification

| Check | Where |
|---|---|
| 47 netlist responses vs ngspice, 20 Hz to 20 kHz | `tests/stage_test.cpp`, `tests/reference` |
| DK discretisation vs analogue response at 96 kHz | same test (max 0.31 dB, 31.5 Hz to 10 kHz, outside stop bands) |
| Transformer fit, unity path, EQ, compressor curve, de-esser, limiter, block-size independence, no allocation, overload recovery, deterministic offline renders, locale | `tests/model_test.cpp` |
| Bus layouts, state, programs, editor lifetime, offline factor | `tests/processor_test.cpp` |

Regenerate the references with `BLUE_REFERENCE_CIR=<deck> tools/make_references.sh`; regenerate the
calibration tables with `BlueChannelCalibrate > Source/Model/Calibration.h`.

## Nominal microphone unity detent

Mic gain index 1 now adds a software level calibration after the mic circuit, +2.754406 dB, rather
than relabelling the old non-unity level. This makes the whole Standard channel nominally unity at
1 kHz with the default 150-ohm source/Classic impedance, trim/output zero. Offline HQ applies the same
post-circuit level correction through a fixed linear controlled-voltage stage in its routing graph;
a Mic gain-switch change restarts its operating point. The transcribed component ladder itself is
unchanged; this is a product calibration, not a newly claimed hardware resistor value. Other switch
positions are unchanged. Changing source impedance, Air, EQ or frequency can change the gain.
