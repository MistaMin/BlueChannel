# ADR-004: Correct two transcription values

Status: accepted. Date: 2026-10-17. Owner: DSP.

1. De-esser frequency pot 20 kohm -> 47 kohm. As transcribed the band covers 3.6 to 9.1 kHz; 47 kohm gives
   2.1 to 8.7 kHz, matching the documented 2.2 to 9.2 kHz. The stage test scales the reference fraction.
2. Instrument trim gang orientation. As transcribed, the instrument stage's gang runs against the trim
   stage's (19.6 to 28 dB total); tracking gives 7.4 to 40.4 dB, matching the documented 10 to 40 dB.

Both are flagged in the netlists and `docs/CIRCUIT_MODEL.md`. Revisit if a re-read of the source schematic
contradicts either.
