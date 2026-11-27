# Blue Channel on OpenPluginSDK

Blue Channel on [OpenPluginSDK](https://github.com/MistaMin/OpenPluginSDK), with its editor drawn by GoodLookin DRI: the build that version 1.1 is made from. The JUCE build that 1.0.0 was made from is at tag `v1.0.0`.

| File | What it holds |
|---|---|
| `src/params.rs` | The parameter table of the 1.0 release: same ids, order, ranges and choices, each pinned to the id the JUCE build showed CLAP, VST3 and Audio Unit hosts. |
| `src/lib.rs` | The plugin: values, buses (mono or stereo, optional key input), state, the release's VST3, Audio Unit, AAX and LV2 identity. |
| `cpp/bridge.cpp`, `src/engine.rs` | The signal path: the C++ model in `../Source` (Model, Circuit, DSP, Dynamics; no JUCE), unchanged, behind a C ABI. |
| `cpp/hq_shim/` | A stand-in for the few `juce_core` calls `../Source/Plugin/HQRenderer.cpp` makes, so HQ Mode's solver is the same source as the JUCE build's. |
| `src/state_import.rs` | Reads the state the JUCE release saved, the editor's settings included. `lib.rs` takes it out of the VST3, Audio Unit and AAX containers. |
| `src/programs.rs` | The six factory programs. |
| `src/shared.rs` | What the engine shares with the editor: meters, the analyser ring, the editor's saved settings. |
| `src/editor/` | The editor: the analogue panel (1560 × 450, the layout of the JUCE build, knobs placed from `../Designs/knobs.csv`) and the Digital skin with its graphs (1250 × 900). Each skin resizes in its own shape. `lettering.rs` draws the legends with the system font on macOS and the toolkit's face elsewhere. |
| `src/bin/standalone.rs` | The standalone app (feature `standalone`). |
| `tests/` | The SDK's conformance kit, the standard host-behaviour, soak and strict fuzz tests, the release's captured states, the offline render policy, HQ Mode, and the null test against the 1.0.0 release. |
| `examples/bench.rs`, `bench/baseline.json` | The standard benchmark and its stored baseline. |

## Building and testing

Cargo takes OpenPluginSDK (GoodLookin DRI is in its `crates/`) from Git at the revision pinned in `Cargo.toml`, so `cargo test` works in a fresh clone. `scripts/build.sh` builds the format shells from a local checkout (it finds one, or set `OPENPLUGIN_SDK`) and stops if that checkout is at another revision. To work against a local SDK, put this in `.cargo/config.toml`, which is not in Git:

```toml
[patch."https://github.com/MistaMin/OpenPluginSDK"]
openplugin = { path = "<OpenPluginSDK>/crates/openplugin" }
openplugin-testing = { path = "<OpenPluginSDK>/crates/openplugin-testing" }
goodlookindri-shell = { path = "<OpenPluginSDK>/crates/goodlookindri/shell" }

[env]
OPENPLUGIN_PORT = "<path to the openplugin-port executable>"   # both for tests/null.rs,
OPENPLUGIN_PORT_ORIGINAL = "<path to the 1.0.0 Blue Channel.vst3>"   # which is skipped without them
```

```bash
cargo test
cargo test --release --test soak -- --ignored   # 24 seeds and the strict fuzz, about ten minutes
cargo test --release --features count-cpp-new --test conformance   # the allocation check with the C++ model counted too
cargo test --release --features hq --test hq # HQ Mode with the solver; needs a shared ngspice library
scripts/build.sh --hq                        # universal CLAP, LV2, VST3, AU, AAX and the app, in dist/
```

Version 1.1.0 was built with OpenPluginSDK `ded6d8d` and GoodLookin DRI 0.2.1; version 1.1.1 is built with OpenPluginSDK `1f608fe`, which has GoodLookin DRI in its tree. The Windows plug-ins (CLAP and VST3, no HQ Mode) and their installer are built by `../.github/workflows/windows.yml` with the SDK's `scripts/windows/build_plugin.ps1`.

`scripts/build.sh --help` lists the options and the variables that say where the VST3, Audio Unit and AAX SDKs and ngspice are. Nothing is installed and nothing is signed beyond ad hoc. The bundles carry the release's ids: installing them replaces the JUCE release in every host.

Development aids: `cargo run --release --example snapshot -- out.ppm [analogue|overview|controls] [scale] [param=value ...]` (a picture of the editor), `ui_bench` (what a frame costs), `tail` (how long the output rings), `sweep` (a host's parameter sweep, offline), `hq_probe` (offline HQ renders; feature `hq`).

For host tests beside the installed release, feature `qa-identity` builds the same plugin as "Blue Channel Port Test" with ids of its own. Never ship a build with it.

## Differences from the JUCE release

- Hosts see parameters in plain units (dB, Hz, positions), where the JUCE build showed 0 to 1. Automation of continuous controls recorded by CLAP hosts against the JUCE build plays back at other values.
- A parameter change inside a block applies at its frame, not from the start of the block.
- A 5 second tail is reported (the release reported none; the model rings for up to 4.2 s after a loud signal, `examples/tail.rs`).
- Non-finite input is treated as silence (one NaN otherwise stays in the model's state for good).
- The VST3 host bypass parameter has the SDK's id, not JUCE's, and there is no "Program" parameter: the factory programs are offered through the SDK's preset interface, which no format publishes to a host's preset menu yet.
- The Output control's travel is linear in dB in a host's generic view (the release skewed it); the SDK has linear and logarithmic tapers only.
- Audio is processed in 32-bit float (the JUCE build also offered 64-bit buffers; the model itself computes in double either way).
- The Digital skin's drop-down menus are rows of segments (the SDK has no popup menu), and there is no undo history.
- The standalone app plays a built-in pattern, or the default input with `--input`; it has no device settings window.

## Known limits

- HQ Mode solves mono instances. A stereo solve fails to find its operating point ("timestep too small") and the render is muted with "HQ failed", as documented for a failed solve. The JUCE build's renderer fails the same way: it is the same source.
- Sessions saved by the JUCE build open in the port in every format (`load_foreign_state`; tests load states captured from the 1.0.0 VST3 and Audio Unit). The released 1.1.0 reads them in CLAP only. The AAX path is tested with the release's state under the chunk id `juce`, not with a chunk a Pro Tools session saved.
- The SDK's shell build cannot ask a library that also holds C++ for its Audio Unit codes, so `scripts/build.sh` passes them.
- Windows is built and tested by machine only: nobody has opened the editor there. Linux has not been tried.
