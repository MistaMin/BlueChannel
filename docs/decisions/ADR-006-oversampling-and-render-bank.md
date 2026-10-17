# ADR-006: Explicit oversampling factors, zero latency, separate render bank

Status: accepted. Date: 2026-10-17. Owner: host integration.

Factors 1x to 16x mean the same multiplication at every rate. Minimum-phase IIR half-bands give zero
reported latency, so no compensation changes between modes. A second complete engine (render bank) runs only
when the host reports offline processing and a different offline factor is chosen; it is reset on entering
offline so repeated bounces are identical, and the live bank keeps its history. Everything is allocated in
`prepareToPlay`, so factor changes never allocate.

Trade-off: minimum-phase filters shift phase near the top of the band; no linear-phase option is offered.
Revisit if users ask for linear phase (then a fixed latency budget across factors is required).
