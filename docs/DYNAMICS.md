# Dynamics side-chains

The reference circuit model keeps the analogue gain elements and replaces the original detectors with
control ports. Blue Channel's detectors are digital (`Source/Dynamics/Detectors.h`), run at the internal
rate, keep their state across host blocks and never depend on the editor.

```text
key (internal, ext key or EQ-filtered) -> detector -> gain reduction (dB)
   -> control current (inverse of the measured VCA law)  -> VCA gain cell (audio)
   -> LED current (inverse of the LDR law and network)   -> optical cell (audio)
```

## Compressor, VCA mode

- Feed-forward from the dynamics input, or from the sidechain bus with Ext Key. EQ sections set to *Comp*
  filter the key instead of the audio.
- Rectifier with a 2 ms peak hold, then a soft-knee curve (10 dB knee), ratio 1.5:1 to 10:1, threshold
  -28 to +12 dB on the connector scale.
- Gain reduction smoothed in dB: attack 0.1 to 100 ms, release 0.1 to 7 s.
- Auto release: a fast path (60 ms release) and a slow path (250 ms attack, 2 s release) that only charges
  during sustained compression; the larger one is applied. Short transients release quickly, long
  compression releases slowly.
- Gain reduction becomes a control current through the inverse of the measured VCA law, so the cell's own
  law and distortion shape the result. Compressor and gate currents add at the same port.
- Blend mixes the gain cell input (dry) with the compressed output after make-up.
- Stereo link: both channels use the louder detector level (de-esser not linked, as on the reference unit).

## Compressor, vintage modes (extension)

An optical shunt attenuator before the gain cell (10 kohm series, LDR to ground). Detector: 4 ms attack,
60 ms release; cell: 10 ms attack and a release that lengthens from 80 ms to about 1.7 s with a memory that
charges during sustained compression (2.5 s) and discharges slowly (6 s). Attack, release, auto release and
ext key are inactive, as documented for the reference unit. Comp: soft 12 dB knee, ratio mapped to 1.5 to 5;
Lim: hard 2 dB knee, ratio mapped to 5 to 20, threshold 4 dB higher.

## Expander / gate

- Peak detector (5 ms release) on the key. Opens at the threshold (-40 to +10 dB); closes below
  threshold - 2 dB, or - 10 dB with Hyst.
- Hold 20 ms to 4 s, then release at a constant rate: Range divided by the Release time (0.1 to 5 s).
- Attack: 2 ms, or 50 us with Fast Att.
- Gate: closed attenuation = Range (0 to 80 dB; the VCA floors near 77 dB). Expand: 1:2 downward expansion
  below threshold, limited to Range.
- Gate key filtering with EQ sections set to *Gate*; Ext Key uses the sidechain bus.

## De-esser

- Detector on the circuit's own band-pass output (0.5 ms attack, 50 ms release).
- Above threshold (-30 to +10 dB), the wanted attenuation at the tuned frequency follows 2:1.
- Wanted gain -> cell resistance by solving `|1 + H w| = g` with the band-pass response H at the centre
  (magnitude and phase from the calibration tables) -> LED current through the inverse cell law, limited
  to 20 mA. Verified: -0.92, -3.10, -6.02 and -8.64 dB for targets of -0.9, -3.1, -6.0 and -8.6 dB; the
  network's limit is about -12 dB at 5 kHz.
- Listen outputs the band-pass signal.

## Soft limiter

Connector-referred peak detector, instant attack, 50 ms release. Static curve from the reference unit's
documentation: below -6 dBFS unity, -6 to -4 dBFS 1.5:1, -4 to 0 dBFS 2:1, above 0 dBFS infinite (ceiling
-2.67 dBFS). All three bands receive the same LED current, computed from the inverse cell law; while a cell
is darker than wanted the drive is increased (3 x the shortfall, at most 20 mA) to shorten the 2 ms optical
attack. Overshoot during the optical attack is real: the limiter is optical, not a clipper.

## Listen

Comp Listen and Gate Listen send the filtered key to the output; De-Ess Listen sends the de-esser band.

## Known limits

- The optical laws and timing are the reference model's estimates, not measured parts.
- The vintage modes are behavioural; they are not in the reference circuit.
- Detector designs are OpenGrid's; the original analogue detectors were not part of the reference model.
- Program-material listening acceptance has not been recorded (`docs/MEASUREMENTS.md`).
