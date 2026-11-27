# Blue Channel 1.1.1

Blue Channel 1.1.1 corrects two defects of 1.1.0 and is otherwise the same plugin: the circuit model of
1.0 on OpenPluginSDK, with its editor drawn by GoodLookin DRI, the same plugin ids and the same 62
parameters.

## What is fixed

- **A project saved with 1.0 opens with its settings in VST3, Audio Unit and AAX hosts.** 1.1.0 read
  them in CLAP hosts only and opened at its defaults in the others.
- With the Mic input, a loud tone below about 100 Hz (from -20 dBFS at 16x oversampling, from -12 dBFS
  at the default 2x) made the output overflow after one to three seconds, and the instance stayed silent
  or broken until it was reset. 1.0.0 and 1.1.0 both did this. The mic transformer's core current is now
  injected without its content at half the internal sample rate. Against 1.0.0 the sound differs by at
  most 3.7e-6 of full scale (about -109 dBFS) in the null test.
- Input below -600 dBFS is treated as silence, so the model does not run on denormal numbers.
- From OpenPluginSDK `1f608fe`: an Audio Unit host that sets a switch between two steps gets the nearest
  step, edits made in a VST3 host while the plugin is not processing are kept in the project, and a VST3
  host that restarts processing during a render no longer gets one unprocessed block.

## Formats and requirements

| Platform | Formats | File |
|---|---|---|
| macOS 11 or later, Apple Silicon and Intel | VST3, Audio Unit, CLAP, LV2, AAX | `Blue-Channel-1.1.1-macos.pkg` |
| Windows 10 or later, x64 | VST3, CLAP | `Blue-Channel-1.1.1-windows-x64-setup.exe` |

The macOS installer is signed with a Developer ID and notarised by Apple, and its AAX plug-in is signed
with PACE Eden for Pro Tools. The Windows installer is not code signed. Run the installer and rescan
plug-ins in the host. Installing 1.1.1 replaces 1.1.0 and 1.0 in the formats it has.

Not in this version: a standalone application, LV2 on Windows, AAX on Windows, and Linux. Blue Channel
1.0.0 stays available for those.

## Read before updating from 1.0

- Automation of continuous controls that a CLAP host recorded with 1.0 plays back at other values,
  because 1.0 showed every parameter as 0 to 1. Automation of switches is unaffected.
- The "Program" parameter of 1.0 is gone. The six factory programs are offered through the plugin's
  preset interface, which hosts do not show in their preset menus yet.
- The Output control's travel in a host's own parameter view is linear in dB; 1.0 skewed it.
- Audio is processed in 32-bit float buffers; the model computes in double precision as before.
- Pro Tools: reading a 1.0 project has been tested with a state of 1.0, not with a session that Pro
  Tools saved. Note the settings of a Pro Tools project before updating.

## What was checked

- macOS, Apple Silicon: the plugin's own tests (parameters, state, the SDK's conformance kit, the host
  behaviour tests, offline rendering, HQ Mode), the conformance kit over 24 seeds, and its strict fuzz.
  The allocation check also counts the C++ model's allocations.
- States captured from the 1.0.0 VST3 and Audio Unit load with their settings.
- A null test of the built VST3, Audio Unit and CLAP against the 1.0.0 VST3: nine runs, largest
  difference 3.7e-6.
- The model's own tests, also under AddressSanitizer.
- Avid's AAX Validator on the unsigned AAX: the six tests that run from the command line pass.
- The editor's pictures are identical, pixel for pixel, to those of the build before the SDK update.
- Windows: the plugin's tests and the build's own checks on a hosted Windows machine.

## Known limits

- Not checked by a person in a host: Pro Tools, Logic, and every Windows host. The Windows build has
  been built and tested by machine only; nobody has opened its editor.
- This version was not loaded in a host on macOS before release: `auval` and the scripted host suite,
  which 1.1.0 passed, were not rerun.
- The editor opens no larger than the main screen has room for. A host that opens it on a second,
  narrower screen can still cut it off; drag the window's corner to make it smaller.
- The Intel half of the macOS build was built and not run.
- Pro Tools shows two parameters named "Bypass": the plug-in's own and the host's.
- HQ Mode (macOS only) solves mono instances. On a stereo track the solve fails and the export is muted
  with "HQ failed". 1.0 fails in the same way; it is the same solver.
- Full HQ is experimental and very slow. A failed HQ export is muted and never replaced by Standard
  audio. See [HQ mode](https://github.com/MistaMin/BlueChannel/blob/main/docs/HQ_MODE.md).
- Listening acceptance and long sessions with many instances remain open, as for 1.0.
