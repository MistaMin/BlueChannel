# Building

Requirements on every platform: CMake 3.22+, a C++20 compiler, Git, Python 3 (packaging tools only).
GoodLookinUI is a git submodule: clone with `--recurse-submodules` or run
`git submodule update --init --recursive`. JUCE 9.0.3 and clap-juce-extensions are fetched by CMake.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Outputs: `build/BlueChannel_artefacts/Release/{VST3,AU,CLAP,LV2,Standalone,AAX}`.

## Options

| Option | Default | Meaning |
|---|---|---|
| `BLUECHANNEL_JUCE_SOURCE` | empty | use a local JUCE checkout instead of downloading |
| `BLUECHANNEL_CLAP_EXT_SOURCE` | empty | use a local clap-juce-extensions checkout |
| `GOODLOOKINUI_ROOT` | `third_party/GoodLookinUI` | toolkit location |
| `BLUECHANNEL_UNIVERSAL` | OFF | macOS arm64 + x86_64 |
| `BLUECHANNEL_BUILD_CLAP` | ON | CLAP target |
| `BLUECHANNEL_BUILD_AAX` | ON | AAX target when an SDK is found |
| `BLUECHANNEL_AAX_SDK` | `../aax-sdk-2-8-1` | AAX SDK location (not in the repository) |
| `BLUECHANNEL_BUILD_PLUGIN` | ON | OFF builds only the JUCE-free engine tests and tools |
| `BLUECHANNEL_DEV_EDITOR` | OFF | developer mode for placing and sizing knobs (see below) |

## Developer mode: knob placement and size

Knob positions, sizes, labels, knob styles, cap colours and caption sizes live in `Designs/knobs.csv`
(GoodLookinUI design format, one row per knob, keyed by parameter ID, coordinates in panel units of the
1560 x 450 panel). The file is baked into every build, so editing it changes release builds too.

```sh
cmake -S . -B build/devmode -G Ninja -DCMAKE_BUILD_TYPE=Release -DBLUECHANNEL_DEV_EDITOR=ON
cmake --build build/devmode --target BlueChannel_Standalone   # or any plugin format
```

Open the editor and press **Dev** (bottom left, next to Bypass). The window grows an inspector:

| Action | Effect |
|---|---|
| click a knob | select it (or pick it in the inspector list) |
| drag | move (2-unit grid; hold Alt for 1) |
| drag the corner handle | resize (minimum 20 x 20) |
| arrow keys / Shift+arrows | nudge 1 / 10 units |
| Cmd/Ctrl+Z | undo the last move, resize or nudge |
| inspector fields | exact X, Y, W, H, label, style, colour, caption size, mark colour (its own Undo button) |

Every change is written to `Designs/knobs.csv` in the source tree within about a third of a second (only
when the text changes, atomically), so `git diff Designs/knobs.csv` shows exactly what moved. A developer
build starts from the file on disk, so a relaunch shows the saved design without rebuilding; any build
re-embeds it automatically the next time it is built. Press **Dev** again to leave developer mode. Release builds contain none
of this code. `BlueChannelSnapshot --design <file>` writes the current layout in the same format.

Only knobs are placed this way; keys, lamps and meters keep their coded positions.

## Platforms

- **macOS**: Xcode command-line tools; minimum macOS 11. `installer/build_macos.sh` builds universal
  binaries, runs the tests (and the model test under Rosetta), and packages.
- **Windows**: Visual Studio 2022 with C++; `installer\build_windows.ps1`. Inno Setup 6 is optional.
- **Linux** (Debian/Ubuntu package names): `ninja-build libasound2-dev libjack-jackd2-dev libx11-dev
  libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev
  libfreetype6-dev libfontconfig1-dev libgl1-mesa-dev`; `installer/build_linux.sh`. The processor test
  opens an editor, so headless machines need `xvfb-run`.

## Licensing of builds

JUCE is dual licensed under AGPLv3 and a commercial licence. You need a JUCE licence that covers how you
use and distribute your build. The AAX SDK has its own terms and is not included.

## Development tools

| Command | Purpose |
|---|---|
| `BlueChannelStageTest tests/reference` | netlists versus ngspice references |
| `BlueChannelModelTest [fit]` | engine checks; `fit` refits the transformer core |
| `BlueChannelCalibrate > Source/Model/Calibration.h` | regenerate panel calibration tables |
| `BlueChannelMeasure response <case>` / `thd <input>` | measurement fixtures |
| `BlueChannelBench [seconds]` | CPU benchmark |
| `BlueChannelSnapshot <dir> [scale]` | render the editor to PNG (both skins) |
| `tools/make_references.sh`, `tools/characterize.py` | ngspice references and device sweeps (need the reference deck via `BLUE_REFERENCE_CIR`) |
