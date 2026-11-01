# DSP engineering fixes — Blue Channel (2026-11-01)

Gates run: dsp_lint (0 current-file findings), ctest 4/4 + Rosetta model test, pluginval strict 10,
clap-validator 44 tests, auval. Findings fixed (committed 4feef21, pushed):

1. Parameter text round trips (clap-validator param-conversions): High Pass / Low Pass / LMF / HMF /
   De-Ess frequencies now parse their displayed Hz/kHz text back to the same value (denormalised);
   Mix (%) and Hold (ms/s) parse their displayed text back.
   File: Source/Plugin/Params.cpp.
2. State restore host refresh: setStateInformation calls updateHostDisplay(programChanged) after
   replaceState so hosts re-read parameter values after a load (the CLAP wrapper sends
   paramsRescan(VALUES) on programChanged). Fixes clap-validator state-reproducibility.

Verification after fixes: clap-validator 34 passed / 0 failed, pluginval SUCCESS, auval PASS,
ctest 4/4. No DSP change: text conversion and state hygiene only.
