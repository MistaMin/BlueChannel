// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once
#include "Model/Controls.h"
#include "Model/Engine.h"
#include <memory>

namespace bluechannel::hq
{
// The component solver allocates and blocks. Call render ONLY when the host reports non-realtime.
// Each renderer owns an isolated simulator library, circuit, and transient history.
class Renderer
{
public:
    enum Status { Idle = 0, Armed, Solving, Failed, Unavailable };
    Renderer();
    ~Renderer();
    void prepare(double sampleRate, int maxBlock, int channels);
    void restart(); // no allocation, also safe from reset()/realtime mode transitions
    bool render(double* const* io, int frames, const double* const* key, int keyChannels,
                const model::Controls&, int factor, model::Telemetry&);
    static bool compiled();
    const char* error() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
