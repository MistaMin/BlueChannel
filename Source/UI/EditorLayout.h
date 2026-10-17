// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
#pragma once
#include "Widgets.h"

namespace bluechannel::ui
{
struct LayoutTarget
{
    juce::Component* component = nullptr;
    juce::String id, parameter;
};

inline juce::var layoutDescription(const LayoutTarget& target)
{
    auto& c=*target.component;
    auto* o=new juce::DynamicObject;
    const auto b=c.getBounds();
    o->setProperty("id",target.id);o->setProperty("parameter",target.parameter);
    o->setProperty("x",b.getX());o->setProperty("y",b.getY());o->setProperty("width",b.getWidth());o->setProperty("height",b.getHeight());
    juce::String label=c.getTitle(), kind="component", style="Neo", colour="#39d0ff";
    float font=10;
    if(auto* k=dynamic_cast<Knob*>(&c))
    {label=k->design.label;style=k->design.style;colour=k->design.colour;font=k->design.fontSize;kind=k->isRotary() ? "knob" : k->getSliderStyle()==juce::Slider::LinearVertical ? "vertical" : "horizontal";}
    else if(auto* slider=dynamic_cast<juce::Slider*>(&c))
    {kind=slider->isRotary() ? "knob" : slider->getSliderStyle()==juce::Slider::LinearVertical ? "vertical" : "horizontal";}
    else if(auto* l=dynamic_cast<juce::Label*>(&c)) {label=l->getText();font=l->getFont().getHeight();kind="label";colour=hexOf(l->findColour(juce::Label::textColourId));}
    else if(auto* button=dynamic_cast<juce::Button*>(&c)) {label=button->getButtonText();kind="button";}
    else if(dynamic_cast<juce::ComboBox*>(&c)) kind="selector";
    if(auto* key=dynamic_cast<LedKey*>(&c)) {colour=hexOf(key->designColour());font=8.5f;}
    if(auto* lamp=dynamic_cast<Lamp*>(&c)) {label=lamp->designLabel();colour=hexOf(lamp->designColour());font=8;}
    if(dynamic_cast<juce::ToggleButton*>(&c)) font=9;
    if(dynamic_cast<juce::ComboBox*>(&c)) font=14;
    auto& properties=c.getProperties();
    if(bool(properties.getWithDefault("layout.dynamic",false))) {label=c.getTitle();colour="#39d0ff";}
    o->setProperty("label",label);
    o->setProperty("kind",kind);
    o->setProperty("style",properties.getWithDefault("layout.style",style));
    o->setProperty("colour",properties.getWithDefault("layout.colour",colour));
    o->setProperty("font",properties.getWithDefault("layout.font",font));
    return juce::var(o);
}

inline bool validLayoutEntry(const LayoutTarget& target,const juce::var& value)
{
    auto* object=value.getDynamicObject();
    if(!object || value["id"].toString()!=target.id || value["parameter"].toString()!=target.parameter) return false;
    for(auto field : {"x","y","width","height","font"})
        if(!object->hasProperty(field) || !(value[field].isInt() || value[field].isInt64() || value[field].isDouble()) || !std::isfinite(double(value[field]))) return false;
    if(int(value["width"])<12 || int(value["height"])<12 || int(value["width"])>3120 || int(value["height"])>900) return false;
    if(int(value["x"]) < -3120 || int(value["x"]) > 3120 || int(value["y"]) < -900 || int(value["y"]) > 900) return false;
    if(double(value["font"])<6 || double(value["font"])>48) return false;
    const auto colour=value["colour"].toString();
    if(colour.length()!=7 || !colour.startsWithChar('#') || !colour.substring(1).containsOnly("0123456789abcdefABCDEF")) return false;
    if(!goodlookinui::findStyle(value["style"].toString().toStdString())) return false;
    if(dynamic_cast<juce::Slider*>(target.component))
    {
        auto kind=value["kind"].toString();
        if(kind!="knob" && kind!="horizontal" && kind!="vertical") return false;
    }
    return true;
}

inline void applyLayoutEntry(const LayoutTarget& target,const juce::var& value)
{
    if(juce::JSON::toString(layoutDescription(target))==juce::JSON::toString(value)) return;
    auto& c=*target.component;
    c.setBounds(int(value["x"]),int(value["y"]),int(value["width"]),int(value["height"]));
    const auto label=value["label"].toString(), style=value["style"].toString(), hex=value["colour"].toString();
    const float font=float(value["font"]);
    const auto colour=juce::Colour::fromString("ff"+hex.substring(1));
    c.setTitle(label);
    c.getProperties().set("layout.label",label);
    c.getProperties().set("layout.style",style);c.getProperties().set("layout.colour",hex);c.getProperties().set("layout.font",font);
    if(auto* slider=dynamic_cast<juce::Slider*>(&c))
    {
        const auto kind=value["kind"].toString();
        slider->setSliderStyle(kind=="knob" ? juce::Slider::RotaryVerticalDrag : kind=="vertical" ? juce::Slider::LinearVertical : juce::Slider::LinearHorizontal);
        if(dynamic_cast<Knob*>(slider) && kind=="knob") slider->setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
        else slider->setTextBoxStyle(kind=="horizontal" ? juce::Slider::TextBoxRight : juce::Slider::TextBoxBelow,false,kind=="horizontal" ? 64 : 100,20);
        slider->setColour(juce::Slider::textBoxTextColourId,colour);
        slider->setColour(juce::Slider::trackColourId,colour);slider->setColour(juce::Slider::thumbColourId,colour);
        slider->updateText();
    }
    if(auto* k=dynamic_cast<Knob*>(&c))
    {auto design=k->design;design.label=label.toStdString();design.style=style.toStdString();design.colour=hex.toStdString();design.fontSize=font;k->applyDesign(design);}
    else if(auto* l=dynamic_cast<juce::Label*>(&c)) {l->setText(label,juce::dontSendNotification);l->setFont(juce::FontOptions(font));l->setColour(juce::Label::textColourId,colour);}
    else if(auto* button=dynamic_cast<juce::Button*>(&c))
    {button->setButtonText(label);button->setColour(juce::TextButton::textColourOnId,colour);button->setColour(juce::TextButton::textColourOffId,colour);}
    else if(auto* selector=dynamic_cast<juce::ComboBox*>(&c)) {selector->setColour(juce::ComboBox::textColourId,colour);}
    c.repaint();
}
}
