# Blue Channel 1.0

The first public Blue Channel release combines the analogue panel and Digital interface with Standard
playback and the optional full Offline HQ solver. New instances and Init select the Mic preamp at
calibrated 0 dB, with trim/output at zero. Existing saved input selections still restore; the former
near-unity Mic detent (index 1) now uses the unity calibration.

Digital provides compressor-curve handles, an EQ response with draggable bands and HP/LP filters, an
output FFT, and All Controls for the remaining controls. Sidechain filters stay available when assigned
outside the audio path. Blue-panel key/selector text is white. Fresh editors start at 1248 × 360.

Offline HQ is the final derp HQ lamp in the analogue Offline selector and a choice in Digital's Offline
menu. The standalone HQ button and the experimental realtime HQ engine are removed. Host parameter IDs
and the six underlying offline-factor choices are retained. HQ preserves the existing integration factor,
with its 4x minimum; normal Offline choices disarm it. Older six-row layouts grow to show the added lamp.

The output transformer is modelled in both engines. Standard includes windings, magnetising inductance
and tertiary feedback, with an ideal driver that suppresses the core injection at the terminals. Full HQ
solves the nonlinear core with the finite output driver. Components and optical/device laws remain
engineering estimates, not measured hardware. Digital detectors remain the product's sidechain design.

The macOS package is universal (arm64/x86_64), with VST3, AU, CLAP, LV2, AAX and Standalone plus user guide,
UI screenshots and applicable licence texts. Code and installer signatures, notarisation results and
checksums are recorded with the release artifacts. AAX is digitally signed with PACE Eden for Pro Tools
compatibility. Windows and Linux CI packages are provided on the release page.

Project source is MIT; official complete binaries are freeware under the publisher's commercial JUCE
licence. JUCE's independent open-source alternative is AGPLv3. End users need no developer licence for
the official binaries. Component permissions and copyright notices remain intact, including the shared
simulator's LGPL rights and corresponding source. Private hardware source documents are excluded.

## Known limits

Full HQ is experimental, very slow and retains growing transient probe history. The continuous horizon
is one hour. Source and Mic gain-switch changes restart its operating point. Failed HQ exports mute
rather than substitute Standard audio. Hosts must report non-realtime processing to engage full HQ.
Listening acceptance, multi-instance DAW validation, long-render memory testing and Windows/Linux
hardware checks remain pending. See [HQ mode](https://github.com/MistaMin/BlueChannel/blob/main/docs/HQ_MODE.md) and [Measurements](https://github.com/MistaMin/BlueChannel/blob/main/docs/MEASUREMENTS.md).

![Analogue interface](https://raw.githubusercontent.com/MistaMin/BlueChannel/main/docs/images/blue-channel-analogue.png)
![Digital overview](https://raw.githubusercontent.com/MistaMin/BlueChannel/main/docs/images/blue-channel-digital.png)
![Digital full controls](https://raw.githubusercontent.com/MistaMin/BlueChannel/main/docs/images/blue-channel-digital-controls.png)
