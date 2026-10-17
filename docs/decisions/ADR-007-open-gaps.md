# ADR-007: Release 0.1.0 with listening and DAW acceptance open

Status: deferred (gaps open). Date: 2026-10-17. Owner: product.

Open gates, each with its next experiment:

| Gap | Next experiment | Threshold |
|---|---|---|
| Program-material listening | Level-matched A/B on vocals, drums, bass, mix bus through each section; sound owner records accept/reject per domain | written decision |
| DAW behaviour | In two DAWs per platform: 32 stereo instances at 48 kHz/64, automation of every continuous control, offline export with Offline = 8x (confirm the factor via the telemetry), session reopen | no underruns, identical reopen, correct offline factor |
| pluginval | strictness 7 on macOS, Windows, Linux | pass |
| Windows / Linux on hardware | install packages, open editor, process audio | works |
| Intel macOS performance | benchmark on physical Intel hardware | 2x everything-in under 25 % of a core |
