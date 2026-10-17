# Blue Channel 1.0 release handoff

Start with README.md, docs/USER_MANUAL.md, docs/MEASUREMENTS.md and docs/RELEASE_1.0.0.md.
The public source is MIT licensed; official binaries are freeware under the publisher's commercial
framework licence. THIRD_PARTY_LICENSES.md and BINARY_LICENSE.txt define the distinction.

The analogue and Digital skins control the same model. New instances select Mic at calibrated 0 dB.
Offline's final derp HQ lamp arms the full component solver; there is no realtime HQ mode. Standard
playback keeps its own oversampling setting. Full HQ remains expensive and experimental; HQ_MODE.md
records convergence, memory and model limits.

All numerical runtime model code is in Source/Circuit. Original reference documents, OCR, schematic
images and extraction tools are private and excluded. Public builds need no hardware source documents.
See SOURCE_PROVENANCE.md and tools/audit_release.py. Never add private engineering inputs to Git or
release payloads. Dependency notices must remain in every distribution.

The macOS release pipeline builds universal VST3, AU, CLAP, LV2 and Standalone, signs the code and
installer, and records notarisation results with the artifacts. It excludes AAX. Windows/Linux CI
builds are Standard-only until their HQ library packaging is validated. Do not imply physical-machine
or listening acceptance from an automated build. Owner listening, multi-instance DAW checks and
Windows/Linux hardware validation remain open; see MEASUREMENTS.md.
