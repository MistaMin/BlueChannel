# Release process

## Distribution matrix

| Platform | Formats distributed | Package |
|---|---|---|
| macOS 11+ (arm64 + x86_64) | VST3, AU, CLAP, LV2, AAX, Standalone | signed and notarised `.dmg`, `.pkg`, bundle `.zip` |
| Windows 10+ x64 | VST3, CLAP, LV2, Standalone | `.zip`, Inno Setup installer when available |
| Linux x86_64 (and aarch64 when built there) | VST3, CLAP, LV2, Standalone | `.tar.gz` |

## AAX format and signing

Official macOS releases include AAX binaries as freeware under the publisher's commercial JUCE licence.
The AAX plugin is built against the Avid AAX SDK (external proprietary dependency, not committed to the repository)
and digitally signed using PACE Eden (`wraptool sign`) with the publisher's PACE account and product Wrap Configuration GUID:

```bash
wraptool sign --verbose \
  --account "$BLUE_PACE_ACCOUNT" \
  --wcguid "$BLUE_AAX_WCGUID" \
  --signid "$BLUE_APP_ID" \
  --in "build/release/BlueChannel_artefacts/Release/AAX/Blue Channel.aaxplugin" \
  --out "$OUT/staging/AAX/Blue Channel.aaxplugin" \
  --autoinstall on
```

The signed AAX bundle is verified via `wraptool verify` and packaged into `BlueChannel-1.0.0-macos.dmg`,
`BlueChannel-1.0.0-macos.pkg`, and `BlueChannel-1.0.0-macos-bundles.zip`. Windows and Linux CI packages exclude AAX.

## Steps

1. Freeze: set `VERSION`, update `docs/MEASUREMENTS.md` and release notes, commit, and confirm a clean tree
   (`tools/release_manifest.py` records `source_tree_clean`).
2. Build and test each platform with its packaging script (or CI for Windows and Linux).
3. macOS signing: `SIGN=1 installer/build_macos.sh` signs the bundles (LV2: the binary inside), builds and
   signs the package, submits it for notarisation, staples and assesses it. `SIGN=1 NOTARIZE=0` signs without
   submitting. Identities: `BLUE_APP_ID`, `BLUE_INSTALLER_ID`; notary profile: `BLUE_NOTARY_PROFILE`.
   A package is notarised only when the service returns *Accepted* and the staple validates.
4. Windows signing (optional): set `SIGN_CERT_THUMBPRINT` before `build_windows.ps1`.
5. Checksums are computed last, after stapling.
6. Inspect the payload: `pkgutil --payload-files`, the zip listings, `release-manifest.json`, the licence
   `INDEX.txt` (30 files on macOS for 0.1.0). `._` entries in a pkgbuild payload are how it records
   extended attributes (for example `com.apple.provenance`); Installer restores them as attributes.
7. Tag `v<version>`, push the tag, upload the packages, checksums and manifest, and verify the remote digests.
8. State platform availability and known limits in the release notes; do not imply that untested platforms
   or hosts were verified.

## Release checklist

```text
Version / tag / source commit:
Clean tree: yes / no
Tests: stage / model / processor (macOS arm64, x86_64 under Rosetta, Windows CI, Linux CI)
auval / pluginval:
Signing and notarisation (submission id, Accepted, staple, Gatekeeper):
Payload inspected, AAX PACE signed & present on macOS:
Licence inventory count:
Checksums (after stapling):
Known limits stated:
```
