# Plugin QA: VST3: Blue Channel Port Test (OpenGrid)

| Check | Status | Detail |
|---|---|---|
| loads | PASS | VST3: Blue Channel Port Test (OpenGrid), 66 parameters |
| finite output | PASS | peak -3.6 dBFS |
| not silent | PASS | rms -14.9 dBFS |
| deterministic render | PASS | repeat differs by -240.0 dB |
| editor opens | PASS | 1080x503, 0.0% animates by itself |
| every parameter changes the sound | PASS |  |
| every parameter is shown in the GUI | FAIL | HF Frequency, HF Hi Range, Mute, Stereo Link, Oversampling, Offline Oversampling, Wet |
| state survives save and reopen | PASS |  |
| editor reopens | PASS | 5 close/open cycles |

| # | Parameter | Context | Audio | Visual |
|---|---|---|---|---|
| 0 | Input |  | ok (-1.2 dB) | ok (0.342%) |
| 1 | Mic Gain |  | ok (1.8 dB) | ok (0.038%) |
| 2 | Line Gain | Input=0.5 | ok (-0.5 dB) | ok (0.083%) |
| 3 | Trim |  | ok (0.7 dB) | ok (0.047%) |
| 4 | Phase |  | ok (2.4 dB) | ok (0.063%) |
| 5 | Impedance |  | ok (-20.3 dB) | ok (0.026%) |
| 6 | Mic Air |  | ok (-17.1 dB) | ok (0.075%) |
| 7 | All EQ | random state 1 | CONDITIONAL (0.6 dB) | ok (0.060%) |
| 8 | Filters | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.061%) |
| 9 | High Pass | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.028%) |
| 10 | Low Pass | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.027%) |
| 11 | Shelving EQ | random state 1 | CONDITIONAL (0.6 dB) | ok (0.074%) |
| 12 | LF Gain | random state 1 | CONDITIONAL (4.7 dB) | ok (0.027%) |
| 13 | LF Frequency | random state 1 | CONDITIONAL (0.6 dB) | ok (0.046%) |
| 14 | LF Hi Range | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.069%) |
| 15 | HF Gain | random state 1 | CONDITIONAL (4.6 dB) | ok (0.027%) |
| 16 | HF Frequency | random state 1 | CONDITIONAL (1.0 dB) | NOT SHOWN (0.000%) |
| 17 | HF Hi Range | random state 1 | CONDITIONAL (-0.1 dB) | NOT SHOWN (0.000%) |
| 18 | Parametric EQ | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.074%) |
| 19 | LMF Gain | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.038%) |
| 20 | LMF Frequency | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.019%) |
| 21 | LMF Q | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.020%) |
| 22 | LMF x3 | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.090%) |
| 23 | HMF Gain | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.027%) |
| 24 | HMF Frequency | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.020%) |
| 25 | HMF Q | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.020%) |
| 26 | HMF x3 | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.087%) |
| 27 | Comp In |  | ok (-9.2 dB) | ok (0.064%) |
| 28 | Comp Type | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.027%) |
| 29 | Ratio | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.024%) |
| 30 | Comp Threshold | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.024%) |
| 31 | Attack | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.023%) |
| 32 | Release | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.024%) |
| 33 | Auto Release | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.064%) |
| 34 | Makeup | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.026%) |
| 35 | Comp Ext Key | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.063%) |
| 36 | Blend | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.062%) |
| 37 | Mix | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.026%) |
| 38 | Comp Listen | random state 1 | CONDITIONAL (6.0 dB) | ok (0.064%) |
| 39 | Gate In |  | ok (-25.6 dB) | ok (0.064%) |
| 40 | Expand | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.064%) |
| 41 | Gate Threshold | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.031%) |
| 42 | Range | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.027%) |
| 43 | Hold | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.027%) |
| 44 | Gate Release | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.027%) |
| 45 | Fast Attack | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.064%) |
| 46 | Hysteresis | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.057%) |
| 47 | Gate Ext Key | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.061%) |
| 48 | Gate Listen | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.073%) |
| 49 | De-Ess In |  | ok (-13.8 dB) | ok (0.064%) |
| 50 | De-Ess Threshold | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.029%) |
| 51 | De-Ess Frequency | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.028%) |
| 52 | De-Ess Listen |  | ok (-0.2 dB) | ok (0.064%) |
| 53 | Dynamics Position | random state 1 | CONDITIONAL (-0.1 dB) | ok (0.057%) |
| 54 | Bypass | random state 1 | CONDITIONAL (2.8 dB) | excused (0.067%) |
| 55 | Output |  | ok (-3.7 dB) | ok (0.032%) |
| 56 | Soft Limiter |  | ok (-24.6 dB) | ok (0.011%) |
| 57 | Mute |  | ok (-3.6 dB) | NOT SHOWN (0.000%) |
| 58 | Stereo Link | random state 1 | CONDITIONAL (-0.1 dB) | NOT SHOWN (0.000%) |
| 59 | Oversampling |  | ok (-1.9 dB) | NOT SHOWN (0.000%) |
| 60 | Offline Oversampling |  | ok (-3.6 dB) | NOT SHOWN (0.000%) |
| 61 | HQ Mode (Offline) |  | ok (-3.6 dB) | ok (0.110%) |
| 62 | Bypass |  | ok (-2.1 dB) | excused (0.000%) |
| 63 | Bypass |  | ok (-2.1 dB) | excused (0.454%) |
| 64 | Wet |  | ok (-2.1 dB) | NOT SHOWN (0.003%) |
| 65 | Delta |  | ok (-3.5 dB) | ok (0.014%) |
