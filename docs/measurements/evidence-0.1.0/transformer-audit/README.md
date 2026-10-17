# Output transformer audit — 2026-10-17

Machine: arm64, macOS 15.7.5. Source: ccd4aa4 plus current uncommitted GUI changes; audio model unchanged.

`audit.cpp` compares OutputStage against the same kOutput DK network with core-current injection held at
zero. Both paths receive the same rail-clipped input. The test levels are below the clipping knee.

Rate: 96 kHz. Duration: two seconds per case. Frequencies: 20, 50, 1000 Hz.
Levels: connector-equivalent +4, +16, +22 dBu (drive voltage is half the corresponding connector voltage).
Bit identity is checked over every sample, including startup; RMS residual uses the final second.

All nine cases are bit-identical, with zero residual. The nonlinear current changes the drive demanded
by the ideal feedback network but not its output voltage in these cases. Its ideal drive correction
has no internal headroom/current limit. This is an output-transformer modelling gap. The microphone
core nonlinearity is separate and remains covered by its existing 50 Hz distortion-fit tests.
The line receiver has no input transformer in this implementation.

This audit neither changes the DSP nor establishes transformer fidelity to hardware.

```sh
c++ -std=c++20 -O2 -I Source docs/measurements/evidence-0.1.0/transformer-audit/audit.cpp -o build/transformer_audit
build/transformer_audit
```
