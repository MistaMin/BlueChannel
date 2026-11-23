# Blue Channel 1.1.0 release: QA record (2026-11-17)

Source: branch `openplugin-port`. macOS bundles built from `17606a8`; Windows built by the `windows`
workflow at `bf80d7c` (run 44075454378), which differs from `17606a8` only in the workflow file.
OpenPluginSDK `ded6d8dac0cb`, GoodLookin DRI 0.2.1 (`9a1d621`), VST3 SDK 3.8.1 build 84.
Build machine: macOS 15 (Darwin 24.6.0), Apple Silicon. Windows: GitHub hosted `windows-2022`, x64.

| Check | Result | Evidence |
|---|---|---|
| `cargo test --release` (27 unit and editor, conformance, null, offline 2, state 3) | pass, 36 | this session |
| `cargo test --release --features hq --test hq` | pass, 3 | this session |
| `cargo clippy --all-targets --features "hq standalone"` | clean | this session |
| Windows: lib, conformance, state, offline tests | pass, 33 | run 44075454378 |
| Windows build checks (64-bit DLLs, no build paths, system imports only, entry points, version resource) | pass, with `UIAutomationCore.DLL` allowed (part of Windows) | run 44075454378 |
| Formats, macOS | CLAP, LV2, VST3, AU, AAX, app: all universal (x86_64 arm64), minimum macOS 11.0, version 1.1.0 | `lipo`, `vtool`, Info.plist |
| Standalone icon | present (`BlueChannel.icns`); the app is not in the installer | `check_standalone_icon.sh` |
| Null against installed 1.0.0 VST3: built VST3, AU, CLAP x sweep, noise, impulses, with switch automation | 9 of 9 pass; largest difference 8.9e-8 | `null-built-bundles.txt` |
| `auval -v aufx BlCT OpGr` (side-by-side test build, arm64) | pass | this session |
| AAX Validator on the unsigned AAX | 13 pass, 1 skipped (`test.cycle_counts` needs HDX hardware) | `aax-validator/` |
| REAPER suite, VST3 test build | 8 of 9 pass | `report-reaper-vst3.md` |
| Developer ID signatures, hardened runtime | CLAP, VST3, AU, AAX bundles and both LV2 libraries | `codesign --verify --deep --strict` |
| PACE Eden signature on the AAX | verified | `wraptool verify` |
| Installer: Developer ID Installer, notarised, stapled | Accepted (submission 2e2cda89-88c9-4f60-8dd3-1063cc2e530d); `spctl`: accepted, Notarized Developer ID | this session |

## Files

| File | Size | SHA-256 |
|---|---|---|
| Blue-Channel-1.1.0-macos.pkg | 40615511 | f1d7abc63c3a1aea867dce9132bbdc04e1ea54af2ba4b3a3e5931d03addd5d8a |
| Blue-Channel-1.1.0-windows-x64-setup.exe | 3871629 | 4ccc4baaa4a97db353ee83f0295026e7aef8dd9d23e239469fe33ec6a056ac0a |

## Notes on the results

- The AAX Validator first reported three aborted tests (`info.productids`, `info.support.general`,
  `test.data_model`). The validator's own Ruby script (`SysInfo.rb`, line 249) fails on a disk that
  reports no container size on this machine and the test never starts. With that line made tolerant
  of the missing value, in a copy of the validator, all three pass
  (`aax-validator/rerun-aborted-tests.summary.txt`). The validator is run from a path without spaces.
- REAPER, "every parameter is shown in the GUI": FAIL for HF Frequency, HF Hi Range, Mute, Stereo Link,
  Oversampling, Offline Oversampling and Wet. The suite's REAPER window was on a 1080-point-wide
  screen and cut the 1560-point editor off at 1080 (`screenshots-reaper/vst3-editor-in-reaper.png`);
  the six plugin parameters are the controls right of that edge, and Wet is REAPER's own. The editor
  tests in the crate cover every parameter's control on both skins. After this run the editor was
  changed to open no larger than the main screen has room for (`23998b5`); on this machine the main
  screen is 1920 points wide, so a window the host puts on the narrow second screen is still cut off.
  Not rerun in REAPER after that change.
- The validators and REAPER ran on the ad hoc signed builds, the null test on the unsigned final
  bundles. The signed bundles were verified by signature only; nothing was installed from the package.

## Not done

- No person has used the build in Pro Tools, Logic or any Windows host. The Windows editor has never
  been seen.
- The x86_64 half of the macOS build was built and not run.
- CLAP and AU were not rerun in REAPER on this SDK revision (they were on `bf55d22`, 2026-11-16).
- pluginval and clap-validator were not run.
- Windows installer is not code signed.

## Open defects carried into the release

- VST3, Audio Unit and AAX: state saved by 1.0.0 is refused by the SDK's shells before the plugin sees
  it, so 1.0.0 projects open at defaults. CLAP imports 1.0.0 state (tests/state.rs).
- Audio Unit: a stepped parameter automated to an in-between value is saved unsnapped (SDK shell).
- HQ Mode: stereo solves fail ("timestep too small"), as in 1.0.0's solver.
