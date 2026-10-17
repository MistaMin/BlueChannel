// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

#include "PluginProcessor.h"
#include "Widgets.h"
#include "EditorLayout.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace bluechannel::ui { class DevTools; class DigitalPanel; }

// The panel is laid out at a fixed logical size and scaled as a whole when the window is resized.
class BlueChannelPanel : public juce::Component
{
public:
    static constexpr int kWidth = 1560, kHeight = 450;

    explicit BlueChannelPanel(BlueChannelProcessor&);
    ~BlueChannelPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void tick();                 // 30 Hz: meters, legends, source-dependent controls
    void applySkin(int skin);
    bluechannel::ui::Theme theme;

    // Knob design (Designs/knobs.csv): one item per knob, keyed by parameter ID, with bounds in panel units.
    std::vector<goodlookinui::Item> designItems() const;
    bool applyDesignCsv(const std::string& csv);          // false (and nothing changed) if the CSV does not fit
    std::string designCsv() const;
    std::vector<bluechannel::ui::LayoutTarget> layoutTargets() const;
    juce::String editorLayoutJson() const;
    bool applyEditorLayoutJson(const juce::String&);
    void showDigitalPage(bool allControls);
    int editorPage() const;
    const std::vector<bluechannel::ui::Knob*>& allKnobs() const { return knobs; }
    std::function<void()> onDevToggle;                    // developer builds: the Dev key

private:
    using Knob = bluechannel::ui::Knob;
    using LedKey = bluechannel::ui::LedKey;
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    BlueChannelProcessor& proc;
    juce::OwnedArray<juce::Component> owned;
    juce::OwnedArray<SliderAtt> sliderAtts;
    juce::OwnedArray<ButtonAtt> buttonAtts;

    Knob* knob(const char* paramId, const juce::String& caption, Knob::Cap cap, juce::Rectangle<int> bounds, bool onSection = true);
    LedKey* key(const char* paramId, const juce::String& caption, juce::Colour led, juce::Rectangle<int> bounds, bool onSection = true);
    template <typename C> C* add(std::unique_ptr<C> c, juce::Rectangle<int> bounds)
    {
        auto* raw = c.get();
        addAndMakeVisible(raw);
        raw->setBounds(bounds);
        owned.add(c.release());
        return raw;
    }

    std::vector<Knob*> knobs;
    Knob *micGain = nullptr, *lineGain = nullptr, *trim = nullptr, *lpf = nullptr, *hpf = nullptr;
    Knob *lmfFreq = nullptr, *hmfFreq = nullptr, *lfFreq = nullptr, *hfFreq = nullptr, *output = nullptr;
    bluechannel::ui::VuMeter* vu = nullptr;
    bluechannel::ui::LedBar *outBar = nullptr, *inBar = nullptr;
    bluechannel::ui::GateLeds* gateLeds = nullptr;
    bluechannel::ui::Lamp *overload = nullptr, *deessActive = nullptr, *keyLamp = nullptr;
    juce::Component *vuSource = nullptr, *vuCal = nullptr;
    LedKey* skinKey = nullptr;
    float overloadHold = 0.0f;
    std::unique_ptr<bluechannel::ui::DigitalPanel> digital;

    struct Section { juce::Rectangle<float> r; juce::String title; };
    std::vector<Section> sections;
};

class BlueChannelEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BlueChannelEditor(BlueChannelProcessor&);
    ~BlueChannelEditor() override;
    void resized() override;
    void paint(juce::Graphics& g) override { g.fillAll(juce::Colours::black); }

private:
    void timerCallback() override { panel.tick(); }
    void toggleDeveloperMode();
    BlueChannelProcessor& proc;
    bool editorSized = false;
    BlueChannelPanel panel;
    juce::TooltipWindow tooltips { this, 600 };
    std::unique_ptr<bluechannel::ui::DevTools> dev;   // developer builds only
};
