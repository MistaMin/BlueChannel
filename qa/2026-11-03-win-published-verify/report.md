# Plugin QA: VST3: Blue Channel

| Check | Status | Detail |
|---|---|---|
| loads | PASS | VST3: Blue Channel (OpenGrid), 67 parameters |
| finite output | PASS | peak -4.0 dBFS |
| not silent | PASS | rms -15.0 dBFS |
| deterministic render | PASS | repeat differs by -240.0 dB |
| editor opens | PASS | 1263x423, 0.0% animates by itself |
| every parameter changes the sound | PASS |  |
| every parameter is shown in the GUI | FAIL | Shelving EQ, HF Frequency, Hold, De-Ess Frequency, De-Ess Listen, Mute, Stereo Link, Oversampling, Offline Oversampling, HQ Mode (Offline), Wet, Delta |
| state survives save and reopen | PASS |  |
| editor reopens | PASS | 5 close/open cycles |

| # | Parameter | Context | Audio | Visual |
|---|---|---|---|---|
| 0 | Input |  | ok (-1.1 dB) | ok (0.945%) |
| 1 | Mic Gain |  | ok (1.7 dB) | ok (0.071%) |
| 2 | Line Gain | Input=0.5 | ok (-0.5 dB) | ok (0.153%) |
| 3 | Trim |  | ok (0.8 dB) | ok (0.057%) |
| 4 | Phase |  | ok (2.1 dB) | ok (0.144%) |
| 5 | Impedance |  | ok (-20.5 dB) | ok (0.091%) |
| 6 | Mic Air |  | ok (-17.3 dB) | ok (0.127%) |
| 7 | All EQ | random state 2 | CONDITIONAL (6.0 dB) | ok (0.121%) |
| 8 | Filters | random state 1 | CONDITIONAL (-14.8 dB) | ok (0.127%) |
| 9 | High Pass | random state 1 | CONDITIONAL (-14.8 dB) | ok (0.053%) |
| 10 | Low Pass | random state 1 | CONDITIONAL (-14.8 dB) | ok (0.064%) |
| 11 | Shelving EQ | random state 1 | CONDITIONAL (-14.8 dB) | NOT SHOWN (0.000%) |
| 12 | LF Gain | random state 1 | CONDITIONAL (1.4 dB) | ok (0.005%) |
| 13 | LF Frequency | random state 1 | CONDITIONAL (-2.3 dB) | ok (0.020%) |
| 14 | LF Hi Range | random state 1 | CONDITIONAL (-6.3 dB) | ok (0.020%) |
| 15 | HF Gain | random state 1 | CONDITIONAL (2.3 dB) | ok (0.007%) |
| 16 | HF Frequency | random state 1 | CONDITIONAL (-3.5 dB) | NOT SHOWN (0.000%) |
| 17 | HF Hi Range | random state 1 | CONDITIONAL (-3.3 dB) | ok (0.005%) |
| 18 | Parametric EQ | random state 1 | CONDITIONAL (-3.4 dB) | ok (0.123%) |
| 19 | LMF Gain | random state 1 | CONDITIONAL (-3.4 dB) | ok (0.065%) |
| 20 | LMF Frequency | random state 1 | CONDITIONAL (-3.4 dB) | ok (0.042%) |
| 21 | LMF Q | random state 1 | CONDITIONAL (-3.4 dB) | ok (0.050%) |
| 22 | LMF x3 | random state 1 | CONDITIONAL (-3.4 dB) | ok (0.171%) |
| 23 | HMF Gain | random state 1 | CONDITIONAL (-3.4 dB) | ok (0.066%) |
| 24 | HMF Frequency | random state 1 | CONDITIONAL (-3.4 dB) | ok (0.040%) |
| 25 | HMF Q | random state 1 | CONDITIONAL (-3.4 dB) | ok (0.007%) |
| 26 | HMF x3 | random state 1 | CONDITIONAL (-3.4 dB) | ok (0.154%) |
| 27 | Comp In |  | ok (-10.0 dB) | ok (0.129%) |
| 28 | Comp Type | random state 1 | CONDITIONAL (-3.4 dB) | ok (0.076%) |
| 29 | Ratio | random state 1 | CONDITIONAL (-3.4 dB) | ok (0.007%) |
| 30 | Comp Threshold | random state 1 | CONDITIONAL (-3.4 dB) | ok (0.009%) |
| 31 | Attack | random state 1 | CONDITIONAL (-3.4 dB) | ok (0.023%) |
| 32 | Release | random state 1 | CONDITIONAL (-3.4 dB) | ok (0.007%) |
| 33 | Auto Release | random state 1 | CONDITIONAL (-3.4 dB) | ok (0.124%) |
| 34 | Makeup | random state 1 | CONDITIONAL (-3.4 dB) | ok (0.016%) |
| 35 | Comp Ext Key | random state 1 | CONDITIONAL (-3.4 dB) | ok (0.135%) |
| 36 | Blend | random state 1 | CONDITIONAL (-3.4 dB) | ok (0.133%) |
| 37 | Mix | random state 1 | CONDITIONAL (-3.4 dB) | ok (0.007%) |
| 38 | Comp Listen | random state 1 | CONDITIONAL (-3.4 dB) | ok (0.151%) |
| 39 | Gate In |  | ok (-25.9 dB) | ok (0.139%) |
| 40 | Expand | random state 1 | CONDITIONAL (-4.3 dB) | ok (0.151%) |
| 41 | Gate Threshold | random state 1 | CONDITIONAL (-4.3 dB) | ok (0.007%) |
| 42 | Range | random state 1 | CONDITIONAL (-4.3 dB) | ok (0.030%) |
| 43 | Hold | random state 1 | CONDITIONAL (-4.3 dB) | NOT SHOWN (0.000%) |
| 44 | Gate Release | random state 1 | CONDITIONAL (-4.3 dB) | ok (0.031%) |
| 45 | Fast Attack | random state 1 | CONDITIONAL (-4.3 dB) | ok (0.120%) |
| 46 | Hysteresis | random state 1 | CONDITIONAL (-4.3 dB) | ok (0.123%) |
| 47 | Gate Ext Key | random state 1 | CONDITIONAL (-4.3 dB) | ok (0.122%) |
| 48 | Gate Listen | random state 1 | CONDITIONAL (-4.3 dB) | ok (0.124%) |
| 49 | De-Ess In |  | ok (-13.6 dB) | ok (0.025%) |
| 50 | De-Ess Threshold | random state 1 | CONDITIONAL (-4.3 dB) | ok (0.020%) |
| 51 | De-Ess Frequency | random state 1 | CONDITIONAL (-4.3 dB) | NOT SHOWN (0.000%) |
| 52 | De-Ess Listen |  | ok (-0.2 dB) | NOT SHOWN (0.000%) |
| 53 | Dynamics Position | random state 1 | CONDITIONAL (-4.3 dB) | ok (0.020%) |
| 54 | Bypass | random state 1 | CONDITIONAL (-4.3 dB) | excused (0.000%) |
| 55 | Output |  | ok (-4.0 dB) | ok (0.025%) |
| 56 | Soft Limiter |  | ok (-24.7 dB) | ok (0.032%) |
| 57 | Mute |  | ok (-4.0 dB) | NOT SHOWN (0.000%) |
| 58 | Stereo Link | random state 1 | CONDITIONAL (-4.3 dB) | NOT SHOWN (0.000%) |
| 59 | Oversampling |  | ok (-2.1 dB) | NOT SHOWN (0.000%) |
| 60 | Offline Oversampling |  | ok (-4.4 dB) | NOT SHOWN (0.000%) |
| 61 | HQ Mode (Offline) |  | ok (-4.0 dB) | NOT SHOWN (0.000%) |
| 62 | Bypass |  | ok (-2.0 dB) | excused (0.000%) |
| 63 | Program |  | ok (-11.7 dB) | ok (0.337%) |
| 64 | Bypass |  | ok (-2.0 dB) | excused (0.603%) |
| 65 | Wet |  | ok (-2.0 dB) | NOT SHOWN (0.000%) |
| 66 | Delta |  | ok (-3.2 dB) | NOT SHOWN (0.000%) |
