# ADR-005: Panel ranges follow the circuit; later-revision features are extensions

Status: accepted. Date: 2026-10-17. Owner: product.

The circuit model and the available user documentation describe different revisions. Where they differ
(mic gain steps, parametric gain range, filter ranges), legends show what the transcribed circuit does,
measured by `tests/calibrate.cpp`; the differences are listed in `docs/CIRCUIT_MODEL.md`. Features present
only in the documentation (input impedance, Air, extra shelf frequencies, vintage optical compressor,
blend) are implemented as clearly labelled extensions built from the same circuit elements or the same
optical model. Inserts, split modes, external sum, phantom power and the converter are left out.
