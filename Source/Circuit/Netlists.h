// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

// Linear stage netlists, transcribed from the reference circuit model (see docs/CIRCUIT_MODEL.md).
// Reference designators follow the model. Rails and grounds are AC ground. Ferrite beads (0.1 ohm) are
// shorted. Elements that only exist for supply decoupling are left out. The transcription is checked
// against the full reference subcircuits with ngspice by tools/spice_reference.py and tests/stage_test.cpp.
//
// Additions that are not in the reference circuit are marked "extension" and documented in
// docs/CIRCUIT_MODEL.md.

namespace bluechannel::circuit::netlists
{
// Balanced line receiver: unity-gain difference amplifier with a servo amplifier that balances the inputs.
inline constexpr const char* kLineInput = R"(
I in hot
R R318 0 cold_mid 5.1k
R R294 cold_mid nminus 5.1k
R R316 hot hot_mid 5.1k
R R295 hot_mid nplus 5.1k
R R293 nminus out 10k
R R317 vp out 10k
R R319 out auxminus 10k
R R320 auxminus auxout 10k
R R296 auxout nplus 10k
C C239 cold_mid ag 100p
C C237 hot_mid ag 100p
C C219 nminus out 100p keep
C C220 auxminus nplus 100p keep
O IC122A nplus nminus out
O IC122B 0 auxminus auxout
Y out out
)";

// Line gain switch: non-inverting amplifier whose input attenuator and feedback share one ladder.
// Param: gain (rotary position 0..6).
inline constexpr const char* kLineGain = R"(
I in in
R R150 in pi 5.1k
R R148 ni oa 5.1k
R R149 vp oa 10k
R R172 out 0 100k
R R142 tap0 tap1 910
R R143 tap1 tap2 3.3k
R R144 tap2 tap3 100k
R R145 tap3 tap4 100k
R R146 tap4 tap5 3.3k
R R147 tap5 tap6 910
R R114 rotor 0 750
S RS101_0 tap0 rotor gain 0
S RS101_1 tap1 rotor gain 1
S RS101_2 tap2 rotor gain 2
S RS101_3 tap3 rotor gain 3
S RS101_4 tap4 rotor gain 4
S RS101_5 tap5 rotor gain 5
S RS101_6 tap6 rotor gain 6
C C104 pi tap0 47u keep
C C103 ni tap6 47u keep
C C167 oa out 47u
O IC100A pi ni oa
Y out out
)";

// Microphone input: 1:5 input transformer (two primaries in parallel) into a non-inverting amplifier with a
// switched feedback ladder; the low-gain positions tap an attenuator on the secondary instead.
// Params: gain (0..11), rsrc (source resistance, ohm), zscale (secondary load scale), air (0/1).
// Input "core" is the non-linear part of the magnetising current (injected by the model, see Stages.h).
// Transformer: winding resistances from the part's data; magnetising and leakage inductance are estimates
// carried over from the reference model (5 H, coupling 0.9999).
// Extensions: source resistance, impedance scaling of the secondary ladder, and the air network.
inline constexpr const char* kMicInput = R"(
I in src
J core pcore
K RSRC src tfhot 1 rsrc
R RDCRP tfhot pl 22
L LLKP pl pcore 0.25m
L LMAG pcore 0 5
X XTF pcore 0 sx 0 5
L LLKS sx sl 12.5m
R RDCRS sl sec 880
C C106 sec snub 220p keep
R R141 snub 0 3k
S RAIR_ON sec airtop air 1
R RAIR airtop airl 15k
L LAIR airl 0 0.4
C C108 rotplus pi 100u
C C114 rotplus pi 100n
R R162 pi 0 220k
R R160 ni fbmid 6.2k
R R161 fbmid amp 6.2k
C C138 fbmid 0 100u
C C139 ni amp 220p
R R170 vp amp 10k
C C156 amp out 100u
R R171 out 0 100k
R R113 g6 fbcap 1k
C C107 fbcap amp 100u
R R106 g0 dcblock 56
C C100 dcblock 0 100u keep
R R107 g0 g1 56
R R108 g1 g2 120
R R109 g2 g3 270
R R110 g3 g4 560
R R111 g4 g5 1.2k
R R112 g5 g6 2.4k
S RS100_N0 g0 ni gain 0
S RS100_N1 g1 ni gain 1
S RS100_N2 g2 ni gain 2
S RS100_N3 g3 ni gain 3
S RS100_N4 g4 ni gain 4
S RS100_N5 g5 ni gain 5
S RS100_N6 g6 ni gain 6
S RS100_N7 g6 ni gain 7
S RS100_N8 g6 ni gain 8
S RS100_N9 g6 ni gain 9
S RS100_N10 g6 ni gain 10
S RS100_N11 g6 ni gain 11
K R100 sec z0 15k zscale
K R101 z0 z1 9.1k zscale
K R102 z1 z2 4.7k zscale
K R103 z2 z3 2.2k zscale
K R104 z3 z4 1k zscale
K R105 z4 0 1k zscale
S RS100_P0 sec rotplus gain 0
S RS100_P1 sec rotplus gain 1
S RS100_P2 sec rotplus gain 2
S RS100_P3 sec rotplus gain 3
S RS100_P4 sec rotplus gain 4
S RS100_P5 sec rotplus gain 5
S RS100_P6 sec rotplus gain 6
S RS100_P7 z0 rotplus gain 7
S RS100_P8 z1 rotplus gain 8
S RS100_P9 z2 rotplus gain 9
S RS100_P10 z3 rotplus gain 10
S RS100_P11 z4 rotplus gain 11
O IC103A pi ni amp
Y out out
Y primary pcore
)";

// Input trim (0 to +20 dB non-inverting amplifier) and the polarity inverter. Params: trim (0..1), phase.
inline constexpr const char* kTrim = R"(
I in in
R R115 potw 0 5.1k
Q RP100A1 0 potw 5k trim
P RP100A2 potw oa 5k trim
R R164 vp oa 10k
R R166 trimcap ni 4.7k
R R163 ni oa 47k
R R350 oa phaseinv 3.3k
R R351 phaseinv phaseout 3.3k
R R352 vp phaseout 10k
R R167 pi 0 220k
R R173 normal 0 100k
R R353 inverted 0 10k
C C158 potw trimcap 100u
C C159 in pi 100u
C C140 ni oa 220p keep
C C168 oa normal 100u
C C270 phaseout inverted 100u
O IC100B pi ni oa
O IC103B 0 phaseinv phaseout
S RL12N normal out phase 0
S RL12I inverted out phase 1
R RLOAD out 0 100k
Y out out
)";

// Third-order low-pass (RC + Sallen-Key) followed by a third-order high-pass, buffered. Every frequency
// determining resistor is the pot section plus 620 ohm, so each corner scales with 1 / (R + 620).
// Params: lpr, hpr (pot resistance, ohm). The gain links are not fitted (0 dB configuration).
inline constexpr const char* kFilters = R"(
I in in
R R165 in 0 10k
K RP102A in lpa 1 lpr
R R154 lpa lpf 620
R R176 lpo lpg 560
K RP102B lpo2 lpb 1 lpr
R R116 lpb lpc 620
R R117 lpc lpd 620
K RP102C lpd lpp 1 lpr
R R151 lpm lpout 560
R R168 hpf hpa 620
K RP101A hpa 0 1 hpr
R R174 hpo hpg 560
K RP101B hpb hpb2 1 hpr
R R202 hpb2 out 620
R R203 hpp hpc 620
K RP101C hpc 0 1 hpr
R R199 hpm out 560
C C111 lpf 0 5.6n
C C105 lpc lpout 5.6n
C C110 lpp 0 5.6n
C C115 lpout hpf 330n
C C169 hpo2 hpb 330n
C C171 hpb hpp 330n
O IC102A lpf lpo lpo
O IC102B lpg lpo2 lpo2
O IC101A lpp lpm lpout
O IC101B hpf hpo hpo
O IC105A hpg hpo2 hpo2
O IC105B hpp hpm out
Y out out
)";

// Low and high shelving equaliser (boost/cut pots across switched capacitor networks).
// Params: lfgain, hfgain (pot fraction 0..1), lfsel (0..7), hfsel (0..7).
// LF positions 0..5 are 33, 56, 95, 160, 270, 460 Hz; HF positions 0..5 are 3.3k, 4.7k, 6.8k, 10k, 15k, 18k.
// Extension: positions 6 and 7 add 20 Hz / 655 Hz (LF) and 1.5 kHz / 2.2 kHz (HF) with capacitors
// extrapolated so their half-boost frequencies keep the ratio of the neighbouring legends (measured by
// tests/calibrate.cpp: 20 Hz uses 2.52 uF, 655 Hz 13 nF, 1.5 kHz 150/330 nF, 2.2 kHz 102/225 nF).
inline constexpr const char* kShelves = R"(
.option short=1
I in in
R R137 lfin 0 10k
R R139 lfin lfa 1k
R R138 lfa lfb 2.4k
P RP109A1 lfb lfw 22k lfgain
Q RP109A2 lfw lfc 22k lfgain
R R140 lfc lfsum 2.4k
R R155 lfsum lfdec 1k
R R233 lfminus lfref 100k
R R232 lfref lfo 100k
R R183 lfground 0 43k
R R252 vp lfo 10k
R R234 hfin 0 4.7k
R R254 hfin hfm 10k
R R255 hfm hfamp 10k
R R253 vp hfamp 10k
R R157 hfin hfa 820
P RP110A1 hfa hfw 22k hfgain
Q RP110A2 hfw hfb 22k hfgain
R R156 hfb hfamp 820
R R159 hfm hfref 9.1k
R R158 hfref hfground 2.4k
R R223 out 0 4.7k
C C136 in lfin 100u
C C102 lfb lfc 10n
C C137 lfw lfground 10n
C C197 lfdec lfo 100u keep
C C195 lfref 0 100u
C C200 lfminus lfo 220p keep
C C198 lfw lfminus 100u
C C199 lfo hfin 100u
C C191 hfground 0 100u
C C206 hfm hfamp 220p keep
C C205 hfamp out 100u
O IC112A 0 lfminus lfo
O IC112B 0 hfm hfamp
S LF0A lfs0a lfground lfsel 0
C C145 lfa lfs0a 1u
C C175 lfa lfs0a 470n
R RRP100A lfa lfs0a 1meg
S LF0B lfs0b lfground lfsel 0
C C148 lfsum lfs0b 1u
C C179 lfsum lfs0b 470n
R RRP101C lfsum lfs0b 1meg
S LF1A lfs1a lfground lfsel 1
C C146 lfa lfs1a 680n
R RRP100B lfa lfs1a 1meg
S LF1B lfs1b lfground lfsel 1
C C149 lfsum lfs1b 680n
R RRP101D lfsum lfs1b 1meg
S LF2A lfs2a lfground lfsel 2
C C177 lfa lfs2a 330n
R RRP100C lfa lfs2a 1meg
S LF2B lfs2b lfground lfsel 2
C C188 lfsum lfs2b 330n
R RRP102A lfsum lfs2b 1meg
S LF3A lfs3a lfground lfsel 3
C C147 lfa lfs3a 150n
R RRP100D lfa lfs3a 1meg
S LF3B lfs3b lfground lfsel 3
C C150 lfsum lfs3b 150n
R RRP102B lfsum lfs3b 1meg
S LF4A lfs4a lfground lfsel 4
C C176 lfa lfs4a 68n
R RRP101A lfa lfs4a 1meg
S LF4B lfs4b lfground lfsel 4
C C180 lfsum lfs4b 68n
R RRP102C lfsum lfs4b 1meg
S LF5A lfs5a lfground lfsel 5
C C178 lfa lfs5a 15n
C C187 lfa lfs5a 15n
R RRP101B lfa lfs5a 1meg
S LF5B lfs5b lfground lfsel 5
C C189 lfsum lfs5b 15n
C C190 lfsum lfs5b 15n
R RRP102D lfsum lfs5b 1meg
S LF6A lfs6a lfground lfsel 6
C CX20A lfa lfs6a 2.52u keep
S LF6B lfs6b lfground lfsel 6
C CX20B lfsum lfs6b 2.52u keep
S LF7A lfs7a lfground lfsel 7
C CX655A lfa lfs7a 13n
S LF7B lfs7b lfground lfsel 7
C CX655B lfsum lfs7b 13n
S HF0A hfs0a hfref hfsel 0
C C151 hfw hfs0a 68n
R RRP103A hfw hfs0a 1meg
S HF0B hfs0b hfref hfsel 0
C C182 hfm hfs0b 150n
R RRP104C hfm hfs0b 1meg
S HF1A hfs1a hfref hfsel 1
C C155 hfw hfs1a 47n
R RRP103B hfw hfs1a 1meg
S HF1B hfs1b hfref hfsel 1
C C185 hfm hfs1b 100n
R RRP104D hfm hfs1b 1meg
S HF2A hfs2a hfref hfsel 2
C C181 hfw hfs2a 33n
R RRP103C hfw hfs2a 1meg
S HF2B hfs2b hfref hfsel 2
C C157 hfm hfs2b 68n
R RRP105A hfm hfs2b 1meg
S HF3A hfs3a hfref hfsel 3
C C152 hfw hfs3a 22n
R RRP103D hfw hfs3a 1meg
S HF3B hfs3b hfref hfsel 3
C C153 hfm hfs3b 47n
R RRP105B hfm hfs3b 1meg
S HF4A hfs4a hfref hfsel 4
C C156 hfw hfs4a 15n
R RRP104A hfw hfs4a 1meg
S HF4B hfs4b hfref hfsel 4
C C186 hfm hfs4b 33n
R RRP105C hfm hfs4b 1meg
S HF5A hfs5a hfref hfsel 5
C C184 hfw hfs5a 10n
R RRP104B hfw hfs5a 1meg
S HF5B hfs5b hfref hfsel 5
C C154 hfm hfs5b 10n
C C183 hfm hfs5b 15n
R RRP105D hfm hfs5b 1meg
S HF6A hfs6a hfref hfsel 6
C CX1K5A hfw hfs6a 150n
S HF6B hfs6b hfref hfsel 6
C CX1K5B hfm hfs6b 330n
S HF7A hfs7a hfref hfsel 7
C CX2K2A hfw hfs7a 102n
S HF7B hfs7b hfref hfsel 7
C CX2K2B hfm hfs7b 225n
Y out out
)";

// Parametric band (low-mid). Gyrator-tuned resonator with a two-section integrator; the x3 switch opens
// the integrator bypass contacts. Params: gain, q, freq (pot fractions 0..1), x3 (0/1).
inline constexpr const char* kLmf = R"(
.option short=1
I in in
R R120 ga2 0 5.1k
R R122 ga ga2 11k
R R121 qt 0 5.6k
R R119 qt qm 1.2k
R R118 qm qlow 1.2k
R R188 rminus reson 8.2k
R R187 drive reson 47k
R R189 vp reson 10k
R R216 qw qtail 1.8k
R R213 qtail 0 1k
R R212 minus qa 6.2k
R R215 qcap minus 15k
R R211 vp qa 10k
R R123 qout 0 5.6k
R R214 gdec minus 6.2k
R R132 gmid 0 5.1k
R R186 drive i2 18k
R R185 drive suma 18k
R R184 vp suma 10k
R R124 suma f1 10k
R R181 f1 ia 1.8k
R R180 ia ix1 1meg
R R179 vp i1 10k
R R206 i1 f2 10k
R R205 f2 ib 1.8k
R R207 ib ix2 1meg
R R204 vp i2 10k
R R125 f1bottom fg1 680
R R126 f2bottom fg2 680
C C119 in gdec 100u
C C120 ga2 drive 100u
C C118 rminus qm 100u keep
C C113 qt i1 100u
C C162 rminus reson 22p
C C112 reson qw 100u keep
C C116 i1 qcap 100u
C C192 minus qa 22p
C C117 qa qout 100u
C C126 qout gmid 100u
C C128 gmid out 100u
C C143 i1 ia 68n
C C142 i1 ix1 150n
C C172 i2 ib 68n
C C173 i2 ix2 150n
C C121 suma f1top 100u keep
C C122 i1 f2top 100u keep
C C123 fg1 0 100u keep
C C124 fg2 0 100u keep
P RGAIN1 in ga 22k gain
Q RGAIN2 ga qout 22k gain
P RQ qlow qw 5k q
P RFREQ1T f1top f1 10k freq
Q RFREQ1B f1 f1bottom 10k freq
P RFREQ2T f2top f2 10k freq
Q RFREQ2B f2 f2bottom 10k freq
S RX3A ix1 ia x3 0
S RX3B ix2 ib x3 0
O IC107A 0 rminus reson
O IC107B qtail minus qa
O IC106A 0 drive suma
O IC104A 0 ia i1
O IC104B 0 ib i2
R RLOAD out 0 100k
Y out out
)";

// Parametric band (high-mid). Same topology as the low-mid band with smaller integrator capacitors.
inline constexpr const char* kHmf = R"(
.option short=1
I in in
R R129 ga2 0 5.1k
R R131 ga ga2 11k
R R130 qt 0 5.6k
R R128 qt qm 1.2k
R R127 qm qlow 1.2k
R R191 rminus reson 8.2k
R R190 drive reson 47k
R R192 vp reson 10k
R R222 qw qtail 1.8k
R R221 qtail 0 1k
R R219 minus qa 6.2k
R R218 qcap minus 15k
R R217 vp qa 10k
R R182 out 0 10k
R R220 gdec minus 6.2k
R R133 in 0 5.6k
R R210 drive i2 18k
R R209 drive suma 18k
R R208 vp suma 10k
R R134 suma f1 10k
R R194 f1 ia 1.8k
R R195 ia ix1 1meg
R R193 vp i1 10k
R R196 i1 f2 10k
R R226 f2 ib 1.8k
R R225 ib ix2 1meg
R R224 vp i2 10k
R R135 f1bottom fg1 680
R R136 f2bottom fg2 680
C C130 in gdec 100u
C C131 ga2 drive 100u
C C161 rminus qm 100u keep
C C129 qt i1 100u
C C163 rminus reson 22p
C C125 reson qw 100u keep
C C127 i1 qcap 100u
C C193 minus qa 22p
C C144 qa out 100u
C C164 i1 ix1 10n
C C165 i1 ia 4.7n
C C174 i2 ix2 10n
C C194 i2 ib 4.7n
C C132 suma f1top 100u keep
C C133 i1 f2top 100u keep
C C134 fg1 0 100u keep
C C135 fg2 0 100u keep
P RGAIN1 in ga 22k gain
Q RGAIN2 ga out 22k gain
P RQ qlow qw 5k q
P RFREQ1T f1top f1 10k freq
Q RFREQ1B f1 f1bottom 10k freq
P RFREQ2T f2top f2 10k freq
Q RFREQ2B f2 f2bottom 10k freq
S RX3A ix1 ia x3 0
S RX3B ix2 ib x3 0
O IC108A 0 rminus reson
O IC108B qtail minus qa
O IC106B 0 drive suma
O IC109A 0 ia i1
O IC109B 0 ib i2
Y out out
)";

// Compressor make-up amplifier (0 to +20 dB) after the gain cell. Param: makeup (pot fraction; 1 = 0 dB).
inline constexpr const char* kMakeup = R"(
I in in
R R120 in 0 47k
R R125 minus mac 4.7k
R R118 mw 0 5.1k
R R116 minus out 47k
C C109 mw mac 100u
C C106 minus out 220p
P RP100BCW 0 mw 5k makeup
Q RP100BCCW mw out 5k makeup
O IC101B in minus out
R RLOAD out 0 100k
Y out out
)";

// De-esser audio network: tuned band-pass (three-gang frequency pot, positive feedback for Q) and its
// unity inverter. The optical attenuator and the summing amplifiers are evaluated by the model.
// Param: freq (pot fraction 0..1).
// Correction: the frequency pot is 47k per gang. The transcribed 20k gives 3.6 to 9.1 kHz; 47k reproduces the
// 2.2 to 9.2 kHz range of the reference unit's documentation, so the transcription value is taken as a misread.
inline constexpr const char* kDeesser = R"(
I in in
C C125 in fin 2.7n
R R138 fin fa 6.2k
R R183 fb fcb 1.3k
C C142 fcb fout 2.7n
R R185 fc fout 6.2k
C C140 fminus fc 680p
C C229 fminus fout 22p
R R186 fplus fout 10k
R R187 fplus 0 6.2k
P RP110A fa fminus 47k freq
P RP110B fminus fb 47k freq
P RP110C fminus fc 47k freq
O IC114A fplus fminus fout
R RLOAD fout 0 10.2k
Y band fout
)";

// Soft-limiter crossover: complementary three-band split (two first-order high-passes). The optical band
// attenuators and the summing amplifier are evaluated by the model.
inline constexpr const char* kLimiterSplit = R"(
I in in
C C180 in ica 47u
C C184 in icb 68n
C C188 in icc 15n
R R272 ica iln 5.1k
R R273 ihigh iln 5.1k
R R274 imid iln 5.1k
R R275 iln ilow 5.1k
R R285 icb imn 5.1k
R R286 ihigh imn 5.1k
R R287 imn imid 5.1k
R R292 icc ihn 5.1k
R R291 ihn ihigh 5.1k
O IC139A 0 iln ilow
O IC140A 0 imn imid
O IC140B 0 ihn ihigh
R RLL ilow 0 10.2k
R RLM imid 0 10.2k
R RLH ihigh 0 10.2k
Y low ilow
Y mid imid
Y high ihigh
)";

// Output level control: buffer, loaded fader track and a x2.19 non-inverting amplifier.
// Param: fader (track fraction 0..1, 1 = fully up).
inline constexpr const char* kFader = R"(
I in master
R R369 master 0 10k
O IC126A master mb mb
Q RP111BTOP mb mout 10k fader
P RP111BBOT mout 0 10k fader
R R190 mout 0 3.6k
R R189 mn 0 4.7k
R R188 mn fader 5.6k
C C143 mn fader 220p keep
O IC109B mout mn fader
R RLOAD fader 0 20k
Y out fader
)";

// Transformer-coupled output: follower, then an amplifier whose complementary emitter followers drive the
// output transformer primary; feedback is taken from the tertiary winding. The emitter followers are
// represented by an ideal follower inside the loop. Transformer: ratios 1 : 2 : 1, winding resistances and
// magnetising inductance as in the reference model (estimates). The cold leg returns through 10 ohm.
// C212 and C214 are kept: they are what stops DC from entering the loop that closes through the transformer.
inline constexpr const char* kOutput = R"(
I in in
J core pcore
R R266 in 0 100k
R R280 in 0 10k
O IC118B in bm bo
R R357 bm bo 1.2k
C C212 bo pi 100u keep
R R276 pi 0 4.7k
O IC118A pi ni center
O BUF center drv drv
R REMIT drv primary 0.11
R R275 nfb ni 1.2k
R R267 nfb fb 240
R R268 fb tertac 1.2k
C C211 nfb tertac 22n
C C214 tertac tert 100u keep
R RPRI primary pcore 20
L LMAG pcore 0 2
X XSEC pcore 0 sx cold 2
R RSEC sx hot 80
X XTER pcore 0 tx 0 1
R RTER tx tert 20
R R340 cold 0 10
R RLOAD hot cold 100k
Y hot hot
Y cold cold
)";
} // namespace bluechannel::circuit::netlists
