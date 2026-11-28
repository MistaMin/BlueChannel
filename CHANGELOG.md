# Changelog

## 1.1.1

See docs/RELEASE_1.1.1.md for what was checked and what was not.

- Built against OpenPluginSDK `07f97e1`. A project saved with 1.0.0 now opens with its settings in VST3,
  Audio Unit and AAX hosts too (1.1.0 read them in CLAP hosts only). The macOS VST3, Audio Unit and AAX
  no longer carry the SDK's unused typefaces.
- Fixed: with the Mic input, a loud tone below about 100 Hz (from -20 dBFS at 16x oversampling, from
  -12 dBFS at the default 2x) made the output overflow after one to three seconds, and the instance stayed
  silent or broken until it was reset. 1.0.0 and 1.1.0 both do this. The mic transformer's core current
  is now injected without its content at half the internal sample rate. The sound differs from 1.0.0 by
  at most 3.7e-6 of full scale (about -109 dBFS) in the null test.
- Input below -600 dBFS is treated as silence, so the model does not run on denormal numbers.
- The JUCE build that 1.0 was made from is no longer in the repository; it is at tag `v1.0.0`. The model
  and its tests stay. The SDK is taken from Git at a pinned revision, so `cargo test` works in a fresh
  clone.
- From the SDK: an Audio Unit host that sets a switch between two steps gets the nearest step, edits made
  in a VST3 host while the plugin is not processing are kept in the project, and a VST3 host that
  restarts processing during a render no longer gets one unprocessed block.

## 1.1.0

The plugin is rebuilt on OpenPluginSDK, with its editor drawn by GoodLookin DRI. The signal path is the
same C++ model as 1.0.0, unchanged, and the plugin ids, the 62 parameters, their ids and their order are
those of 1.0.0. See docs/RELEASE_1.1.0.md for what was checked and what was not.

- The Digital skin has a new layout at 1250 × 900: rows of keys for choices, sliders with their values,
  and the graphs of 1.0.0. The analogue skin keeps the panel of 1.0.0 at 1560 × 450, with lit knobs and
  sharper lettering. Each skin resizes in its own shape and opens no larger than the main screen has
  room for.
- The Digital skin's numbers for input, output and gain reduction change every 1.5 seconds and show the
  highest reading of that time.
- Parameters are shown to hosts in their own units (dB, Hz, positions), where 1.0.0 showed 0 to 1.
- A parameter change inside an audio block applies at its position in the block.
- A 5 second tail is reported to the host.
- Non-finite input is treated as silence.
- The factory programs are offered through the plugin's preset interface; the "Program" parameter is gone.
- The macOS installer has VST3, Audio Unit, CLAP, LV2 and AAX. Windows has VST3 and CLAP. No standalone
  application, no Windows LV2 and no Linux build in this version.
- Known limit: in VST3, Audio Unit and AAX hosts a project saved with 1.0.0 opens at the default
  settings. CLAP projects open with their settings.

## 1.0.0

First public release. Adds the graphical Digital interface, full control page, compressor/EQ editing,
FFT, larger startup GUI, white blue-panel labels, Mic unity startup, and full Offline HQ selected by an
extra derp HQ lamp. Includes public authored numerical models, dependency notices, binary freeware terms
and private-reference exclusion/auditing. See docs/RELEASE_1.0.0.md for scope and validation limits.

- Quantise host-visible switch values so fractional host automation recalls consistently.
- Open the editor at 1248 × 360, resetting enlarged preview windows once and retaining later resizing.
- Remove previous full-HQ circuit records before restart to avoid duplicate source-history cleanup at shutdown.
- Include the standalone macOS audio-input usage description and hardened-runtime entitlement.
- Correct the overload test's final partial buffer and add Linux engine AddressSanitizer checks.
