# ADR-003: Digital detectors drive the circuit's control ports

Status: accepted. Date: 2026-10-17. Owner: DSP.

The reference model removes the original analogue detectors and exposes current ports: VCA sink current,
de-esser LED current, three limiter LED currents. Blue Channel's detectors are digital and output those
quantities through the inverse of the measured device laws, so the analogue gain elements (and their law and
distortion) remain in the audio path. Compressor and gate currents add at one port, as in the reference.

Consequences: detector feel is OpenGrid's design, not a reproduction of the original detectors; it is
documented in `docs/DYNAMICS.md` and needs listening acceptance. The optical laws are estimates.
