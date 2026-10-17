// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

// Editor controls. Every control is bound to a host parameter through a JUCE attachment, so automation,
// undo and session restore act on the parameter and the widgets only reflect it. Drawing uses GoodLookinUI.

#include "Theme.h"

#include <GoodLookinUI.h>
#include <Hardware.h>
#include <Meters.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>

namespace bluechannel::ui
{
namespace gl = goodlookinui::juce_adapter;

inline juce::String hexOf(juce::Colour c) { return "#" + c.toDisplayString(false); }

inline void drawText(juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, float size, juce::Colour c,
                     juce::Justification j = juce::Justification::centred, bool bold = true)
{
    g.setColour(c);
    g.setFont(juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain));
    g.drawText(s, r, j, false);
}

// Rotary control: caption above, GoodLookinUI knob, end legends below.
class Knob : public juce::Slider
{
public:
    enum class Cap { Yellow, Grey, Black };

    Knob(const Theme& t, juce::String caption, Cap c, bool onSection = true)
        : juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox), theme(t), section(onSection)
    {
        setRotaryParameters(-0.8f * juce::MathConstants<float>::pi, 0.8f * juce::MathConstants<float>::pi, true);
        setVelocityBasedMode(false);
        setMouseDragSensitivity(220);
        const auto base = analogueTheme();
        design.label = caption.toStdString();
        design.style = "FS";
        design.colour = hexOf(c == Cap::Yellow ? base.knobYellow : c == Cap::Grey ? base.knobGrey : base.knobBlack).toStdString();
        design.fontSize = 10.0f;
        setTitle(caption);
    }

    // Look and placement from the design file (Designs/knobs.csv): label, knob style, cap colour, caption size.
    // Bounds are applied by the panel. The digital skin draws the flat variant of the same style.
    goodlookinui::Item design;
    void applyDesign(const goodlookinui::Item& item)
    {
        design = item;
        setTitle(juce::String(item.label));
        repaint();
    }
    bool onSection() const { return section; }

    juce::String minLegend, maxLegend;
    std::function<juce::String()> extraLegend;   // optional second legend line (e.g. x3 range)

    juce::String getTooltip() override { return juce::String(design.label) + ": " + getTextFromValue(getValue()); }
    void mouseEnter(const juce::MouseEvent& e) override { juce::Slider::mouseEnter(e); repaint(); }
    void mouseExit(const juce::MouseEvent& e) override { juce::Slider::mouseExit(e); repaint(); }

    void paint(juce::Graphics& g) override
    {
        if (!isRotary()) { juce::Slider::paint(g); return; }
        auto r = getLocalBounds().toFloat();
        const juce::Colour ink = section ? theme.inkOnSection : theme.inkOnPanel;
        const float captionSize = design.fontSize * 0.95f;
        drawText(g, juce::String(design.label), r.removeFromTop(captionSize + 2.5f), captionSize, ink);
        auto legends = r.removeFromBottom(11.0f);
        const float d = juce::jmin(r.getWidth(), r.getHeight());
        auto knob = juce::Rectangle<float>(d, d).withCentre(r.getCentre());
        goodlookinui::Item item = design;
        const bool flatStyle = item.style.rfind("flat_", 0) == 0;
        if (theme.flat && !flatStyle) item.style = "flat_" + item.style;
        const auto backdrop = section ? theme.section : theme.panelTop;
        gl::drawKnob(g, knob, valueToProportionOfLength(getValue()), item, isEnabled(), backdrop);
        const auto dim = section ? theme.inkDim : theme.inkOnPanel.withAlpha(0.75f);
        // While the knob is hovered or turned, the legend line shows its value instead of the end legends.
        if (isMouseOverOrDragging() && isEnabled())
        {
            drawText(g, getTextFromValue(getValue()), legends.expanded(6.0f, 0.0f), 8.5f, ink, juce::Justification::centred);
            return;
        }
        drawText(g, minLegend, legends.withWidth(legends.getWidth() * 0.5f), 8.0f, dim, juce::Justification::centredLeft, false);
        drawText(g, maxLegend, legends.withTrimmedLeft(legends.getWidth() * 0.5f), 8.0f, dim, juce::Justification::centredRight, false);
        if (extraLegend)
            drawText(g, extraLegend(), legends, 8.0f, theme.flat ? theme.accent : juce::Colour(0xff8a6d00), juce::Justification::centred, false);
    }

private:
    const Theme& theme;
    bool section;
};

// Square key with an LED cap; the legend is printed below it.
class LedKey : public juce::Button
{
public:
    LedKey(const Theme& t, juce::String text, juce::Colour led, bool onSection = true)
        : juce::Button(text), theme(t), ledColour(led), section(onSection)
    {
        setClickingTogglesState(true);
        setTitle(text);
    }
    juce::Colour designColour() const { return ledColour; }
    std::function<bool()> litOverride;   // lamp state when it is not the toggle state

    void paintButton(juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat();
        auto key = juce::Rectangle<float>(20.0f, 12.0f).withCentre({ r.getCentreX(), r.getY() + 8.0f });
        const bool lit = litOverride ? litOverride() : getToggleState();
        const auto cap=getProperties().contains("layout.colour") ? juce::Colour::fromString("ff"+getProperties()["layout.colour"].toString().substring(1)) : ledColour;
        gl::hardware::drawLedKey(g, key, cap, lit, down);
        if (over) { g.setColour(juce::Colour(0x22ffffff)); g.fillRoundedRectangle(key.expanded(2.5f), 3.0f); }
        drawText(g, getButtonText(), r.withTrimmedTop(16.0f),float(getProperties().getWithDefault("layout.font",8.5f)), section ? theme.inkOnSection : theme.inkOnPanel,
                 juce::Justification::centredTop);
    }

private:
    const Theme& theme;
    juce::Colour ledColour;
    bool section;
};

// Indicator lamp with an optional legend to its right.
class Lamp : public juce::Component, public juce::SettableTooltipClient
{
public:
    Lamp(const Theme& t, juce::String text, juce::Colour c, bool onSection = true)
        : theme(t), label(std::move(text)), colour(c), section(onSection) { setInterceptsMouseClicks(false, false); }
    juce::String designLabel() const { return label; }
    juce::Colour designColour() const { return colour; }
    void set(float brightness)
    {
        brightness = juce::jlimit(0.0f, 1.0f, brightness);
        if (std::abs(brightness - level) > 0.02f) { level = brightness; repaint(); }
    }
    void paint(juce::Graphics& g) override
    {
        // The lit glow is twice the lamp radius, so the lamp sits 7 px in from the left and at the centre of its
        // row, leaving room for the glow inside these bounds.
        auto r = getLocalBounds().toFloat();
        const float rad = 3.2f;
        const juce::Point<float> c(r.getX() + 7.0f, r.getCentreY());
        const auto tint=getProperties().contains("layout.colour") ? juce::Colour::fromString("ff"+getProperties()["layout.colour"].toString().substring(1)) : colour;
        gl::meters::drawLamp(g, c, rad, tint.withMultipliedBrightness(0.35f + 0.65f * level), level > 0.05f);
        drawText(g,getProperties().getWithDefault("layout.label",label).toString(), r.withTrimmedLeft(17.0f),float(getProperties().getWithDefault("layout.font",8.0f)), section ? theme.inkOnSection : theme.inkOnPanel,
                 juce::Justification::centredLeft, false);
    }

private:
    const Theme& theme;
    juce::String label;
    juce::Colour colour;
    bool section;
    float level = 0.0f;
};

// Three exclusive keys (In / Comp / Gate) on a four-state routing parameter (Out, In, Comp key, Gate key).
class RouteKeys : public juce::Component
{
public:
    RouteKeys(const Theme& t, juce::RangedAudioParameter& p, juce::UndoManager* um)
        : in(t, "In", t.ledRed), comp(t, "Comp", t.ledAmber), gate(t, "Gate", t.ledGreen),
          attachment(p, [this](float v) { value = juce::roundToInt(v); refresh(); }, um)
    {
        LedKey* keys[] = { &in, &comp, &gate };
        for (int k = 0; k < 3; ++k)
        {
            keys[k]->setClickingTogglesState(false);
            keys[k]->onClick = [this, k] { attachment.setValueAsCompleteGesture(float(value == k + 1 ? 0 : k + 1)); };
            addAndMakeVisible(*keys[k]);
        }
        in.setTooltip("Section in the audio path");
        comp.setTooltip("Section filters the compressor key instead of the audio");
        gate.setTooltip("Section filters the gate key instead of the audio");
        attachment.sendInitialUpdate();
    }
    void resized() override
    {
        auto r = getLocalBounds();
        const int w = r.getWidth() / 3;
        in.setBounds(r.removeFromLeft(w));
        comp.setBounds(r.removeFromLeft(w));
        gate.setBounds(r);
    }

private:
    LedKey in, comp, gate;
    juce::ParameterAttachment attachment;
    int value = 0;
    void refresh()
    {
        in.setToggleState(value == 1, juce::dontSendNotification);
        comp.setToggleState(value == 2, juce::dontSendNotification);
        gate.setToggleState(value == 3, juce::dontSendNotification);
    }
};

// Hardware-style selector: one key steps through the options, a column of lamps shows the current one.
class StepSelector : public juce::Component
{
public:
    StepSelector(const Theme& t, juce::String text, juce::RangedAudioParameter& p, juce::StringArray options,
                 juce::UndoManager* um, juce::Colour led, bool onSection = true, juce::RangedAudioParameter* extraChoice = nullptr)
        : key(t, text, led, onSection), names(std::move(options)),
          attachment(p, [this](float v) { value = juce::roundToInt(v); refreshLamps(); }, um)
    {
        key.setClickingTogglesState(false);
        key.litOverride = [] { return false; };
        key.onClick = [this] {
            const int next=(selectedOption()+1)%names.size();
            if(extraAttachment)
            {
                if(next==names.size()-1){extraAttachment->setValueAsCompleteGesture(1);return;}
                extraAttachment->setValueAsCompleteGesture(0);
            }
            attachment.setValueAsCompleteGesture(float(next));
        };
        addAndMakeVisible(key);
        for (auto& n : names)
        {
            lamps.add(new Lamp(t, n, led, onSection));
            addAndMakeVisible(lamps.getLast());
        }
        if(extraChoice)
        {
            extraAttachment=std::make_unique<juce::ParameterAttachment>(*extraChoice,
                [this](float v){extraSelected=v>.5f;refreshLamps();},um);
            extraAttachment->sendInitialUpdate();
        }
        attachment.sendInitialUpdate();
    }
    int selectedOption() const {return extraSelected ? names.size()-1 : value;}
    std::function<juce::String(int)> tooltipFor;
    int keyWidth = 46;
    void resized() override
    {
        // Old saved layouts had six rows; migrate their height so the new HQ lamp cannot be clipped.
        if(extraAttachment && getHeight()<lamps.size()*14){setSize(getWidth(),lamps.size()*14);return;}
        // The key uses the same 34 px row as the plain keys beside it, so selectors line up with them. Lamp rows
        // are 14 px so each lamp's glow stays inside its own bounds.
        auto r = getLocalBounds();
        key.setBounds(r.removeFromLeft(keyWidth).withSizeKeepingCentre(keyWidth, 34));
        const int h = juce::jmax(14, r.getHeight() / juce::jmax(1, names.size()));
        for (auto* l : lamps) l->setBounds(r.removeFromTop(h).withTrimmedLeft(2));
    }

private:
    LedKey key;
    juce::StringArray names;
    juce::OwnedArray<Lamp> lamps;
    juce::ParameterAttachment attachment;
    int value = 0;
    bool extraSelected=false;
    std::unique_ptr<juce::ParameterAttachment> extraAttachment;
    void refreshLamps()
    {
        for (int i = 0; i < lamps.size(); ++i) lamps[i]->set(i == selectedOption() ? 1.0f : 0.0f);
        key.setTooltip(names[selectedOption()]);
    }
};

// Vertical LED bar (dBFS), with printed scale on the left when `scale` is set.
class LedBar : public juce::Component
{
public:
    LedBar(const Theme& t, bool printScale) : theme(t), scale(printScale) { setInterceptsMouseClicks(false, false); }
    static constexpr float kMarks[] = { 0, -2, -4, -6, -8, -10, -12, -18, -24, -32, -42 };
    void set(float db)
    {
        if (std::abs(db - level) > 0.3f) { level = db; repaint(); }
    }
    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        const int n = int(std::size(kMarks));
        const float step = r.getHeight() / float(n);
        for (int i = 0; i < n; ++i)
        {
            const float db = kMarks[i];
            const auto c = db >= -2 ? theme.ledRed : db >= -10 ? theme.ledAmber : theme.ledGreen;
            auto cell = juce::Rectangle<float>(r.getRight() - 9.0f, r.getY() + step * float(i), 9.0f, step).reduced(1.0f, 1.2f);
            const bool on = level >= db;
            g.setColour(on ? c : c.withMultipliedBrightness(0.22f));
            g.fillRoundedRectangle(cell, 1.2f);
            if (on) { g.setColour(c.withAlpha(0.25f)); g.fillRoundedRectangle(cell.expanded(1.5f), 2.0f); }
            if (scale)
                drawText(g, juce::String(int(db)), { r.getX(), cell.getY() - 2.0f, r.getWidth() - 12.0f, cell.getHeight() + 4.0f }, 7.5f,
                         theme.inkOnPanel, juce::Justification::centredRight, false);
        }
    }

private:
    const Theme& theme;
    bool scale;
    float level = -100.0f;
};

// Gate gain-reduction ladder (dB of attenuation).
class GateLeds : public juce::Component
{
public:
    explicit GateLeds(const Theme& t) : theme(t) { setInterceptsMouseClicks(false, false); }
    void set(float attenuationDb)
    {
        if (std::abs(attenuationDb - att) > 0.5f) { att = attenuationDb; repaint(); }
    }
    void paint(juce::Graphics& g) override
    {
        static constexpr int marks[] = { 30, 20, 15, 10, 5 };
        auto r = getLocalBounds().toFloat();
        const float w = r.getWidth() / 5.0f;
        for (int i = 0; i < 5; ++i)
        {
            auto cell = r.withX(r.getX() + w * float(i)).withWidth(w);
            const bool on = att >= float(marks[i]);
            gl::meters::drawLamp(g, { cell.getCentreX(), cell.getY() + 7.0f }, 3.0f, theme.ledGreen, on);
            drawText(g, "-" + juce::String(marks[i]), cell.withTrimmedTop(14.0f), 7.5f, theme.inkOnSection, juce::Justification::centredTop, false);
        }
    }

private:
    const Theme& theme;
    float att = 0.0f;
};

// Needle VU meter. needle() is the 0..1 position from GoodLookinUI's VU law.
class VuMeter : public juce::Component
{
public:
    explicit VuMeter(const Theme& t) : theme(t) { setInterceptsMouseClicks(false, false); }
    void setVu(float vu)
    {
        const float target = gl::meters::vuPositionForDb(vu);
        // VU ballistics: about 300 ms to 99 % at the 30 Hz UI rate.
        needle += (target - needle) * 0.28f;
        repaint();
    }
    void paint(juce::Graphics& g) override
    {
        gl::meters::drawVuMeter(g, getLocalBounds().toFloat(), needle, theme.flat ? gl::meters::VuFace::Black : gl::meters::VuFace::Cream);
    }

private:
    const Theme& theme;
    float needle = 0.0f;
};
} // namespace bluechannel::ui
