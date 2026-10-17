# Gain staging

One conversion links host samples to circuit volts (`Source/Model/Levels.h`):

```text
host sample 1.0 (sine peak)  =  +22 dBu  =  13.80 V peak, differential, at the input or output connector
-18 dBFS sine                =  +4 dBu
```

This is the reference unit's converter alignment (0 dBFS = +22 dBu). dBFS values refer to sine peak.

## Ledger (line input, 1 kHz, all controls at unity)

| Point | Level for a -18 dBFS input | Note |
|---|---|---|
| Line connector | +4.0 dBu | host sample x 13.80 V |
| Line receiver output | -2.2 dBu | the receiver halves the differential signal (-6.19 dB) |
| Line gain (0 dB position), trim (0 dB) | -2.2 dBu | |
| EQ, dynamics, de-esser | -2.2 dBu | internal nodes run about 6 dB below connector level |
| Fader at Output 0 dB | -2.0 dBu | fader +0.18 dB compensates the receiver/transformer difference |
| Output transformer secondary | +4.0 dBu | x2 step-up (+6.01 dB) |
| Output | -18 dBFS | measured unity: -0.007 dB |

Microphone and instrument inputs have no receiver loss, so their panel legends are given input to output
(preamp gain + 6.19 dB).

## Headroom

Op-amps swing to +14.07 V / -12.77 V (2 V from the reference rails). An internal node clips at about
+20 dBu, so the line path clips near +28 dBu at the connector (+6 dBFS): 2.4 % THD at +6 dBFS, 0.0002 % at
0 dBFS (`docs/measurements/engine/thd_line.csv`). Asymmetric rails make clipping add even harmonics.
The output stage swings 28 V peak differential (about +27 dBu).

## Thresholds and meters

Compressor, gate and de-esser thresholds are on the connector scale: +4 dBu on a threshold scale means a
+4 dBu (-18 dBFS) signal. The detectors listen at internal nodes and subtract 6.02 dB internally.

The input meter reads after trim and phase, the output meter after the soft limiter; both are referred to
the connector (internal level x 2) and shown in dBFS. The VU reads input or output RMS against
0 VU = +4 dBu or +18 dBu, or compressor gain reduction (0 to -20).

The Audio O/L lamp lights when the input tap, the dynamics output or the limiter output passes half the
op-amp swing (6 dB below clipping).
