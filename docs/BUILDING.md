# Building

Requirements on every platform: CMake 3.22+, a C++20 compiler, Git, Python 3 (packaging tools only).
GoodLookinUI runtime headers are vendored; no submodule initialisation is needed.
JUCE 9.0.3 and clap-juce-extensions are fetched by CMake.

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
| `BLUECHANNEL_HQ` | OFF | full offline component solver; see [HQ mode](HQ_MODE.md) |
| `BLUECHANNEL_DEV_EDITOR` | OFF | developer mode for placing and sizing knobs (see below) |

## Developer mode: full editor layout

```sh
cmake -S . -B build/devmode -G Ninja -DCMAKE_BUILD_TYPE=Release -DBLUECHANNEL_DEV_EDITOR=ON
cmake --build build/devmode --target BlueChannel_Standalone BlueChannel_VST3 BlueChannel_AU BlueChannel_CLAP BlueChannel_LV2
```

Press **Dev** on the analogue panel or **DEV** in the Digital toolbar. The window opens an inspector
below the panel. Its **Page** selector switches between Analogue, Digital / Overview and Digital /
All Controls. Only controls visible on that page appear in the control selector.

- Click a control to select it, or pick it in **Control**. Drag it to move; drag its lower-right corner
  to resize. Hold Alt for a 1-unit grid instead of 2 units.
- Arrow keys nudge by 1; Shift+arrows by 10. Cmd/Ctrl+Z and **Undo** restore the previous edit.
- **Type** switches continuous controls between a knob, horizontal slider and vertical slider. The
  parameter binding, range and automation identity stay the same. Resize the control to suit its type.
- **Knob Style**, the colour wheel, label and font fields edit presentation. X/Y/Width/Height give exact
  logical panel coordinates. Labels, keys, selectors and graph panels can also be moved and resized.
  Painted section backgrounds and headings are fixed.
- Enable **Use controls** to play with the controls while keeping the inspector open; disable it to
  resume placing controls. **Close Dev** returns to the normal editor.

Changes autosave to `Designs/editor-layout.json` in this repository after about 350 ms and flush when
leaving developer mode or closing the editor. The status line reports success or failure. Each skin and
Digital page has its own entries. Parameter values are not saved in this layout file.

Developer builds reload that file on launch. CMake also embeds it in every build, so the next release
build uses the saved layout. `Designs/knobs.csv` remains the underlying analogue knob design; the JSON
layout applies afterward and takes precedence. The default JSON contains no overrides. Back up or
restore that file to preserve or revert a layout. Do not edit the same layout in multiple instances at
once; each instance saves its current full layout.

`BlueChannelSnapshot --layout <file>` exports the complete layout to JSON;
`BlueChannelSnapshot --design <file>` exports only the legacy analogue knob CSV.

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

## Initial editor size

New editors start at 1560 x 450 logical screen units, twice the previous effective 780 x 225 startup
size. Legacy saved editor sizes are doubled once, up to the existing 3120 x 900 limit. Later user
resizing is preserved across reopen. Developer mode adds the inspector below the panel.

## HQ development build

The optional offline component solver is configured separately from the GUI developer tools. See
[HQ mode](HQ_MODE.md) for the public numerical model, simulator dependency and runtime packaging.
