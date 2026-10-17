# Oversampling, latency and offline rendering

## Oversampling

Explicit factors: 1x, 2x (default), 4x, 8x and 16x (`\( ᐖ)/ - Derp mode`; host-facing text "16x Derp mode").
A factor means the same multiplication at every session rate. The whole chain, detectors included, runs at
the internal rate; time constants are computed for that rate, so changing the factor does not change
compressor timing.

The half-band filters are minimum-phase IIR (polyphase all-pass pairs) designed for 120 dB stop band, with
the pass band kept to 20 kHz (or 0.4535 x the session rate). Reported latency is **zero** at every factor.
Zero latency does not mean zero phase effect: minimum-phase filters add phase shift near the top of the band.

Every oversampler is allocated in `prepareToPlay`; changing the factor during playback recomputes
coefficients and resets histories with a 10 ms fade-in, without allocation.

CPU (M1 Max, stereo, all sections engaged, 64-frame blocks): about 10 % of one core at 2x and 48 kHz, 20 % at
4x, 40 % at 8x and 80 % at 16x (`docs/measurements/evidence-0.1.0/bench.txt`). 8x and 16x are intended for
offline rendering.

## Render policy

| Activity | Bank and factor |
|---|---|
| Playback, monitoring, recording | live bank, playback factor |
| Realtime export | live bank, playback factor |
| Host-reported offline render, Offline = Same as playback | live bank, playback factor |
| Host-reported offline render, Offline = another factor | render bank, offline factor |

Only the host's non-realtime flag selects the render bank. The render bank is a complete second engine:
entering an offline render resets it, so repeated bounces start from the same state and are bit-identical
(`tests/processor_test.cpp`); the live bank keeps its history meanwhile. Latency is zero in both, so
switching banks never changes host compensation.

Not verified: that every host sets the non-realtime flag correctly. In a host that does not, offline exports
use the playback factor.
