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
        : juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox),
          theme(t), name(std::move(caption)), cap(c), section(onSection)
    {
        setRotaryParameters(-0.8f * juce::MathConstants<float>::pi, 0.8f * juce::MathConstants<float>::pi, true);
        setVelocityBasedMode(false);
        setMouseDragSensitivity(220);
        setTitle(name);
    }

    juce::String minLegend, maxLegend;
    std::function<juce::String()> extraLegend;   // optional second legend line (e.g. x3 range)

    juce::String getTooltip() override { return name + ": " + getTextFromValue(getValue()); }
    void mouseEnter(const juce::MouseEvent& e) override { juce::Slider::mouseEnter(e); repaint(); }
    void mouseExit(const juce::MouseEvent& e) override { juce::Slider::mouseExit(e); repaint(); }

    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        const juce::Colour ink = section ? theme.inkOnSection : theme.inkOnPanel;
        drawText(g, name, r.removeFromTop(12.0f), 9.5f, ink);
        auto legends = r.removeFromBottom(11.0f);
        const float d = juce::jmin(r.getWidth(), r.getHeight());
        auto knob = juce::Rectangle<float>(d, d).withCentre(r.getCentre());
        goodlookinui::Item item;
        item.id = name.toStdString();
        item.style = theme.knobStyle().toStdString();
        item.colour = hexOf(cap == Cap::Yellow ? theme.knobYellow : cap == Cap::Grey ? theme.knobGrey : theme.knobBlack).toStdString();
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
    juce::String name;
    Cap cap;
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
    std::function<bool()> litOverride;   // lamp state when it is not the toggle state

    void paintButton(juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat();
        auto key = juce::Rectangle<float>(20.0f, 12.0f).withCentre({ r.getCentreX(), r.getY() + 8.0f });
        const bool lit = litOverride ? litOverride() : getToggleState();
        gl::hardware::drawLedKey(g, key, ledColour, lit, down);
        if (over) { g.setColour(juce::Colour(0x22ffffff)); g.fillRoundedRectangle(key.expanded(2.5f), 3.0f); }
        drawText(g, getButtonText(), r.withTrimmedTop(16.0f), 8.5f, section ? theme.inkOnSection : theme.inkOnPanel,
                 juce::Justification::centredTop);
    }

private:
    const Theme& theme;
    juce::Colour ledColour;
    bool section;
};

// Indicator lamp with an optional legend to its right.
class Lamp : public juce::Component
{
public:
    Lamp(const Theme& t, juce::String text, juce::Colour c, bool onSection = true)
        : theme(t), label(std::move(text)), colour(c), section(onSection) { setInterceptsMouseClicks(false, false); }
    void set(float brightness)
    {
        brightness = juce::jlimit(0.0f, 1.0f, brightness);
        if (std::abs(brightness - level) > 0.02f) { level = brightness; repaint(); }
    }
    void paint(juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        const float rad = 3.6f;
        const juce::Point<float> c(r.getX() + rad + 1.0f, r.getCentreY());
        gl::meters::drawLamp(g, c, rad, colour.withMultipliedBrightness(0.35f + 0.65f * level), level > 0.05f);
        drawText(g, label, r.withTrimmedLeft(rad * 2 + 5.0f), 8.0f, section ? theme.inkOnSection : theme.inkOnPanel,
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
                 juce::UndoManager* um, juce::Colour led)
        : key(t, text, led), names(std::move(options)),
          attachment(p, [this](float v) { value = juce::roundToInt(v); refreshLamps(); }, um)
    {
        key.setClickingTogglesState(false);
        key.litOverride = [] { return false; };
        key.onClick = [this] { attachment.setValueAsCompleteGesture(float((value + 1) % names.size())); };
        addAndMakeVisible(key);
        for (auto& n : names)
        {
            lamps.add(new Lamp(t, n, led));
            addAndMakeVisible(lamps.getLast());
        }
        attachment.sendInitialUpdate();
    }
    std::function<juce::String(int)> tooltipFor;
    int keyWidth = 46;
    void resized() override
    {
        auto r = getLocalBounds();
        key.setBounds(r.removeFromLeft(keyWidth));
        const int h = juce::jmin(12, r.getHeight() / juce::jmax(1, names.size()));
        for (auto* l : lamps) l->setBounds(r.removeFromTop(h).withTrimmedLeft(2));
    }

private:
    LedKey key;
    juce::StringArray names;
    juce::OwnedArray<Lamp> lamps;
    juce::ParameterAttachment attachment;
    int value = 0;
    void refreshLamps()
    {
        for (int i = 0; i < lamps.size(); ++i) lamps[i]->set(i == value ? 1.0f : 0.0f);
        key.setTooltip(names[value]);
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
            gl::meters::drawLamp(g, { cell.getCentreX(), cell.getY() + 5.0f }, 3.4f, theme.ledGreen, on);
            drawText(g, "-" + juce::String(marks[i]), cell.withTrimmedTop(10.0f), 7.5f, theme.inkOnSection, juce::Justification::centredTop, false);
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
