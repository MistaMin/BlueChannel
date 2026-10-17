# Digital editor verification — 2026-10-17

Source: `ccd4aa4` plus the uncommitted Digital editor changes in this working tree.
Machine: arm64, macOS 15.7.5. Configuration: `build/dev`, Release, developer editor disabled.

The Digital presentation now has a graphical Overview and an All Controls page. Both use the existing
parameter IDs and ranges. The audio model is unchanged. A bounded atomic tap captures the left output;
the editor performs the 2048-point Hann-windowed FFT. The EQ curve comes from the analogue networks,
not an unrelated filter approximation. Compressor handles edit threshold, ratio and makeup; its curve
shows the fixed-knee detector target rather than gain-cell distortion or time-dependent ballistics.

Automated checks cover the complete parameter inventory, slider/selector/toggle bindings, host updates,
page navigation, skin restoration, compressor graph gestures, filter dragging on a sidechain route,
filter dimming on unused routes, and analyser sample/rate reset behavior. The stage, model and processor
suites pass; see `tests.txt`.

Component renders are generated with:

```sh
build/dev/BlueChannelSnapshot_artefacts/Release/BlueChannelSnapshot build/ui-review
build/dev/BlueChannelSnapshot_artefacts/Release/BlueChannelSnapshot build/ui-review 1 --controls
```

The Digital renders include seeded synthetic audio for the FFT. Both pages were inspected at 1x and
0.85x. The analogue PNG is byte-identical to the pre-change render at 1x. Current documentation renders
are in `docs/images/`. These are offscreen component renders; they do not establish native popup,
physical-display or DAW acceptance. Listening and cross-platform acceptance remain open.

Local macOS targets were rebuilt. The signed installer in `dist/macos` was not regenerated.
