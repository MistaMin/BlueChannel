# ADR-008: Separate offline HQ component solver

Status: accepted for experimental development, 2026-10-17.

The owner requested a separate HQ mode resolving all nonlinear components, and explicitly prioritised a
full solve over real-time use. This does not change ADR-005: Digital remains a skin; HQ is a new independent,
session-persistent processing parameter, default off.

Use an optional shared ngspice component transient engine, with an isolated simulator per plugin instance.
Keep the existing Standard engine for playback; run HQ only under the host's non-realtime flag. Preserve
component states through host blocks and implement controls as live circuit controls. A failed HQ render
must be visibly failed and muted, never silently rendered through Standard.

Include the reference audio nonlinear devices and finite amplifier macros. Solve the estimated transformer
flux law implicitly with the finite driver and feedback. Preserve the product's digital detector design.
Do not claim complete internal amplifier silicon, supply startup, measured hysteresis or hardware accuracy
for missing/estimated data. See ../HQ_MODE.md for scope, integration policy and outstanding validation.

For 1.0.0, the authored numerical runtime circuit is checked in at
`Source/Circuit/HQAudioCircuit.h`. Private research material is not a build input.
Official macOS packages include a universal shared solver and corresponding source notices.
