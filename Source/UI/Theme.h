// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

// Editor palettes. The skin changes presentation only: both skins expose the same controls and the DSP is
// unaffected (the circuit model always runs).

#include <juce_gui_basics/juce_gui_basics.h>

namespace bluechannel::ui
{
struct Theme
{
    bool flat = false;
    juce::Colour panelTop, panelBottom;     // blue front panel
    juce::Colour section, sectionEdge;      // grey control fields
    juce::Colour inkOnPanel, inkOnSection, inkDim;
    juce::Colour knobYellow, knobGrey, knobBlack;
    juce::Colour ledRed, ledGreen, ledAmber;
    juce::Colour accent;

    juce::String knobStyle() const { return flat ? "flat_FS" : "FS"; }
};

inline Theme analogueTheme()
{
    Theme t;
    t.flat = false;
    t.panelTop = juce::Colour(0xff3c4fb0);
    t.panelBottom = juce::Colour(0xff2d3c94);
    t.section = juce::Colour(0xffb9bdc4);
    t.sectionEdge = juce::Colour(0xff8a8f99);
    t.inkOnPanel = juce::Colour(0xfff1f3fb);
    t.inkOnSection = juce::Colour(0xff1d2230);
    t.inkDim = juce::Colour(0xff5d6372);
    t.knobYellow = juce::Colour(0xfff4d81c);
    t.knobGrey = juce::Colour(0xffd9dcdf);
    t.knobBlack = juce::Colour(0xff26292e);
    t.ledRed = juce::Colour(0xffff3b30);
    t.ledGreen = juce::Colour(0xff34d058);
    t.ledAmber = juce::Colour(0xffffc21a);
    t.accent = juce::Colour(0xfff4d81c);
    return t;
}

inline Theme digitalTheme()
{
    Theme t = analogueTheme();
    t.flat = true;
    t.panelTop = juce::Colour(0xff1d2552);
    t.panelBottom = juce::Colour(0xff161c40);
    t.section = juce::Colour(0xff2a3366);
    t.sectionEdge = juce::Colour(0xff3c4a8c);
    t.inkOnPanel = juce::Colour(0xffe6e9f8);
    t.inkOnSection = juce::Colour(0xffe6e9f8);
    t.inkDim = juce::Colour(0xff9aa3cc);
    t.knobGrey = juce::Colour(0xffaeb6d8);
    return t;
}
} // namespace bluechannel::ui
