# Offline HQ selector lamp — 2026-10-17

UI-only follow-up to the offline HQ / Mic unity build. Native macOS arm64, M1 Max, macOS 15.7.5.
The analogue Offline selector has seven lamps: Same, 1x, 2x, 4x, 8x, derp 16x, derp HQ.
The standalone HQ button is removed. Digital's Offline menu has the same extra HQ choice, with a
read-only header lamp visible on both pages.

The GUI maps its seventh state to the existing `hq` boolean; `offlineos` retains six host choices.
Selecting HQ leaves the integration factor unchanged (full solver minimum 4x). Selecting a regular
Offline choice turns HQ off and sets that factor. Host automation and restored state update both
presentations. Older analogue layouts grow to seven rows; later layout identifiers retain their
previous indices. No DSP, Mic unity calibration, host IDs or host parameter choice counts change.

Processor fixtures exercise cycling into/out of HQ, host automation, the Digital menu mapping and
old-layout height migration. Standard stage/model/processor regressions are retained. The full solver's
92.3105 s extended validation belongs to the preceding build; the solver is unchanged by this GUI edit.

Final validation: all three stage/model/processor suites passed. GUI snapshots were inspected and all
five packaged formats passed signature verification. Simulator libraries and licences are complete.
