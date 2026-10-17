# 1.0.0 release evidence

Universal macOS Release, Apple M1 Max, Apple clang 16, JUCE 9.0.3, full Offline HQ enabled,
developer editor disabled. Release artefact signatures/notarisation are recorded with the downloads.

- `ctest-arm64.log`: four groups passed, including default Mic HQ float/double startup.
- `pluginval.log`: VST3 strictness 7 passed, in-process, GUI tests skipped.
- `hq-extended.log`: isolated component instances, block continuation, reset, source changes and dynamics controls.
- `rosetta-stage.log`, `rosetta-model.log`, `rosetta-processor.log`: x86_64 slices under Rosetta.

The three [UI images](../../images/) are component snapshots from this release at 1560 × 450.
Native standalone windows were also inspected. Screenshots use representative settings with HQ armed;
HQ remains off in a fresh instance. Native standalone state restoration was preserved during the check.
Listening, target DAW acceptance, long-render memory and physical Windows/Linux/Intel tests remain open.
