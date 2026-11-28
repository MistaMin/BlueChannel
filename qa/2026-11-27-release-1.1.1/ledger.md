# Blue Channel 1.1.1 release: QA record (2026-11-28)

Source: branch `openplugin-port`. macOS bundles and the Windows installer built from `fedf17b`
(Windows: `windows` workflow run 46117472180). OpenPluginSDK `07f97e1`, VST3 SDK 3.8.1 build 84.
Build machine: macOS 15 (Darwin 24.6.0), Apple Silicon. Windows: GitHub hosted `windows-2022`, x64.
A first build on 2026-11-27 (SDK `1f608fe`, source `693ef5e`) was not published; this record is of the
rebuild. Earlier checks of the source are in `../2026-11-27-sdk-1f608fe/ledger.md`.

| Check | Result | Evidence |
|---|---|---|
| Lint, clippy, all release tests with the null test, soak (24 seeds), strict fuzz, bench | pass | full tier on the working tree that became `fedf17b` |
| Formats, macOS | CLAP, LV2, VST3, AU, AAX: universal (x86_64 arm64), minimum macOS 11.0, version 1.1.1 | `lipo`, `vtool`, Info.plist |
| Null against the 1.0.0 VST3: built VST3, CLAP, AU x sweep, noise, impulses | 9 of 9 pass; largest difference 3.7e-6 | `null-built-bundles.txt` |
| Exported symbols of the macOS shells | VST3: `GetPluginFactory`, `bundleEntry`, `bundleExit`; Audio Unit: `OpenPluginAUFactory`; AAX: the seven `ACF*` functions. No bundled SDK typeface in any binary | `nm -gU`, `strings` |
| AAX Validator, native tests, on the unsigned AAX | 6 of 6 pass; warnings: two parameters named "Bypass", gaps in the algorithm context | `aax-validator-summary.txt` |
| Developer ID signatures, hardened runtime | CLAP, VST3, AU, AAX bundles and both LV2 libraries | `codesign --verify --deep --strict` |
| PACE Eden signature on the AAX | verified | `wraptool verify` |
| Installer: Developer ID Installer, notarised, stapled | Accepted (submission 8d5d227c-0d75-48a4-a027-7c5c67ddf42c); `spctl`: accepted, Notarized Developer ID | this session |
| Windows: tests, build checks, installer | pass | run 46117472180 |
| `tools/audit_release.py` on the signed bundles and the history | no issues (no private term list given) | this session |
| Manual PDF | built, 5 pages, carries the 1.1.1 text | `tools/build_user_manual.py` |
| `auval`, REAPER suite, Pro Tools | SKIPPED: nothing was installed on the build machine | |

## Files

| File | Size | SHA-256 |
|---|---|---|
| Blue-Channel-1.1.1-macos.pkg | 35844178 | 4efa1c9d38388ba627859aa99bbd29de201fb3c2a65098148baf1237c9e65f27 |
| Blue-Channel-1.1.1-windows-x64-setup.exe | 3882799 | 2485fae9c228cb8ea9b78c687b3f5a95bffbd4d80f642903b07aa79164f559c7 |

## Notes

- OpenPluginSDK `07f97e1` makes each macOS shell export only its entry points and link with
  `-dead_strip`. The VST3, Audio Unit and AAX binaries are 5 MB each (16 MB in the first build, which
  carried the SDK's five unused typefaces), and the installer is 35.8 MB (1.1.0: 40.6 MB).
- The package stores the system's `com.apple.provenance` attribute as AppleDouble entries, as the
  1.1.0 package does; the attribute cannot be removed on this system.
- The AAX Validator ran on the unsigned AAX of this build, not on the signed one. No host loaded the
  bundles: the narrower export lists are covered by the null test (VST3, CLAP, Audio Unit loaded by
  the port tool) and the AAX Validator only.
