// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
//
// Renders the editor panel offscreen to PNG (both skins), for layout checks and documentation images.
// These are component renders: they do not exercise native windows, menus or host scaling.
// Usage: BlueChannelSnapshot <output dir> [scale] [--controls]
//        BlueChannelSnapshot --layout <file>      write the complete editor layout JSON
//        BlueChannelSnapshot --design <file>      write the current knob design (Designs/knobs.csv format)

#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <juce_audio_utils/juce_audio_utils.h>

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc > 2 && juce::String(argv[1]) == "--layout")
    {
        BlueChannelProcessor proc;BlueChannelPanel panel(proc);
        const auto file=juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]);
        file.getParentDirectory().createDirectory();
        return file.replaceWithText(panel.editorLayoutJson()) ? 0 : 1;
    }
    if (argc > 2 && juce::String(argv[1]) == "--design")
    {
        BlueChannelProcessor proc;
        BlueChannelPanel panel(proc);
        const auto file = juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]);
        file.getParentDirectory().createDirectory();
        file.replaceWithText(juce::String(panel.designCsv()), false, false, "\n");
        std::printf("wrote %s\n", file.getFullPathName().toRawUTF8());
        return 0;
    }
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
        for(int arg=3;arg<argc;++arg)if(juce::String(argv[arg])=="--hq")set(id::hq,1);
        BlueChannelPanel panel(proc);
        if (skin == 1)
        {
            proc.setPlayConfigDetails(2, 2, 48000, 512);
            proc.prepareToPlay(48000,512);
            juce::Random random(73);
            float low=0;
            juce::AudioBuffer<float> buffer(2,512);
            juce::MidiBuffer midi;
            for(int block=0;block<24;++block)
            {
                for(int i=0;i<512;++i)
                {
                    low=0.97f*low+0.03f*(random.nextFloat()*2-1);
                    const float sample=low*0.7f+0.025f*std::sin(float(block*512+i)*0.0576f);
                    buffer.setSample(0,i,sample);buffer.setSample(1,i,sample);
                }
                proc.processBlock(buffer,midi);
            }
        }
        panel.tick();
        const bool controls = skin==1 && argc>3 && juce::String(argv[3])=="--controls";
        if(controls)
            if(auto* digital=panel.findChildWithID("digital-panel"))
                if(auto* page=dynamic_cast<juce::TextButton*>(digital->findChildWithID("digital-controls"))) {page->onClick();panel.tick();}
        auto img = panel.createComponentSnapshot(panel.getLocalBounds(), true, scale);
        const auto file = dir.getChildFile(skin == 0 ? "blue-channel-analogue.png" : controls ? "blue-channel-digital-controls.png" : "blue-channel-digital.png");
        file.deleteFile();
        juce::FileOutputStream out(file);
        juce::PNGImageFormat().writeImageToStream(img, out);
        std::printf("wrote %s (%d x %d)\n", file.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    }
#if BLUECHANNEL_DEV_EDITOR
    {
        // Developer mode, for checking the overlay and inspector (component render).
        BlueChannelProcessor proc;
        std::unique_ptr<juce::AudioProcessorEditor> ed(proc.createEditor());
        if (auto* pn = dynamic_cast<BlueChannelPanel*>(ed->getChildComponent(0))) pn->onDevToggle();
        auto img = ed->createComponentSnapshot(ed->getLocalBounds(), true, 1.0f);
        const auto file = dir.getChildFile("blue-channel-devmode.png");
        file.deleteFile();
        juce::FileOutputStream out(file);
        juce::PNGImageFormat().writeImageToStream(img, out);
        std::printf("wrote %s (%d x %d)\n", file.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    }
    for(bool allControls : {false,true})
    {
        BlueChannelProcessor proc;proc.setSkin(1);
        std::unique_ptr<juce::AudioProcessorEditor> ed(proc.createEditor());
        if(auto* panel=dynamic_cast<BlueChannelPanel*>(ed->getChildComponent(0)))
        {
            panel->showDigitalPage(allControls);
            panel->onDevToggle();
            auto img=ed->createComponentSnapshot(ed->getLocalBounds(),true,1.0f);
            const auto file=dir.getChildFile(allControls ? "blue-channel-digital-controls-dev.png" : "blue-channel-digital-overview-dev.png");
            file.deleteFile();juce::FileOutputStream out(file);juce::PNGImageFormat().writeImageToStream(img,out);
        }
    }
#endif
    return 0;
}
