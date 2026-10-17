# ADR-002: Keep every capacitor in the shelving and parametric stages

Status: accepted. Date: 2026-10-17. Owner: DSP.

Shorting coupling capacitors of 10 uF and above (and opening those of 470 pF and below) keeps state counts
small. In the shelving and parametric stages it cost up to 0.68 dB near 20 Hz against ngspice, because the
coupling networks sit inside the tuned paths. Keeping all capacitors there brought the error to 0.13 dB at
15 to 18 states per stage (about 300 multiply-adds per sample per stage). Elsewhere the rule stays, with
`keep` on any capacitor whose corner is in band (for example 220 pF across 47 kohm).

Revisit: if CPU becomes a constraint, diagonalise each stage's state matrix into parallel sections when its
coefficients change (cost per sample O(n) instead of O(n^2)), and verify against the same references.
