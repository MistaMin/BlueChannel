// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once
#if BLUECHANNEL_DEV_EDITOR
#include "PluginEditor.h"
#include <DesignFiles.h>
#include <Studio.h>

namespace bluechannel::ui
{
class DevLayoutOverlay : public juce::Component
{
public:
    explicit DevLayoutOverlay(BlueChannelPanel& p) : panel(p) {setWantsKeyboardFocus(true);}
    std::function<void(juce::Component*)> onSelect;
    std::function<void()> onEdited;
    juce::Component* selected=nullptr;
    bool visible(const LayoutTarget& target) const
    {
        for(auto* c=target.component;c && c!=&panel;c=c->getParentComponent()) if(!c->isVisible()) return false;
        return true;
    }
    bool hitTest(int,int y) override {return !(panel.theme.flat && y<40);}
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0x16000000));
        for(const auto& target : panel.layoutTargets())
        {
            if(!visible(target)) continue;
            const auto b=target.component->getBounds().toFloat();
            const bool chosen=target.component==selected;
            g.setColour(juce::Colour(chosen ? 0xffffb000 : 0x6639d0ff));
            g.drawRect(b,chosen ? 2.0f : 0.7f);
            if(chosen)
            {
                g.fillRect(handle(b));
                drawText(g,target.id,{b.getX(),juce::jmax(0.0f,b.getY()-16),juce::jmax(200.0f,b.getWidth()),14},10,juce::Colour(0xffffb000),juce::Justification::centredLeft);
            }
        }
        drawText(g,"LAYOUT: drag to move / corner to resize / arrows to nudge / Cmd+Z to undo",{8,float(getHeight()-18),1000,16},11,juce::Colour(0xffffb000),juce::Justification::centredLeft);
    }
    void mouseDown(const juce::MouseEvent& e) override
    {
        grabKeyboardFocus();resizing=selected && selected->isVisible() && handle(selected->getBounds().toFloat()).expanded(3).contains(e.position);
        if(!resizing)
        {
            selected=nullptr;
            auto targets=panel.layoutTargets();
            for(auto i=targets.rbegin();i!=targets.rend();++i)
                if(visible(*i) && i->component->getBounds().contains(e.getPosition())) {selected=i->component;break;}
            if(onSelect) onSelect(selected);
        }
        if(selected) {start=selected->getBounds();before=panel.editorLayoutJson();}
        repaint();
    }
    void mouseDrag(const juce::MouseEvent& e) override
    {
        if(!selected) return;
        const int grid=e.mods.isAltDown() ? 1 : 2;
        auto snap=[grid](int v){return grid*juce::roundToInt(float(v)/float(grid));};
        const auto delta=e.getOffsetFromDragStart();
        if(resizing) selected->setSize(juce::jlimit(12,3120,snap(start.getWidth()+delta.x)),juce::jlimit(12,900,snap(start.getHeight()+delta.y)));
        else selected->setTopLeftPosition(snap(start.getX()+delta.x),snap(start.getY()+delta.y));
        repaint();
    }
    void mouseUp(const juce::MouseEvent&) override
    {
        if(selected && selected->getBounds()!=start) {push(before);if(onEdited) onEdited();}
    }
    void push(const juce::String& state) {history.push_back(state);if(history.size()>200) history.erase(history.begin());}
    void undo()
    {
        if(history.empty()) return;
        panel.applyEditorLayoutJson(history.back());history.pop_back();if(onEdited) onEdited();repaint();
    }
    bool keyPressed(const juce::KeyPress& key) override
    {
        if(key==juce::KeyPress('z',juce::ModifierKeys::commandModifier,0)) {undo();return true;}
        if(!selected) return false;
        juce::Point<int> delta;const int step=key.getModifiers().isShiftDown() ? 10 : 1;
        if(key.isKeyCode(juce::KeyPress::leftKey)) delta={-step,0};
        else if(key.isKeyCode(juce::KeyPress::rightKey)) delta={step,0};
        else if(key.isKeyCode(juce::KeyPress::upKey)) delta={0,-step};
        else if(key.isKeyCode(juce::KeyPress::downKey)) delta={0,step};
        else if(key.isKeyCode(juce::KeyPress::escapeKey)) {selected=nullptr;repaint();return true;}
        else return false;
        push(panel.editorLayoutJson());selected->setTopLeftPosition(selected->getPosition()+delta);
        if(onEdited) onEdited();repaint();return true;
    }
private:
    BlueChannelPanel& panel;
    bool resizing=false;
    juce::Rectangle<int> start;
    juce::String before;
    std::vector<juce::String> history;
    static juce::Rectangle<float> handle(juce::Rectangle<float> b) {return {b.getRight()-8,b.getBottom()-8,8,8};}
};

class DevTools : public juce::Component
{
public:
    static constexpr int kInspectorHeight=156;
    DevTools(BlueChannelPanel& p,juce::Component& editor) : panel(p),overlay(p),autosave(juce::File(BLUECHANNEL_DESIGN_DIR))
    {
        panel.addChildComponent(overlay);editor.addChildComponent(*this);
        setComponentID("layout-inspector");overlay.setComponentID("layout-overlay");
        for(auto* c : std::vector<juce::Component*>{&selector,&page,&kind,&style,&chip,&caption,&font,&x,&y,&w,&h,&interact,&undoButton,&closeButton,&status}) addAndMakeVisible(c);
        page.addItemList({"Analogue","Digital / Overview","Digital / All Controls"},1);
        kind.addItemList({"Knob","Horizontal slider","Vertical slider"},1);
        for(int i=0;i<int(std::size(goodlookinui::knobStyles));++i)
            style.addItem(juce::String(goodlookinui::knobStyles[i].code)+" / "+goodlookinui::knobStyles[i].description,i+1);
        selector.setComponentID("layout-target");kind.setComponentID("layout-kind");style.setComponentID("layout-style");
        selector.onChange=[this]{const auto n=selector.getSelectedId()-1;auto targets=panel.layoutTargets();if(n>=0 && n<int(targets.size())) {overlay.selected=targets[size_t(n)].component;show();overlay.repaint();}};
        page.onChange=[this]{if(page.getSelectedId()==1) panel.applySkin(0);else panel.showDigitalPage(page.getSelectedId()==3);overlay.selected=nullptr;refreshList();overlay.toFront(false);};
        overlay.onSelect=[this](juce::Component* c){const auto targets=panel.layoutTargets();for(size_t i=0;i<targets.size();++i) if(targets[i].component==c) {selector.setSelectedId(int(i)+1,juce::dontSendNotification);break;}show();};
        overlay.onEdited=[this]{show();save();};
        kind.onChange=[this]{edit();};style.onChange=[this]{edit();};
        chip.onChange=[this](juce::Colour c){colour=hexOf(c);edit();};
        for(auto* field : {&caption,&font,&x,&y,&w,&h}) {field->setSelectAllWhenFocused(true);field->onReturnKey=[this]{edit();};field->onFocusLost=[this]{edit();};}
        interact.setButtonText("Use controls");interact.onClick=[this]{refreshList();overlay.setVisible(active && !interact.getToggleState());};
        undoButton.setButtonText("Undo");undoButton.onClick=[this]{overlay.undo();};
        closeButton.setButtonText("Close Dev");closeButton.onClick=[this]{if(panel.onDevToggle) panel.onDevToggle();};
        autosave.onSaved=[this](const goodlookinui::juce_adapter::DesignAutosave::Event& e){status.setText(e.ok ? "Saved: "+e.file.getFullPathName() : "SAVE FAILED: "+e.error,juce::dontSendNotification);};
        refreshList();
    }
    ~DevTools() override {autosave.flush();autosave.onSaved=nullptr;}
    bool isActive() const {return active;}
    void setActive(bool on)
    {
        active=on;setVisible(on);overlay.setBounds(panel.getLocalBounds());overlay.setVisible(on && !interact.getToggleState());
        if(on) {page.setSelectedId(panel.editorPage()+1,juce::dontSendNotification);refreshList();overlay.toFront(false);status.setText("Autosave: "+autosave.fileFor("editor-layout.json").getFullPathName(),juce::dontSendNotification);}
        else autosave.flush();
    }
    void layout(juce::Rectangle<int> r) {setBounds(r);}
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff18222c));
        for(auto pair : std::vector<std::pair<juce::String,juce::Rectangle<float>>>{{"PAGE",{8,4,220,12}},{"CONTROL",{238,4,300,12}},{"TYPE",{548,4,150,12}},{"KNOB STYLE",{708,4,220,12}},{"COLOUR",{938,4,100,12}},{"LABEL",{1048,4,210,12}}})
            drawText(g,pair.first,pair.second,9,juce::Colour(0xff9fb0b8),juce::Justification::centredLeft);
        const char* fields[]={"X","Y","WIDTH","HEIGHT","FONT"};
        for(int i=0;i<5;++i) drawText(g,fields[i],{float(8+i*110),58,100,12},9,juce::Colour(0xff9fb0b8),juce::Justification::centredLeft);
    }
    void resized() override
    {
        page.setBounds(8,20,220,26);selector.setBounds(238,20,300,26);kind.setBounds(548,20,150,26);style.setBounds(708,20,220,26);
        chip.setBounds(938,20,100,26);caption.setBounds(1048,20,juce::jmax(180,getWidth()-1056),26);
        int left=8;for(auto* c : {&x,&y,&w,&h,&font}) {c->setBounds(left,74,100,24);left+=110;}
        interact.setBounds(580,74,150,24);undoButton.setBounds(748,74,90,24);closeButton.setBounds(856,74,110,24);
        status.setBounds(8,112,getWidth()-16,28);
    }
private:
    BlueChannelPanel& panel;
    DevLayoutOverlay overlay;
    goodlookinui::juce_adapter::DesignAutosave autosave;
    juce::ComboBox selector,page,kind,style;
    goodlookinui::juce_adapter::ColourChip chip;
    juce::TextEditor caption,font,x,y,w,h;
    juce::ToggleButton interact;
    juce::TextButton undoButton,closeButton;
    juce::Label status;
    juce::String colour="#39d0ff";
    bool active=false,busy=false;
    void refreshList()
    {
        busy=true;selector.clear(juce::dontSendNotification);
        const auto targets=panel.layoutTargets();
        for(size_t i=0;i<targets.size();++i)
            if(overlay.visible(targets[i])) selector.addItem(targets[i].id+" / "+layoutDescription(targets[i])["label"].toString(),int(i)+1);
        busy=false;
        int preferred=0;
        for(size_t i=0;i<targets.size();++i)
            if(overlay.visible(targets[i]))
            {
                if(targets[i].component==overlay.selected) {preferred=int(i)+1;break;}
                if(preferred==0 && dynamic_cast<juce::Slider*>(targets[i].component)) preferred=int(i)+1;
            }
        if(preferred) selector.setSelectedId(preferred,juce::sendNotificationSync);
    }
    void show()
    {
        if(!overlay.selected) return;
        for(const auto& target : panel.layoutTargets()) if(target.component==overlay.selected)
        {
            busy=true;auto value=layoutDescription(target);
            const auto type=value["kind"].toString();
            const bool slider=dynamic_cast<juce::Slider*>(target.component)!=nullptr;
            kind.setEnabled(slider);style.setEnabled(slider);
            kind.setSelectedId(type=="knob" ? 1 : type=="vertical" ? 3 : 2,juce::dontSendNotification);
            if(auto* found=goodlookinui::findStyle(value["style"].toString().toStdString())) style.setSelectedId(int(found-goodlookinui::knobStyles)+1,juce::dontSendNotification);
            colour=value["colour"].toString();chip.set(juce::Colour::fromString("ff"+colour.substring(1)));
            caption.setText(value["label"].toString(),false);
            font.setText(value["font"].toString(),false);x.setText(value["x"].toString(),false);y.setText(value["y"].toString(),false);
            w.setText(value["width"].toString(),false);h.setText(value["height"].toString(),false);
            busy=false;break;
        }
    }
    void edit()
    {
        if(busy || !overlay.selected) return;
        for(const auto& target : panel.layoutTargets()) if(target.component==overlay.selected)
        {
            auto value=layoutDescription(target);auto* o=value.getDynamicObject();
            if(dynamic_cast<juce::Slider*>(target.component))
            {o->setProperty("kind",kind.getSelectedId()==1 ? "knob" : kind.getSelectedId()==3 ? "vertical" : "horizontal");if(style.getSelectedId()>0)o->setProperty("style",goodlookinui::knobStyles[style.getSelectedId()-1].code);}
            o->setProperty("label",caption.getText());o->setProperty("colour",colour);
            // Reject incomplete numeric text instead of silently moving a control to zero.
            for(auto pair : std::vector<std::pair<juce::Identifier,juce::TextEditor*>>{{"x",&x},{"y",&y},{"width",&w},{"height",&h},{"font",&font}})
            {
                const auto text=pair.second->getText().trim();size_t end=0;
                try {const auto number=std::stod(text.toStdString(),&end);if(end!=size_t(text.length())) throw std::invalid_argument("number");o->setProperty(pair.first,number);}
                catch(...) {status.setText("Enter a valid number",juce::dontSendNotification);return;}
            }
            if(!validLayoutEntry(target,value)) {status.setText("Invalid size, position, font or style",juce::dontSendNotification);return;}
            if(juce::JSON::toString(value)==juce::JSON::toString(layoutDescription(target))) return;
            overlay.push(panel.editorLayoutJson());applyLayoutEntry(target,value);overlay.repaint();show();save();break;
        }
    }
    void save() {autosave.request("editor-layout.json",panel.editorLayoutJson().toStdString());}
};
}
#else
namespace bluechannel::ui
{
class DevTools
{
public:
    static constexpr int kInspectorHeight=0;
    bool isActive() const {return false;}
};
}
#endif
