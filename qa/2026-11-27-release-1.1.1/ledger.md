# Blue Channel 1.1.1 release: QA record (2026-11-27)

Source: branch `openplugin-port`. macOS bundles and the Windows installer built from `693ef5e`
(Windows: `windows` workflow run 46011103497). OpenPluginSDK `1f608fe`, VST3 SDK 3.8.1 build 84.
Build machine: macOS 15 (Darwin 24.6.0), Apple Silicon. Windows: GitHub hosted `windows-2022`, x64.
The checks of the source before the version change are in `../2026-11-27-sdk-1f608fe/ledger.md`.

| Check | Result | Evidence |
|---|---|---|
| Formats, macOS | CLAP, LV2, VST3, AU, AAX: universal (x86_64 arm64), minimum macOS 11.0, version 1.1.1 | `lipo`, `vtool`, Info.plist |
| Null against the 1.0.0 VST3: built VST3, CLAP, AU x sweep, noise, impulses | 9 of 9 pass; largest difference 3.7e-6 | `null-built-bundles.txt` |
| Developer ID signatures, hardened runtime | CLAP, VST3, AU, AAX bundles and both LV2 libraries | `codesign --verify --deep --strict` |
| PACE Eden signature on the AAX | verified | `wraptool verify` |
| Installer: Developer ID Installer, notarised, stapled | Accepted (submission 93e6f989-50f6-462e-8424-892bd2187679); `spctl`: accepted, Notarized Developer ID | this session |
| Windows: tests, build checks, installer | pass | run 46011103497 |
| `tools/audit_release.py` on the signed bundles and the history | no issues (no private term list given) | this session |
| Manual PDF | built, 5 pages, carries the 1.1.1 text | `tools/build_user_manual.py` |
| `auval`, REAPER suite, Pro Tools | SKIPPED: nothing was installed on the build machine | |

## Files

| File | Size | SHA-256 |
|---|---|---|
| Blue-Channel-1.1.1-macos.pkg | 50516099 | 76b711c26d3a6419cff33fa5675287f16f69deb10964d23540b62f45e9beda9d |
| Blue-Channel-1.1.1-windows-x64-setup.exe | 3880480 | b8842deb3b9992521a1e76329e4fe68aac851b76e4a937c7db8282e64ef56c52 |

## Notes

- The macOS VST3, Audio Unit and AAX binaries are 6 MB larger than in 1.1.0. The SDK now bundles five
  typefaces, and its shells link the whole Rust library, so the faces are in these binaries although
  Blue Channel does not draw with them. Their licence texts are in each bundle. The CLAP and LV2
  binaries do not carry them.
- The package stores the system's `com.apple.provenance` attribute as AppleDouble entries, as the
  1.1.0 package does; the attribute cannot be removed on this system.
- The validators, the host suite and the AAX Validator were not run on the signed bundles; the AAX
  Validator ran on the unsigned AAX of the same source before the version change.
