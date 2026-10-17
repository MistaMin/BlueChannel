// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

// Developer mode (builds configured with -DBLUECHANNEL_DEV_EDITOR=ON only): place and size the knobs on the
// running editor and save the result to Designs/knobs.csv in the source tree.
//
//   click        select a knob (also selectable in the inspector)
//   drag         move it (snaps to a 2-unit grid; hold Alt for 1)
//   drag corner  resize it (bottom-right handle)
//   arrows       nudge 1 unit, Shift+arrows 10 units; Cmd/Ctrl+Z undoes the last move or resize
//
// The inspector below the panel (GoodLookinUI Studio) edits label, style, cap colour, caption size and exact
// X/Y/W/H. Every change is written to the design file shortly after it happens (GoodLookinUI DesignAutosave);
// the file is baked into the next build, so release builds use the saved layout.

#if BLUECHANNEL_DEV_EDITOR
#include "PluginEditor.h"

#include <DesignFiles.h>
#include <Studio.h>

#include <sstream>

namespace bluechannel::ui
{
class DevLayoutOverlay : public juce::Component
{
public:
    explicit DevLayoutOverlay(BlueChannelPanel& p) : panel(p) { setWantsKeyboardFocus(true); }

    std::function<void(Knob*)> onSelect;
    std::function<void()> onEdited;            // a move or resize finished (or was undone)

    void select(Knob* k) { selected = k; repaint(); }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0x18000000));
        for (auto* k : panel.allKnobs())
        {
            if (!k->isVisible()) continue;
            const auto b = k->getBounds().toFloat();
            const bool sel = k == selected;
            g.setColour(sel ? juce::Colour(0xffffb000) : juce::Colour(0x9900e0ff));
            const float dash[] = { 4.0f, 3.0f };
            juce::Path outline;
            outline.addRectangle(b);
            juce::PathStrokeType(sel ? 1.6f : 1.0f).createDashedStroke(outline, outline, dash, 2);
            g.fillPath(outline);
            if (sel)
            {
                g.fillRect(handle(b));
                const auto text = juce::String(k->design.id) + "  " + juce::String(int(b.getX())) + ", " + juce::String(int(b.getY()))
                                  + "  " + juce::String(int(b.getWidth())) + " x " + juce::String(int(b.getHeight()));
                g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
                auto tag = juce::Rectangle<float>(b.getX(), b.getY() - 15.0f, juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), text) + 8.0f, 13.0f);
                if (tag.getY() < 0) tag.setY(b.getBottom() + 2.0f);
                g.setColour(juce::Colour(0xe0101418));
                g.fillRoundedRectangle(tag, 2.0f);
                g.setColour(juce::Colour(0xffffb000));
                g.drawText(text, tag, juce::Justification::centred);
            }
        }
        g.setColour(juce::Colour(0xffffb000));
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText("DEVELOPER MODE  -  drag to move, corner to resize, arrows to nudge, Cmd/Ctrl+Z to undo", getLocalBounds().removeFromBottom(16).reduced(8, 0),
                   juce::Justification::centredLeft);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        grabKeyboardFocus();
        if (selected && handle(selected->getBounds().toFloat()).expanded(3.0f).contains(e.position)) mode = Mode::Resize;
        else
        {
            selected = knobAt(e.getPosition());
            mode = selected ? Mode::Move : Mode::None;
            if (onSelect) onSelect(selected);
        }
        if (selected) { start = selected->getBounds(); pushUndo(); }
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (!selected || mode == Mode::None) return;
        const int grid = e.mods.isAltDown() ? 1 : 2;
        auto snap = [grid](int v) { return grid * juce::roundToInt(float(v) / float(grid)); };
        const auto d = e.getOffsetFromDragStart();
        if (mode == Mode::Move)
            selected->setTopLeftPosition(snap(start.getX() + d.x), snap(start.getY() + d.y));
        else
            selected->setSize(std::max(20, snap(start.getWidth() + d.x)), std::max(20, snap(start.getHeight() + d.y)));
        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        if (selected && mode != Mode::None && selected->getBounds() != start && onEdited) onEdited();
        else if (!undo.empty() && selected && selected->getBounds() == start) undo.pop_back();   // a click, not an edit
        mode = Mode::None;
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0))
        {
            if (undo.empty()) return true;
            panel.applyDesignCsv(undo.back());
            undo.pop_back();
            if (onEdited) onEdited();
            repaint();
            return true;
        }
        if (!selected) return false;
        const int step = key.getModifiers().isShiftDown() ? 10 : 1;
        juce::Point<int> d;
        if (key.isKeyCode(juce::KeyPress::leftKey)) d = { -step, 0 };
        else if (key.isKeyCode(juce::KeyPress::rightKey)) d = { step, 0 };
        else if (key.isKeyCode(juce::KeyPress::upKey)) d = { 0, -step };
        else if (key.isKeyCode(juce::KeyPress::downKey)) d = { 0, step };
        else if (key.isKeyCode(juce::KeyPress::escapeKey)) { selected = nullptr; repaint(); return true; }
        else return false;
        pushUndo();
        selected->setTopLeftPosition(selected->getPosition() + d);
        if (onEdited) onEdited();
        repaint();
        return true;
    }

private:
    enum class Mode { None, Move, Resize };
    BlueChannelPanel& panel;
    Knob* selected = nullptr;
    Mode mode = Mode::None;
    juce::Rectangle<int> start;
    std::vector<std::string> undo;

    static juce::Rectangle<float> handle(juce::Rectangle<float> b) { return { b.getRight() - 8.0f, b.getBottom() - 8.0f, 8.0f, 8.0f }; }
    Knob* knobAt(juce::Point<int> p) const
    {
        const auto& ks = panel.allKnobs();
        for (auto it = ks.rbegin(); it != ks.rend(); ++it)
            if ((*it)->isVisible() && (*it)->getBounds().contains(p)) return *it;
        return nullptr;
    }
    void pushUndo()
    {
        undo.push_back(panel.designCsv());
        if (undo.size() > 200) undo.erase(undo.begin());
    }
};

// Owns the overlay, the inspector and the autosaver. Lives in the editor; toggled by the panel's Dev key.
class DevTools
{
public:
    static constexpr int kInspectorHeight = 116;

    DevTools(BlueChannelPanel& p, juce::Component& editor)
        : panel(p), overlay(p), autosave(juce::File(BLUECHANNEL_DESIGN_DIR))
    {
        panel.addChildComponent(overlay);
        editor.addChildComponent(studio);
        for (auto* k : panel.allKnobs())
        {
            auto item = k->design;
            const auto b = k->getBounds();
            item.x = float(b.getX()); item.y = float(b.getY()); item.width = float(b.getWidth()); item.height = float(b.getHeight());
            studio.add(item, *k, [k](const goodlookinui::Item& i) { k->applyDesign(i); }, item.id + "  (" + item.label + ")");
        }
        overlay.onSelect = [this](Knob* k) { if (k) studio.select(k); };
        overlay.onEdited = [this] { syncFromPanel(); };
        studio.onSelect = [this](juce::Component* c) { overlay.select(dynamic_cast<Knob*>(c)); };
        studio.onChanged = [this] { save(studio.toCsv()); overlay.repaint(); };
        autosave.onSaved = [this](const goodlookinui::juce_adapter::DesignAutosave::Event& e) {
            studio.setStatus(e.ok ? (e.wrote ? "Saved " + e.file.getFullPathName() : "Up to date: " + e.file.getFullPathName())
                                  : "Save failed: " + e.error);
        };
    }

    bool isActive() const { return active; }
    void setActive(bool on)
    {
        active = on;
        overlay.setBounds(panel.getLocalBounds());
        overlay.setVisible(on);
        studio.setVisible(on);
        if (on)
        {
            overlay.toFront(true);
            studio.fromCsv(panel.designCsv());
            studio.setStatus("Design folder: " + autosave.getFolder().getFullPathName());
        }
        else
            autosave.flush();
    }
    void layout(juce::Rectangle<int> inspectorArea) { studio.setBounds(inspectorArea); }

private:
    BlueChannelPanel& panel;
    DevLayoutOverlay overlay;
    goodlookinui::juce_adapter::Studio studio;
    goodlookinui::juce_adapter::DesignAutosave autosave;
    bool active = false;

    void syncFromPanel()
    {
        const auto csv = panel.designCsv();
        studio.fromCsv(csv);        // refresh the inspector's fields with the new bounds
        save(csv);
    }
    void save(const std::string& csv) { autosave.request("knobs.csv", csv); }
};
} // namespace bluechannel::ui
#else
namespace bluechannel::ui
{
// Release builds: no developer tools.
class DevTools
{
public:
    static constexpr int kInspectorHeight = 0;
    bool isActive() const { return false; }
};
} // namespace bluechannel::ui
#endif
