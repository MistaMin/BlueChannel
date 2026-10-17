# Offline HQ mode (development)

**Offline HQ** is an optional full component solver for host non-realtime export. Playback uses Standard
and retains its normal oversampling selector. HQ is independent of the Digital skin, defaults off and is
preserved by factory programs. The experimental realtime HQ mode has been removed at the owner's request.

On the analogue panel, step **Offline** to its last lamp, **\( ᐖ)/ HQ**, to arm the solver.
The separate HQ button has been removed. In Digital, choose the matching HQ entry in the Offline menu
on All Controls; the header lamp indicates its state. Selecting a normal Offline entry disarms HQ.
The existing `hq` boolean and `offlineos` host choice remain separate for session/automation compatibility;
HQ retains the selected offline integration factor, with its usual 4x minimum. The status reads **Standard / full HQ on export**
during playback and **HQ: component solve** during a host non-realtime callback. Hosts that never report
non-realtime processing continue using Standard. The full solve can take hundreds or thousands of times
the audio duration; begin with a short selection.

The Mic preamp now starts selected at a calibrated **0 dB** detent. A software level correction after
the mic circuit makes that detent nominally unity input-to-output at 1 kHz (150-ohm source, Classic input
impedance, air off, trim/output at zero). Both Standard and Offline HQ apply this correction. It does not
replace either transformer model or imply a flat response or unity at every source impedance/frequency.

HQ resolves the reference audio circuits using ngspice's coupled nonlinear transient solver: bipolar
transistors (including stored charge and junction capacitance), instrument FET, diodes, optical LEDs and
photoresistors, and finite-gain amplifier macros with bandwidth, slew, rail swing and output-current limits.
The input and output transformer excess magnetising currents are implicit functions of the current flux
state, solved alongside the winding currents and amplifier feedback. The output driver includes its diode
bias and complementary transistor pair. It no longer has the Standard model's ideal follower that makes the
core injection completely inaudible at the terminals.

Capacitor, inductor, optical and flux states persist across host blocks. Continuous controls glide over 20 ms through controlled components, rather than rebuilding the circuit
on every automation change. Switching physical input source or the Mic gain switch restarts the full solver and its histories. Stereo instances have separate
channels and linked digital compressor/gate detectors; each plugin instance has an isolated simulator
library and transient state. Entering offline, resetting, switching HQ, changing the input source or Mic gain switch, or changing the integration factor
starts a new solve from the circuit operating point.

The existing digital detector algorithms remain the product's sidechain design. Compressor/gate control
currents and optical LED drive update on the oversampled grid. The product's added opto compressor timing
and memory law remains its digital design; it is not a newly claimed transcription of physical hardware.
EQ routing, key EQ, listen, dynamics order, blend, makeup, output and mute remain available. Selected-out
circuits stay powered in the component graph. Switching and inter-section routing use the product's
functional routing graph, rather than claiming to reproduce omitted hardware switching circuitry.

Audio input reconstruction and output anti-alias filtering use the existing half-band oversampler, with a
minimum of 4x in HQ. Offline 8x/16x tightens the grid further; Same uses the playback setting with that 4x
minimum. The circuit solver also chooses smaller steps when its convergence/error checks require them.
The configured Newton tolerances are reltol 1e-5, abstol 1e-12 A and vntol 1e-6 V, Gear integration order at
most 2, and a 1 TΩ numerical shunt. These settings are finite numerical tolerances, not an exact solution.

If a component solve fails, HQ mutes that offline block and subsequent blocks, and displays **HQ failed:
render muted**. There is no automatic substitution of Standard audio. Reset or start a new offline pass to
retry. A build without the optional solver reports HQ unavailable and likewise mutes an attempted HQ export.

## Model limits

Solving the equations fully does not make estimated components measured hardware. The same semiconductor,
optical and winding estimates remain. The core law is a smooth saturating excess-current law with a 0.5 Hz
flux leak, **not a measured hysteresis loop**. Output feedback can strongly suppress the core's terminal
contribution; HQ is not a promise of obvious transformer colour on line input. Line input uses the electronic
balanced receiver; the microphone transformer is in the microphone path.

Rails use the reference's settled voltages; the supply rectifier, regulator warmup, temperature drift,
component tolerance/noise, omitted inserts/converters and other unmodelled hardware are not added by HQ.
Amplifiers remain nonlinear macros, not transistor-level reconstructions of their internal silicon.

The transient horizon is one hour; a longer continuous pass fails explicitly at that boundary.
The simulator retains saved transient probe history, so memory grows with render length. Long-render
memory limits, numerical convergence sweeps, listening acceptance and DAW bounce acceptance are still open.
Release 1.0.0 packages a universal macOS build. Signing, notarisation and validation results are recorded
with the release artifacts. Windows/Linux HQ packaging is not validated.

## Build

The authored numerical model is checked in as `Source/Circuit/HQAudioCircuit.h` under MIT. A public build
does not read private reference documents, diagrams or extraction tools. Install shared ngspice headers
and a library, then configure `-DBLUECHANNEL_HQ=ON`. Standard-only builds can leave this option off; their
HQ status reports unavailable if an HQ export is attempted.

For the universal macOS release, run `tools/build_hq_simulator_macos.sh`, then configure:

```sh
cmake -S . -B build/release-macos -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DBLUECHANNEL_UNIVERSAL=ON -DBLUECHANNEL_HQ=ON \
  -DBLUECHANNEL_NGSPICE_INCLUDE="$PWD/build/deps/ngspice-universal/include" \
  -DBLUECHANNEL_NGSPICE_LIBRARY="$PWD/build/deps/ngspice-universal/lib/libngspice.dylib" \
  -DBLUECHANNEL_HQ_NOTICES="$PWD/build/deps/ngspice-universal/COPYING"
cmake --build build/release-macos
ctest --test-dir build/release-macos --output-on-failure
```

The package embeds the shared simulator beside each executable and includes its notices. Each offline
instance loads a private temporary copy. See `NGSPICE_SOURCE_NOTICE.txt` for the corresponding-source
archive, build options and replacement permissions. No optional external code-model modules are shipped.
The [shared-library interface](https://ngspice.sourceforge.io/shared.html) documents the callbacks used.

The Mic unity calibration is a fixed linear controlled-voltage stage selected at operating-point setup;
a Mic gain-switch change restarts the solve. Mic/line ladder selectors round to the nearest switch index.
