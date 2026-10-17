// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
// Parameterised component audio model. Original reference documents are not distributed.
#pragma once
namespace bluechannel::hq {
inline constexpr const char* circuitHash = "fba0647bbe600e3144bb41d06e8168176175d469d7445a18efe60a95ee8641f9";
inline constexpr const char* circuit = R"HQ(.model device0 NPN(IS=2.25e-14 BF=400 VAF=100 IKF=.1 RB=40 RC=1 RE=.4 CJE=8p CJC=7.22p VJC=.75 MJC=.33 TF=1n TR=30n)
.model device1 PNP(IS=2.25e-14 BF=400 VAF=100 IKF=.1 RB=40 RC=1 RE=.4 CJE=8p CJC=9.62p VJC=.75 MJC=.33 TF=.95n TR=30n)
.model device2 NPN(IS=2e-14 BF=1500 VAF=100 IKF=.15 RB=30 RC=.7 RE=.3 CJE=10p CJC=8.42p VJC=.75 MJC=.33 TF=.55n TR=50n)
.model device3 NPN(IS=1e-13 BF=220 VAF=80 IKF=.8 RB=4 RC=.1 RE=.05 CJE=60p CJC=33.68p VJC=.75 MJC=.33 TF=1.5n TR=100n)
.model device4 PNP(IS=1e-13 BF=220 VAF=80 IKF=.8 RB=4 RC=.1 RE=.05 CJE=70p CJC=55.34p VJC=.75 MJC=.33 TF=1.5n TR=80n)
.model device5 NPN(IS=6e-15 BF=700 VAF=150 IKF=.1 RB=20 RC=.5 RE=.4 CJE=5p CJC=4p VJC=.75 MJC=.33 TF=.22n TR=30n)
.model device6 PNP(IS=1e-13 BF=165 VAF=100 IKF=.05 RB=10 RC=.4 RE=.3 CJE=8p CJC=6p VJC=.75 MJC=.33 TF=.55n TR=30n)
.model device7 NJF(VTO=-2 BETA=.015 LAMBDA=.005 RD=.2 RS=.2 CGS=23p CGD=27p PB=1 IS=1p)
.model device8 NMOS(LEVEL=1 VTO=1.3 KP=1 LAMBDA=.01 RD=.08 RS=.2 IS=1e-12)
.model device9 D(IS=1e-18 N=2 RS=10 CJO=20p)
.model device10 D(Is=2.52n N=1.752 Rs=0.568 Cjo=4p M=0.4 Tt=20n Bv=100 Ibv=0.1u)
.model device11 D(Is=20n Rs=0.1 N=1.8 Tt=2u Bv=100 Ibv=5u)
.model device12 D(Is=20n Rs=0.1 N=1.8 Tt=2u Bv=1000 Ibv=5u)
.model device13 D(Is=1n N=1.5 Rs=5 Bv=8.2 Ibv=1m)
.model device14 D(Is=1n N=1.5 Rs=5 Bv=33 Ibv=1m)
.model device15 D(Is=1n N=1.5 Rs=5 Bv=3.3 Ibv=1m)
.model device16 D(Is=1e-20 N=2 Rs=10 Cjo=15p Bv=5)
.model device17 D(Is=1e-20 N=2 Rs=10 Cjo=15p Bv=5)
.subckt circuit2 p1 p2 p3 p4 s5 s6 shield
RP1 p1 pa 44
RP2 p4 pb 44
RS s5 sc 880
LP1 pa p2 {((5))}
LP2 pb p3 {((5))}
LS sc s6 {25*((5))}
K12 LP1 LP2 {(0.9999)}
K1S LP1 LS {(0.9999)}
K2S LP2 LS {(0.9999)}
BFLUX 0 flux I={V(pa,p2)}
CFLUX flux 0 1
RFLUX flux 0 0.3183098861837907
BCORE pa p2 I={V(flux)/((5))*pow(sqrt(V(flux)^2+1e-24)/0.0183,1.4)}
.ends
.subckt circuit3 p n vp vn out
RDIFF p n 300000
BGMAIN 0 pole I={0.0127323954474*tanh(0.1*V(p,n)/0.0127323954474)}
RPOLE pole 0 1000000
CPOLE pole 0 1.59154943092e-09
BANTIWIND pole 0 I={(.5*((V(pole)-V(vp)-1)+sqrt((V(pole)-V(vp)-1)^2+1e-12)))-(.5*((V(vn)-1-V(pole))+sqrt((V(vn)-1-V(pole))^2+1e-12)))}
BCMD cmd 0 V={((((((V(vp))-(.5*(((V(vp))-(V(vn)+2.0))+sqrt(((V(vp))-(V(vn)+2.0))^2+1e-12)))))+(.5*(((V(pole))-(((V(vp))-(.5*(((V(vp))-(V(vn)+2.0))+sqrt(((V(vp))-(V(vn)+2.0))^2+1e-12))))))+sqrt(((V(pole))-(((V(vp))-(.5*(((V(vp))-(V(vn)+2.0))+sqrt(((V(vp))-(V(vn)+2.0))^2+1e-12))))))^2+1e-12)))))-(.5*(((((((V(vp))-(.5*(((V(vp))-(V(vn)+2.0))+sqrt(((V(vp))-(V(vn)+2.0))^2+1e-12)))))+(.5*(((V(pole))-(((V(vp))-(.5*(((V(vp))-(V(vn)+2.0))+sqrt(((V(vp))-(V(vn)+2.0))^2+1e-12))))))+sqrt(((V(pole))-(((V(vp))-(.5*(((V(vp))-(V(vn)+2.0))+sqrt(((V(vp))-(V(vn)+2.0))^2+1e-12))))))^2+1e-12)))))-(((V(vp)-2.0)+(.5*(((V(vn))-(V(vp)-2.0))+sqrt(((V(vn))-(V(vp)-2.0))^2+1e-12))))))+sqrt(((((((V(vp))-(.5*(((V(vp))-(V(vn)+2.0))+sqrt(((V(vp))-(V(vn)+2.0))^2+1e-12)))))+(.5*(((V(pole))-(((V(vp))-(.5*(((V(vp))-(V(vn)+2.0))+sqrt(((V(vp))-(V(vn)+2.0))^2+1e-12))))))+sqrt(((V(pole))-(((V(vp))-(.5*(((V(vp))-(V(vn)+2.0))+sqrt(((V(vp))-(V(vn)+2.0))^2+1e-12))))))^2+1e-12)))))-(((V(vp)-2.0)+(.5*(((V(vn))-(V(vp)-2.0))+sqrt(((V(vn))-(V(vp)-2.0))^2+1e-12))))))^2+1e-12))))}
BOUTPUT 0 sensed I={(-0.038+(.5*((((V(cmd)-V(sensed))/50.0)+0.038)+sqrt((((V(cmd)-V(sensed))/50.0)+0.038)^2+1e-12)))-(.5*((((V(cmd)-V(sensed))/50.0)-0.038)+sqrt((((V(cmd)-V(sensed))/50.0)-0.038)^2+1e-12))))}
RSENSED sensed 0 10Meg
VSENSE sensed out 0
BIQUIESC vp vn I={0.0045*min(1,max(0,V(vp,vn)/5))}
BLOADPOS vp 0 I={.5*(I(VSENSE)+sqrt(I(VSENSE)^2+1e-24))}
BLOADNEG 0 vn I={.5*(-I(VSENSE)+sqrt(I(VSENSE)^2+1e-24))}
.ends
.subckt circuit1 hot cold out vp vn phantom ag ch
RF105 hot tfhot {(0.1)}
RF104 cold tfcold {(0.1)}
C249 hot ch 100p
C248 cold ch 100p
R197 tfhot ph 6.8k
R198 tfcold ph 6.8k
RRL8 phantom ph {(0) ? 1m : 1e15}
C106 sec snub 220p
R141 snub ag 3k
C108 rotplus pi 100u
C114 rotplus pi 100n
R162 pi ag 220k
R160 ni fbmid 6.2k
R161 fbmid amp 6.2k
C138 fbmid ag 100u
C139 ni amp 220p
R170 vp amp 10k
C156 amp out 100u
R171 out ag 100k
R113 g6 fbcap 1k
C107 fbcap amp 100u
R106 g0 dcblock 56
C100 dcblock ag 100u
XTF201 tfhot tfcold tfcold tfhot sec ag ch circuit2
R107 g0 g1 56
R108 g1 g2 120
R109 g2 g3 270
R110 g3 g4 560
R111 g4 g5 1.2k
R112 g5 g6 2.4k
RRS100_NEG0 g0 ni R={(floor(v(c_micgain)+0.5))==0 ? 1m : 1e15}
RRS100_NEG1 g1 ni R={(floor(v(c_micgain)+0.5))==1 ? 1m : 1e15}
RRS100_NEG2 g2 ni R={(floor(v(c_micgain)+0.5))==2 ? 1m : 1e15}
RRS100_NEG3 g3 ni R={(floor(v(c_micgain)+0.5))==3 ? 1m : 1e15}
RRS100_NEG4 g4 ni R={(floor(v(c_micgain)+0.5))==4 ? 1m : 1e15}
RRS100_NEG5 g5 ni R={(floor(v(c_micgain)+0.5))==5 ? 1m : 1e15}
RRS100_NEG6 g6 ni R={(floor(v(c_micgain)+0.5))==6 ? 1m : 1e15}
RRS100_NEG7 g6 ni R={(floor(v(c_micgain)+0.5))==7 ? 1m : 1e15}
RRS100_NEG8 g6 ni R={(floor(v(c_micgain)+0.5))==8 ? 1m : 1e15}
RRS100_NEG9 g6 ni R={(floor(v(c_micgain)+0.5))==9 ? 1m : 1e15}
RRS100_NEG10 g6 ni R={(floor(v(c_micgain)+0.5))==10 ? 1m : 1e15}
RRS100_NEG11 g6 ni R={(floor(v(c_micgain)+0.5))==11 ? 1m : 1e15}
R100 sec z0 R={15k*v(c_zscale)}
R101 z0 z1 R={9.1k*v(c_zscale)}
R102 z1 z2 R={4.7k*v(c_zscale)}
R103 z2 z3 R={2.2k*v(c_zscale)}
R104 z3 z4 R={1k*v(c_zscale)}
R105 z4 ag R={1k*v(c_zscale)}
RRS100_POS0 sec rotplus R={(floor(v(c_micgain)+0.5))==0 ? 1m : 1e15}
RRS100_POS1 sec rotplus R={(floor(v(c_micgain)+0.5))==1 ? 1m : 1e15}
RRS100_POS2 sec rotplus R={(floor(v(c_micgain)+0.5))==2 ? 1m : 1e15}
RRS100_POS3 sec rotplus R={(floor(v(c_micgain)+0.5))==3 ? 1m : 1e15}
RRS100_POS4 sec rotplus R={(floor(v(c_micgain)+0.5))==4 ? 1m : 1e15}
RRS100_POS5 sec rotplus R={(floor(v(c_micgain)+0.5))==5 ? 1m : 1e15}
RRS100_POS6 sec rotplus R={(floor(v(c_micgain)+0.5))==6 ? 1m : 1e15}
RRS100_POS7 z0 rotplus R={(floor(v(c_micgain)+0.5))==7 ? 1m : 1e15}
RRS100_POS8 z1 rotplus R={(floor(v(c_micgain)+0.5))==8 ? 1m : 1e15}
RRS100_POS9 z2 rotplus R={(floor(v(c_micgain)+0.5))==9 ? 1m : 1e15}
RRS100_POS10 z3 rotplus R={(floor(v(c_micgain)+0.5))==10 ? 1m : 1e15}
RRS100_POS11 z4 rotplus R={(floor(v(c_micgain)+0.5))==11 ? 1m : 1e15}
XIC103_A pi ni vp vn amp circuit3
RAIRSW sec airtop R={v(c_air)>0.5 ? 1m : 1e15}
RAIR airtop airl 15k
LAIR airl ag 0.4
.ends
.subckt circuit4 hot cold out vp vn ag ch
RF103 cold cold_rf {(0.1)}
RF102 hot hot_rf {(0.1)}
R318 cold_rf cold_mid 5.1k
R294 cold_mid nminus 5.1k
R316 hot_rf hot_mid 5.1k
R295 hot_mid nplus 5.1k
R293 nminus out 10k
R317 vp out 10k
R319 out auxminus 10k
R320 auxminus auxout 10k
R296 auxout nplus 10k
C241 cold_rf ch 330p
C239 cold_mid ag 100p
C238 hot_rf ch 330p
C237 hot_mid ag 100p
C219 nminus out 100p
C220 auxminus nplus 100p
XIC122_A nplus nminus vp vn out circuit3
XIC122_B ag auxminus vp vn auxout circuit3
.ends
.subckt circuit5 in out vp vn ag
R150 in pi 5.1k
R148 ni oa 5.1k
R149 vp oa 10k
R172 out ag 100k
R142 tap0 tap1 910
R143 tap1 tap2 3.3k
R144 tap2 tap3 100k
R145 tap3 tap4 100k
R146 tap4 tap5 3.3k
R147 tap5 tap6 910
R114 rotor ag 750
RRS101_0 tap0 rotor R={(floor(v(c_linegain)+0.5))==0 ? 1m : 1e15}
RRS101_1 tap1 rotor R={(floor(v(c_linegain)+0.5))==1 ? 1m : 1e15}
RRS101_2 tap2 rotor R={(floor(v(c_linegain)+0.5))==2 ? 1m : 1e15}
RRS101_3 tap3 rotor R={(floor(v(c_linegain)+0.5))==3 ? 1m : 1e15}
RRS101_4 tap4 rotor R={(floor(v(c_linegain)+0.5))==4 ? 1m : 1e15}
RRS101_5 tap5 rotor R={(floor(v(c_linegain)+0.5))==5 ? 1m : 1e15}
RRS101_6 tap6 rotor R={(floor(v(c_linegain)+0.5))==6 ? 1m : 1e15}
C104 pi tap0 47u
C103 ni tap6 47u
C167 oa out 47u
XIC100_A pi ni vp vn oa circuit3
.ends
.subckt circuit6 front rear out p30 ag ch
C201 selected coupling 220n
RFRONT_JACK front selected {(1) ? 1m : 1e15}
RREAR_JACK rear selected {(1) ? 1e15 : 1m}
R239 coupling gate 1k
C202 gate ag 220p
R238 gate bias 1Meg
R241 supply bias 33k
R235 bias ag 10k
C208 bias ag 100u
R237 rear ag 4.7k
C236 ag ch 100n
R257 p30 supply 220
C207 supply ag 100u
R240 supply drain 620
R236 source ag 2k
R243 emit source 1k
R242 source trimtop 120
RP100_B trimtop bypass R={max(1m,1000*(v(c_insttrim)))}
C101 bypass ag 47u
C203 emit out 47u
R256 out ag 100k
JQ100 drain gate source device7
Q101 emit drain supply device1
.ends
.subckt circuit7 in out vp vn ag
R115 potw ag 5.1k
RP100_A_1 ag potw R={max(1m,5000*(1-(v(c_trim))))}
RP100_A_2 potw oa R={max(1m,5000*(v(c_trim)))}
R164 vp oa 10k
R166 trimcap ni 4.7k
R163 ni oa 47k
R350 oa phaseinv 3.3k
R351 phaseinv phaseout 3.3k
R352 vp phaseout 10k
R167 pi ag 220k
R173 normal ag 100k
R353 inverted ag 10k
C158 potw trimcap 100u
C159 in pi 100u
C140 ni oa 220p
C168 oa normal 100u
C270 phaseout inverted 100u
XIC100_B pi ni vp vn oa circuit3
XIC103_B ag phaseinv vp vn phaseout circuit3
RRL12_NORMAL normal out R={(v(c_phase)) ? 1e15 : 1m}
RRL12_INVERTED inverted out R={(v(c_phase)) ? 1m : 1e15}
.ends
.subckt circuit8 in out vp vn ag
R165 in ag 10k
RP102_A in lpa R={(v(c_lpr))}
R154 lpa lpf 620
R176 lpo lpg 560
R178 lpg ag {(0) ? 1000 : 1e15}
R177 lpg ag {(0) ? 560 : 1e15}
RP102_B lpo2 lpb R={(v(c_lpr))}
R116 lpb lpc 620
R117 lpc lpd 620
RP102_C lpd lpp R={(v(c_lpr))}
R151 lpm lpout 560
R153 lpm ag {(0) ? 1000 : 1e15}
R152 lpm ag {(0) ? 560 : 1e15}
R168 hpf hpa 620
RP101_A hpa ag R={(v(c_hpr))}
R174 hpo hpg 560
R169 hpg ag {(0) ? 1000 : 1e15}
R175 hpg ag {(0) ? 560 : 1e15}
RP101_B hpb hpb2 R={(v(c_hpr))}
R202 hpb2 out 620
R203 hpp hpc 620
RP101_C hpc ag R={(v(c_hpr))}
R199 hpm out 560
R200 hpm ag {(0) ? 1000 : 1e15}
R201 hpm ag {(0) ? 560 : 1e15}
C111 lpf ag 5.6n
C105 lpc lpout 5.6n
C110 lpp ag 5.6n
C115 lpout hpf 330n
C169 hpo2 hpb 330n
C171 hpb hpp 330n
XIC102_A lpf lpo vp vn lpo circuit3
XIC102_B lpg lpo2 vp vn lpo2 circuit3
XIC101_A lpp lpm vp vn lpout circuit3
XIC101_B hpf hpo vp vn hpo circuit3
XIC105_A hpg hpo2 vp vn hpo2 circuit3
XIC105_B hpp hpm vp vn out circuit3
.ends
.subckt circuit9 in out vp vn ag
R137 lfin ag 10k
R139 lfin lfa 1k
R138 lfa lfb 2.4k
RP109_A_1 lfb lfw R={max(1m,22000*(v(c_lfpot)))}
RP109_A_2 lfw lfc R={max(1m,22000*(1-(v(c_lfpot))))}
R140 lfc lfsum 2.4k
R155 lfsum lfdec 1k
R233 lfminus lfref 100k
R232 lfref lfo 100k
R183 lfground ag 43k
R252 vp lfo 10k
R234 hfin ag 4.7k
R254 hfin hfm 10k
R255 hfm hfamp 10k
R253 vp hfamp 10k
R157 hfin hfa 820
RP110_A_1 hfa hfw R={max(1m,22000*(v(c_hfpot)))}
RP110_A_2 hfw hfb R={max(1m,22000*(1-(v(c_hfpot))))}
R156 hfb hfamp 820
R159 hfm hfref 9.1k
R158 hfref hfground 2.4k
R223 out ag 4.7k
C136 in lfin 100u
C102 lfb lfc 10n
C137 lfw lfground 10n
C197 lfdec lfo 100u
C195 lfref ag 100u
C200 lfminus lfo 220p
C198 lfw lfminus 100u
C199 lfo hfin 100u
C191 hfground ag 100u
C206 hfm hfamp 220p
C205 hfamp out 100u
XIC112_A ag lfminus vp vn lfo circuit3
XIC112_B ag hfm vp vn hfamp circuit3
RlfSW0_0 lfsw0_0 lfground R={(v(c_lfsel))==0 ? 1m : 1e15}
C145 lfa lfsw0_0 1u
RRP100_A lfa lfsw0_0 1Meg
C175 lfa lfsw0_0 470n
RlfSW0_1 lfsw0_1 lfground R={(v(c_lfsel))==0 ? 1m : 1e15}
C148 lfsum lfsw0_1 1u
RRP101_C lfsum lfsw0_1 1Meg
C179 lfsum lfsw0_1 470n
RlfSW1_0 lfsw1_0 lfground R={(v(c_lfsel))==1 ? 1m : 1e15}
C146 lfa lfsw1_0 680n
RRP100_B lfa lfsw1_0 1Meg
RlfSW1_1 lfsw1_1 lfground R={(v(c_lfsel))==1 ? 1m : 1e15}
C149 lfsum lfsw1_1 680n
RRP101_D lfsum lfsw1_1 1Meg
RlfSW2_0 lfsw2_0 lfground R={(v(c_lfsel))==2 ? 1m : 1e15}
C177 lfa lfsw2_0 330n
RRP100_C lfa lfsw2_0 1Meg
RlfSW2_1 lfsw2_1 lfground R={(v(c_lfsel))==2 ? 1m : 1e15}
C188 lfsum lfsw2_1 330n
RRP102_A lfsum lfsw2_1 1Meg
RlfSW3_0 lfsw3_0 lfground R={(v(c_lfsel))==3 ? 1m : 1e15}
C147 lfa lfsw3_0 150n
RRP100_D lfa lfsw3_0 1Meg
RlfSW3_1 lfsw3_1 lfground R={(v(c_lfsel))==3 ? 1m : 1e15}
C150 lfsum lfsw3_1 150n
RRP102_B lfsum lfsw3_1 1Meg
RlfSW4_0 lfsw4_0 lfground R={(v(c_lfsel))==4 ? 1m : 1e15}
C176 lfa lfsw4_0 68n
RRP101_A lfa lfsw4_0 1Meg
RlfSW4_1 lfsw4_1 lfground R={(v(c_lfsel))==4 ? 1m : 1e15}
C180 lfsum lfsw4_1 68n
RRP102_C lfsum lfsw4_1 1Meg
RlfSW5_0 lfsw5_0 lfground R={(v(c_lfsel))==5 ? 1m : 1e15}
C178 lfa lfsw5_0 15n
RRP101_B lfa lfsw5_0 1Meg
C187 lfa lfsw5_0 15n
RlfSW5_1 lfsw5_1 lfground R={(v(c_lfsel))==5 ? 1m : 1e15}
C189 lfsum lfsw5_1 15n
RRP102_D lfsum lfsw5_1 1Meg
C190 lfsum lfsw5_1 15n
RhfSW0_0 hfsw0_0 hfref R={(v(c_hfsel))==0 ? 1m : 1e15}
C151 hfw hfsw0_0 68n
RRP103_A hfw hfsw0_0 1Meg
RhfSW0_1 hfsw0_1 hfref R={(v(c_hfsel))==0 ? 1m : 1e15}
C182 hfm hfsw0_1 150n
RRP104_C hfm hfsw0_1 1Meg
RhfSW1_0 hfsw1_0 hfref R={(v(c_hfsel))==1 ? 1m : 1e15}
C155 hfw hfsw1_0 47n
RRP103_B hfw hfsw1_0 1Meg
RhfSW1_1 hfsw1_1 hfref R={(v(c_hfsel))==1 ? 1m : 1e15}
C185 hfm hfsw1_1 100n
RRP104_D hfm hfsw1_1 1Meg
RhfSW2_0 hfsw2_0 hfref R={(v(c_hfsel))==2 ? 1m : 1e15}
C181 hfw hfsw2_0 33n
RRP103_C hfw hfsw2_0 1Meg
RhfSW2_1 hfsw2_1 hfref R={(v(c_hfsel))==2 ? 1m : 1e15}
C157 hfm hfsw2_1 68n
RRP105_A hfm hfsw2_1 1Meg
RhfSW3_0 hfsw3_0 hfref R={(v(c_hfsel))==3 ? 1m : 1e15}
C152 hfw hfsw3_0 22n
RRP103_D hfw hfsw3_0 1Meg
RhfSW3_1 hfsw3_1 hfref R={(v(c_hfsel))==3 ? 1m : 1e15}
C153 hfm hfsw3_1 47n
RRP105_B hfm hfsw3_1 1Meg
RhfSW4_0 hfsw4_0 hfref R={(v(c_hfsel))==4 ? 1m : 1e15}
C156 hfw hfsw4_0 15n
RRP104_A hfw hfsw4_0 1Meg
RhfSW4_1 hfsw4_1 hfref R={(v(c_hfsel))==4 ? 1m : 1e15}
C186 hfm hfsw4_1 33n
RRP105_C hfm hfsw4_1 1Meg
RhfSW5_0 hfsw5_0 hfref R={(v(c_hfsel))==5 ? 1m : 1e15}
C184 hfw hfsw5_0 10n
RRP104_B hfw hfsw5_0 1Meg
RhfSW5_1 hfsw5_1 hfref R={(v(c_hfsel))==5 ? 1m : 1e15}
C154 hfm hfsw5_1 10n
RRP105_D hfm hfsw5_1 1Meg
C183 hfm hfsw5_1 15n
.ends
.subckt circuit10 in out vp vn ag
R120 ga2 ag 5.1k
R122 ga ga2 11k
R121 qt ag 5.6k
R119 qt qm 1.2k
R118 qm qlow 1.2k
R188 rminus reson 8.2k
R187 drive reson 47k
R189 vp reson 10k
R216 qw qtail 1.8k
R213 qtail ag 1k
R212 minus qa 6.2k
R215 qcap minus 15k
R211 vp qa 10k
R123 qout ag 5.6k
R214 gdec minus 6.2k
R132 gmid ag 5.1k
R186 drive i2 18k
R185 drive suma 18k
R184 vp suma 10k
R124 suma f1 10k
R181 f1 ia 1.8k
R180 ia ix1 1Meg
R179 vp i1 10k
R206 i1 f2 10k
R205 f2 ib 1.8k
R207 ib ix2 1Meg
R204 vp i2 10k
R125 f1bottom fg1 680
R126 f2bottom fg2 680
C119 in gdec 100u
C120 ga2 drive 100u
C118 rminus qm 100u
C113 qt i1 100u
C162 rminus reson 22p
C112 reson qw 100u
C116 i1 qcap 100u
C192 minus qa 22p
C117 qa qout 100u
C126 qout gmid 100u
C128 gmid out 100u
C143 i1 ia 68n
C142 i1 ix1 150n
C172 i2 ib 68n
C173 i2 ix2 150n
C121 suma f1top 100u
C122 i1 f2top 100u
C123 fg1 ag 100u
C124 fg2 ag 100u
RGAIN1 in ga R={max(1m,22000*(v(c_lmfgain)))}
RGAIN2 ga qout R={max(1m,22000*(1-(v(c_lmfgain))))}
RQ qlow qw R={max(1m,5000*(v(c_lmfq)))}
RFREQ1T f1top f1 R={max(1m,10000*(v(c_lmffreq)))}
RFREQ1B f1 f1bottom R={max(1m,10000*(1-(v(c_lmffreq))))}
RFREQ2T f2top f2 R={max(1m,10000*(v(c_lmffreq)))}
RFREQ2B f2 f2bottom R={max(1m,10000*(1-(v(c_lmffreq))))}
RX3A ix1 ia R={(v(c_lmfx3)) ? 1e15 : 1m}
RX3B ix2 ib R={(v(c_lmfx3)) ? 1e15 : 1m}
XIC107_A ag rminus vp vn reson circuit3
XIC107_B qtail minus vp vn qa circuit3
XIC106_A ag drive vp vn suma circuit3
XIC104_A ag ia vp vn i1 circuit3
XIC104_B ag ib vp vn i2 circuit3
.ends
.subckt circuit11 in out vp vn ag
R129 ga2 ag 5.1k
R131 ga ga2 11k
R130 qt ag 5.6k
R128 qt qm 1.2k
R127 qm qlow 1.2k
R191 rminus reson 8.2k
R190 drive reson 47k
R192 vp reson 10k
R222 qw qtail 1.8k
R221 qtail ag 1k
R219 minus qa 6.2k
R218 qcap minus 15k
R217 vp qa 10k
R182 out ag 10k
R220 gdec minus 6.2k
R133 in ag 5.6k
R210 drive i2 18k
R209 drive suma 18k
R208 vp suma 10k
R134 suma f1 10k
R194 f1 ia 1.8k
R195 ia ix1 1Meg
R193 vp i1 10k
R196 i1 f2 10k
R226 f2 ib 1.8k
R225 ib ix2 1Meg
R224 vp i2 10k
R135 f1bottom fg1 680
R136 f2bottom fg2 680
C130 in gdec 100u
C131 ga2 drive 100u
C161 rminus qm 100u
C129 qt i1 100u
C163 rminus reson 22p
C125 reson qw 100u
C127 i1 qcap 100u
C193 minus qa 22p
C144 qa out 100u
C164 i1 ix1 10n
C165 i1 ia 4.7n
C174 i2 ix2 10n
C194 i2 ib 4.7n
C132 suma f1top 100u
C133 i1 f2top 100u
C134 fg1 ag 100u
C135 fg2 ag 100u
RGAIN1 in ga R={max(1m,22000*(v(c_hmfgain)))}
RGAIN2 ga out R={max(1m,22000*(1-(v(c_hmfgain))))}
RQ qlow qw R={max(1m,5000*(v(c_hmfq)))}
RFREQ1T f1top f1 R={max(1m,10000*(v(c_hmffreq)))}
RFREQ1B f1 f1bottom R={max(1m,10000*(1-(v(c_hmffreq))))}
RFREQ2T f2top f2 R={max(1m,10000*(v(c_hmffreq)))}
RFREQ2B f2 f2bottom R={max(1m,10000*(1-(v(c_hmffreq))))}
RX3A ix1 ia R={(v(c_hmfx3)) ? 1e15 : 1m}
RX3B ix2 ib R={(v(c_hmfx3)) ? 1e15 : 1m}
XIC108_A ag rminus vp vn reson circuit3
XIC108_B qtail minus vp vn qa circuit3
XIC106_B ag drive vp vn suma circuit3
XIC109_A ag ia vp vn i1 circuit3
XIC109_B ag ib vp vn i2 circuit3
.ends
.subckt circuit13 p n vp vn out
RDIFF p n 1e+12
BGMAIN 0 pole I={0.0835563451232*tanh(1.5*V(p,n)/0.0835563451232)}
RPOLE pole 0 1000000
CPOLE pole 0 2.98415518297e-08
BANTIWIND pole 0 I={(.5*((V(pole)-V(vp)-1)+sqrt((V(pole)-V(vp)-1)^2+1e-12)))-(.5*((V(vn)-1-V(pole))+sqrt((V(vn)-1-V(pole))^2+1e-12)))}
BCMD cmd 0 V={((((((V(vp))-(.5*(((V(vp))-(V(vn)+1.5))+sqrt(((V(vp))-(V(vn)+1.5))^2+1e-12)))))+(.5*(((V(pole))-(((V(vp))-(.5*(((V(vp))-(V(vn)+1.5))+sqrt(((V(vp))-(V(vn)+1.5))^2+1e-12))))))+sqrt(((V(pole))-(((V(vp))-(.5*(((V(vp))-(V(vn)+1.5))+sqrt(((V(vp))-(V(vn)+1.5))^2+1e-12))))))^2+1e-12)))))-(.5*(((((((V(vp))-(.5*(((V(vp))-(V(vn)+1.5))+sqrt(((V(vp))-(V(vn)+1.5))^2+1e-12)))))+(.5*(((V(pole))-(((V(vp))-(.5*(((V(vp))-(V(vn)+1.5))+sqrt(((V(vp))-(V(vn)+1.5))^2+1e-12))))))+sqrt(((V(pole))-(((V(vp))-(.5*(((V(vp))-(V(vn)+1.5))+sqrt(((V(vp))-(V(vn)+1.5))^2+1e-12))))))^2+1e-12)))))-(((V(vp)-1.5)+(.5*(((V(vn))-(V(vp)-1.5))+sqrt(((V(vn))-(V(vp)-1.5))^2+1e-12))))))+sqrt(((((((V(vp))-(.5*(((V(vp))-(V(vn)+1.5))+sqrt(((V(vp))-(V(vn)+1.5))^2+1e-12)))))+(.5*(((V(pole))-(((V(vp))-(.5*(((V(vp))-(V(vn)+1.5))+sqrt(((V(vp))-(V(vn)+1.5))^2+1e-12))))))+sqrt(((V(pole))-(((V(vp))-(.5*(((V(vp))-(V(vn)+1.5))+sqrt(((V(vp))-(V(vn)+1.5))^2+1e-12))))))^2+1e-12)))))-(((V(vp)-1.5)+(.5*(((V(vn))-(V(vp)-1.5))+sqrt(((V(vn))-(V(vp)-1.5))^2+1e-12))))))^2+1e-12))))}
BOUTPUT 0 sensed I={(-0.037+(.5*((((V(cmd)-V(sensed))/70.0)+0.037)+sqrt((((V(cmd)-V(sensed))/70.0)+0.037)^2+1e-12)))-(.5*((((V(cmd)-V(sensed))/70.0)-0.03)+sqrt((((V(cmd)-V(sensed))/70.0)-0.03)^2+1e-12))))}
RSENSED sensed 0 10Meg
VSENSE sensed out 0
BIQUIESC vp vn I={0.0029*min(1,max(0,V(vp,vn)/5))}
BLOADPOS vp 0 I={.5*(I(VSENSE)+sqrt(I(VSENSE)^2+1e-24))}
BLOADNEG 0 vn I={.5*(-I(VSENSE)+sqrt(I(VSENSE)^2+1e-24))}
.ends
.subckt circuit12 dyn_in out sc_sink vp vn ag ch
R229 vref vn 4.7k
R157 dyn_in ag 100k
R154 input_ac inv_sum 4.7k
R158 inv_sum inv_out 4.7k
R155 inv_out vp 10k
R160 inv_ac ag 4.7k
R143 tail_ac tail_sense_a 2.21k
R142 tail_sense_a vn 2.21k
R156 tail_inv_ac tail_sense_b 2.21k
R159 tail_sense_b vn 2.21k
R193 offset_w vn 470k
R115 vca_base ag 22
R101 sum_a common_out 2k
R113 sum_b common_out 2k
R100 sum_a vca_a 2.21k
R114 sum_b vca_b 2.21k
R105 vp vca_a 10k
R106 vp vca_b 10k
R110 vca_a common_plus 5.1k
R111 common_plus vca_b 5.1k
R112 common_minus ag 2.2k
R104 vca_b diff_minus 4.7k
R107 vca_a diff_plus 4.7k
R108 diff_minus diff_out 4.7k
R109 diff_plus ag 4.7k
R103 vp diff_out 10k
R120 makeup_plus ag 47k
R125 makeup_ac makeup_minus 4.7k
R118 makeup_w ag 5.1k
R116 makeup_minus makeup_out 47k
R117 vp makeup_out 10k
R102 makeup_final ag 100k
R255 link_tip mix_minus 20k
R254 mix_minus mix_out 20k
R251 mix_out bend_base 1.6k
R247 bend_base ag 100
R248 vp bend_minus 4.99k
R246 bend_tail bend_drive 1k
R253 vp mirror_e_b 1k
R230 vp mirror_e_a 1k
R252 bend_out bend_ctrl 820
R249 bend_ctrl ag 22
R228 bend_sense vn 2.21k
R250 bend_out vca_base 820
C170 ag vref 100n
C172 vp ag 100n
C108 vn ag 100n
C144 dyn_in input_ac 100u
C145 inv_sum inv_out 22p
C156 inv_out inv_ac 100u
C146 inv_ac tail_inv_ac 47u
C113 dyn_in tail_ac 47u
C100 sum_a vca_a 330p
C104 sum_b vca_b 330p
C103 common_out common_minus 680p
C101 diff_minus diff_out 22p
C102 diff_plus ag 22p
C107 diff_out makeup_plus 100u
C109 makeup_w makeup_ac 100u
C106 makeup_minus makeup_out 220p
C105 makeup_out makeup_final 100u
C226 link_tip ag 22n
C168 mix_minus mix_out 220p
C166 bend_minus bend_drive 33p
C167 bend_cmp bend_out 220p
D141 ag vref_d1 device10
D142 vref_d1 vref_d2 device10
D143 vref_d2 vref device10
D112 tail_sense_a tail_base_a device10
D126 tail_sense_b tail_base_b device10
D135 link_tip sc_sink device10
XIC123_A ag inv_sum vp vn inv_out circuit3
XIC110_A vref tail_sense_a vp vn tail_base_a circuit3
XIC110_B vref tail_sense_b vp vn tail_base_b circuit3
Q105 tail_a tail_base_a tail_sense_a device2
Q104 tail_b tail_base_b tail_sense_b device2
RPR4_CW tail_sense_a offset_w {max(1m,200000*(0.5))}
RPR4_CCW offset_w tail_sense_b {max(1m,200000*(1-(0.5)))}
Q102_A sum_a vca_base tail_a vn device5
Q101_B sum_a vca_base tail_a vn device5
Q101_A ag ag tail_a vn device5
Q102_B ag ag tail_a vn device5
Q102_C ag ag tail_b vn device5
Q101_D ag ag tail_b vn device5
Q101_C sum_b vca_base tail_b vn device5
Q102_D sum_b vca_base tail_b vn device5
XIC100_A ag sum_a vp vn vca_a circuit3
XIC100_B ag sum_b vp vn vca_b circuit3
XIC104_A common_plus common_minus vp vn common_out circuit3
XIC101_A diff_plus diff_minus vp vn diff_out circuit3
RP100_B_CW ag makeup_w R={max(1m,5000*(v(c_makeup)))}
RP100_B_CCW makeup_w makeup_out R={max(1m,5000*(1-(v(c_makeup))))}
XIC101_B makeup_plus makeup_minus vp vn makeup_out circuit3
RRL30_A_3_4 makeup_plus out {(1) ? 1e15 : 1m}
RRL30_A_5_4 makeup_final out {(1) ? 1m : 1e15}
XIC129_A ag mix_minus vp vn mix_out circuit3
XIC127 ag bend_minus vp vn bend_drive circuit13
XIC128 ag bend_cmp vp vn bend_out circuit13
Q110_D bend_minus bend_base bend_tail vn device5
Q110_C bend_cmp ag bend_tail vn device5
Q110_A mirror_cb bend_ctrl bend_sink vn device5
Q110_B ag ag bend_sink vn device5
Q111_B bend_cmp mirror_cb mirror_e_b device6
Q111_A mirror_cb mirror_cb mirror_e_a device6
Q109 bend_sink bend_tail_base bend_sense device2
XIC123_B vref bend_sense vp vn bend_tail_base circuit3
RSC_OPEN sc_sink ag 1T
.ends
.subckt circuit15 p n vp vn out
RDIFF p n 300000
BGMAIN 0 pole I={0.00663145596216*tanh(0.1*V(p,n)/0.00663145596216)}
RPOLE pole 0 1000000
CPOLE pole 0 1.32629119243e-09
BANTIWIND pole 0 I={(.5*((V(pole)-V(vp)-1)+sqrt((V(pole)-V(vp)-1)^2+1e-12)))-(.5*((V(vn)-1-V(pole))+sqrt((V(vn)-1-V(pole))^2+1e-12)))}
BCMD cmd 0 V={((((((V(vp))-(.5*(((V(vp))-(V(vn)+2.0))+sqrt(((V(vp))-(V(vn)+2.0))^2+1e-12)))))+(.5*(((V(pole))-(((V(vp))-(.5*(((V(vp))-(V(vn)+2.0))+sqrt(((V(vp))-(V(vn)+2.0))^2+1e-12))))))+sqrt(((V(pole))-(((V(vp))-(.5*(((V(vp))-(V(vn)+2.0))+sqrt(((V(vp))-(V(vn)+2.0))^2+1e-12))))))^2+1e-12)))))-(.5*(((((((V(vp))-(.5*(((V(vp))-(V(vn)+2.0))+sqrt(((V(vp))-(V(vn)+2.0))^2+1e-12)))))+(.5*(((V(pole))-(((V(vp))-(.5*(((V(vp))-(V(vn)+2.0))+sqrt(((V(vp))-(V(vn)+2.0))^2+1e-12))))))+sqrt(((V(pole))-(((V(vp))-(.5*(((V(vp))-(V(vn)+2.0))+sqrt(((V(vp))-(V(vn)+2.0))^2+1e-12))))))^2+1e-12)))))-(((V(vp)-2.0)+(.5*(((V(vn))-(V(vp)-2.0))+sqrt(((V(vn))-(V(vp)-2.0))^2+1e-12))))))+sqrt(((((((V(vp))-(.5*(((V(vp))-(V(vn)+2.0))+sqrt(((V(vp))-(V(vn)+2.0))^2+1e-12)))))+(.5*(((V(pole))-(((V(vp))-(.5*(((V(vp))-(V(vn)+2.0))+sqrt(((V(vp))-(V(vn)+2.0))^2+1e-12))))))+sqrt(((V(pole))-(((V(vp))-(.5*(((V(vp))-(V(vn)+2.0))+sqrt(((V(vp))-(V(vn)+2.0))^2+1e-12))))))^2+1e-12)))))-(((V(vp)-2.0)+(.5*(((V(vn))-(V(vp)-2.0))+sqrt(((V(vn))-(V(vp)-2.0))^2+1e-12))))))^2+1e-12))))}
BOUTPUT 0 sensed I={(-0.038+(.5*((((V(cmd)-V(sensed))/50.0)+0.038)+sqrt((((V(cmd)-V(sensed))/50.0)+0.038)^2+1e-12)))-(.5*((((V(cmd)-V(sensed))/50.0)-0.038)+sqrt((((V(cmd)-V(sensed))/50.0)-0.038)^2+1e-12))))}
RSENSED sensed 0 10Meg
VSENSE sensed out 0
BIQUIESC vp vn I={0.003*min(1,max(0,V(vp,vn)/5))}
BLOADPOS vp 0 I={.5*(I(VSENSE)+sqrt(I(VSENSE)^2+1e-24))}
BLOADNEG 0 vn I={.5*(-I(VSENSE)+sqrt(I(VSENSE)^2+1e-24))}
.ends
.subckt circuit16 ldr1 ldr2 anode cathode
VSENSE anode led 0
DLED led cathode device16
BILLUM illum 0 V={max(0,I(VSENSE))/(1m)}
RDELAY illum state R={V(illum)>V(state) ? (2m)/1u : (20m)/1u}
CDELAY state 0 1u
RLDR ldr1 ldr2 R={(60)+((100Meg)-(60))/(1+((100Meg)-(60))/(500)*pow(max(1e-12,V(state)),0.9))}
.ends
.subckt circuit14 in out led_anode led_cathode vp vn ag
R244 in audio_sum 9.1k
R243 filter_out audio_sum 8.2k
R241 audio_sum audio_inv 9.1k
R240 vp audio_inv 10k
R245 audio_plus ag 3k
R367 audio_inv output_sum 9.1k
R368 output_sum output_amp 9.1k
C165 output_amp out 47u
R257 out ag 20k
C125 in freq_input 2.7n
R138 freq_input freq_a 6.2k
R183 freq_b freq_cb 1.3k
C142 freq_cb filter_out 2.7n
R185 freq_c filter_out 6.2k
C140 filter_minus freq_c 680p
C229 filter_minus filter_out 22p
R186 filter_plus filter_out 10k
R187 filter_plus ag 6.2k
R207 filter_out invert_sum 5.1k
R205 invert_sum invert_out 5.1k
R206 invert_out optical_audio 3.9k
R242 optical_audio audio_sum 4.3k
R261 vp opt_supply 2.2
R262 opt_supply led_anode 220
C176 opt_supply ag 47u
RP110_A freq_a filter_minus R={max(1m,47000*(v(c_deessfreq)))}
RP110_B filter_minus freq_b R={max(1m,47000*(v(c_deessfreq)))}
RP110_C filter_minus freq_c R={max(1m,47000*(v(c_deessfreq)))}
XIC114_A filter_plus filter_minus vp vn filter_out circuit15
XIC114_B ag invert_sum vp vn invert_out circuit15
XIC125_A audio_plus audio_sum vp vn audio_inv circuit3
XIC125_B ag output_sum vp vn output_amp circuit3
XOPT2 optical_audio ag led_anode led_cathode circuit16
.ends
.subckt circuit17 master fader vp vn ag
R369 master ag 10k
RP111_B_TOP mb mout R={max(1m,10000*(1-(v(c_fader))))}
RP111_B_BOT mout ag R={max(1m,10000*(v(c_fader)))}
R190 mout ag 3.6k
R189 mn ag 4.7k
R188 mn fader 5.6k
C143 mn fader 220p
XIC126_A master mb vp vn mb circuit3
XIC109_B mout mn vp vn fader circuit3
.ends
.subckt circuit19 ldr1 ldr2 anode cathode
VSENSE anode led 0
DLED led cathode device17
BILLUM illum 0 V={max(0,I(VSENSE))/(1m)}
BDELAY 0 state I={(V(illum)-V(state))*1u/(V(illum)>V(state) ? (2m) : (20m))}
CDELAY state 0 1u
RLEAK state 0 1e12
RLDR ldr1 ldr2 R={(60)+((100Meg)-(60))/(1+pow(max(1e-12,V(state))*100000,1.08))}
.ends
.subckt circuit18 internal_in internal_out external_in external_out led_supply led_low_k led_mid_k led_high_k vp vn ag
C180 internal_in ica 47u
C184 internal_in icb 68n
C188 internal_in icc 15n
R272 ica iln 5.1k
R273 ihigh iln 5.1k
R274 imid iln 5.1k
R275 iln ilow 5.1k
R285 icb imn 5.1k
R286 ihigh imn 5.1k
R287 imn imid 5.1k
R292 icc ihn 5.1k
R291 ihn ihigh 5.1k
XIC139_A ag iln vp vn ilow circuit3
XIC140_A ag imn vp vn imid circuit3
XIC140_B ag ihn vp vn ihigh circuit3
C181 ilow ilow_ac 47u
R276 ilow_ac ilow_att 5.1k
R283 ilow_att isum 5.1k
XOPT3 ilow_att ag led_supply led_low_link circuit19
C186 imid imid_ac 47u
R288 imid_ac imid_att 5.1k
R290 imid_att isum 5.1k
XOPT4 imid_att ag led_supply led_mid_link circuit19
C189 ihigh ihigh_ac 47u
R294 ihigh_ac ihigh_att 5.1k
R293 ihigh_att isum 5.1k
XOPT5 ihigh_att ag led_supply led_high_link circuit19
R281 isum internal_out 10k
R282 vp internal_out 10k
XIC139_B ag isum vp vn internal_out circuit3
C192 external_in eca 47u
C197 external_in ecb 68n
C204 external_in ecc 15n
R298 eca eln 5.1k
R299 ehigh eln 5.1k
R300 emid eln 5.1k
R321 eln elow 5.1k
R322 ecb emn 5.1k
R323 ehigh emn 5.1k
R324 emn emid 5.1k
R341 ecc ehn 5.1k
R340 ehn ehigh 5.1k
XIC143_A ag eln vp vn elow circuit3
XIC148_A ag emn vp vn emid circuit3
XIC148_B ag ehn vp vn ehigh circuit3
C193 elow elow_ac 47u
R302 elow_ac elow_att 5.1k
R311 elow_att esum 5.1k
XOPT6 elow_att ag led_low_link led_low_k circuit19
C202 emid emid_ac 47u
R325 emid_ac emid_att 5.1k
R333 emid_att esum 5.1k
XOPT8 emid_att ag led_mid_link led_mid_k circuit19
C208 ehigh ehigh_ac 47u
R343 ehigh_ac ehigh_att 5.1k
R342 ehigh_att esum 5.1k
XOPT7 ehigh_att ag led_high_link led_high_k circuit19
R309 esum external_out 10k
R310 vp external_out 10k
XIC143_B ag esum vp vn external_out circuit3
.ends
.subckt circuit21 pr pn sp sn tp tn
RPRI pr p  {(20)}
RSEC sp s  {(80)}
RTER tp t  {(20)}
LPRI p pn {((2))}
LSEC s sn {((2))*((2))*((2))}
LTER t tn {((2))*((1))*((1))}
KPS LPRI LSEC {(0.9999)}
KPT LPRI LTER {(0.9999)}
KST LSEC LTER {(0.9999)}
BFLUX 0 flux I={V(p,pn)}
CFLUX flux 0 1
RFLUX flux 0 0.3183098861837907
BCORE p pn I={V(flux)/((2))*pow(sqrt(V(flux)^2+1e-24)/0.35,1.4)}
.ends
.subckt circuit20 in hot cold vp vn lp dg ag ch bitdepth
R266 in ag 100k
RRL29_A_PASS in oi {(0) ? 1e15 : 1m}
RRL29_A_MUTE oi ag {(0) ? 1m : 1e15}
R280 oi ag 10k
R357 bm bo 1.2k
R356 bm bc 2k
C255 bc contact32 100u
RRL32_A contact32 ag {(0) ? 1m : 1e15}
C212 bo pi 100u
R276 pi ag 4.7k
R275 nfb ni 1.2k
R267 nfb fb 240
R268 fb tertac 1.2k
C211 nfb tertac 22n
C214 tertac tertiary 100u
C213 bn bp 4.7n
R282 vp bn 1.2k
R277 bp vn 1.2k
R281 en primary 0.22
R269 primary ep 0.22
R340 cold ch 10
C254 cold ch 100n
R355 bitdepth dg 10k
RRL32_COIL lp coil32 {(125)}
XIC118_B oi bm vp vn bo circuit3
XIC118_A pi ni vp vn center circuit3
D131 bn center device10
D128 center bp device10
D132 coil32 lp device10
Q103 vp bn en device3
Q102 vn bp ep device4
Q104 coil32 bitdepth dg device0
XTF202 primary ag hot cold tertiary ag circuit21
.ends
.subckt channel vp vn p30 lp phantom
Xmic micp micn micraw vp vn phantom 0 0 circuit1
EMICUNITY mic 0 micraw 0 {micunity}
Xline hot cold line vp vn 0 0 circuit4
Xgain line linegain vp vn 0 circuit5
Xinst instrumentin 0 inst p30 0 0 circuit6
Bsource source 0 V={v(c_source)<0.5 ? v(mic) : (v(c_source)<1.5 ? v(linegain) : v(inst))}
Xtrim source inputtap vp vn 0 circuit7
Beqin eqin 0 V={v(c_pre)>0.5 ? v(listened) : v(inputtap)}
Xeqfilter eqin eqfilterraw vp vn 0 circuit8
Beqfilter eqfilter 0 V={(v(c_filterroute)==1 && v(c_alleq)>0.5 && v(c_bypass)<0.5) ? v(eqfilterraw) : v(eqin)}
Xeqshelf eqfilter eqshelfraw vp vn 0 circuit9
Beqshelf eqshelf 0 V={(v(c_shelfroute)==1 && v(c_alleq)>0.5 && v(c_bypass)<0.5) ? v(eqshelfraw) : v(eqfilter)}
Xeqlmf eqshelf eqlmfraw vp vn 0 circuit10
Beqlmf eqlmf 0 V={(v(c_paramroute)==1 && v(c_alleq)>0.5 && v(c_bypass)<0.5) ? v(eqlmfraw) : v(eqshelf)}
Xeqhmf eqlmf eqhmfraw vp vn 0 circuit11
Beqhmf eqhmf 0 V={(v(c_paramroute)==1 && v(c_alleq)>0.5 && v(c_bypass)<0.5) ? v(eqhmfraw) : v(eqlmf)}
Bdynin dynin 0 V={v(c_pre)>0.5 ? v(inputtap) : v(eqhmf)}
Bcompkeyin compkeyin 0 V={v(c_compkey)>0.5 ? v(key) : v(dynin)}
Bgatekeyin gatekeyin 0 V={v(c_gatekey)>0.5 ? v(key) : v(dynin)}
Xckfilter compkeyin ckfilterraw vp vn 0 circuit8
Bckfilter ckfilter 0 V={(v(c_filterroute)==2) ? v(ckfilterraw) : v(compkeyin)}
Xckshelf ckfilter ckshelfraw vp vn 0 circuit9
Bckshelf ckshelf 0 V={(v(c_shelfroute)==2) ? v(ckshelfraw) : v(ckfilter)}
Xcklmf ckshelf cklmfraw vp vn 0 circuit10
Bcklmf cklmf 0 V={(v(c_paramroute)==2) ? v(cklmfraw) : v(ckshelf)}
Xckhmf cklmf ckhmfraw vp vn 0 circuit11
Bckhmf ckhmf 0 V={(v(c_paramroute)==2) ? v(ckhmfraw) : v(cklmf)}
Xgkfilter gatekeyin gkfilterraw vp vn 0 circuit8
Bgkfilter gkfilter 0 V={(v(c_filterroute)==3) ? v(gkfilterraw) : v(gatekeyin)}
Xgkshelf gkfilter gkshelfraw vp vn 0 circuit9
Bgkshelf gkshelf 0 V={(v(c_shelfroute)==3) ? v(gkshelfraw) : v(gkfilter)}
Xgklmf gkshelf gklmfraw vp vn 0 circuit10
Bgklmf gklmf 0 V={(v(c_paramroute)==3) ? v(gklmfraw) : v(gkshelf)}
Xgkhmf gklmf gkhmfraw vp vn 0 circuit11
Bgkhmf gkhmf 0 V={(v(c_paramroute)==3) ? v(gkhmfraw) : v(gklmf)}
Bck compkey 0 V={v(ckhmf)}
Bgk gatekey 0 V={v(gkhmf)}
Boptogain vcain 0 V={v(dynin)*v(optogain)}
Xvca vcain vcaraw vcasink vp vn 0 0 circuit12
ICOMP vcasink vn dc 0 external
Bdyn compressed 0 V={v(c_vcaactive)>0.5 ? (v(c_mix)*v(vcaraw)+(1-v(c_mix) )*v(dynin)) : v(dynin)}
Xdeess compressed deessraw deessa deessk vp vn 0 circuit14
IDEESS deessk 0 dc 0 external
Bdeess deessed 0 V={v(c_deessin)>0.5 ? v(deessraw) : v(compressed)}
Blisten listened 0 V={v(c_complisten)>0.5 ? v(compkey) : (v(c_gatelisten)>0.5 ? v(gatekey) : (v(c_deesslisten)>0.5 ? v(xdeess.filter_out) : v(deessed)))}
Bpost posteq 0 V={v(c_bypass)>0.5 ? v(inputtap) : (v(c_pre)>0.5 ? v(eqhmf) : v(listened))}
Xfader posteq faded vp vn 0 circuit17
Xlimit faded limited 0 unused vp lk0 lk1 lk2 vp vn 0 circuit18
ILIMIT0 lk0 0 dc 0 external
ILIMIT1 lk1 0 dc 0 external
ILIMIT2 lk2 0 dc 0 external
Boutput outputtap 0 V={v(c_mute)>0.5 ? 0 : (v(c_limit)>0.5 ? v(limited) : v(faded))}
Xoutput outputtap outp outn vp vn lp 0 0 0 0 circuit20
RLOAD outp outn 100k
VINPUT input 0 dc 0 external
EHOT hot 0 input 0 0.5
ECOLD cold 0 input 0 -0.5
EMICP micps 0 micinput 0 0.5
EMICN micns 0 micinput 0 -0.5
VMIC micinput 0 dc 0 external
VINST instrumentin 0 dc 0 external
RMICP micps micp R={max(1m,v(c_sourceohms)/2)}
RMICN micns micn R={max(1m,v(c_sourceohms)/2)}
VKEY key 0 dc 0 external
VOPTO optogain 0 dc 0 external
.ends)HQ";
inline constexpr const char* biasSeeds[] = {"xmic.amp=1.42608590849e-06","xmic.dcblock=1.39287128597e-06","xmic.fbcap=1.39287128597e-06","xmic.fbmid=1.40947859723e-06","xmic.g0=1.39287128597e-06","xmic.g1=1.39287128597e-06","xmic.g2=1.39287128597e-06","xmic.g3=1.39287128597e-06","xmic.g4=1.39287128597e-06","xmic.g5=1.39287128597e-06","xmic.g6=1.39287128597e-06","xmic.ni=1.39287128597e-06","xmic.ph=1.63960709258e-10","xmic.pi=5.89291697912e-07","xmic.rotplus=0","xmic.sec=0","xmic.snub=0","xmic.tfcold=2.41115144274e-15","xmic.tfhot=2.41115144274e-15","xmic.xic103_a.cmd=-0.0803579570288","xmic.xic103_a.pole=-0.0803579570288","xmic.xic103_a.sensed=1.42608590849e-06","xmic.xtf201.pa=2.41115144274e-15","xmic.xtf201.pb=2.41115144274e-15","xmic.xtf201.sc=0","xmic.z0=0","xmic.z1=0","xmic.z2=0","xmic.z3=0","xmic.z4=0","xline.auxminus=8.27573626284e-12","xline.auxout=-8.1960601303e-07","xline.cold_mid=2.02366891528e-07","xline.cold_rf=2.93670722377e-09","xline.hot_mid=-2.02362596451e-07","xline.hot_rf=-2.93664489449e-09","xline.nminus=4.01797075832e-07","xline.nplus=-4.01788548007e-07","xline.xic122_a.cmd=-0.0803585606078","xline.xic122_a.pole=-0.0803585606078","xline.xic122_a.sensed=8.1962284036e-07","xline.xic122_b.cmd=-8.25797269413e-07","xline.xic122_b.pole=-8.25797269446e-07","xline.xic122_b.sensed=-8.1960601303e-07","xgain.ni=1.63686096943e-06","xgain.oa=1.65052178378e-06","xgain.pi=8.33283654711e-07","xgain.rotor=0","xgain.tap0=0","xgain.tap1=0","xgain.tap2=0","xgain.tap3=0","xgain.tap4=0","xgain.tap5=0","xgain.tap6=0","xgain.xic100_a.cmd=-0.0803577314696","xgain.xic100_a.pole=-0.0803577314696","xgain.xic100_a.sensed=1.65052178378e-06","xinst.bias=7.26819089311","xinst.bypass=9.01571528146","xinst.coupling=7.2706992267","xinst.drain=30.5890191307","xinst.emit=12.4449533647","xinst.gate=7.2706992267","xinst.selected=0","xinst.source=9.01571528146","xinst.supply=31.2531380654","xinst.trimtop=9.01571528146","xtrim.inverted=6.14562030236e-18","xtrim.ni=1.39286896237e-06","xtrim.normal=6.14562024096e-07","xtrim.oa=1.51876288783e-06","xtrim.phaseinv=8.03593051162e-07","xtrim.phaseout=9.7262738052e-08","xtrim.pi=5.89290714848e-07","xtrim.potw=1.51876228629e-06","xtrim.trimcap=1.39286896237e-06","xtrim.xic100_b.cmd=-0.0803578229746","xtrim.xic100_b.pole=-0.0803578229746","xtrim.xic100_b.sensed=1.51876288783e-06","xtrim.xic103_b.cmd=-0.0803593033386","xtrim.xic103_b.pole=-0.0803593033386","xtrim.xic103_b.sensed=9.7262738052e-08","xvca.bend_base=-0.00044700835304","xvca.bend_cmp=-1.70469279157e-07","xvca.bend_ctrl=0.00563153043956","xvca.bend_drive=-7.20411947396","xvca.bend_minus=5.10624789631e-06","xvca.bend_out=0.21922618745","xvca.bend_sense=-1.89415269419","xvca.bend_sink=-0.693812575073","xvca.bend_tail=-0.701235192589","xvca.bend_tail_base=-1.20868572299","xvca.common_minus=4.9452450242e-07","xvca.common_out=6.42236497742","xvca.common_plus=6.79296839233e-05","xvca.diff_minus=3.50613172636e-05","xvca.diff_out=1.63234135279e-06","xvca.diff_plus=3.42577361912e-05","xvca.input_ac=8.03585717698e-07","xvca.inv_ac=0","xvca.inv_out=8.16175227275e-07","xvca.inv_sum=8.03585717698e-07","xvca.link_tip=-1.39670807404e-10","xvca.makeup_ac=9.29477770638e-07","xvca.makeup_final=7.43906628741e-07","xvca.makeup_minus=9.29477770638e-07","xvca.makeup_out=1.05537245429e-06","xvca.makeup_plus=1.25894683656e-07","xvca.makeup_w=1.05537203628e-06","xvca.mirror_cb=12.1547690207","xvca.mirror_e_a=12.7837862306","xvca.mirror_e_b=12.7806697538","xvca.mix_minus=-1.39670810197e-10","xvca.mix_out=-1.48982200337e-10","xvca.offset_w=-3.13201547891","xvca.sum_a=8.02886430077e-07","xvca.sum_b=8.02886553587e-07","xvca.tail_a=-0.676152033605","xvca.tail_ac=-1.89415269476","xvca.tail_base_a=-1.20862952602","xvca.tail_base_b=-1.20862952602","xvca.tail_b=-0.676152033605","xvca.tail_inv_ac=-1.89415269476","xvca.tail_sense_a=-1.89415269476","xvca.tail_sense_b=-1.89415269476","xvca.vca_a=6.85028829456e-05","xvca.vca_b=6.85028826112e-05","xvca.vca_base=0.00552825601653","xvca.vref_d1=-0.631388259701","xvca.vref=-1.8941647791","xvca.vref_d2=-1.2627765194","xvca.xic100_a.cmd=-0.0802886430425","xvca.xic100_a.pole=-0.0802886430425","xvca.xic100_a.sensed=6.85028829456e-05","xvca.xic100_b.cmd=-0.0802886515916","xvca.xic100_b.pole=-0.0802886515916","xvca.xic100_b.sensed=6.85028826112e-05","xvca.xic101_a.cmd=-0.0803581055023","xvca.xic101_a.pole=-0.0803581055023","xvca.xic101_a.sensed=1.63234135279e-06","xvca.xic101_b.cmd=-0.0803583086971","xvca.xic101_b.pole=-0.0803583086971","xvca.xic101_b.sensed=1.05537245429e-06","xvca.xic104_a.cmd=6.74351529809","xvca.xic104_a.pole=6.74351529809","xvca.xic104_a.sensed=6.42236497742","xvca.xic110_a.cmd=-1.20843452508","xvca.xic110_a.pole=-1.20843452508","xvca.xic110_a.sensed=-1.20862952602","xvca.xic110_b.cmd=-1.20843452508","xvca.xic110_b.pole=-1.20843452508","xvca.xic110_b.sensed=-1.20862952602","xvca.xic123_a.cmd=-0.080358569992","xvca.xic123_a.pole=-0.080358569992","xvca.xic123_a.sensed=8.16175227275e-07","xvca.xic123_b.cmd=-1.20849125701","xvca.xic123_b.pole=-1.20849125701","xvca.xic123_b.sensed=-1.20868572299","xvca.xic127.cmd=-7.65937180259","xvca.xic127.pole=-7.65937180259","xvca.xic127.sensed=-7.20411947396","xvca.xic128.cmd=0.255703918735","xvca.xic128.pole=0.255703918735","xvca.xic128.sensed=0.21922618745","xvca.xic129_a.cmd=1.3968857376e-05","xvca.xic129_a.pole=1.39688573766e-05","xvca.xic129_a.sensed=-1.48982200337e-10","xdeess.audio_inv=2.7050499228e-06","xdeess.audio_plus=8.03566483088e-09","xdeess.audio_sum=8.11602147919e-07","xdeess.filter_minus=1.12586910607e-13","xdeess.filter_out=1.16218428258e-13","xdeess.filter_plus=4.53365837094e-14","xdeess.freq_a=1.12586910607e-13","xdeess.freq_b=1.12586910607e-13","xdeess.freq_c=1.14828588169e-13","xdeess.freq_cb=1.12586910607e-13","xdeess.freq_input=1.12586910607e-13","xdeess.invert_out=1.94254277355e-14","xdeess.invert_sum=6.7250300443e-14","xdeess.opt_supply=16.0718780717","xdeess.optical_audio=3.85998015651e-07","xdeess.output_amp=-2.70499470036e-06","xdeess.output_sum=2.71987098261e-11","xdeess.xic114_a.cmd=-4.94867527941e-09","xdeess.xic114_a.pole=-4.94867585034e-09","xdeess.xic114_a.sensed=1.16218428258e-13","xdeess.xic114_b.cmd=-4.94867285783e-09","xdeess.xic114_b.pole=-4.94867320493e-09","xdeess.xic114_b.sensed=1.94254277355e-14","xdeess.xic125_a.cmd=-0.0803566465311","xdeess.xic125_a.pole=-0.0803566465311","xdeess.xic125_a.sensed=2.7050499228e-06","xdeess.xic125_b.cmd=-2.71987098264e-06","xdeess.xic125_b.pole=-2.71987098262e-06","xdeess.xic125_b.sensed=-2.70499470036e-06","xdeess.xopt2.illum=0","xdeess.xopt2.led=16.0718780717","xdeess.xopt2.state=5.5379797703e-116","xfader.mb=1.45463413587e-17","xfader.mn=1.78491802782e-14","xfader.mout=8.41348631608e-17","xfader.xic109_b.cmd=-1.48010346895e-13","xfader.xic109_b.pole=-1.47702149542e-13","xfader.xic109_b.sensed=3.94479028758e-14","xfader.xic126_a.cmd=3.02834458207e-18","xfader.xic126_a.pole=-6.42197983644e-16","xfader.xic126_a.sensed=1.45463413587e-17","xlimit.eca=-8.27573180186e-21","xlimit.ecb=-8.27566666612e-21","xlimit.ecc=-4.00295806252e-22","xlimit.ehigh=-4.07100834958e-22","xlimit.ehigh_ac=8.03544569878e-07","xlimit.ehigh_att=8.03544569878e-07","xlimit.ehn=-4.00295806252e-22","xlimit.eln=-8.27573180186e-21","xlimit.elow=-8.27586318066e-21","xlimit.elow_ac=8.03544569878e-07","xlimit.elow_att=8.03544569878e-07","xlimit.emid=-1.62849188306e-20","xlimit.emid_ac=8.03544569878e-07","xlimit.emid_att=8.03544569878e-07","xlimit.emn=-8.27566666612e-21","xlimit.esum=8.03585550652e-07","xlimit.ica=-8.27573180186e-21","xlimit.icb=-8.27566666612e-21","xlimit.icc=-4.00295806252e-22","xlimit.ihigh=-4.07100834958e-22","xlimit.ihigh_ac=8.03544546134e-07","xlimit.ihigh_att=8.03544546134e-07","xlimit.ihn=-4.00295806252e-22","xlimit.iln=-8.27573180186e-21","xlimit.ilow_ac=8.03544546134e-07","xlimit.ilow=-8.27586318066e-21","xlimit.ilow_att=8.03544546134e-07","xlimit.imid=-1.62849188306e-20","xlimit.imid_ac=8.03544546134e-07","xlimit.imid_att=8.03544546134e-07","xlimit.imn=-8.27566666612e-21","xlimit.isum=8.03585526907e-07","xlimit.led_high_link=5.19033544105","xlimit.led_low_link=5.19033544105","xlimit.led_mid_link=5.19033544105","xlimit.xic139_a.cmd=1.73731295587e-16","xlimit.xic139_a.pole=8.27522451185e-16","xlimit.xic139_a.sensed=-8.27586318066e-21","xlimit.xic139_b.cmd=-0.0803585509127","xlimit.xic139_b.pole=-0.0803585509127","xlimit.xic139_b.sensed=8.30612774508e-07","xlimit.xic140_a.cmd=1.73723155937e-16","xlimit.xic140_a.pole=8.27515939556e-16","xlimit.xic140_a.sensed=-1.62849188306e-20","xlimit.xic140_b.cmd=2.00466234114e-16","xlimit.xic140_b.pole=3.99788531892e-17","xlimit.xic140_b.sensed=-4.07100834958e-22","xlimit.xic143_a.cmd=1.73731295587e-16","xlimit.xic143_a.pole=8.27522451185e-16","xlimit.xic143_a.sensed=-8.27586318066e-21","xlimit.xic143_b.cmd=-0.0803585550657","xlimit.xic143_b.pole=-0.0803585550657","xlimit.xic143_b.sensed=8.30612799051e-07","xlimit.xic148_a.cmd=1.73723155937e-16","xlimit.xic148_a.pole=8.27515939556e-16","xlimit.xic148_a.sensed=-1.62849188306e-20","xlimit.xic148_b.cmd=2.00466234114e-16","xlimit.xic148_b.pole=3.99788531892e-17","xlimit.xic148_b.sensed=-4.07100834958e-22","xlimit.xopt3.illum=4.24607072778e-14","xlimit.xopt3.led=5.19033544105","xlimit.xopt3.state=4.24607071928e-14","xlimit.xopt4.illum=4.24607072778e-14","xlimit.xopt4.led=5.19033544105","xlimit.xopt4.state=4.24607071928e-14","xlimit.xopt5.illum=4.24607072778e-14","xlimit.xopt5.led=5.19033544105","xlimit.xopt5.state=4.24607071928e-14","xlimit.xopt6.illum=1.73576190095e-50","xlimit.xopt6.led=5.19033544105","xlimit.xopt6.state=1.73576189748e-50","xlimit.xopt7.illum=1.73576190095e-50","xlimit.xopt7.led=5.19033544105","xlimit.xopt7.state=1.73576189748e-50","xlimit.xopt8.illum=1.73576190095e-50","xlimit.xopt8.led=5.19033544105","xlimit.xopt8.state=1.73576189748e-50","xoutput.bc=8.30604311751e-07","xoutput.bm=8.30604311751e-07","xoutput.bn=0.755657427333","xoutput.bo=8.30604278598e-07","xoutput.bp=-0.652157495099","xoutput.center=0.0496065327438","xoutput.coil32=5.19033531129","xoutput.contact32=0","xoutput.en=0.0610575905101","xoutput.ep=0.0411084512231","xoutput.fb=0","xoutput.nfb=0","xoutput.ni=0","xoutput.oi=8.30612600079e-07","xoutput.pi=0","xoutput.primary=0.0508036010608","xoutput.tertac=0","xoutput.tertiary=0","xoutput.xic118_a.cmd=1.77635683746e-09","xoutput.xic118_a.pole=1.7763568361e-09","xoutput.xic118_a.sensed=0.0496065327438","xoutput.xic118_b.cmd=8.30608430623e-07","xoutput.xic118_b.pole=8.30608430871e-07","xoutput.xic118_b.sensed=8.30604278598e-07","xoutput.xtf202.p=-1.19002030452e-15","xoutput.xtf202.s=0","xoutput.xtf202.t=0","xeqfilter.hpa=-6.53240374844e-22","xeqfilter.hpb2=-6.19958537666e-21","xeqfilter.hpb=-6.19958537666e-21","xeqfilter.hpc=-8.3686746061e-22","xeqfilter.hpf=-6.618575883e-22","xeqfilter.hpg=-4.8420832522e-21","xeqfilter.hpm=-6.18961418987e-21","xeqfilter.hpo=-4.83147699415e-21","xeqfilter.hpo2=-1.05240072091e-20","xeqfilter.hpp=-8.47906988814e-22","xeqfilter.lpa=6.14561845873e-07","xeqfilter.lpb=6.14549566183e-07","xeqfilter.lpc=6.14549553428e-07","xeqfilter.lpd=6.14549540674e-07","xeqfilter.lpf=6.14561833209e-07","xeqfilter.lpg=6.14555693946e-07","xeqfilter.lpm=6.14543395209e-07","xeqfilter.lpo2=6.14549566183e-07","xeqfilter.lpout=6.14543383739e-07","xeqfilter.lpo=6.14555705385e-07","xeqfilter.lpp=6.14549540674e-07","xeqfilter.xic101_a.cmd=6.14546455751e-07","xeqfilter.xic101_a.pole=6.14546456298e-07","xeqfilter.xic101_a.sensed=6.14543383739e-07","xeqfilter.xic101_b.cmd=1.76747071369e-16","xeqfilter.xic101_b.pole=4.16911221523e-16","xeqfilter.xic101_b.sensed=-4.83147699415e-21","xeqfilter.xic102_a.cmd=6.14558778564e-07","xeqfilter.xic102_a.pole=6.14558779032e-07","xeqfilter.xic102_a.sensed=6.14555705385e-07","xeqfilter.xic102_b.cmd=6.14552639202e-07","xeqfilter.xic102_b.pole=6.14552638863e-07","xeqfilter.xic102_b.sensed=6.14549566183e-07","xeqfilter.xic105_a.cmd=1.79883779867e-16","xeqfilter.xic105_a.pole=5.68141706524e-16","xeqfilter.xic105_a.sensed=-1.05240072091e-20","xeqfilter.xic105_b.cmd=1.82557296257e-16","xeqfilter.xic105_b.pole=5.34119995946e-16","xeqfilter.xic105_b.sensed=-6.19958537666e-21","xeqshelf.hfa=4.06707411304e-07","xeqshelf.hfamp=1.25768264578e-06","xeqshelf.hfb=1.22709902891e-06","xeqshelf.hfground=8.03699659039e-07","xeqshelf.hfin=3.76134562133e-07","xeqshelf.hfm=8.03581240505e-07","xeqshelf.hfref=8.03699659039e-07","xeqshelf.hfsw0_0=8.03699659053e-07","xeqshelf.hfsw0_1=8.03699659039e-07","xeqshelf.hfsw1_0=8.16830997733e-07","xeqshelf.hfsw1_1=8.03581240505e-07","xeqshelf.hfsw2_0=8.16830997733e-07","xeqshelf.hfsw2_1=8.03581240505e-07","xeqshelf.hfsw3_0=8.16830997733e-07","xeqshelf.hfsw3_1=8.03581240505e-07","xeqshelf.hfsw4_0=8.16830997733e-07","xeqshelf.hfsw4_1=8.03581240505e-07","xeqshelf.hfsw5_0=8.16830997733e-07","xeqshelf.hfsw5_1=8.03581240505e-07","xeqshelf.hfw=8.16830997746e-07","xeqshelf.lfa=0","xeqshelf.lfb=0","xeqshelf.lfc=0","xeqshelf.lfdec=0","xeqshelf.lfground=0","xeqshelf.lfin=0","xeqshelf.lfminus=8.03580460257e-07","xeqshelf.lfo=1.33930076709e-06","xeqshelf.lfref=1.07144061367e-06","xeqshelf.lfsum=0","xeqshelf.lfsw0_0=0","xeqshelf.lfsw0_1=0","xeqshelf.lfsw1_0=0","xeqshelf.lfsw1_1=0","xeqshelf.lfsw2_0=0","xeqshelf.lfsw2_1=0","xeqshelf.lfsw3_0=0","xeqshelf.lfsw3_1=0","xeqshelf.lfsw4_0=0","xeqshelf.lfsw4_1=0","xeqshelf.lfsw5_0=0","xeqshelf.lfsw5_1=0","xeqshelf.lfw=0","xeqshelf.xic112_a.cmd=-0.0803580442483","xeqshelf.xic112_a.pole=-0.0803580442483","xeqshelf.xic112_a.sensed=1.33930076709e-06","xeqshelf.xic112_b.cmd=-0.0803581222735","xeqshelf.xic112_b.pole=-0.0803581222735","xeqshelf.xic112_b.sensed=1.25768264578e-06","xeqlmf.drive=8.0358552568e-07","xeqlmf.f1bottom=8.08407021827e-07","xeqlmf.f1=8.08407021827e-07","xeqlmf.f1top=8.08407021827e-07","xeqlmf.f2bottom=8.08407277521e-07","xeqlmf.f2top=8.08407277521e-07","xeqlmf.f2=8.08407277521e-07","xeqlmf.fg1=8.08407021827e-07","xeqlmf.fg2=8.08407277521e-07","xeqlmf.ga=3.58915732964e-29","xeqlmf.ga2=1.1369380361e-29","xeqlmf.gdec=8.06062871336e-07","xeqlmf.gmid=0","xeqlmf.i1=8.35193469476e-07","xeqlmf.i2=8.11780937522e-07","xeqlmf.ia=8.03585508797e-07","xeqlmf.ib=8.03585762969e-07","xeqlmf.ix1=8.03585508797e-07","xeqlmf.ix2=8.03585762969e-07","xeqlmf.minus=8.06062871336e-07","xeqlmf.qa=8.22670308152e-07","xeqlmf.qcap=8.06062871336e-07","xeqlmf.qlow=1.61119930307e-09","xeqlmf.qm=1.36951940761e-09","xeqlmf.qout=1.21080006301e-29","xeqlmf.qt=1.12783951215e-09","xeqlmf.qtail=2.47721892847e-09","xeqlmf.qw=2.11469908528e-09","xeqlmf.reson=8.25550296915e-07","xeqlmf.rminus=8.03585623214e-07","xeqlmf.suma=8.3519320533e-07","xeqlmf.xic104_a.cmd=-0.0803585508786","xeqlmf.xic104_a.pole=-0.0803585508786","xeqlmf.xic104_a.sensed=8.35193469476e-07","xeqlmf.xic104_b.cmd=-0.0803585745195","xeqlmf.xic104_b.pole=-0.0803585745195","xeqlmf.xic104_b.sensed=8.11780937522e-07","xeqlmf.xic106_a.cmd=-0.0803585507911","xeqlmf.xic106_a.pole=-0.0803585507911","xeqlmf.xic106_a.sensed=8.3519320533e-07","xeqlmf.xic107_a.cmd=-0.0803585605467","xeqlmf.xic107_a.pole=-0.0803585605467","xeqlmf.xic107_a.sensed=8.25550296915e-07","xeqlmf.xic107_b.cmd=-0.0803585634645","xeqlmf.xic107_b.pole=-0.0803585634645","xeqlmf.xic107_b.sensed=8.22670308152e-07","xeqhmf.drive=8.0358552568e-07","xeqhmf.f1=8.08407021829e-07","xeqhmf.f1bottom=8.08407021829e-07","xeqhmf.f1top=8.08407021829e-07","xeqhmf.f2=8.08407277523e-07","xeqhmf.f2bottom=8.08407277523e-07","xeqhmf.f2top=8.08407277523e-07","xeqhmf.fg1=8.08407021829e-07","xeqhmf.fg2=8.08407277523e-07","xeqhmf.ga=9.51112097799e-18","xeqhmf.ga2=3.01283956446e-18","xeqhmf.gdec=8.06062871336e-07","xeqhmf.i1=8.3519346949e-07","xeqhmf.i2=8.11780937509e-07","xeqhmf.ia=8.03585508797e-07","xeqhmf.ib=8.03585762969e-07","xeqhmf.ix1=8.03585508797e-07","xeqhmf.ix2=8.03585762969e-07","xeqhmf.minus=8.06062871336e-07","xeqhmf.qa=8.22670308152e-07","xeqhmf.qcap=8.06062871336e-07","xeqhmf.qlow=1.61119930307e-09","xeqhmf.qm=1.36951940761e-09","xeqhmf.qt=1.12783951215e-09","xeqhmf.qtail=2.47721892847e-09","xeqhmf.qw=2.11469908528e-09","xeqhmf.reson=8.25550296915e-07","xeqhmf.rminus=8.03585623214e-07","xeqhmf.suma=8.35193205344e-07","xeqhmf.xic106_b.cmd=-0.0803585507911","xeqhmf.xic106_b.pole=-0.0803585507911","xeqhmf.xic106_b.sensed=8.35193205344e-07","xeqhmf.xic108_a.cmd=-0.0803585605467","xeqhmf.xic108_a.pole=-0.0803585605467","xeqhmf.xic108_a.sensed=8.25550296915e-07","xeqhmf.xic108_b.cmd=-0.0803585634645","xeqhmf.xic108_b.pole=-0.0803585634645","xeqhmf.xic108_b.sensed=8.22670308152e-07","xeqhmf.xic109_a.cmd=-0.0803585508786","xeqhmf.xic109_a.pole=-0.0803585508786","xeqhmf.xic109_a.sensed=8.3519346949e-07","xeqhmf.xic109_b.cmd=-0.0803585745195","xeqhmf.xic109_b.pole=-0.0803585745195","xeqhmf.xic109_b.sensed=8.11780937509e-07","xckfilter.hpa=-6.53240374844e-22","xckfilter.hpb2=-6.19958537666e-21","xckfilter.hpb=-6.19958537666e-21","xckfilter.hpc=-8.3686746061e-22","xckfilter.hpf=-6.618575883e-22","xckfilter.hpg=-4.8420832522e-21","xckfilter.hpm=-6.18961418987e-21","xckfilter.hpo=-4.83147699415e-21","xckfilter.hpo2=-1.05240072091e-20","xckfilter.hpp=-8.47906988814e-22","xckfilter.lpa=6.14561845873e-07","xckfilter.lpb=6.14549566183e-07","xckfilter.lpc=6.14549553428e-07","xckfilter.lpd=6.14549540674e-07","xckfilter.lpf=6.14561833209e-07","xckfilter.lpg=6.14555693946e-07","xckfilter.lpm=6.14543395209e-07","xckfilter.lpo2=6.14549566183e-07","xckfilter.lpout=6.14543383739e-07","xckfilter.lpo=6.14555705385e-07","xckfilter.lpp=6.14549540674e-07","xckfilter.xic101_a.cmd=6.14546455751e-07","xckfilter.xic101_a.pole=6.14546456298e-07","xckfilter.xic101_a.sensed=6.14543383739e-07","xckfilter.xic101_b.cmd=1.76747071369e-16","xckfilter.xic101_b.pole=4.16911221523e-16","xckfilter.xic101_b.sensed=-4.83147699415e-21","xckfilter.xic102_a.cmd=6.14558778564e-07","xckfilter.xic102_a.pole=6.14558779032e-07","xckfilter.xic102_a.sensed=6.14555705385e-07","xckfilter.xic102_b.cmd=6.14552639202e-07","xckfilter.xic102_b.pole=6.14552638863e-07","xckfilter.xic102_b.sensed=6.14549566183e-07","xckfilter.xic105_a.cmd=1.79883779867e-16","xckfilter.xic105_a.pole=5.68141706524e-16","xckfilter.xic105_a.sensed=-1.05240072091e-20","xckfilter.xic105_b.cmd=1.82557296257e-16","xckfilter.xic105_b.pole=5.34119995946e-16","xckfilter.xic105_b.sensed=-6.19958537666e-21","xckshelf.hfa=4.06707411304e-07","xckshelf.hfamp=1.25768264578e-06","xckshelf.hfb=1.22709902891e-06","xckshelf.hfground=8.03699659039e-07","xckshelf.hfin=3.76134562133e-07","xckshelf.hfm=8.03581240505e-07","xckshelf.hfref=8.03699659039e-07","xckshelf.hfsw0_0=8.03699659053e-07","xckshelf.hfsw0_1=8.03699659039e-07","xckshelf.hfsw1_0=8.16830997733e-07","xckshelf.hfsw1_1=8.03581240505e-07","xckshelf.hfsw2_0=8.16830997733e-07","xckshelf.hfsw2_1=8.03581240505e-07","xckshelf.hfsw3_0=8.16830997733e-07","xckshelf.hfsw3_1=8.03581240505e-07","xckshelf.hfsw4_0=8.16830997733e-07","xckshelf.hfsw4_1=8.03581240505e-07","xckshelf.hfsw5_0=8.16830997733e-07","xckshelf.hfsw5_1=8.03581240505e-07","xckshelf.hfw=8.16830997746e-07","xckshelf.lfa=0","xckshelf.lfb=0","xckshelf.lfc=0","xckshelf.lfdec=0","xckshelf.lfground=0","xckshelf.lfin=0","xckshelf.lfminus=8.03580460257e-07","xckshelf.lfo=1.33930076709e-06","xckshelf.lfref=1.07144061367e-06","xckshelf.lfsum=0","xckshelf.lfsw0_0=0","xckshelf.lfsw0_1=0","xckshelf.lfsw1_0=0","xckshelf.lfsw1_1=0","xckshelf.lfsw2_0=0","xckshelf.lfsw2_1=0","xckshelf.lfsw3_0=0","xckshelf.lfsw3_1=0","xckshelf.lfsw4_0=0","xckshelf.lfsw4_1=0","xckshelf.lfsw5_0=0","xckshelf.lfsw5_1=0","xckshelf.lfw=0","xckshelf.xic112_a.cmd=-0.0803580442483","xckshelf.xic112_a.pole=-0.0803580442483","xckshelf.xic112_a.sensed=1.33930076709e-06","xckshelf.xic112_b.cmd=-0.0803581222735","xckshelf.xic112_b.pole=-0.0803581222735","xckshelf.xic112_b.sensed=1.25768264578e-06","xcklmf.drive=8.0358552568e-07","xcklmf.f1bottom=8.08407021827e-07","xcklmf.f1=8.08407021827e-07","xcklmf.f1top=8.08407021827e-07","xcklmf.f2bottom=8.08407277521e-07","xcklmf.f2top=8.08407277521e-07","xcklmf.f2=8.08407277521e-07","xcklmf.fg1=8.08407021827e-07","xcklmf.fg2=8.08407277521e-07","xcklmf.ga=3.58915732964e-29","xcklmf.ga2=1.1369380361e-29","xcklmf.gdec=8.06062871336e-07","xcklmf.gmid=0","xcklmf.i1=8.35193469476e-07","xcklmf.i2=8.11780937522e-07","xcklmf.ia=8.03585508797e-07","xcklmf.ib=8.03585762969e-07","xcklmf.ix1=8.03585508797e-07","xcklmf.ix2=8.03585762969e-07","xcklmf.minus=8.06062871336e-07","xcklmf.qa=8.22670308152e-07","xcklmf.qcap=8.06062871336e-07","xcklmf.qlow=1.61119930307e-09","xcklmf.qm=1.36951940761e-09","xcklmf.qout=1.21080006301e-29","xcklmf.qt=1.12783951215e-09","xcklmf.qtail=2.47721892847e-09","xcklmf.qw=2.11469908528e-09","xcklmf.reson=8.25550296915e-07","xcklmf.rminus=8.03585623214e-07","xcklmf.suma=8.3519320533e-07","xcklmf.xic104_a.cmd=-0.0803585508786","xcklmf.xic104_a.pole=-0.0803585508786","xcklmf.xic104_a.sensed=8.35193469476e-07","xcklmf.xic104_b.cmd=-0.0803585745195","xcklmf.xic104_b.pole=-0.0803585745195","xcklmf.xic104_b.sensed=8.11780937522e-07","xcklmf.xic106_a.cmd=-0.0803585507911","xcklmf.xic106_a.pole=-0.0803585507911","xcklmf.xic106_a.sensed=8.3519320533e-07","xcklmf.xic107_a.cmd=-0.0803585605467","xcklmf.xic107_a.pole=-0.0803585605467","xcklmf.xic107_a.sensed=8.25550296915e-07","xcklmf.xic107_b.cmd=-0.0803585634645","xcklmf.xic107_b.pole=-0.0803585634645","xcklmf.xic107_b.sensed=8.22670308152e-07","xckhmf.drive=8.0358552568e-07","xckhmf.f1=8.08407021829e-07","xckhmf.f1bottom=8.08407021829e-07","xckhmf.f1top=8.08407021829e-07","xckhmf.f2=8.08407277523e-07","xckhmf.f2bottom=8.08407277523e-07","xckhmf.f2top=8.08407277523e-07","xckhmf.fg1=8.08407021829e-07","xckhmf.fg2=8.08407277523e-07","xckhmf.ga=9.51112097799e-18","xckhmf.ga2=3.01283956446e-18","xckhmf.gdec=8.06062871336e-07","xckhmf.i1=8.3519346949e-07","xckhmf.i2=8.11780937509e-07","xckhmf.ia=8.03585508797e-07","xckhmf.ib=8.03585762969e-07","xckhmf.ix1=8.03585508797e-07","xckhmf.ix2=8.03585762969e-07","xckhmf.minus=8.06062871336e-07","xckhmf.qa=8.22670308152e-07","xckhmf.qcap=8.06062871336e-07","xckhmf.qlow=1.61119930307e-09","xckhmf.qm=1.36951940761e-09","xckhmf.qt=1.12783951215e-09","xckhmf.qtail=2.47721892847e-09","xckhmf.qw=2.11469908528e-09","xckhmf.reson=8.25550296915e-07","xckhmf.rminus=8.03585623214e-07","xckhmf.suma=8.35193205344e-07","xckhmf.xic106_b.cmd=-0.0803585507911","xckhmf.xic106_b.pole=-0.0803585507911","xckhmf.xic106_b.sensed=8.35193205344e-07","xckhmf.xic108_a.cmd=-0.0803585605467","xckhmf.xic108_a.pole=-0.0803585605467","xckhmf.xic108_a.sensed=8.25550296915e-07","xckhmf.xic108_b.cmd=-0.0803585634645","xckhmf.xic108_b.pole=-0.0803585634645","xckhmf.xic108_b.sensed=8.22670308152e-07","xckhmf.xic109_a.cmd=-0.0803585508786","xckhmf.xic109_a.pole=-0.0803585508786","xckhmf.xic109_a.sensed=8.3519346949e-07","xckhmf.xic109_b.cmd=-0.0803585745195","xckhmf.xic109_b.pole=-0.0803585745195","xckhmf.xic109_b.sensed=8.11780937509e-07","xgkfilter.hpa=-6.53240374844e-22","xgkfilter.hpb2=-6.19958537666e-21","xgkfilter.hpb=-6.19958537666e-21","xgkfilter.hpc=-8.3686746061e-22","xgkfilter.hpf=-6.618575883e-22","xgkfilter.hpg=-4.8420832522e-21","xgkfilter.hpm=-6.18961418987e-21","xgkfilter.hpo=-4.83147699415e-21","xgkfilter.hpo2=-1.05240072091e-20","xgkfilter.hpp=-8.47906988814e-22","xgkfilter.lpa=6.14561845873e-07","xgkfilter.lpb=6.14549566183e-07","xgkfilter.lpc=6.14549553428e-07","xgkfilter.lpd=6.14549540674e-07","xgkfilter.lpf=6.14561833209e-07","xgkfilter.lpg=6.14555693946e-07","xgkfilter.lpm=6.14543395209e-07","xgkfilter.lpo2=6.14549566183e-07","xgkfilter.lpout=6.14543383739e-07","xgkfilter.lpo=6.14555705385e-07","xgkfilter.lpp=6.14549540674e-07","xgkfilter.xic101_a.cmd=6.14546455751e-07","xgkfilter.xic101_a.pole=6.14546456298e-07","xgkfilter.xic101_a.sensed=6.14543383739e-07","xgkfilter.xic101_b.cmd=1.76747071369e-16","xgkfilter.xic101_b.pole=4.16911221523e-16","xgkfilter.xic101_b.sensed=-4.83147699415e-21","xgkfilter.xic102_a.cmd=6.14558778564e-07","xgkfilter.xic102_a.pole=6.14558779032e-07","xgkfilter.xic102_a.sensed=6.14555705385e-07","xgkfilter.xic102_b.cmd=6.14552639202e-07","xgkfilter.xic102_b.pole=6.14552638863e-07","xgkfilter.xic102_b.sensed=6.14549566183e-07","xgkfilter.xic105_a.cmd=1.79883779867e-16","xgkfilter.xic105_a.pole=5.68141706524e-16","xgkfilter.xic105_a.sensed=-1.05240072091e-20","xgkfilter.xic105_b.cmd=1.82557296257e-16","xgkfilter.xic105_b.pole=5.34119995946e-16","xgkfilter.xic105_b.sensed=-6.19958537666e-21","xgkshelf.hfa=4.06707411304e-07","xgkshelf.hfamp=1.25768264578e-06","xgkshelf.hfb=1.22709902891e-06","xgkshelf.hfground=8.03699659039e-07","xgkshelf.hfin=3.76134562133e-07","xgkshelf.hfm=8.03581240505e-07","xgkshelf.hfref=8.03699659039e-07","xgkshelf.hfsw0_0=8.03699659053e-07","xgkshelf.hfsw0_1=8.03699659039e-07","xgkshelf.hfsw1_0=8.16830997733e-07","xgkshelf.hfsw1_1=8.03581240505e-07","xgkshelf.hfsw2_0=8.16830997733e-07","xgkshelf.hfsw2_1=8.03581240505e-07","xgkshelf.hfsw3_0=8.16830997733e-07","xgkshelf.hfsw3_1=8.03581240505e-07","xgkshelf.hfsw4_0=8.16830997733e-07","xgkshelf.hfsw4_1=8.03581240505e-07","xgkshelf.hfsw5_0=8.16830997733e-07","xgkshelf.hfsw5_1=8.03581240505e-07","xgkshelf.hfw=8.16830997746e-07","xgkshelf.lfa=0","xgkshelf.lfb=0","xgkshelf.lfc=0","xgkshelf.lfdec=0","xgkshelf.lfground=0","xgkshelf.lfin=0","xgkshelf.lfminus=8.03580460257e-07","xgkshelf.lfo=1.33930076709e-06","xgkshelf.lfref=1.07144061367e-06","xgkshelf.lfsum=0","xgkshelf.lfsw0_0=0","xgkshelf.lfsw0_1=0","xgkshelf.lfsw1_0=0","xgkshelf.lfsw1_1=0","xgkshelf.lfsw2_0=0","xgkshelf.lfsw2_1=0","xgkshelf.lfsw3_0=0","xgkshelf.lfsw3_1=0","xgkshelf.lfsw4_0=0","xgkshelf.lfsw4_1=0","xgkshelf.lfsw5_0=0","xgkshelf.lfsw5_1=0","xgkshelf.lfw=0","xgkshelf.xic112_a.cmd=-0.0803580442483","xgkshelf.xic112_a.pole=-0.0803580442483","xgkshelf.xic112_a.sensed=1.33930076709e-06","xgkshelf.xic112_b.cmd=-0.0803581222735","xgkshelf.xic112_b.pole=-0.0803581222735","xgkshelf.xic112_b.sensed=1.25768264578e-06","xgklmf.drive=8.0358552568e-07","xgklmf.f1bottom=8.08407021827e-07","xgklmf.f1=8.08407021827e-07","xgklmf.f1top=8.08407021827e-07","xgklmf.f2bottom=8.08407277521e-07","xgklmf.f2top=8.08407277521e-07","xgklmf.f2=8.08407277521e-07","xgklmf.fg1=8.08407021827e-07","xgklmf.fg2=8.08407277521e-07","xgklmf.ga=3.58915732964e-29","xgklmf.ga2=1.1369380361e-29","xgklmf.gdec=8.06062871336e-07","xgklmf.gmid=0","xgklmf.i1=8.35193469476e-07","xgklmf.i2=8.11780937522e-07","xgklmf.ia=8.03585508797e-07","xgklmf.ib=8.03585762969e-07","xgklmf.ix1=8.03585508797e-07","xgklmf.ix2=8.03585762969e-07","xgklmf.minus=8.06062871336e-07","xgklmf.qa=8.22670308152e-07","xgklmf.qcap=8.06062871336e-07","xgklmf.qlow=1.61119930307e-09","xgklmf.qm=1.36951940761e-09","xgklmf.qout=1.21080006301e-29","xgklmf.qt=1.12783951215e-09","xgklmf.qtail=2.47721892847e-09","xgklmf.qw=2.11469908528e-09","xgklmf.reson=8.25550296915e-07","xgklmf.rminus=8.03585623214e-07","xgklmf.suma=8.3519320533e-07","xgklmf.xic104_a.cmd=-0.0803585508786","xgklmf.xic104_a.pole=-0.0803585508786","xgklmf.xic104_a.sensed=8.35193469476e-07","xgklmf.xic104_b.cmd=-0.0803585745195","xgklmf.xic104_b.pole=-0.0803585745195","xgklmf.xic104_b.sensed=8.11780937522e-07","xgklmf.xic106_a.cmd=-0.0803585507911","xgklmf.xic106_a.pole=-0.0803585507911","xgklmf.xic106_a.sensed=8.3519320533e-07","xgklmf.xic107_a.cmd=-0.0803585605467","xgklmf.xic107_a.pole=-0.0803585605467","xgklmf.xic107_a.sensed=8.25550296915e-07","xgklmf.xic107_b.cmd=-0.0803585634645","xgklmf.xic107_b.pole=-0.0803585634645","xgklmf.xic107_b.sensed=8.22670308152e-07","xgkhmf.drive=8.0358552568e-07","xgkhmf.f1=8.08407021829e-07","xgkhmf.f1bottom=8.08407021829e-07","xgkhmf.f1top=8.08407021829e-07","xgkhmf.f2=8.08407277523e-07","xgkhmf.f2bottom=8.08407277523e-07","xgkhmf.f2top=8.08407277523e-07","xgkhmf.fg1=8.08407021829e-07","xgkhmf.fg2=8.08407277523e-07","xgkhmf.ga=9.51112097799e-18","xgkhmf.ga2=3.01283956446e-18","xgkhmf.gdec=8.06062871336e-07","xgkhmf.i1=8.3519346949e-07","xgkhmf.i2=8.11780937509e-07","xgkhmf.ia=8.03585508797e-07","xgkhmf.ib=8.03585762969e-07","xgkhmf.ix1=8.03585508797e-07","xgkhmf.ix2=8.03585762969e-07","xgkhmf.minus=8.06062871336e-07","xgkhmf.qa=8.22670308152e-07","xgkhmf.qcap=8.06062871336e-07","xgkhmf.qlow=1.61119930307e-09","xgkhmf.qm=1.36951940761e-09","xgkhmf.qt=1.12783951215e-09","xgkhmf.qtail=2.47721892847e-09","xgkhmf.qw=2.11469908528e-09","xgkhmf.reson=8.25550296915e-07","xgkhmf.rminus=8.03585623214e-07","xgkhmf.suma=8.35193205344e-07","xgkhmf.xic106_b.cmd=-0.0803585507911","xgkhmf.xic106_b.pole=-0.0803585507911","xgkhmf.xic106_b.sensed=8.35193205344e-07","xgkhmf.xic108_a.cmd=-0.0803585605467","xgkhmf.xic108_a.pole=-0.0803585605467","xgkhmf.xic108_a.sensed=8.25550296915e-07","xgkhmf.xic108_b.cmd=-0.0803585634645","xgkhmf.xic108_b.pole=-0.0803585634645","xgkhmf.xic108_b.sensed=8.22670308152e-07","xgkhmf.xic109_a.cmd=-0.0803585508786","xgkhmf.xic109_a.pole=-0.0803585508786","xgkhmf.xic109_a.sensed=8.3519346949e-07","xgkhmf.xic109_b.cmd=-0.0803585745195","xgkhmf.xic109_b.pole=-0.0803585745195","xgkhmf.xic109_b.sensed=8.11780937509e-07"};
inline constexpr const char* controlNames[] = {"air","alleq","bypass","compkey","complisten","deessfreq","deessin","deesslisten","fader","filterroute","gatekey","gatelisten","hfpot","hfsel","hmffreq","hmfgain","hmfq","hmfx3","hpr","insttrim","lfpot","lfsel","limit","linegain","lmffreq","lmfgain","lmfq","lmfx3","lpr","makeup","micgain","mix","mute","paramroute","phase","pre","shelfroute","source","sourceohms","trim","vcaactive","zscale"};
}
