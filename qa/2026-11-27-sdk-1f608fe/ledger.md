# Blue Channel after 1.1.0: QA record (2026-11-27)

Source: branch `openplugin-port` at `428c558`. OpenPluginSDK `1f608fe` (GitHub `main` on this day),
VST3 SDK 3.8.1 build 84. Build machine: macOS 15 (Darwin 24.6.0), Apple Silicon, busy with other work
during every timed check. Nothing was installed and nothing was signed beyond ad hoc.

Scope: what changed since 1.1.0.

| Item | Result | Evidence |
|---|---|---|
| Mic input no longer overflows on a loud low tone | verified | `tests/host_behaviour.rs`; soak over 24 seeds and strict fuzz pass |
| A 1.0.0 project opens with its settings in VST3 and Audio Unit | verified in tests | `tests/state.rs`, states captured from the 1.0.0 VST3 and Audio Unit |
| The same in AAX | unverified | tested with the release's state under the chunk id `juce`, not with a chunk a Pro Tools session saved |
| Sound unchanged against 1.0.0 apart from the Mic fix | verified | null, below |
| Builds against OpenPluginSDK `1f608fe` from a new checkout | verified | commit `428c558` tested in its own worktree; `windows` and `build` workflows |
| Editor unchanged by the SDK update | verified | all three pages at 2x are byte-identical to the build on SDK `19fac89`; each page looked at |

| Check | Result | Evidence |
|---|---|---|
| Realtime lint, `cargo clippy --all-targets -- -D warnings` | pass | this session |
| `cargo test --release` (28 unit and editor, conformance, host behaviour 4, null, offline 2, state) | pass | this session |
| Soak, 24 seeds; strict fuzz | pass | this session |
| Conformance, soak and fuzz with C++ `new` counted (`--features count-cpp-new`) | pass | this session |
| `cargo test --release --features hq --test hq` | pass, 3 | this session |
| Model tests under AddressSanitizer (macOS, and Linux in the `build` workflow) | pass, 2 of 2 | run 45965287728 |
| Windows: tests, plug-ins, installer | pass | run 45965282043 (commit `4a17149`) |
| Bench against the stored baseline | no regression; two more runs within 2 % of it | `plugin/bench/baseline.json` |
| Formats, macOS, with HQ Mode | CLAP, LV2, VST3, AU, AAX, app: universal (x86_64 arm64), ad hoc signed, no build path in any binary | `scripts/build.sh --hq --archs universal` |
| Null against the 1.0.0 VST3: built VST3, CLAP, AU x sweep, noise, impulses | 9 of 9 pass; largest difference 3.7e-6 | this session |
| AAX Validator, native tests, on the unsigned AAX | 6 of 6 pass, 2 warnings | `aax-validator-summary.txt` |
| Editor frame benchmark, old and new SDK in turn | SKIPPED as a comparison: load average over 100; the one quiet round was equal (4.3 and 4.8 ms a frame, analogue) | this session |
| `auval`, REAPER suite, Pro Tools | SKIPPED: they need the build installed | |
| pluginval, clap-validator | SKIPPED: not on this machine | |

## Exceptions

- NEW: the null test's tolerance is 1e-5, where it was 1e-6. The Mic fix changes the sound on purpose
  (the transformer's core current no longer carries its content at half the internal rate); the largest
  difference measured is 3.7e-6.

## Findings, not changed

- AAX has two parameters named "Bypass": the plug-in's own and the host bypass the SDK adds. The
  validator warns about it.
- The validator also warns that the algorithm context has gaps between registered fields.
- The Digital skin's level readouts show tenths and more than three digits (`-100.0 dBFS`), as in 1.1.0.
- HQ Mode: stereo solves fail ("timestep too small"), as in 1.0.0's solver.
- The x86_64 half was built and not run.
