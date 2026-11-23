// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Marcos Quirino Deida
#pragma once

// C ABI around bluechannel::model::Engine (Source/Model, no JUCE) for the Rust plugin (src/engine.rs).
// Every function is noexcept; nothing but bc_create, bc_prepare and bc_destroy allocates, except that
// bc_process runs the component solver (which allocates and blocks) while HQ Mode is on and `offline` is set.

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BcEngine BcEngine;

// What the engine measured during one bc_process call.
typedef struct BcTelemetry
{
    float inputPeak[2], outputPeak[2];       // linear, full scale = 1
    float inputSquare[2], outputSquare[2];   // mean square over the block
    float compGr, gateGr, limiterGr;         // dB of reduction, largest in the block
    float deessLed;                          // 0..1
    int overload;                            // a stage clipped
    int activeFactor;                        // oversampling factor in use
    int rendering;                           // the offline render bank is in use
    int hqStatus;                            // hq::Renderer::Status: Idle, Armed, Solving, Failed, Unavailable
} BcTelemetry;

BcEngine* bc_create(void);                   // null when out of memory
void bc_destroy(BcEngine*);
int bc_prepare(BcEngine*, double sampleRate, int maxBlock, int channels);   // 1 on success
void bc_reset(BcEngine*);
// values: the plugin's parameter table in plain units, kBcParamCount entries in table order.
// io: `channels` buffers processed in place. key: `keyChannels` (0..2) buffers, or null.
void bc_process(BcEngine*, float* const* io, int channels, int frames, const float* const* key, int keyChannels,
                const float* values, int offline, BcTelemetry* out);
int bc_param_count(void);
int bc_hq_compiled(void);                      // 1 when the offline component solver is in this build
const char* bc_hq_error(BcEngine*);           // why the last HQ block failed; read only while not processing

// The equaliser's analogue response for the editor's graph. Not for the audio thread: it owns its own
// stages and recompiles them when an equaliser control changes.
typedef struct BcEqView BcEqView;
enum { kBcCurvePoints = 161, kBcHandles = 6 };
BcEqView* bc_eq_create(void);                // null on failure
void bc_eq_destroy(BcEqView*);
// curve: response in dB of the sections in the audio path at 161 log-spaced points from 20 Hz to 20 kHz.
// filterCurve: the filters alone (drawn when they feed a key). handles: (Hz, dB) of LF, LMF, HMF, HF, HP, LP.
// Returns 1 when the curves were recomputed, 0 when nothing changed (the outputs are still written).
int bc_eq_update(BcEqView*, const float* values, float* curve, float* filterCurve, float* handles);

#ifdef __cplusplus
}
#endif
