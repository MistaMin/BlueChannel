# Blue Channel QA ledger — 2026-11-01 DSP/QA pass

Scope sources: README.md, CHANGELOG.md, SOURCE_PROVENANCE.md, docs/ (USER_MANUAL, PARAMETERS,
RELEASE_1.0.0), plugin-qa/projects/bluechannel.md, qa/exceptions.txt.

Build under test: HEAD 4feef21 (bundle-release), Release, universal (arm64+x86_64), Ninja
build/release-macos. Formats shipped: VST3, AU, CLAP, LV2, AAX (+Standalone local).

| # | Item | Source | Code | GUI | Test | Evidence | Status |
|---|---|---|---|---|---|---|---|
| S1 | 66 host parameters load, automatable | manual | Source/Plugin/Params.cpp | — | REAPER suite | report | todo |
| S2 | Every parameter changes sound (Line Gain @ Input=Line context) | qa/exceptions.txt | Source/Circuit | — | audio sweep | report | todo |
| S3 | Every parameter visible in GUI | coverage + visual sweep | Source/Plugin/PluginEditor.cpp | — | visual sweep | report | todo |
| S4 | Editor opens 1248x360; Digital interface; control page; FFT | CHANGELOG | PluginEditor | — | GUI test + suite | ui_snapshot | todo |
| S5 | State survives save/reopen (Program preset excused) | qa/exceptions.txt | setStateInformation | — | state check | report | todo |
| S6 | ctest (hq_component_solver, model, processor, stage_netlists) | profile | tests/ | — | ctest | 4/4 pass + Rosetta model test (2026-11-01) | verified |
| S7 | VST3 pluginval strict 10 | profile | — | — | pluginval | SUCCESS (2026-11-01) | verified |
| S8 | CLAP clap-validator 44 tests | profile | — | — | clap-validator | 34 passed, 0 failed (2026-11-01, after fixes) | verified |
| S9 | AU auval PASS | profile | — | — | auval aufx BlCh OpGr | AU VALIDATION SUCCEEDED (2026-11-01) | verified |
| S10 | Frequency/Mix/Hold text round trips | clap-validator | Params.cpp hzAttrFor etc. | — | param-conversions | fixed 2026-11-01, 0 failures | verified |
| S11 | CLAP paramsRescan after state load | clap-validator | setStateInformation | — | state-reproducibility | fixed 2026-11-01, 0 failures | verified |
| S12 | AAX PACE-signed | manual | AAX bundle | — | wraptool verify | signed (2026-11-01) | verified |
| S13 | dsp_lint clean (current files) | dsp rule 9 | — | — | dsp_lint | 0 current-file findings | verified |
| S14 | Payload lint: only ngspice "Thermionic" physics-term string | dsp rule 9 | — | — | dsp_lint --payload | recorded exception (third-party binary) | verified |
| S15 | Standalone icon | CHANGELOG 9b43943 | CMake ICON_BIG/SMALL | app | check_standalone_icon | AppIcon.icns verified | verified |

Fixes made this pass (committed as 4feef21, pushed to main):
- Params.cpp: denormalised Hz/kHz, % and ms/s parsers for HP/LP/LMF/HMF/De-Ess freq, Mix, Hold.
- PluginProcessor.cpp: updateHostDisplay(programChanged) after replaceState.

Not run this pass (SKIPPED): Pro Tools AAX acceptance (no admin AAX install), Studio One interactive.

## 2026-11-01 (late): OpenGrid9 host suite + activation race fix

OpenGrid9 (the Ardour-based host) was added as a host target on request. Its headless
Lua backend (`qa/og9qa_worker.lua` + `qa/og9qa_run.py` in the OpenGrid repo) runs
load / parameter-enumeration / full min-max sweep / state save-reopen checks per format
(VST3, AU, LV2; this OpenGrid9 build has no CLAP host).

Findings fixed (all four plugins):
- **prepareToPlay rebuild race**: OpenGrid9 activates plugins from its disk thread while the
  audio callback can still run. Rebuilding DSP state from prepareToPlay (circuit solvers,
  engine banks, chain swap) freed memory the callback was still using (use-after-free,
  double-free, segfaults during activation). prepareToPlay now defers all allocation into
  the audio callback via a pending flag; the first activation still prepares synchronously
  so the latency contract after prepareToPlay is unchanged.
- **Audio-thread host calls**: the deferred prepare must not call setLatencySamples from the
  audio thread (strict CLAP validator); latency is announced only when it actually changes.
- **Scan cache staleness**: OpenGrid9 caches failed plugin scans; a manual re-scan
  (ardour-vst3-scanner / ardour-au-scanner) confirmed all bundles scan clean.

Verification after fixes: pluginval SUCCESS, clap-validator 0 failures (HybridEQ: the two
documented 45 s fuzz-budget timeouts only), OpenGrid9 matrix 12/12 PASS (HybridEQ 46/45
params, OpenPreamp 25/24, Tube Channel 86/85, Blue Channel 63/62, LV2 loads all), local
suites green (HybridEQ 812/0, OpenPreamp 555/0 + 203/0, Tube Channel ctest 18/18, Blue
Channel ctest + Rosetta model test).
