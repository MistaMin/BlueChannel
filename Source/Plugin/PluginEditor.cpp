// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid

#include "PluginEditor.h"

#include "Model/ControlMap.h"

using namespace bluechannel;
using namespace bluechannel::ui;
namespace pid = bluechannel::params::id;

namespace
{
// Selector for editor-only state (meter source and calibration): same look as StepSelector.
class PropertySelector : public juce::Component
{
public:
    PropertySelector(const Theme& t, juce::String text, juce::StringArray options, juce::Colour led,
                     std::function<int()> getter, std::function<void(int)> setter, bool onSection = false)
        : key(t, text, led, onSection), names(std::move(options)), get(std::move(getter)), set(std::move(setter))
    {
        key.setClickingTogglesState(false);
        key.litOverride = [] { return false; };
        key.onClick = [this] { set((get() + 1) % names.size()); refresh(); };
        addAndMakeVisible(key);
        for (auto& n : names) { lamps.add(new Lamp(t, n, led, onSection)); addAndMakeVisible(lamps.getLast()); }
        refresh();
    }
    void refresh()
    {
        for (int i = 0; i < lamps.size(); ++i) lamps[i]->set(i == get() ? 1.0f : 0.0f);
    }
    void resized() override
    {
        auto r = getLocalBounds();
        key.setBounds(r.removeFromLeft(46));
        const int h = juce::jmin(12, r.getHeight() / juce::jmax(1, names.size()));
        for (auto* l : lamps) l->setBounds(r.removeFromTop(h).withTrimmedLeft(2));
    }

private:
    LedKey key;
    juce::StringArray names;
    juce::OwnedArray<Lamp> lamps;
    std::function<int()> get;
    std::function<void(int)> set;
};

// The OpenGrid mark: a small 3 x 3 grid of squares.
void drawMark(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    const float s = r.getHeight() / 3.0f;
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x)
        {
            g.setColour(c.withAlpha((x + y) % 2 ? 0.55f : 1.0f));
            g.fillRoundedRectangle(r.getX() + s * float(x) + 1.0f, r.getY() + s * float(y) + 1.0f, s - 2.0f, s - 2.0f, 1.0f);
        }
}
} // namespace

BlueChannelPanel::BlueChannelPanel(BlueChannelProcessor& p) : proc(p)
{
    theme = proc.skin() == 1 ? digitalTheme() : analogueTheme();
    setSize(kWidth, kHeight);
    auto& st = proc.apvts;
    auto* um = &proc.undo;
    using R = juce::Rectangle<int>;
    const auto& t = theme;

    // ---- top row ------------------------------------------------------------------------------------
    vu = add(std::make_unique<VuMeter>(t), R(22, 22, 220, 128));
    vuSource = add(std::make_unique<PropertySelector>(t, "Meter", juce::StringArray { "Input", "Comp GR", "Output" }, t.ledRed,
                                                      [this] { return proc.vuSource(); }, [this](int v) { proc.setVuSource(v); }),
                   R(24, 160, 100, 40));
    vuCal = add(std::make_unique<PropertySelector>(t, "0 VU", juce::StringArray { "+4 dBu", "+18 dBu" }, t.ledRed,
                                                   [this] { return proc.vuHighCal() ? 1 : 0; }, [this](int v) { proc.setVuHighCal(v == 1); }),
                R(136, 160, 104, 30));

    sections.push_back({ { 252, 12, 300, 210 }, "Input" });
    micGain = knob(pid::micGain, "Gain", Knob::Cap::Black, R(262, 32, 92, 112));
    lineGain = knob(pid::lineGain, "Gain", Knob::Cap::Black, R(262, 32, 92, 112));
    micGain->minLegend = "-14"; micGain->maxLegend = "+54";
    lineGain->minLegend = "-18"; lineGain->maxLegend = "+18";
    trim = knob(pid::trim, "Trim", Knob::Cap::Grey, R(360, 42, 72, 92));
    trim->minLegend = "0"; trim->maxLegend = "+20";
    add(std::make_unique<StepSelector>(t, "Select", *st.getParameter(pid::source), juce::StringArray { "Mic", "Line", "Inst" }, um, t.ledRed),
        R(442, 26, 104, 40));
    add(std::make_unique<StepSelector>(t, "Imped.", *st.getParameter(pid::impedance),
                                       juce::StringArray { "Low", "Classic", "Med", "High" }, um, t.ledRed),
        R(442, 74, 104, 52));
    key(pid::air, "Mic Air", t.ledRed, R(440, 150, 52, 34));
    key(pid::phase, "Phase", t.ledRed, R(494, 150, 52, 34));
    overload = add(std::make_unique<Lamp>(t, "Audio O/L", t.ledRed), R(266, 166, 90, 14));

    sections.push_back({ { 560, 12, 150, 210 }, "Filters" });
    key(pid::allEq, "All EQ", t.ledRed, R(566, 22, 40, 34));
    add(std::make_unique<RouteKeys>(t, *st.getParameter(pid::filterRoute), um), R(608, 22, 98, 34));
    lpf = knob(pid::lpf, "Low Pass", Knob::Cap::Yellow, R(566, 70, 66, 88));
    hpf = knob(pid::hpf, "High Pass", Knob::Cap::Yellow, R(638, 70, 66, 88));
    auto shortHz = [](double hz) {
        return hz >= 1000.0 ? juce::String(hz / 1000.0, hz >= 10000.0 ? 0 : 1) + "k" : juce::String(juce::roundToInt(hz));
    };
    lpf->minLegend = shortHz(model::controlmap::lowPassMinHz());
    lpf->maxLegend = shortHz(model::controlmap::lowPassMaxHz());
    hpf->minLegend = shortHz(model::controlmap::highPassMinHz());
    hpf->maxLegend = shortHz(model::controlmap::highPassMaxHz());

    sections.push_back({ { 718, 12, 388, 210 }, "Parametric EQ" });
    add(std::make_unique<RouteKeys>(t, *st.getParameter(pid::paramRoute), um), R(726, 22, 98, 34));
    auto qLegends = [](Knob* k) { k->minLegend = "0.85"; k->maxLegend = "2.9"; };
    { auto* k = knob(pid::lmfGain, "LMF", Knob::Cap::Grey, R(724, 62, 76, 100)); k->minLegend = "-15"; k->maxLegend = "+15"; }
    lmfFreq = knob(pid::lmfFreq, "Freq", Knob::Cap::Yellow, R(802, 66, 54, 84));
    qLegends(knob(pid::lmfQ, "Q", Knob::Cap::Yellow, R(858, 66, 52, 84)));
    key(pid::lmfX3, "x3", t.ledRed, R(803, 160, 52, 34));
    { auto* k = knob(pid::hmfGain, "HMF", Knob::Cap::Grey, R(914, 62, 76, 100)); k->minLegend = "-15"; k->maxLegend = "+15"; }
    hmfFreq = knob(pid::hmfFreq, "Freq", Knob::Cap::Yellow, R(992, 66, 54, 84));
    qLegends(knob(pid::hmfQ, "Q", Knob::Cap::Yellow, R(1048, 66, 52, 84)));
    key(pid::hmfX3, "x3", t.ledRed, R(993, 160, 52, 34));

    sections.push_back({ { 1114, 12, 276, 210 }, "Shelving EQ" });
    add(std::make_unique<RouteKeys>(t, *st.getParameter(pid::shelfRoute), um), R(1122, 22, 98, 34));
    { auto* k = knob(pid::lfGain, "LF", Knob::Cap::Grey, R(1120, 62, 76, 100)); k->minLegend = "-18"; k->maxLegend = "+18"; }
    lfFreq = knob(pid::lfFreq, "Freq", Knob::Cap::Yellow, R(1198, 66, 54, 84));
    key(pid::lfHigh, "Hi Range", t.ledRed, R(1199, 160, 52, 34));
    { auto* k = knob(pid::hfGain, "HF", Knob::Cap::Grey, R(1256, 62, 76, 100)); k->minLegend = "-18"; k->maxLegend = "+18"; }
    hfFreq = knob(pid::hfFreq, "Freq", Knob::Cap::Yellow, R(1334, 66, 54, 84));
    key(pid::hfHigh, "Hi Range", t.ledRed, R(1335, 160, 52, 34));

    outBar = add(std::make_unique<LedBar>(t, true), R(1400, 30, 40, 150));
    inBar = add(std::make_unique<LedBar>(t, false), R(1446, 30, 16, 150));
    key(pid::softLimit, "Soft Limit", t.ledRed, R(1470, 30, 74, 34), false);
    key(pid::link, "Stereo Link", t.ledGreen, R(1470, 76, 74, 34), false);
    keyLamp = add(std::make_unique<Lamp>(t, "Key input", t.ledGreen, false), R(1474, 122, 74, 14));

    // ---- bottom row ---------------------------------------------------------------------------------
    add(std::make_unique<StepSelector>(t, "Dynamics", *st.getParameter(pid::dynPosition), juce::StringArray { "Post-EQ", "Pre-EQ" }, um, t.ledRed),
        R(24, 330, 116, 30));
    key(pid::bypass, "Bypass", t.ledRed, R(150, 326, 52, 34), false);

    sections.push_back({ { 252, 230, 420, 208 }, "Compressor" });
    key(pid::compIn, "Comp In", t.ledRed, R(260, 240, 52, 34));
    add(std::make_unique<StepSelector>(t, "Vintage", *st.getParameter(pid::compType), juce::StringArray { "VCA", "Comp", "Lim" }, um, t.ledAmber),
        R(314, 238, 88, 40));
    key(pid::compListen, "Listen", t.ledRed, R(404, 240, 52, 34));
    key(pid::compKey, "Ext Key", t.ledGreen, R(458, 240, 52, 34));
    key(pid::autoRelease, "Auto Rel", t.ledRed, R(512, 240, 52, 34));
    key(pid::blend, "Blend", t.ledRed, R(566, 240, 52, 34));
    {
        const char* ids[] = { pid::ratio, pid::attack, pid::release, pid::makeup, pid::threshold, pid::mix };
        const char* names[] = { "Ratio", "Attack", "Release", "Makeup", "Threshold", "Mix" };
        const char* lo[] = { "1.5", "0.1ms", "0.1s", "0", "-28", "dry" };
        const char* hi[] = { "10", "100ms", "7s", "+20", "+12", "comp" };
        for (int i = 0; i < 6; ++i)
        {
            auto* k = knob(ids[i], names[i], Knob::Cap::Yellow, R(262 + i * 68, 300, 64, 92));
            k->minLegend = lo[i];
            k->maxLegend = hi[i];
        }
    }

    sections.push_back({ { 680, 230, 340, 208 }, "Expander / Gate" });
    key(pid::gateIn, "Gate In", t.ledRed, R(688, 240, 50, 34));
    key(pid::expand, "Expand", t.ledRed, R(740, 240, 50, 34));
    key(pid::gateListen, "Listen", t.ledRed, R(792, 240, 50, 34));
    key(pid::gateKey, "Ext Key", t.ledGreen, R(844, 240, 50, 34));
    key(pid::fastAttack, "Fast Att", t.ledRed, R(896, 240, 50, 34));
    key(pid::hysteresis, "Hyst", t.ledRed, R(948, 240, 50, 34));
    gateLeds = add(std::make_unique<GateLeds>(t), R(700, 282, 140, 22));
    {
        const char* ids[] = { pid::range, pid::gateThreshold, pid::hold, pid::gateRelease };
        const char* names[] = { "Range", "Threshold", "Hold", "Release" };
        const char* lo[] = { "0", "-40", "20ms", "0.1s" };
        const char* hi[] = { "80", "+10", "4s", "5s" };
        for (int i = 0; i < 4; ++i)
        {
            auto* k = knob(ids[i], names[i], Knob::Cap::Yellow, R(694 + i * 80, 312, 70, 96));
            k->minLegend = lo[i];
            k->maxLegend = hi[i];
        }
    }

    sections.push_back({ { 1028, 230, 180, 208 }, "De-Esser" });
    key(pid::deessIn, "De-Ess In", t.ledRed, R(1036, 240, 52, 34));
    key(pid::deessListen, "Listen", t.ledRed, R(1092, 240, 52, 34));
    deessActive = add(std::make_unique<Lamp>(t, "Active", t.ledRed), R(1150, 250, 54, 14));
    {
        auto* f = knob(pid::deessFreq, "Frequency", Knob::Cap::Yellow, R(1036, 300, 80, 96));
        f->minLegend = "2.1k"; f->maxLegend = "8.7k";
        auto* th = knob(pid::deessThreshold, "Threshold", Knob::Cap::Yellow, R(1120, 300, 80, 96));
        th->minLegend = "-30"; th->maxLegend = "+10";
    }

    sections.push_back({ { 1216, 230, 170, 208 }, "Output" });
    output = knob(pid::output, "Output", Knob::Cap::Grey, R(1226, 270, 96, 120));
    output->minLegend = "-60"; output->maxLegend = "+6";
    key(pid::mute, "Mute", t.ledRed, R(1328, 290, 52, 34));

    auto* os = add(std::make_unique<StepSelector>(t, "Oversample", *st.getParameter(pid::oversampling),
                                       juce::StringArray { "1x", "2x", "4x", "8x", juce::String::fromUTF8("\\( \xe1\x90\x96)/ 16x") }, um, t.ledRed),
        R(1400, 240, 150, 62));
    os->keyWidth = 58;
    os->resized();
    auto* off = add(std::make_unique<StepSelector>(t, "Offline", *st.getParameter(pid::offlineOversampling),
                                       juce::StringArray { "Same", "1x", "2x", "4x", "8x", juce::String::fromUTF8("\\( \xe1\x90\x96)/ 16x") }, um, t.ledAmber),
        R(1400, 310, 150, 74));
    off->keyWidth = 58;
    off->resized();
    skinKey = add(std::make_unique<LedKey>(t, "Digital skin", t.ledGreen, false), R(1400, 394, 70, 34));
    skinKey->setClickingTogglesState(false);
    skinKey->litOverride = [this] { return proc.skin() == 1; };
    skinKey->onClick = [this] { applySkin(proc.skin() == 1 ? 0 : 1); };
    skinKey->setTooltip("Switch between the hardware look and a flat look (display only)");

    tick();
}

BlueChannelPanel::~BlueChannelPanel()
{
    sliderAtts.clear();
    buttonAtts.clear();
}

BlueChannelPanel::Knob* BlueChannelPanel::knob(const char* paramId, const juce::String& caption, Knob::Cap cap,
                                                juce::Rectangle<int> bounds, bool onSection)
{
    auto* k = add(std::make_unique<Knob>(theme, caption, cap, onSection), bounds);
    sliderAtts.add(new SliderAtt(proc.apvts, paramId, *k));
    if (auto* p = proc.apvts.getParameter(paramId))
        k->setDoubleClickReturnValue(true, p->convertFrom0to1(p->getDefaultValue()));
    return k;
}

BlueChannelPanel::LedKey* BlueChannelPanel::key(const char* paramId, const juce::String& caption, juce::Colour led,
                                                 juce::Rectangle<int> bounds, bool onSection)
{
    auto* k = add(std::make_unique<LedKey>(theme, caption, led, onSection), bounds);
    buttonAtts.add(new ButtonAtt(proc.apvts, paramId, *k));
    if (auto* p = proc.apvts.getParameter(paramId)) k->setTooltip(p->getName(64));
    return k;
}

void BlueChannelPanel::applySkin(int skin)
{
    proc.setSkin(skin);
    theme = skin == 1 ? digitalTheme() : analogueTheme();
    repaint();
    for (auto* c : owned) c->repaint();
}

void BlueChannelPanel::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    if (theme.flat)
    {
        g.setColour(theme.panelTop);
        g.fillRect(r);
    }
    else
    {
        g.setGradientFill(juce::ColourGradient(theme.panelTop, 0, 0, theme.panelBottom, 0, r.getHeight(), false));
        g.fillRect(r);
        // Fine horizontal brushing on the anodised panel.
        for (int y = 0; y < int(r.getHeight()); y += 2)
        {
            const unsigned h = unsigned(y) * 2654435761u;
            g.setColour(juce::Colour((h >> 7) & 1 ? 0x07ffffff : 0x07000000));
            g.drawHorizontalLine(y, 0.0f, r.getWidth());
        }
    }
    for (auto& s : sections)
    {
        if (!theme.flat)
        {
            g.setColour(juce::Colour(0x55000000));
            g.fillRoundedRectangle(s.r.translated(0, 2), 7.0f);
        }
        g.setColour(theme.section);
        g.fillRoundedRectangle(s.r, 7.0f);
        g.setColour(theme.sectionEdge);
        g.drawRoundedRectangle(s.r, 7.0f, 1.0f);
        drawText(g, s.title, { s.r.getX() + 8, s.r.getBottom() - 16, s.r.getWidth() - 16, 13 }, 9.0f, theme.inkDim,
                 juce::Justification::centredRight);
    }
    // Brand block (bottom left).
    drawText(g, "Blue Channel", { 22, 236, 222, 34 }, 27.0f, theme.inkOnPanel, juce::Justification::centredLeft);
    drawText(g, "analogue-modelled channel strip", { 23, 268, 222, 14 }, 9.5f, theme.inkOnPanel.withAlpha(0.8f),
             juce::Justification::centredLeft, false);
    drawMark(g, { 24, 392, 24, 24 }, theme.inkOnPanel);
    drawText(g, "OpenGrid", { 54, 392, 120, 24 }, 15.0f, theme.inkOnPanel, juce::Justification::centredLeft);
    drawText(g, juce::String("v") + BLUECHANNEL_VERSION, { 150, 396, 90, 18 }, 8.5f, theme.inkOnPanel.withAlpha(0.6f),
             juce::Justification::centredRight, false);
    // Meter captions.
    drawText(g, "Out", { 1402, 14, 36, 12 }, 8.5f, theme.inkOnPanel);
    drawText(g, "In", { 1442, 14, 24, 12 }, 8.5f, theme.inkOnPanel);
    drawText(g, "dBFS", { 1400, 184, 64, 12 }, 8.0f, theme.inkOnPanel.withAlpha(0.7f), juce::Justification::centred, false);
}

void BlueChannelPanel::resized() {}

void BlueChannelPanel::tick()
{
    auto& tm = proc.telemetry();
    auto& st = proc.apvts;
    const int source = juce::roundToInt(st.getRawParameterValue(pid::source)->load());
    micGain->setVisible(source == 0);
    lineGain->setVisible(source != 0);
    lineGain->setEnabled(source == 1);
    trim->extraLegend = source == 2 ? std::function<juce::String()>([] { return juce::String("inst +7..+40"); }) : nullptr;

    // Legends that follow range switches.
    const bool lx3 = st.getRawParameterValue(pid::lmfX3)->load() > 0.5f, hx3 = st.getRawParameterValue(pid::hmfX3)->load() > 0.5f;
    lmfFreq->minLegend = lx3 ? "120" : "40";
    lmfFreq->maxLegend = lx3 ? "1.2k" : "400";
    hmfFreq->minLegend = hx3 ? "1.8k" : "600";
    hmfFreq->maxLegend = hx3 ? "18k" : "6k";
    const bool lfh = st.getRawParameterValue(pid::lfHigh)->load() > 0.5f, hfh = st.getRawParameterValue(pid::hfHigh)->load() > 0.5f;
    const int lfp = juce::roundToInt(st.getRawParameterValue(pid::lfFreq)->load());
    const int hfp = juce::roundToInt(st.getRawParameterValue(pid::hfFreq)->load());
    const auto lfText = params::lowShelfLabel(lfp, lfh), hfText = params::highShelfLabel(hfp, hfh);
    lfFreq->extraLegend = [lfText] { return lfText; };
    hfFreq->extraLegend = [hfText] { return hfText; };
    for (auto* k : { lmfFreq, hmfFreq, lfFreq, hfFreq, trim }) k->repaint();

    // Meters.
    const int chans = proc.activeChannels();
    float inPk = 0, outPk = 0, inMs = 0, outMs = 0;
    for (int ch = 0; ch < chans; ++ch)
    {
        inPk = std::max(inPk, tm.inputPeak[ch].exchange(0.0f));
        outPk = std::max(outPk, tm.outputPeak[ch].exchange(0.0f));
        inMs = std::max(inMs, tm.inputSquare[ch].load());
        outMs = std::max(outMs, tm.outputSquare[ch].load());
    }
    outBar->set(20.0f * std::log10(outPk + 1e-9f));
    inBar->set(20.0f * std::log10(inPk + 1e-9f));
    const float compGr = tm.compGr.exchange(0.0f);
    const float cal = proc.vuHighCal() ? 18.0f : 4.0f;
    auto dbuOf = [](float ms) { return 20.0f * std::log10(std::sqrt(ms) * float(model::kFullScalePeak) / float(model::kDbuVolts) + 1e-9f); };
    float vuValue = -30.0f;
    switch (proc.vuSource())
    {
        case 0: vuValue = dbuOf(inMs) - cal; break;
        case 1: vuValue = -compGr; break;
        default: vuValue = dbuOf(outMs) - cal; break;
    }
    vu->setVu(vuValue);
    static_cast<PropertySelector*>(vuSource)->refresh();
    static_cast<PropertySelector*>(vuCal)->refresh();
    gateLeds->set(tm.gateGr.exchange(0.0f));
    deessActive->set(tm.deessLed.exchange(0.0f) / 1.5f);
    if (tm.overload.exchange(false)) overloadHold = 1.0f;
    overload->set(overloadHold);
    overloadHold = std::max(0.0f, overloadHold - 0.05f);
    keyLamp->set(proc.sidechainConnected() ? 1.0f : 0.0f);
    skinKey->repaint();
}

BlueChannelEditor::BlueChannelEditor(BlueChannelProcessor& p) : AudioProcessorEditor(p), proc(p), panel(p)
{
    addAndMakeVisible(panel);
    setResizable(true, true);
    const double aspect = double(BlueChannelPanel::kWidth) / double(BlueChannelPanel::kHeight);
    getConstrainer()->setFixedAspectRatio(aspect);
    setResizeLimits(int(BlueChannelPanel::kWidth * 0.5), int(BlueChannelPanel::kHeight * 0.5), BlueChannelPanel::kWidth * 2,
                    BlueChannelPanel::kHeight * 2);
    const double scale = double(proc.apvts.state.getProperty("uiscale", 0.85));
    setSize(int(BlueChannelPanel::kWidth * scale), int(BlueChannelPanel::kHeight * scale));
    startTimerHz(30);
}

BlueChannelEditor::~BlueChannelEditor() { stopTimer(); }

void BlueChannelEditor::resized()
{
    const float s = float(getWidth()) / float(BlueChannelPanel::kWidth);
    panel.setTransform(juce::AffineTransform::scale(s));
    panel.setBounds(0, 0, BlueChannelPanel::kWidth, BlueChannelPanel::kHeight);
    proc.apvts.state.setProperty("uiscale", s, nullptr);
}
