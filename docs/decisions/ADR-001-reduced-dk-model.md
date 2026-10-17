# ADR-001: DK state-space linear stages with measured non-linear parts

Status: accepted. Date: 2026-10-17. Owner: DSP.

## Problem and constraints
The reference is a component-level SPICE model of about 700 nodes with transistor, transformer and optical
devices. It must run in real time for two channels, oversampled, on ordinary machines.

## Options
1. Run the whole netlist with a sparse Newton solver (as the Tube Channel circuit engine did): exact but
   far too slow at this size (Tube Channel needed tens of percent of a core for a smaller circuit).
2. Hand-design behavioural filters from the panel legends: cheap, but loses the circuit's actual shapes.
3. Transcribe each linear stage, discretise it with the nodal DK method at runtime, and reduce only the
   non-linear parts to measured behaviour.

## Evidence
Option 3: 47 stage comparisons against ngspice (`docs/MEASUREMENTS.md`); 10 % of one M1 Max core for stereo
with everything engaged at 2x.

## Decision and consequences
Option 3. Op-amps are ideal, with swing limits at stage outputs only; internal op-amp clipping inside a
stage is not modelled. Non-linear parts are fitted to the reference model or published data (transformer,
VCA, instrument stage, optical cells) and are only as good as those data.

## Revisit
If listening finds a stage whose character depends on internal clipping or op-amp slew, model that stage's
non-linearity inside the DK solve (one Newton variable) and compare against an ngspice transient.
