// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once

#include "PluginProcessor.h"
#include "Widgets.h"
#include "DigitalGraphs.h"
#include "EditorLayout.h"

namespace bluechannel::ui
{
// Dedicated digital presentation. Attachments share the hardware panel's parameters, undo and gestures.
class DigitalPanel : public juce::Component
{
    struct Look : juce::LookAndFeel_V4
    {
        Look()
        {
            setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffdbe8ff));
            setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
            setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff101e2e));
            setColour(juce::ComboBox::textColourId, juce::Colour(0xffdbe8ff));
            setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a5679));
            setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff101e2e));
            setColour(juce::PopupMenu::textColourId, juce::Colour(0xffdbe8ff));
            setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff23465a));
            setColour(juce::TextButton::buttonColourId, juce::Colour(0xff101e2e));
            setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff173d50));
            setColour(juce::TextButton::textColourOffId, juce::Colour(0xffdbe8ff));
            setColour(juce::TextButton::textColourOnId, juce::Colour(0xff39d0ff));
        }
        juce::Label* createSliderTextBox(juce::Slider& slider) override
        {
            auto* l=juce::LookAndFeel_V4::createSliderTextBox(slider);
            l->setFont(juce::FontOptions(float(slider.getProperties().getWithDefault("layout.font",10.0f)),juce::Font::bold));
            l->setColour(juce::Label::textColourId,juce::Colour(0xff39d0ff));
            return l;
        }
        void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float value,float,float,juce::Slider& slider) override
        {
            goodlookinui::Item item;
            item.style=slider.getProperties().getWithDefault("layout.style","Neo").toString().toStdString();
            item.colour=slider.getProperties().getWithDefault("layout.colour","#39d0ff").toString().toStdString();
            const float d=float(juce::jmin(w,h));
            gl::drawKnob(g,juce::Rectangle<float>(d,d).withCentre({float(x)+float(w)*0.5f,float(y)+float(h)*0.5f}),value,item,slider.isEnabled(),juce::Colour(0xff0e1a2a));
        }
        void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                              juce::Slider::SliderStyle style, juce::Slider& s) override
        {
            auto hex=s.getProperties().getWithDefault("layout.colour","#39d0ff").toString();
            const auto accent=juce::Colour::fromString("ff"+hex.substring(1));
            g.setColour(juce::Colour(0xff24364b));
            if(style==juce::Slider::LinearVertical)
            {
                const float cx=float(x)+float(w)*0.5f;
                g.fillRoundedRectangle(cx-2,float(y),4,float(h),2);
                g.setColour(s.isEnabled() ? accent : juce::Colour(0xff53657b));
                g.fillRoundedRectangle(cx-2,pos,4,juce::jmax(0.0f,float(y+h)-pos),2);
                g.fillEllipse(cx-4,pos-4,8,8);
            }
            else
            {
                const float cy=float(y)+float(h)*0.5f;
                g.fillRoundedRectangle(float(x),cy-2,float(w),4,2);
                g.setColour(s.isEnabled() ? accent : juce::Colour(0xff53657b));
                g.fillRoundedRectangle(float(x),cy-2,juce::jmax(0.0f,pos-float(x)),4,2);
                g.fillEllipse(pos-4,cy-4,8,8);
            }
        }
        void drawToggleButton(juce::Graphics& g, juce::ToggleButton& b, bool over, bool down) override
        {
            const auto r = b.getLocalBounds().toFloat().reduced(1);
            const bool on = b.getToggleState();
            const auto accent=juce::Colour::fromString("ff"+b.getProperties().getWithDefault("layout.colour","#39d0ff").toString().substring(1));
            g.setColour(juce::Colour(on ? 0xff173d50 : 0xff101e2e).brighter(over || down ? 0.15f : 0.0f));
            g.fillRoundedRectangle(r, 3);
            g.setColour(on ? accent : juce::Colour(0xff3a5679));
            g.drawRoundedRectangle(r, 3, 1);
            drawText(g, b.getButtonText(), r.reduced(3, 0),float(b.getProperties().getWithDefault("layout.font",9)),on ? accent : juce::Colour(0xffdbe8ff));
        }
    };
    using R = juce::Rectangle<int>;
    using State = juce::AudioProcessorValueTreeState;
    struct Card { R bounds; juce::String title; };
    Look look; // Must outlive controls and popup menus using it.
    BlueChannelProcessor& proc;
    juce::OwnedArray<juce::Component> controls;
    juce::OwnedArray<State::SliderAttachment> sliders;
    juce::OwnedArray<State::ButtonAttachment> buttons;
    juce::OwnedArray<State::ComboBoxAttachment> choices;
    std::vector<Card> cards;
    juce::Slider *mic = nullptr, *line = nullptr;
    juce::Label *micLabel = nullptr, *lineLabel = nullptr;
    juce::Label *meters[5] {};
    juce::TextButton analogue { "ANALOGUE" };
    juce::ComboBox *meterSource = nullptr, *meterCal = nullptr;
    juce::Label* vuReadout = nullptr;
    juce::TextButton pageButton { "ALL CONTROLS" }, overviewButton { "OVERVIEW" }, dynamicsButton { "GATE / DE-ESSER" };
    Theme hqTheme=digitalTheme();
    Lamp hqLamp {hqTheme,juce::String::fromUTF8("\\( \xe1\x90\x96)/ HQ"),hqTheme.ledAmber,false};
    juce::ComboBox* offlineMenu=nullptr;
    bool offlineHQSelected=false;
    int offlineFactor=0;
    std::unique_ptr<juce::ParameterAttachment> offlineFactorAttachment,hqAttachment;
    void refreshOfflineMenu()
    {
        offlineMenu->setSelectedItemIndex(offlineHQSelected?6:offlineFactor,juce::dontSendNotification);
        hqLamp.set(offlineHQSelected?1.f:0.f);
    }
    bool details = false;
    int detailEnd = 0, headerEnd = 0;
    juce::Slider *mainMic = nullptr, *mainLine = nullptr;
    juce::Slider *mainHP = nullptr, *mainLP = nullptr;
    DigitalEqGraph* eqGraph = nullptr;
    DigitalCompGraph* compGraph = nullptr;
    juce::Label* mainMeters[4] {};

    void showPage()
    {
        for(int i=0;i<controls.size();++i) controls[i]->setVisible(i<detailEnd ? details : i<headerEnd || !details);
        pageButton.setToggleState(details,juce::dontSendNotification);
        overviewButton.setToggleState(!details,juce::dontSendNotification);
        dynamicsButton.setVisible(!details);
#if BLUECHANNEL_DEV_EDITOR
        if(auto* dev=findChildWithID("digital-dev")) dev->setVisible(true);
#endif
        repaint();
    }
    juce::Slider* hero(const char* id, R bounds)
    {
        auto* s=add(std::make_unique<juce::Slider>(juce::Slider::RotaryVerticalDrag,juce::Slider::TextBoxBelow),bounds);
        s->setComponentID(id);s->setTitle(proc.apvts.getParameter(id)->getName(64));
        s->setTextBoxStyle(juce::Slider::TextBoxBelow,false,110,20);
        s->setRotaryParameters(juce::MathConstants<float>::pi*1.2f,juce::MathConstants<float>::pi*2.8f,true);
        sliders.add(new State::SliderAttachment(proc.apvts,id,*s));
        auto* p=proc.apvts.getParameter(id);s->setDoubleClickReturnValue(true,p->convertFrom0to1(p->getDefaultValue()));
        return s;
    }

    template<class T> T* add(std::unique_ptr<T> c, R r)
    {
        auto* p = c.get(); addAndMakeVisible(p); p->setBounds(r); controls.add(c.release()); return p;
    }
    juce::Label* label(juce::String name, R r)
    {
        auto* l = add(std::make_unique<juce::Label>(juce::String(), name), r);
        l->setFont(juce::FontOptions(9.5f));
        l->setColour(juce::Label::textColourId, juce::Colour(0xffdbe8ff));
        return l;
    }
    juce::Slider* slider(const char* id, const char* name, int x, int y, int w)
    {
        const int caption = w <= 224 ? 60 : 72;
        const int valueWidth = w <= 224 ? 60 : 74;
        label(name, {x, y, caption, 24});
        auto* s = add(std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight), {x + caption + 2, y, w - caption - 2, 24});
        s->setComponentID(id); s->setTitle(name); s->setTooltip(proc.apvts.getParameter(id)->getName(64));
        s->setTextBoxStyle(juce::Slider::TextBoxRight, false, valueWidth, 20);
        sliders.add(new State::SliderAttachment(proc.apvts, id, *s));
        auto* p = proc.apvts.getParameter(id);
        s->setDoubleClickReturnValue(true, p->convertFrom0to1(p->getDefaultValue()));
        return s;
    }
    void toggle(const char* id, const char* name, int x, int y, int w)
    {
        auto* b = add(std::make_unique<juce::ToggleButton>(name), {x,y,w,22});
        b->setComponentID(id); b->setTitle(name);
        buttons.add(new State::ButtonAttachment(proc.apvts, id, *b));
    }
    void choice(const char* id, const char* name, int x, int y, int w)
    {
        label(name, {x,y,72,24});
        auto* c = add(std::make_unique<juce::ComboBox>(), {x+74,y+2,w-74,20});
        c->setComponentID(id); c->setTitle(name);
        c->addItemList(proc.apvts.getParameter(id)->getAllValueStrings(), 1);
        if(juce::String(id)==bluechannel::params::id::offlineOversampling)
        {
            offlineMenu=c;c->addItem(juce::String::fromUTF8("\\( \xe1\x90\x96)/ HQ"),7);
            offlineFactorAttachment=std::make_unique<juce::ParameterAttachment>(*proc.apvts.getParameter(id),[this](float v){offlineFactor=juce::roundToInt(v);refreshOfflineMenu();},&proc.undo);
            hqAttachment=std::make_unique<juce::ParameterAttachment>(*proc.apvts.getParameter(bluechannel::params::id::hq),[this](float v){offlineHQSelected=v>.5f;refreshOfflineMenu();},&proc.undo);
            c->onChange=[this] {
                const int selected=offlineMenu->getSelectedItemIndex();
                if(selected<0)return;
                if(selected==6)hqAttachment->setValueAsCompleteGesture(1);
                else {hqAttachment->setValueAsCompleteGesture(0);offlineFactorAttachment->setValueAsCompleteGesture(float(selected));}
            };
            offlineFactorAttachment->sendInitialUpdate();hqAttachment->sendInitialUpdate();
            refreshOfflineMenu();
        }
        else choices.add(new State::ComboBoxAttachment(proc.apvts, id, *c));
    }
public:
    explicit DigitalPanel(BlueChannelProcessor& p) : proc(p)
    {
        namespace id = bluechannel::params::id;
        setLookAndFeel(&look);
        setComponentID("digital-panel");
        cards = {{{12,40,300,194},"INPUT"}, {{324,40,200,194},"FILTERS"}, {{536,40,400,194},"PARAMETRIC EQ"},
                 {{948,40,300,194},"SHELVING EQ"}, {{1260,40,288,194},"LEVELS / OUTPUT"},
                 {{12,246,480,192},"COMPRESSOR"}, {{504,246,400,192},"EXPANDER / GATE"},
                 {{916,246,300,192},"DE-ESSER / ROUTING"}, {{1228,246,320,192},"PROCESSING"}};
        choice(id::source,"Source",22,66,280); choice(id::impedance,"Impedance",22,92,280);
        mic = slider(id::micGain,"Mic gain",22,118,280); micLabel = static_cast<juce::Label*>(controls[controls.size()-2]);
        line = slider(id::lineGain,"Line gain",22,118,280); lineLabel = static_cast<juce::Label*>(controls[controls.size()-2]);
        slider(id::trim,"Trim",22,146,280);
        toggle(id::air,"AIR",22,190,130); toggle(id::phase,"PHASE",162,190,140);
        choice(id::filterRoute,"Route",334,66,180);
        slider(id::hpf,"High pass",334,102,180); slider(id::lpf,"Low pass",334,134,180);
        toggle(id::allEq,"ALL EQ",334,190,180);
        choice(id::paramRoute,"Route",546,66,380);
        slider(id::lmfGain,"LMF gain",546,94,182); slider(id::hmfGain,"HMF gain",744,94,182);
        slider(id::lmfFreq,"Frequency",546,122,182); slider(id::hmfFreq,"Frequency",744,122,182);
        slider(id::lmfQ,"Q",546,150,182); slider(id::hmfQ,"Q",744,150,182);
        toggle(id::lmfX3,"LMF x3",546,190,182); toggle(id::hmfX3,"HMF x3",744,190,182);
        choice(id::shelfRoute,"Route",958,66,280);
        slider(id::lfGain,"LF gain",958,94,280); choice(id::lfFreq,"LF freq",958,120,280);
        slider(id::hfGain,"HF gain",958,146,280); choice(id::hfFreq,"HF freq",958,172,280);
        toggle(id::lfHigh,"LF HI RANGE",958,204,134); toggle(id::hfHigh,"HF HI RANGE",1102,204,136);
        for (int i=0; i<5; ++i) {meters[i] = label("",{1270,66+i*20,268,20});meters[i]->getProperties().set("layout.dynamic",true);}
        slider(id::output,"Output",1270,170,268);
        toggle(id::mute,"MUTE",1270,204,80); toggle(id::softLimit,"LIMIT",1360,204,80); toggle(id::link,"LINK",1450,204,88);
        choice(id::compType,"Type",22,272,224);
        slider(id::ratio,"Ratio",22,300,224); slider(id::threshold,"Threshold",258,300,224);
        slider(id::attack,"Attack",22,328,224); slider(id::release,"Release",258,328,224);
        slider(id::makeup,"Makeup",22,356,224); slider(id::mix,"Mix",258,356,224);
        const char* ci[] = {id::compIn,id::autoRelease,id::compKey,id::compListen,id::blend};
        const char* cn[] = {"ACTIVE","AUTO REL","EXT KEY","LISTEN","BLEND"};
        for(int i=0;i<5;++i) toggle(ci[i],cn[i],22+i*92,402,86);
        slider(id::gateThreshold,"Threshold",514,272,380); slider(id::range,"Range",514,300,380);
        slider(id::hold,"Hold",514,328,380); slider(id::gateRelease,"Release",514,356,380);
        const char* gi[] = {id::gateIn,id::expand,id::gateKey,id::gateListen,id::fastAttack,id::hysteresis};
        const char* gn[] = {"ACTIVE","EXPAND","EXT KEY","LISTEN","FAST ATT","HYST"};
        for(int i=0;i<6;++i) toggle(gi[i],gn[i],514+i*64,402,60);
        slider(id::deessFreq,"Frequency",926,272,280); slider(id::deessThreshold,"Threshold",926,300,280);
        toggle(id::deessIn,"ACTIVE",926,336,134); toggle(id::deessListen,"LISTEN",1070,336,136);
        choice(id::dynPosition,"Dynamics",926,376,280);
        choice(id::oversampling,"Playback",1238,272,300); choice(id::offlineOversampling,"Offline",1238,304,300);
        toggle(id::bypass,"BYPASS",1238,350,300);
        label("Circuit model at every factor",{1238,390,300,24});
        detailEnd = controls.size();
        label("Meter", {700,8,48,24});
        meterSource = add(std::make_unique<juce::ComboBox>(), {750,10,125,20});
        meterSource->addItemList({"Input", "Comp GR", "Output"},1);
        meterSource->onChange = [this] { proc.setVuSource(meterSource->getSelectedItemIndex()); };
        label("0 VU", {890,8,42,24});
        meterCal = add(std::make_unique<juce::ComboBox>(), {934,10,110,20});
        meterCal->addItemList({"+4 dBu", "+18 dBu"},1);
        meterCal->onChange = [this] { proc.setVuHighCal(meterCal->getSelectedItemIndex()==1); };
        vuReadout = label("", {1060,8,60,24});
        addAndMakeVisible(hqLamp);hqLamp.setBounds(1206,8,88,24);
        hqLamp.setComponentID(id::hq);
        hqLamp.setTooltip("Full component solve is selected through Offline in All Controls. Playback uses Standard.");
        addAndMakeVisible(analogue); analogue.setBounds(1438,8,110,24);
        analogue.onClick = [this] { if(onAnalogue) onAnalogue(); };
        addAndMakeVisible(pageButton);pageButton.setBounds(458,8,190,24);
        addAndMakeVisible(overviewButton);overviewButton.setBounds(320,8,126,24);
        overviewButton.setComponentID("digital-overview");
        overviewButton.onClick=[this] {details=false;showPage();};
        addAndMakeVisible(dynamicsButton);dynamicsButton.setBounds(24,356,210,24);
        dynamicsButton.onClick=[this] {details=true;showPage();};
        pageButton.setComponentID("digital-controls");
        pageButton.onClick=[this] {details=true;showPage();};
        headerEnd=controls.size();
        choice(id::source,"Source",24,74,210);
        mainMic=hero(id::micGain,{60,112,140,140});mainLine=hero(id::lineGain,{60,112,140,140});
        slider(id::trim,"Trim",24,278,210);
        choice(id::impedance,"Impedance",24,316,210);
        toggle(id::air,"AIR",24,402,100);toggle(id::phase,"PHASE",134,402,100);
        choice(id::compType,"Type",264,74,296);
        compGraph=add(std::make_unique<DigitalCompGraph>(proc),{264,110,296,178});compGraph->setComponentID("compressor-curve");
        slider(id::attack,"Attack",264,304,296);slider(id::release,"Release",264,336,296);slider(id::makeup,"Makeup",264,368,296);
        toggle(id::compIn,"ACTIVE",264,402,90);toggle(id::autoRelease,"AUTO REL",364,402,90);toggle(id::compKey,"EXT KEY",464,402,96);
        choice(id::filterRoute,"Filters",590,74,205);choice(id::paramRoute,"Mids",809,74,205);choice(id::shelfRoute,"Shelves",1028,74,218);
        eqGraph=add(std::make_unique<DigitalEqGraph>(proc),{590,110,656,242});eqGraph->setComponentID("eq-spectrum");
        mainHP=slider(id::hpf,"High pass",590,366,310);mainLP=slider(id::lpf,"Low pass",928,366,318);
        toggle(id::allEq,"EQ ACTIVE",590,402,110);toggle(id::lfHigh,"LF RANGE",710,402,122);
        toggle(id::lmfX3,"LMF x3",842,402,122);toggle(id::hmfX3,"HMF x3",974,402,122);toggle(id::hfHigh,"HF RANGE",1106,402,140);
        hero(id::output,{1320,74,160,150});
        for(int i=0;i<4;++i) {mainMeters[i]=label("",{1284,244+i*30,250,24});mainMeters[i]->getProperties().set("layout.dynamic",true);}
        toggle(id::mute,"MUTE",1284,366,76);toggle(id::softLimit,"LIMIT",1370,366,76);toggle(id::link,"LINK",1456,366,76);
        toggle(id::bypass,"BYPASS",1284,402,248);
#if BLUECHANNEL_DEV_EDITOR
        auto* dev=add(std::make_unique<juce::TextButton>("DEV"),{1310,8,110,24});
        dev->setComponentID("digital-dev");
        dev->onClick=[this] {if(onDevToggle) onDevToggle();};
#endif
        showPage();
        update(0,0,0,0,0,false,false);
    }
    ~DigitalPanel() override { juce::PopupMenu::dismissAllActiveMenus(); setLookAndFeel(nullptr); }
    std::function<void()> onAnalogue;
    std::function<void()> onDevToggle;
    void showControls(bool all) {details=all;showPage();}
    bool showingControls() const {return details;}
    std::vector<LayoutTarget> layoutTargets()
    {
        std::vector<LayoutTarget> result;
        for(int i=0;i<controls.size();++i)
        {
            const juce::String page=i<detailEnd ? "controls" : i<headerEnd ? "header" : "overview";
            result.push_back({controls[i],"digital."+page+"."+juce::String(i),controls[i]->getComponentID()});
        }
        result.push_back({&hqLamp,"digital.navigation.hq",bluechannel::params::id::hq});
        result.push_back({&analogue,"digital.navigation.analogue",{}});
        result.push_back({&pageButton,"digital.navigation.controls",{}});
        result.push_back({&overviewButton,"digital.navigation.overview",{}});
        result.push_back({&dynamicsButton,"digital.navigation.dynamics",{}});
        return result;
    }
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff08111c));
        drawText(g,"Blue Channel",{14,7,190,25},20,juce::Colour(0xffdbe8ff),juce::Justification::centredLeft);
        drawText(g,"DIGITAL",{210,10,100,20},10,juce::Colour(0xff39d0ff),juce::Justification::centredLeft);
        drawText(g,proc.hqStatusText(),{14,439,1532,11},8,juce::Colour(0xffd2b977),juce::Justification::centredLeft);
        if (!details)
        {
            g.setColour(juce::Colour(0xff0e1a2a));g.fillRoundedRectangle(12,42,1536,396,5);
            for(int x : {248,574,1262}) {g.setColour(juce::Colour(0xff3a5679).withAlpha(0.4f));g.drawVerticalLine(x,56,424);}
            const juce::String names[]={"INPUT","COMPRESSOR","EQUALIZER","OUTPUT"};
            const float left[]={24,264,590,1284},width[]={210,296,656,248};
            for(int i=0;i<4;++i) drawText(g,names[i],{left[i],48,width[i],16},10,juce::Colour(0xffdbe8ff));
            drawText(g,"DRAG FREQUENCY / GAIN / SCROLL MID BANDS FOR Q",{590,352,656,12},8,juce::Colour(0xff6f86a8));
            return;
        }
        for (auto& c : cards)
        {
            g.setColour(juce::Colour(0xff0e1a2a)); g.fillRoundedRectangle(c.bounds.toFloat(),4);
            g.setColour(juce::Colour(0xff3a5679)); g.drawHorizontalLine(c.bounds.getY()+19,float(c.bounds.getX()+10),float(c.bounds.getRight()-10));
            drawText(g,c.title,{float(c.bounds.getX()+10),float(c.bounds.getY()+2),float(c.bounds.getWidth()-20),16},10,juce::Colour(0xffdbe8ff),juce::Justification::centredLeft);
        }
    }
    void update(float in, float out, float comp, float gate, float deess, bool overload, bool key, float vu = -30)
    {
        namespace id = bluechannel::params::id;
        repaint(0,438,1560,12);
        meterSource->setSelectedItemIndex(proc.vuSource(), juce::dontSendNotification);
        meterCal->setSelectedItemIndex(proc.vuHighCal() ? 1 : 0, juce::dontSendNotification);
        vuReadout->setText(vu < -30 ? "< -30 VU" : juce::String(vu,1)+" VU", juce::dontSendNotification);
        const int source = juce::roundToInt(proc.apvts.getRawParameterValue(id::source)->load());
        mic->setVisible(details && source==0); micLabel->setVisible(details && source==0);
        line->setVisible(details && source!=0); lineLabel->setVisible(details && source!=0); line->setEnabled(source==1);
        mainMic->setVisible(!details && source==0);mainLine->setVisible(!details && source!=0);mainLine->setEnabled(source==1);
        const int filterRoute=juce::roundToInt(proc.apvts.getRawParameterValue(id::filterRoute)->load());
        const bool keyRoute=filterRoute>=2;
        const bool filterActive=keyRoute || (filterRoute==1 && proc.apvts.getRawParameterValue(id::allEq)->load()>0.5f
                                           && proc.apvts.getRawParameterValue(id::bypass)->load()<0.5f);
        mainHP->setAlpha(filterActive ? 1.0f : 0.3f);mainLP->setAlpha(filterActive ? 1.0f : 0.3f);
        if(isVisible() && !details) {eqGraph->refresh();compGraph->repaint();}
        const juce::String mainNames[]={"INPUT", "OUTPUT", "COMP GR", "GATE GR"};
        const float mainValues[]={20*std::log10(in+1e-9f),20*std::log10(out+1e-9f),comp,gate};
        for(int i=0;i<4;++i)
        {
            mainMeters[i]->setText(mainNames[i]+"   "+juce::String(juce::jmax(-100.0f,mainValues[i]),1)+(i<2 ? " dBFS" : " dB"),juce::dontSendNotification);
            mainMeters[i]->setColour(juce::Label::textColourId,overload && i==0 ? juce::Colour(0xffff6a5a) : juce::Colour::fromString("ff"+mainMeters[i]->getProperties().getWithDefault("layout.colour","#39d0ff").toString().substring(1)));
        }
        const juce::String names[] = {"INPUT", "OUTPUT", "COMP GR", "GATE GR", "DE-ESS"};
        const float values[] = {20*std::log10(in+1e-9f),20*std::log10(out+1e-9f),comp,gate,deess};
        for(int i=0;i<5;++i)
        {
            meters[i]->setText(names[i]+"    "+juce::String(juce::jmax(-100.0f,values[i]),1)+(i<2 ? " dBFS" : i<4 ? " dB" : ""),juce::dontSendNotification);
            meters[i]->setColour(juce::Label::textColourId,overload && i==0 ? juce::Colour(0xffff6a5a) : juce::Colour::fromString("ff"+meters[i]->getProperties().getWithDefault("layout.colour","#39d0ff").toString().substring(1)));
        }
        meters[4]->setText(juce::String("DE-ESS ")+(deess>0.05f ? "ACTIVE" : "IDLE")+(key ? "   EXT KEY CONNECTED" : ""),juce::dontSendNotification);
    }
};
} // namespace bluechannel::ui
