# Offline HQ / microphone unity development evidence — 2026-10-17

Machine: Apple M1 Max, macOS 15.7.5, native arm64 Release. Base commit ccd4aa4 plus local Digital UI,
full offline HQ, realtime-HQ removal, label contrast and microphone unity changes. Original signed
universal installer unchanged. This is an ad-hoc signed, unnotarised developer build.

The realtime host parameter, GUI, native nonlinear ports and associated fixed-rate policy are removed.
Standard playback again uses its regular oversampling setting. Full Offline HQ remains available.
Historical realtime evidence and archives describe the earlier experiment, not this build.

The microphone switch retains twelve positions and all prior IDs/indices. Index 1 replaces the old
approximately -2 dB near-unity choice with a 0 dB unity detent; old saved index 1 consequently adopts the
new calibration. A +2.754406 dB software correction follows the mic circuit. This is a product calibration,
not a change to the transcribed resistor ladder. Other switch positions keep their previous gains.
Both engines apply the correction. Fresh instances and Init select Mic/index 1, trim/output zero.

`model-tests.txt` verifies actual whole-channel Mic unity within 0.05 dB at 1 kHz, default 150-ohm source,
Classic impedance, Air off and 0 dB trim/output; the result is approximately 0.0000 dB. Its original line
path and allocation/determinism fixtures remain explicit line tests. Host tests independently render the
fresh plugin and check unity, the 0 dB readout, Mic selection, Init and absence of the realtime HQ control.

The output transformer remains in both engines: windings, magnetising inductance and tertiary feedback.
The Standard ideal driver suppresses the excess-core injection's terminal contribution; full Offline HQ
couples nonlinear core current to its finite output driver. Component estimates and feedback suppression
limits remain; this is not evidence of audible transformer coloration.

Blue-panel keys and selector labels use white ink. Grey-section legends retain their dark ink. Actual
snapshots are inspected for both skins and the Digital All Controls page. Listening, DAW acceptance,
Windows/Linux validation and notarisation remain open.

The full solver uses a fixed linear controlled-voltage stage for the post-mic calibration, avoiding a
nonlinear control-voltage product in its DC operating-point equations. Mic gain-switch changes restart
the solve to update that fixed gain. Mic/line discrete ladder selections round the selector voltage to
the nearest index. The focused `BlueChannelHQTest --startup` fixture passed at the new default, using
actual float and double non-realtime callbacks. Source/continuous-control/model limits remain documented
in HQ_MODE.md; the numerical tolerances and 1 TΩ shunt have not been relaxed to obtain this result.

Final validation: all three CTest suites passed. The extended full Offline HQ fixture passed in
92.3105 seconds (block-split residual 1.71003e-7 FS), including the new default Mic startup and actual
float/double host callbacks. All five packaged formats passed signature checks; simulator libraries
and licence notices are complete. See regression-tests.txt, offline-hq-tests.txt and package-checks.txt.
