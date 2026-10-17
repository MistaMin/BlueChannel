# Blue Channel

An analogue-modelled channel strip by **OpenGrid**: transformer microphone preamp, line and instrument
inputs, third-order filters, shelving and two-band parametric EQ, discrete class-A VCA compressor with
optical "vintage" modes, expander/gate, optical de-esser and a three-band optical soft limiter, all modelled
from a component-level circuit model.

![Blue Channel, analogue skin](docs/images/blue-channel-analogue.png)

*Component render of the editor (offscreen snapshot, analogue skin). See [the manual](docs/USER_MANUAL.md).*

- Formats: VST3, AU (macOS), CLAP, LV2 and Standalone on macOS, Windows and Linux. AAX builds are produced
  for internal testing when an AAX SDK is present and are not distributed.
- Mono and stereo, with an optional mono or stereo sidechain (external key for compressor and gate).
- Zero latency. Oversampling 1x to 16x (`\( ᐖ)/ - Derp mode`), with a separate factor for offline renders.
- MIT licensed. Built with JUCE and the [GoodLookinUI](https://github.com/MistaMin/GoodLookinUI) toolkit.

## How it is modelled

Every linear stage (line receiver, gain switches, trim, filters, shelves, both parametric bands, compressor
make-up, de-esser band-pass, limiter crossover, fader, output transformer stage) is a transcription of the
reference circuit model, discretised at runtime with the nodal DK method from its actual component values.
Each stage is checked against ngspice runs of the full reference subcircuits (47 comparisons, 20 Hz to
20 kHz). Outside the microphone stage, 34 of 35 agree within 0.13 dB and 1.8 degrees; the exception is the
limiter's narrow mid band (0.36 dB). The 12 microphone-gain positions agree within 0.30 dB, and within
4.8 degrees up to 10 kHz (`tests/stage_test.cpp`, `docs/MEASUREMENTS.md`).

The parts a linear netlist cannot express are reduced to measured behaviour: the input transformer core
(fitted to its published distortion), the VCA gain cell (gain law and distortion measured in ngspice), the
JFET instrument stage, op-amp swing on the reference supply rails, and the LED/LDR optical cells. Detectors
are digital and drive the circuit's control ports (VCA control current, LED currents), as the reference
model specifies. Details: [docs/CIRCUIT_MODEL.md](docs/CIRCUIT_MODEL.md).

## Build

```sh
git clone --recurse-submodules <this repository>
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Packaging: `installer/build_macos.sh`, `installer/build_windows.ps1`, `installer/build_linux.sh`. Platform
requirements, local dependency checkouts and AAX are covered in [docs/BUILDING.md](docs/BUILDING.md);
releases in [docs/RELEASE.md](docs/RELEASE.md). CI builds and tests all three platforms
(`.github/workflows/build.yml`).

JUCE is dual licensed (AGPLv3 or commercial). Building and distributing binaries requires a JUCE licence
that covers your use. See [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md).

## Repository layout

| Path | Contents |
|---|---|
| `Source/Circuit` | netlist parser, DK state-space engine, stage netlists |
| `Source/Model` | stages, channel chain, engine, calibration tables, control maps |
| `Source/Dynamics` | compressor, opto, gate, de-esser and limiter side-chains |
| `Source/DSP` | half-band oversampling |
| `Source/Plugin`, `Source/UI` | JUCE processor, parameters, editor |
| `tests` | stage-vs-ngspice comparison, model checks, processor checks, calibration, benchmark, snapshots |
| `tools` | ngspice reference and characterisation scripts, release manifest, licence collection |
| `docs` | manual, engineering notes, decision records, measurements |

## Status

Version 0.1.0. Engineering checks pass (see `docs/MEASUREMENTS.md`). Not yet done: program-material listening
acceptance, multi-instance testing in real DAWs, Windows and Linux runs on physical machines (CI only), and
pluginval locally. The reference is a circuit model, not measured hardware; estimated parts are listed in
`docs/CIRCUIT_MODEL.md`.
