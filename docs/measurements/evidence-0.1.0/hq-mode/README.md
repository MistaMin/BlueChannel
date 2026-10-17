# HQ development evidence — 2026-10-17

Machine: macOS arm64. Base commit ccd4aa4 plus the local Digital editor/HQ changes. Native development build,
not the signed universal 0.1.0 installer. Component simulator: shared ngspice 47. See ../../../HQ_MODE.md.

`output-audit.csv` compares the component output circuit with and without its implicit excess magnetising
current. Generated with a private engineering audit tool. The same finite amplifier,
bias diodes, complementary driver, windings, tertiary feedback and 100k differential load remain in both runs.

The residual is small (sub-microvolt). Independent adaptive time grids/interpolation and numerical error
contribute to that residual; **it is not a measured isolated core THD figure**. The THD column describes the
whole output model in that fixture, including finite driver and low-frequency settling. Do not attribute it
solely to the core. These component estimates remain unvalidated by hardware measurement/listening.

`BlueChannelHQTest` checks actual coupled component rendering, private simulator instances, block
continuation, repeat-bounce reset, controls and the processor's float/double non-realtime path. The extended
run also exercises dynamics control ports and a microphone input restart. Source changes restart the full
component state; continuous controls use a 20 ms glide. Normal continuation keeps every circuit state.

Final validation: all three standard suites passed; extended HQ checks, including the actual processor
float/double offline path, passed in 105.705 s. The packaged simulator was placed beside the test executable
before the host-wrapper portion of that run. Generated circuit SHA-256: `7c1efd5f2ee014e1d7af13339d114b36ab94e580172922db6af6132736654162`.
