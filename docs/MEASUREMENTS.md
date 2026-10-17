# Measurements and evidence

## 1.0.0 release validation

The universal macOS Release build (developer editor disabled, full Offline HQ enabled) passed
all four CTest groups: stage netlists, model, processor and component solver. The VST3 passed
pluginval at strictness 7, in-process with GUI tests skipped. A host-visible switch-normalisation
fix is covered by fractional automation/state-recall cases. Native standalone analogue and Digital
windows were inspected; documentation images are deterministic 1560 × 450 component captures.

The extended HQ fixture passed with a block-split residual of 1.71003e-7 FS. Its 94.17 s wall time
includes operating-point restarts and concurrent release work; it is not a render-throughput benchmark.
The x86_64 stage, model and processor slices also passed under Rosetta. This does not establish
physical Intel performance. Logs are in [1.0.0 evidence](measurements/evidence-1.0.0/README.md).

Listening acceptance, multi-instance DAW/offline-flag acceptance, long HQ renders and Windows/Linux
hardware checks remain open. The historical measurements below describe 0.1.0; in particular its
lowest-Mic-detent measurements are not the new calibrated 0 dB startup detent.

## Historical 0.1.0 evidence

Raw outputs are in `docs/measurements/evidence-0.1.0` (stage, model and processor tests, benchmark),
`docs/measurements/characterisation` (ngspice device sweeps) and `docs/measurements/engine` (engine
responses and THD, `BlueChannelMeasure`). All results below are engineering evidence on one machine
(Apple M1 Max, macOS 15.7, Apple clang, Release build of the working tree before the first commit).

## What is and is not established

| Question | Evidence | Status |
|---|---|---|
| Netlists reproduce the reference subcircuits | 47 ngspice comparisons | passed (tolerances below) |
| Discretisation matches the analogue networks | DK vs analogue at 96 kHz | passed, max 0.31 dB outside stop bands |
| Fitted parts match their data | transformer THD, VCA law, instrument stage | fitted; training data only |
| Engine is real-time safe and deterministic | model and processor tests | passed for the tested cases |
| Host compatibility | auval (macOS AU) | passed; pluginval runs in CI only |
| Windows and Linux builds | CI workflow | configured; not run on physical machines here |
| Sound acceptance on program material | listening by a named owner | **not done** |
| Multi-instance DAW playback, offline flag in each host | target DAWs | **not done** |
| Hardware fidelity | the reference is a circuit model | **not claimed** |

## Netlists versus the reference subcircuits

`BlueChannelStageTest tests/reference`: magnitude and phase from 20 Hz to 20 kHz (phase where the reference
is above -40 dB).

| Group | Cases | Max magnitude error | Max phase error |
|---|---|---|---|
| Line receiver, line gain (7), trim (3), filters (3), shelves (4), LMF (4), HMF (3), de-esser (3), limiter low/high, fader (3), output | 34 | 0.132 dB | 1.76 deg |
| Limiter mid band (difference of two high-passes, -21 dB at 17.8 kHz) | 1 | 0.359 dB | 0.37 deg |
| Microphone input, 12 gain positions, model amplifier pole applied | 12 | 0.302 dB | 4.75 deg (to 10 kHz) |

The microphone residual is the reference op-amp macro's extra phase lag at high gain (about 8 degrees at
17.8 kHz at maximum gain). The de-esser cases use the corrected 47 kohm pot (fraction scaled from the 20 kohm
reference).

## Fitted and calibrated parts

- Input transformer core: 0.200 % THD at 50 Hz / 0 dBu and 0.99 % at +10 dBu (targets 0.2 % and 1 %).
- VCA law: table of 16 ngspice points (0 to 250 uA); third-harmonic term from the 25 and 50 uA sweeps.
- Instrument stage: 13 gain points; distortion term from 5 trim positions x 6 levels.
- Panel calibration (`Source/Model/Calibration.h`): measured from the netlists, so legends state what the
  circuit does. High-pass corner = 934318 / (R + 620), low-pass = 14639100 / (R + 620).

## Output transformer audit (2026-10-17)

The output stage was compared sample-for-sample against the same network with its nonlinear core-current
input held at zero: 96 kHz, two seconds per case, 20/50/1000 Hz, connector-equivalent +4/+16/+22 dBu.
All nine cases were bit-identical. The current ideal tertiary-feedback/driver model cancels the effect
of that current on terminal voltage. Output-transformer saturation remains an open modelling gap.
The line source bypasses the microphone transformer; low line-path THD does not verify output-core fidelity.
Raw results and the reproducible audit are in `measurements/evidence-0.1.0/transformer-audit/`.

## Engine checks (`BlueChannelModelTest`)

| Check | Result |
|---|---|
| Unity gain, line path, 1 kHz, -18 dBFS | -0.007 dB, THD below 1e-5, non-inverting |
| LMF +12 dB at 200 Hz | +12.22 dB |
| HPF at 200 Hz, 50 Hz tone | -47.9 dB |
| VCA compressor, 26 dB over threshold, 4:1 | -18.84 dB (static curve with cell offset: -18.85 dB) |
| De-esser, -20 dB threshold, -18 dBFS at 5 kHz | -8.5 dB |
| Soft limiter, +6 dBFS steady sine | -2.88 dBFS (ceiling -2.67 dBFS) |
| Block sizes 1, 17, 64 versus 512 | identical output |
| Allocations and frees inside process(), including factor and switch changes | 0 |
| +20 dBFS burst then 3 s silence | finite, decays to 0.0004 |
| Repeated offline renders | identical |
| Netlist values under a comma-decimal locale | identical |

## Engine responses and distortion (`docs/measurements/engine`)

- Flat path: -0.05 dB at 20 Hz, -0.15 dB at 20 kHz (2x, 48 kHz).
- Shelves +12 / -12 dB at 160 Hz / 6.8 kHz positions: +12.0 dB at 20 Hz, -12.0 dB at 20 kHz.
- Filters at 100 Hz / 8 kHz: -3.2 dB at 100 Hz, -3.1 dB at 8 kHz.
- Microphone, lowest gain, 150 ohm source: -8.7 dB; High impedance +0.85 dB, Low -1.4 dB relative to
  Classic; Air: about +2 dB relative lift from 10 to 20 kHz.
- Line path THD: 0.0002 % up to 0 dBFS, 2.4 % at +6 dBFS (clipping).
- Microphone path THD at 50 Hz, lowest gain: 0.05 % at -30 dBFS, 0.32 % at -18 dBFS, 5.4 % at 0 dBFS
  (transformer core); at 1 kHz below 0.005 %.
- Instrument path at maximum gain: 0.2 % at -30 dBFS, 3.1 % at -18 dBFS, 9.5 % at -12 dBFS.

## Performance (`BlueChannelBench`, stereo, 64-frame blocks, 10 s per case)

| Case | 48 kHz 2x | 48 kHz 4x | 96 kHz 2x | 48 kHz 16x |
|---|---|---|---|---|
| Line path, everything out | 2.5 % | 4.9 % | 4.7 % | 19.3 % |
| All sections engaged | 10.1 % | 20.0 % | 20.1 % | 80.4 % |

Percent of one core (CPU time / audio time). Worst callback at 48 kHz 2x with everything engaged: 0.21 ms of
a 1.33 ms budget. One 5.8 ms outlier appeared in a 10 s line-path run at 8x (scheduler preemption; not
reproduced). Not measured: Intel hardware, Windows, Linux, DAW capacity.

## Evidence ledger

```text
Claim: netlists match reference subcircuits / owner: DSP / threshold: 0.25 dB, 5 deg (mic 0.35 dB, 10 kHz)
Source: working tree 0.1.0 / tests/stage_test.cpp / tests/reference (ngspice 47)
Result: pass (see table) / limits: reference is a circuit model / decision: accepted 2026-10-17

Claim: real-time safety and determinism / owner: host integration / threshold: 0 allocations, exact output
Source: tests/model_test.cpp, tests/processor_test.cpp / result: pass / limits: tested scenarios only

Claim: sound acceptance / owner: sound / status: open - needs level-matched program-material listening
Claim: DAW behaviour / owner: host integration / status: open - instance count, automation, offline flag
```

## Experimental HQ component solver (2026-10-17)

A separate offline HQ path now resolves the component audio circuits, including finite amplifier macros,
semiconductors, optical cells and implicit transformer cores. Standard remains the default; the signed
0.1.0 installer has not changed. Scope and numerical limits are in [HQ mode](HQ_MODE.md).

The initial extended component fixture passed isolation, block continuation, repeat reset, input-source
restarts and dynamics-control exercise. Its block-split residual was 2.3128e-7 FS and its wall time was
98.96 s. This short fixture includes multiple operating-point starts and is not a sustained render-speed
benchmark. Listening, DAW bounce acceptance, tolerance sweeps and long-render memory testing remain open.
The output-only audit's sub-microvolt residual includes numerical/interpolation error and must not be
reported as isolated transformer-core distortion. See [HQ evidence](measurements/evidence-0.1.0/hq-mode/README.md).


## Offline-only HQ and unity Mic startup (2026-10-17)

The owner withdrew native realtime HQ; its host control, GUI and DSP have been removed. Earlier
realtime evidence is archived privately and does not describe
the current build. Offline HQ remains the full component solver; Standard handles playback.

Fresh instances/Init now select the Mic circuit at the calibrated 0 dB detent, trim/output zero. The
nominal calibration is at 1 kHz with a 150-ohm source, Classic impedance and air off. Blue-panel keys
and selector legends use white text. See [current validation](measurements/evidence-0.1.0/hq-unity/README.md).
