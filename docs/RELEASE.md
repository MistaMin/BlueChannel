# Release process

## Distribution matrix

| Platform | Formats distributed | Package |
|---|---|---|
| macOS 11+ (arm64 + x86_64) | VST3, AU, CLAP, LV2, AAX | signed and notarised `.pkg` |
| Windows 10+ x64 | VST3, CLAP | Inno Setup installer (`-setup.exe`) |

## Steps

1. Freeze: set `VERSION` and the version in `plugin/Cargo.toml` (a test compares them), update
   `CHANGELOG.md` and the release notes, commit, and confirm a clean tree.
2. Test: `cargo test --release` and the soak in `plugin/` (see `plugin/README.md`), and `ctest` for the
   model (see [Building](BUILDING.md)).
3. macOS: `plugin/scripts/build.sh --hq` builds the universal bundles. Sign them with the Developer ID,
   sign the AAX bundle with PACE Eden (`wraptool sign`, then re-sign with the hardened runtime and run
   `wraptool verify`), build and sign the package, notarise and staple it. The AAX SDK is an external
   proprietary dependency and is never committed.
4. Windows: `.github/workflows/windows.yml` builds, tests and packages on a pushed tag or branch.
5. Checksums are computed last, after stapling.
6. Inspect the payload (`pkgutil --payload-files`) and run `tools/audit_release.py`.
7. Tag `v<version>`, push the tag, upload the packages, checksums and manifest, and verify the remote digests.
8. State platform availability and known limits in the release notes; do not imply that untested platforms
   or hosts were verified.

What was checked for each release is in `docs/RELEASE_<version>.md`. The process for 1.0 (the JUCE
build) is in this file at tag `v1.0.0`.
