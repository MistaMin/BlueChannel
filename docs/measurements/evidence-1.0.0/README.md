# 1.0.0 release evidence

Universal macOS Release, Apple M1 Max, Apple clang 16, JUCE 9.0.3, full Offline HQ enabled,
developer editor disabled. Release artefact signatures/notarisation are recorded with the downloads.

- `ctest-arm64.log`: four groups passed, including default Mic HQ float/double startup.
- `processor-arm64.log`: final 1248 × 360 startup, old-preview reset and later resize recall.
- `pluginval.log`: VST3 strictness 7 passed, in-process, GUI tests skipped.
- `hq-cleanup-repeat.log`: three consecutive component fixtures, including restart and clean simulator teardown.
- `hq-extended.log`: `MallocScribble=1`, clean teardown, isolated component instances, block continuation, reset, source changes and dynamics controls.
- `rosetta-stage.log`, `rosetta-model.log`, `rosetta-processor.log`, `rosetta-hq.log`: x86_64 slices under Rosetta.

The three [UI images](../../images/) are component snapshots from this release at 1248 × 360.
The native resized standalone is captured in `native-digital.png`. Native standalone windows were also inspected. Screenshots use representative settings with HQ armed;
HQ remains off in a fresh instance. Native standalone state restoration was preserved during the check.
Listening, target DAW acceptance, long-render memory and physical Windows/Linux/Intel tests remain open.

The final lifecycle fix removes the previous ngspice circuit before loading a new one. This avoids
duplicate source-history cleanup at shutdown after restarts. The shared simulator source is unmodified.

`model-asan.log` records the corrected overload fixture under AddressSanitizer. Its final silence
block uses the remaining frame count. The original fixture overran its caller-owned buffer on Linux.
The model and processor runtime code is unchanged by this test correction.
