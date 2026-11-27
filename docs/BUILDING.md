# Building

The plugin is built from `plugin/` with Rust and OpenPluginSDK: see [plugin/README.md](../plugin/README.md).
This page covers the model's own tests and tools in the repository root, which need only CMake 3.22+
and a C++20 compiler.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

| Command | Purpose |
|---|---|
| `BlueChannelStageTest tests/reference` | netlists versus ngspice references |
| `BlueChannelModelTest [fit]` | engine checks; `fit` refits the transformer core |
| `BlueChannelCalibrate > Source/Model/Calibration.h` | regenerate panel calibration tables |
| `BlueChannelMeasure response <case>` / `thd <input>` | measurement fixtures |
| `BlueChannelBench [seconds]` | CPU benchmark |
| `tools/make_references.sh`, `tools/characterize.py` | ngspice references and device sweeps (need the reference deck via `BLUE_REFERENCE_CIR`) |

The offline component solver is described in [HQ mode](HQ_MODE.md).

The JUCE build that version 1.0 was made from, with its editor, installers and developer layout mode,
is at tag `v1.0.0`.
