#!/bin/sh
# SPDX-License-Identifier: MIT
# Regenerates tests/reference/*.csv from the reference circuit model with ngspice.
# Usage: BLUE_REFERENCE_CIR=/path/to/model.cir tools/make_references.sh
set -e
cd "$(dirname "$0")/.."
out=tests/reference
mkdir -p "$out"
ref() { name=$1; shift; python3 tools/spice_reference.py --ppd 12 --out "$out/$name.csv" "$@"; echo "$name"; }

ref line_input --subckt LINE_INPUT --input hot --output out
for k in 0 1 2 3 4 5 6; do ref line_gain_$k --subckt LINE_SECOND --params GAIN_SEL=$k --input in --output out; done
for k in 0 1 2 3 4 5 6 7 8 9 10 11; do ref mic_gain_$k --subckt MIC_INPUT --params GAIN_SEL=$k --input hot --output out; done
ref trim_0 --subckt INPUT_TRIM --params TRIM=0,PHASE=0 --input in --output out
ref trim_50 --subckt INPUT_TRIM --params TRIM=0.5,PHASE=0 --input in --output out
ref trim_100 --subckt INPUT_TRIM --params TRIM=1,PHASE=1 --input in --output out
ref filters_wide --subckt FILTERS --params LP_R=0.001,HP_R=47000 --input in --output out
ref filters_mid --subckt FILTERS --params LP_R=10000,HP_R=1000 --input in --output out
ref filters_narrow --subckt FILTERS --params LP_R=47000,HP_R=0.001 --input in --output out
ref shelves_a --subckt SHELVES --params LF_GAIN=0,HF_GAIN=1,LF_SEL=0,HF_SEL=0 --input in --output out
ref shelves_b --subckt SHELVES --params LF_GAIN=1,HF_GAIN=0,LF_SEL=3,HF_SEL=3 --input in --output out
ref shelves_c --subckt SHELVES --params LF_GAIN=0.25,HF_GAIN=0.75,LF_SEL=5,HF_SEL=5 --input in --output out
ref shelves_d --subckt SHELVES --params LF_GAIN=0.5,HF_GAIN=0.5,LF_SEL=2,HF_SEL=4 --input in --output out
ref lmf_a --subckt LMF_EQ --params GAIN=0,QPOS=0.5,FREQ_POS=0.5,X3=0 --input in --output out
ref lmf_b --subckt LMF_EQ --params GAIN=1,QPOS=1,FREQ_POS=0,X3=1 --input in --output out
ref lmf_c --subckt LMF_EQ --params GAIN=0.2,QPOS=0,FREQ_POS=1,X3=0 --input in --output out
ref lmf_d --subckt LMF_EQ --params GAIN=0.5,QPOS=0.5,FREQ_POS=0.5,X3=0 --input in --output out
ref hmf_a --subckt HMF_EQ --params GAIN=0,QPOS=0.5,FREQ_POS=0.5,X3=0 --input in --output out
ref hmf_b --subckt HMF_EQ --params GAIN=1,QPOS=1,FREQ_POS=0,X3=1 --input in --output out
ref hmf_c --subckt HMF_EQ --params GAIN=0.2,QPOS=0,FREQ_POS=1,X3=1 --input in --output out
ref deess_a --subckt DEESSER_AUDIO_CURRENT --params FREQUENCY=0.5 --input in --output x1.filter_out
ref deess_b --subckt DEESSER_AUDIO_CURRENT --params FREQUENCY=0.05 --input in --output x1.filter_out
ref deess_c --subckt DEESSER_AUDIO_CURRENT --params FREQUENCY=1 --input in --output x1.filter_out
ref deess_out --subckt DEESSER_AUDIO_CURRENT --params FREQUENCY=0.5 --input in --output out
ref limiter_low --subckt LIMITER_AUDIO_DIGITAL --input internal_in --output x1.ilow --extra "REXT n_external_in 0 10k" --extra "VLS n_led_supply 0 5" --extra "RLL n_led_low_k 0 1e12" --extra "RLM n_led_mid_k 0 1e12" --extra "RLH n_led_high_k 0 1e12"
ref limiter_mid --subckt LIMITER_AUDIO_DIGITAL --input internal_in --output x1.imid --extra "REXT n_external_in 0 10k" --extra "VLS n_led_supply 0 5" --extra "RLL n_led_low_k 0 1e12" --extra "RLM n_led_mid_k 0 1e12" --extra "RLH n_led_high_k 0 1e12"
ref limiter_high --subckt LIMITER_AUDIO_DIGITAL --input internal_in --output x1.ihigh --extra "REXT n_external_in 0 10k" --extra "VLS n_led_supply 0 5" --extra "RLL n_led_low_k 0 1e12" --extra "RLM n_led_mid_k 0 1e12" --extra "RLH n_led_high_k 0 1e12"
ref limiter_out --subckt LIMITER_AUDIO_DIGITAL --input internal_in --output internal_out --extra "REXT n_external_in 0 10k" --extra "VLS n_led_supply 0 5" --extra "RLL n_led_low_k 0 1e12" --extra "RLM n_led_mid_k 0 1e12" --extra "RLH n_led_high_k 0 1e12"
ref fader_100 --subckt EXTERNAL_SUM --params FADER=1 --input master --output fader
ref fader_71 --subckt EXTERNAL_SUM --params FADER=0.71 --input master --output fader
ref fader_20 --subckt EXTERNAL_SUM --params FADER=0.2 --input master --output fader
ref output --subckt TRANSFORMER_OUTPUT --input in --output hot --minus cold --extra "RLOAD n_hot n_cold 100k"
