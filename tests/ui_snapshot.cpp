// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
//
// Renders the editor panel offscreen to PNG (both skins), for layout checks and documentation images.
// These are component renders: they do not exercise native windows, menus or host scaling.
// Usage: BlueChannelSnapshot <output dir> [scale]

#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <juce_audio_utils/juce_audio_utils.h>

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File dir = juce::File::getCurrentWorkingDirectory().getChildFile(argc > 1 ? argv[1] : "snapshots");
    const float scale = argc > 2 ? float(std::atof(argv[2])) : 1.0f;
    dir.createDirectory();
    for (int skin : { 0, 1 })
    {
        BlueChannelProcessor proc;
        proc.setSkin(skin);
        namespace id = bluechannel::params::id;
        auto set = [&](const char* name, float v) {
            auto* p = proc.apvts.getParameter(name);
            p->setValueNotifyingHost(p->convertTo0to1(v));
        };
        // A representative setting so the image shows lit keys and turned knobs.
        set(id::paramRoute, 1); set(id::lmfGain, 4.0f); set(id::hmfGain, -3.0f); set(id::hmfX3, 1);
        set(id::shelfRoute, 1); set(id::lfGain, 3.0f); set(id::hfHigh, 1);
        set(id::compIn, 1); set(id::threshold, -10.0f); set(id::ratio, 4.0f); set(id::autoRelease, 1);
        set(id::filterRoute, 2); set(id::hpf, 80.0f);
        BlueChannelPanel panel(proc);
        panel.tick();
        auto img = panel.createComponentSnapshot(panel.getLocalBounds(), true, scale);
        const auto file = dir.getChildFile(skin == 0 ? "blue-channel-analogue.png" : "blue-channel-digital.png");
        file.deleteFile();
        juce::FileOutputStream out(file);
        juce::PNGImageFormat().writeImageToStream(img, out);
        std::printf("wrote %s (%d x %d)\n", file.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    }
    return 0;
}
