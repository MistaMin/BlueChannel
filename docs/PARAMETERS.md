# Parameters and session compatibility

Schema version 1 (Blue Channel 0.1.0). Parameter IDs, order, ranges and choice lists are part of saved
sessions and automation. Rules for later versions:

- Never change the meaning, range, skew or choice order of an existing ID. Append new parameters, with a
  higher JUCE version hint.
- A state that lacks a parameter restores that parameter to its **default**, so restoring into a modified
  instance gives the same result as restoring into a fresh one (`setStateInformation`). If a future default
  would change the sound of old sessions, add an explicit migration for the missing field instead.
- Malformed or foreign state is ignored and the current state kept.
- The state stores `schema` and `version` attributes, plus editor-only properties: `skin`, `vusource`,
  `vucal`, `uiscale`.

## Registry

Order below is host order. Choice values are stored as indices.

| ID | Name | Range / choices | Default |
|---|---|---|---|
| source | Input | Mic, Line, Inst | Mic |
| micgain | Mic Gain | 12 positions: -8, 0, +5, +11, +17, +23, +29, +35, +41, +48, +54, +60 dB input to output | 0 dB (index 1) |
| linegain | Line Gain | -18, -12, -6, 0, +6, +12, +18 dB | 0 dB |
| trim | Trim | 0 to 20.81 dB (instrument gain follows the same pot) | 0 |
| phase | Phase | off / on | off |
| impedance | Impedance | Low 600, Classic 1.4k, Medium 2.4k, High 6.8k | Classic |
| air | Mic Air | off / on | off |
| alleq | All EQ | off / on | on |
| filtroute | Filters | Out, In, Comp key, Gate key | Out |
| hpf | High Pass | 19.6 Hz to 1.51 kHz, log | 19.6 Hz |
| lpf | Low Pass | 307 Hz to 23.6 kHz, log | 23.6 kHz |
| shelfroute | Shelving EQ | Out, In, Comp key, Gate key | Out |
| lfgain | LF Gain | -17.5 to +17.5 dB | 0 |
| lffreq | LF Frequency | 20/33, 56/95, 160/270, 460/655 Hz | 56/95 |
| lfhigh | LF Hi Range | off / on | off |
| hfgain | HF Gain | -17.5 to +17.5 dB | 0 |
| hffreq | HF Frequency | 1.5/2.2, 3.3/4.7, 6.8/10, 15/18 kHz | 3.3/4.7 |
| hfhigh | HF Hi Range | off / on | off |
| paramroute | Parametric EQ | Out, In, Comp key, Gate key | Out |
| lmfgain | LMF Gain | -14.8 to +14.8 dB | 0 |
| lmffreq | LMF Frequency | 35.5 to 406 Hz (x1 legend), log | 120 Hz |
| lmfq | LMF Q | 0.85 to 2.91, log | 1.2 |
| lmfx3 | LMF x3 | off / on (frequency x3) | off |
| hmfgain | HMF Gain | -14.8 to +14.8 dB | 0 |
| hmffreq | HMF Frequency | 540 Hz to 6.0 kHz (x1 legend), log | 1.8 kHz |
| hmfq | HMF Q | 0.85 to 2.91, log | 1.2 |
| hmfx3 | HMF x3 | off / on | off |
| compin | Comp In | off / on | off |
| comptype | Comp Type | VCA, Vintage Comp, Vintage Lim | VCA |
| ratio | Ratio | 1.5:1 to 10:1, log | 3:1 |
| threshold | Comp Threshold | -28 to +12 dB (connector scale) | 0 |
| attack | Attack | 0.1 to 100 ms, log | 10 ms |
| release | Release | 0.1 to 7 s, log | 0.3 s |
| autorel | Auto Release | off / on | off |
| makeup | Makeup | 0 to 20.8 dB | 0 |
| compkey | Comp Ext Key | off / on | off |
| blend | Blend | off / on | off |
| mix | Mix | 0 to 100 % | 100 % |
| complisten | Comp Listen | off / on | off |
| gatein | Gate In | off / on | off |
| expand | Expand | off / on | off |
| gatethresh | Gate Threshold | -40 to +10 dB | -40 |
| range | Range | 0 to 80 dB | 20 |
| hold | Hold | 20 ms to 4 s, log | 100 ms |
| gaterelease | Gate Release | 0.1 to 5 s, log | 0.5 s |
| fastatt | Fast Attack | off / on | off |
| hyst | Hysteresis | off / on | on |
| gatekey | Gate Ext Key | off / on | off |
| gatelisten | Gate Listen | off / on | off |
| deessin | De-Ess In | off / on | off |
| deessthresh | De-Ess Threshold | -30 to +10 dB | 0 |
| deessfreq | De-Ess Frequency | 2.06 to 8.73 kHz, log | 5 kHz |
| deesslisten | De-Ess Listen | off / on | off |
| dynpos | Dynamics Position | Post-EQ, Pre-EQ | Post-EQ |
| bypass | Bypass | off / on (EQ, dynamics and limiter out; preamp and output stay in) | off |
| output | Output | -60 to +6.6 dB (skewed) | 0 |
| softlimit | Soft Limiter | off / on | off |
| mute | Mute | off / on | off |
| link | Stereo Link | off / on | on |
| oversampling | Oversampling | 1x, 2x, 4x, 8x, 16x Derp mode | 2x |
| offlineos | Offline Oversampling | Same as playback, 1x, 2x, 4x, 8x, 16x Derp mode | Same as playback |

Ranges that come from the circuit (EQ frequencies, gains, Q, filters, trim, make-up, output) are generated
from `Source/Model/Calibration.h`. If the calibration is regenerated, compare the ranges with this table:
a changed range changes the meaning of saved automation.

## Scope

| Operation | What it changes |
|---|---|
| Factory program | every sound parameter (reset to default, then the program's values); not oversampling or offline oversampling, not editor properties |
| Project save / restore | every parameter and the editor properties |
| Reset (host) | DSP histories only |

Factory programs: Init, Vocal, Drum Bus, Bass DI, Gated Toms, Mix Polish.

## HQ mode

Appended host parameter `hq` (Offline HQ): boolean, default off. The full component solve is used only
in host non-realtime exports; playback remains Standard. Factory programs preserve this preference.
The experimental `rthq` parameter and native solver have been removed. Earlier parameter IDs/order
remain unchanged. See [HQ mode](HQ_MODE.md).

The existing microphone switch's index 1 is now a calibrated 0 dB detent instead of the old approximately
-2 dB detent. Its nominal software correction is +2.754406 dB after the mic circuit, calibrated with a
150-ohm source and Classic impedance at 1 kHz. Other mic switch indices keep their previous values.
Fresh instances and Init select Mic/index 1, trim 0 and output 0. Saved source/gain selections still
restore, but an earlier saved mic index 1 now uses the new unity calibration.


The GUI combines these controls: the final Offline lamp/menu choice, `\( ᐖ)/ HQ`, sets `hq` on without
changing `offlineos`. Selecting a normal Offline choice sets `hq` off and updates `offlineos`. No host
parameter indices or choice counts have changed; the seventh GUI state is a presentation of the two
existing parameters. The Digital header HQ lamp is an indicator, not a separate toggle.
