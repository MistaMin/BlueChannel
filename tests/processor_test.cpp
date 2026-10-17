// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid
//
// Host-level checks of the real plugin processor: bus layouts, state round trip and migration rules,
// malformed state, programs, editor lifetime, offline rendering and odd block sizes.

#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <juce_audio_utils/juce_audio_utils.h>

#include <cstdio>
#include <memory>

static int failures = 0;
static void check(bool ok, const char* what)
{
    std::printf("%-64s %s\n", what, ok ? "ok" : "FAIL");
    failures += !ok;
}

static void setParam(BlueChannelProcessor& p, const char* id, float value)
{
    auto* r = p.apvts.getParameter(id);
    r->setValueNotifyingHost(r->convertTo0to1(value));
}
static float getParam(BlueChannelProcessor& p, const char* id) { return p.apvts.getRawParameterValue(id)->load(); }

static void fillNoise(juce::AudioBuffer<float>& b, int seed)
{
    juce::Random r(seed);
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i) b.setSample(ch, i, (r.nextFloat() * 2.0f - 1.0f) * 0.2f);
}

static bool finite(const juce::AudioBuffer<float>& b)
{
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i)
            if (!std::isfinite(b.getSample(ch, i))) return false;
    return true;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    namespace id = bluechannel::params::id;

    // New instances and Init use Mic at actual unity; the removed realtime mode has no host control.
    {
        BlueChannelProcessor p;
        check(getParam(p,id::source)==0 && p.apvts.getParameter(id::micGain)->getCurrentValueAsText()=="0 dB",
              "fresh processor selects Mic preamp with a 0 dB gain readout");
        check(p.apvts.getParameter("rthq")==nullptr,"realtime HQ has been removed from the host parameter registry");
        p.setPlayConfigDetails(1,1,48000,240);p.prepareToPlay(48000,240);
        juce::AudioBuffer<float> audio(1,240);juce::MidiBuffer midi;double in=0,out=0;
        for(int block=0;block<100;++block)
        {
            for(int i=0;i<240;++i)audio.setSample(0,i,float(.01*std::sin(2*juce::MathConstants<double>::pi*1000*(block*240+i)/48000)));
            p.processBlock(audio,midi);
            if(block>=80)for(int i=0;i<240;++i){const double v=.01*std::sin(2*juce::MathConstants<double>::pi*1000*(block*240+i)/48000);in+=v*v;const double y=audio.getSample(0,i);out+=y*y;}
        }
        check(std::abs(10*std::log10(out/in))<.05,"fresh host microphone path renders at unity gain");
        setParam(p,id::source,1);setParam(p,id::micGain,7);p.setCurrentProgram(0);
        check(getParam(p,id::source)==0 && p.apvts.getParameter(id::micGain)->getCurrentValueAsText()=="0 dB",
              "Init program restores Mic at 0 dB");
    }

    // HQ is session-persistent and does not invoke the blocking solver during playback.
    {
        BlueChannelProcessor standard, armed;
        check(getParam(armed,id::hq)==0, "HQ defaults off for existing sessions");
        setParam(armed,id::hq,1);
        juce::MemoryBlock state; armed.getStateInformation(state);
        BlueChannelProcessor restored;
        restored.setStateInformation(state.getData(),int(state.getSize()));
        check(getParam(restored,id::hq)==1,"HQ selection survives session reopen");
        armed.setCurrentProgram(1);
        check(getParam(armed,id::hq)==1,"factory programs retain HQ processing preference");
        armed.setCurrentProgram(0);
        for(auto* p : {&standard,&armed}) {p->prepareToPlay(48000,32);p->setNonRealtime(false);}
        juce::AudioBuffer<float> a(2,32), b(2,32);fillNoise(a,177);b.makeCopyOf(a);
        juce::MidiBuffer midi;standard.processBlock(a,midi);armed.processBlock(b,midi);
        bool same=true;
        for(int ch=0;ch<2;++ch) for(int i=0;i<32;++i) same=same && a.getSample(ch,i)==b.getSample(ch,i);
        check(same,"HQ armed playback is identical to Standard");
        check(armed.hqStatus.load()!=bluechannel::hq::Renderer::Solving,"HQ never starts the component solve during playback");
    }

    // The final Offline lamp selects HQ without changing host IDs or creating another button.
    {
        BlueChannelProcessor p;BlueChannelPanel panel(p);
        auto* selector=dynamic_cast<bluechannel::ui::StepSelector*>(panel.findChildWithID("offline-selector"));
        check(selector && selector->getNumChildComponents()==8,"Offline selector has one key and seven lamps");
        if(selector){selector->setSize(150,86);check(selector->getHeight()>=98,"legacy Offline layout grows to fit its seventh lamp");}
        auto* key=selector ? dynamic_cast<juce::Button*>(selector->getChildComponent(0)) : nullptr;
        if(key)
        {
            for(int i=0;i<6;++i)key->onClick();
            check(getParam(p,id::hq)==1 && selector->selectedOption()==6,"seventh Offline lamp arms full HQ");
            key->onClick();
            check(getParam(p,id::hq)==0 && getParam(p,id::offlineOversampling)==0,"Offline key cycles HQ back to Same and disarms it");
            setParam(p,id::hq,1);juce::MessageManager::getInstance()->runDispatchLoopUntil(10);
            check(selector->selectedOption()==6,"host HQ automation selects the extra Offline lamp");
        }
    }

    // Layouts.
    {
        BlueChannelProcessor p;
        juce::AudioProcessor::BusesLayout stereo, mono, bad;
        stereo.inputBuses.add(juce::AudioChannelSet::stereo());
        stereo.inputBuses.add(juce::AudioChannelSet::stereo());
        stereo.outputBuses.add(juce::AudioChannelSet::stereo());
        mono.inputBuses.add(juce::AudioChannelSet::mono());
        mono.inputBuses.add(juce::AudioChannelSet::disabled());
        mono.outputBuses.add(juce::AudioChannelSet::mono());
        bad.inputBuses.add(juce::AudioChannelSet::mono());
        bad.outputBuses.add(juce::AudioChannelSet::stereo());
        check(p.checkBusesLayoutSupported(stereo), "stereo with stereo sidechain supported");
        check(p.checkBusesLayoutSupported(mono), "mono without sidechain supported");
        check(!p.checkBusesLayoutSupported(bad), "mono in / stereo out rejected");
    }

    // Some hosts automate switches with fractional normalised values.
    {
        BlueChannelProcessor original;
        for (float value : {0.17f, 0.49f, 0.51f, 0.83f})
        {
            for (auto* param : original.getParameters())
                if (param->isBoolean()) param->setValueNotifyingHost(value);
            juce::MemoryBlock saved; original.getStateInformation(saved);
            BlueChannelProcessor restored;
            restored.setStateInformation(saved.getData(), int(saved.getSize()));
            bool equal = true;
            for (int i = 0; i < original.getParameters().size(); ++i)
                if (original.getParameters()[i]->isBoolean())
                    equal &= original.getParameters()[i]->getValue() == restored.getParameters()[i]->getValue();
            check(equal, "fractional host switch values survive state recall");
        }
    }

    // State round trip, partial state and malformed state.
    {
        BlueChannelProcessor a;
        setParam(a, id::lmfGain, 7.5f);
        setParam(a, id::source, 0);
        setParam(a, id::compIn, 1);
        setParam(a, id::threshold, -12.0f);
        a.setSkin(1);
        juce::MemoryBlock blob;
        a.getStateInformation(blob);

        BlueChannelProcessor b;
        b.setStateInformation(blob.getData(), int(blob.getSize()));
        check(std::abs(getParam(b, id::lmfGain) - 7.5f) < 1e-3f && getParam(b, id::source) == 0.0f
                  && getParam(b, id::compIn) > 0.5f && std::abs(getParam(b, id::threshold) + 12.0f) < 1e-3f,
              "state round trip restores parameters");
        check(b.skin() == 1, "state round trip restores editor skin");

        // A state without the threshold entry restores that parameter to its default, even into a modified instance.
        auto xml = juce::AudioProcessor::getXmlFromBinary(blob.getData(), int(blob.getSize()));
        for (auto* child = xml->getFirstChildElement(); child != nullptr; child = child->getNextElement())
            if (child->getStringAttribute("id") == id::threshold) { xml->removeChildElement(child, true); break; }
        juce::MemoryBlock partial;
        juce::AudioProcessor::copyXmlToBinary(*xml, partial);
        BlueChannelProcessor c;
        setParam(c, id::threshold, 5.0f);
        c.setStateInformation(partial.getData(), int(partial.getSize()));
        check(std::abs(getParam(c, id::threshold) - 0.0f) < 1e-3f, "missing parameter restores to default");

        const char junk[] = "not a state";
        setParam(c, id::hmfGain, 3.0f);
        c.setStateInformation(junk, int(sizeof(junk)));
        check(std::abs(getParam(c, id::hmfGain) - 3.0f) < 1e-3f, "malformed state ignored, current state kept");
    }

    // Processing: stereo with sidechain, odd and oversized blocks, editor open and closed, programs.
    {
        BlueChannelProcessor p;
        p.setPlayConfigDetails(4, 2, 48000.0, 256);
        p.prepareToPlay(48000.0, 256);
        juce::MidiBuffer midi;
        bool ok = true;
        for (int blk : { 256, 1, 17, 129, 1024 })   // 1024 exceeds the announced size
        {
            juce::AudioBuffer<float> buf(4, blk);
            fillNoise(buf, blk);
            p.processBlock(buf, midi);
            ok = ok && finite(buf);
        }
        check(ok, "finite output for 1, 17, 129, 256 and oversized blocks");

        for (int prog = 0; prog < p.getNumPrograms(); ++prog)
        {
            p.setCurrentProgram(prog);
            juce::AudioBuffer<float> buf(4, 256);
            fillNoise(buf, prog);
            p.processBlock(buf, midi);
            ok = ok && finite(buf);
        }
        check(ok, "every factory program processes finite audio");

        std::unique_ptr<juce::AudioProcessorEditor> ed(p.createEditor());
        juce::AudioBuffer<float> buf(4, 256);
        fillNoise(buf, 3);
        p.processBlock(buf, midi);
        ed.reset();
        p.processBlock(buf, midi);
        check(finite(buf), "editor open and close around processing");
    }

    // Knob design: round trip, partial and malformed files, and the developer-mode toggle (developer builds).
    {
        BlueChannelProcessor p;
        BlueChannelPanel panel(p);
        auto csv = panel.designCsv();
        const auto before = panel.designItems();
        // Move the LMF gain knob by editing its row.
        juce::StringArray lines;
        lines.addLines(juce::String(csv));
        for (auto& l : lines)
            if (l.startsWith("lmfgain,"))
            {
                auto f = juce::StringArray::fromTokens(l, ",", "");
                f.set(5, "700"); f.set(7, "90");
                l = f.joinIntoString(",");
            }
        check(panel.applyDesignCsv(lines.joinIntoString("\n").toStdString()), "edited design file applies");
        bool movedOk = false;
        for (auto& i : panel.designItems())
            if (i.id == "lmfgain") movedOk = i.x == 700.0f && i.width == 90.0f;
        check(movedOk, "design edit moves and resizes the knob");
        check(!panel.applyDesignCsv("not a design"), "malformed design ignored");
        check(panel.applyDesignCsv("GoodLookinUI design version 1\nid,parameter,label,style,colour,x,y,width,height,font size\n"
                                   "trim,trim,Trim,FS,#D9DCDF,400,40,70,90,10\n"),
              "partial design applies to the knobs it names");
        std::unique_ptr<juce::AudioProcessorEditor> ed(p.createEditor());
        const int h0 = ed->getHeight();
        if (auto* e = dynamic_cast<BlueChannelEditor*>(ed.get()))
        {
            juce::ignoreUnused(e);
#if BLUECHANNEL_DEV_EDITOR
            if (auto* pn = dynamic_cast<BlueChannelPanel*>(e->getChildComponent(0))) pn->onDevToggle();
            check(ed->getHeight() > h0, "developer mode adds the inspector below the panel");
            if (auto* pn = dynamic_cast<BlueChannelPanel*>(e->getChildComponent(0))) pn->onDevToggle();
            check(std::abs(ed->getHeight() - int(std::round(ed->getWidth() * 450.0 / 1560.0))) <= 2, "leaving developer mode restores the panel aspect");
#else
            check(h0 > 0, "release editor has no developer mode");
#endif
        }
    }

    // Initial sizing must survive setup callbacks and migrate old small windows exactly once.
    {
        BlueChannelProcessor p;
        {
            std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
            check(editor->getWidth()==1560 && editor->getHeight()==450,"new editors start at double the previous 780 x 225 size");
            editor->setSize(1872,540);
        }
        {
            std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
            check(editor->getWidth()==1872 && editor->getHeight()==540,"reopening retains user sizing without doubling it again");
        }
        BlueChannelProcessor old;
        old.apvts.state.setProperty("uiscale",0.85,nullptr);
        std::unique_ptr<juce::AudioProcessorEditor> migrated(old.createEditor());
        check(migrated->getWidth()==2652 && std::abs(migrated->getHeight()-765)<=1,"legacy saved editor sizes migrate to twice their previous dimensions");
    }

    // Layout overrides survive reload and preserve parameter identity when a slider becomes a knob.
    {
        BlueChannelProcessor p;BlueChannelPanel panel(p);panel.showDigitalPage(false);
        auto targets=panel.layoutTargets();
        auto target=std::find_if(targets.begin(),targets.end(),[](const auto& t){return t.id.startsWith("digital.overview.") && t.parameter==id::hpf;});
        check(target!=targets.end(),"developer layout includes Digital overview sliders");
        if(target!=targets.end())
        {
            auto value=bluechannel::ui::layoutDescription(*target);
            auto* object=value.getDynamicObject();
            object->setProperty("x",600);object->setProperty("y",280);object->setProperty("width",100);object->setProperty("height",110);
            object->setProperty("kind","knob");object->setProperty("style","Arc");object->setProperty("colour","#ffaa22");
            juce::Array<juce::var> entries;entries.add(value);
            auto* root=new juce::DynamicObject;root->setProperty("version",1);root->setProperty("controls",entries);
            check(panel.applyEditorLayoutJson(juce::JSON::toString(juce::var(root))),"developer layout applies a partial override");
            auto* slider=dynamic_cast<juce::Slider*>(target->component);
            check(slider && slider->isRotary() && slider->getBounds()==juce::Rectangle<int>(600,280,100,110),"developer layout converts and places a Digital knob");
            slider->setValue(200,juce::sendNotificationSync);
            check(std::abs(getParam(p,id::hpf)-200)<0.1f,"converted knob retains its host parameter attachment");
            const auto saved=panel.editorLayoutJson();
            BlueChannelPanel reopened(p);
            check(reopened.applyEditorLayoutJson(saved),"complete editor layout round trip reloads");
            auto reloaded=reopened.layoutTargets();
            auto copy=std::find_if(reloaded.begin(),reloaded.end(),[&](const auto& t){return t.id==target->id;});
            check(copy!=reloaded.end() && dynamic_cast<juce::Slider*>(copy->component)->isRotary()
                  && copy->component->getBounds()==slider->getBounds()
                  && copy->component->getProperties()["layout.style"].toString()=="Arc","reloaded layout preserves kind, bounds and knob style");
            object->setProperty("kind","vertical");
            check(bluechannel::ui::validLayoutEntry(*target,value),"vertical slider is a valid developer control type");
            bluechannel::ui::applyLayoutEntry(*target,value);
            check(slider->getSliderStyle()==juce::Slider::LinearVertical,"developer layout converts a knob to a vertical slider");
            object->setProperty("parameter","wrong-binding");
            check(!bluechannel::ui::validLayoutEntry(*target,value),"layout rejects a changed parameter binding");
            check(!panel.applyEditorLayoutJson("not JSON"),"malformed editor layout is ignored");
        }
    }

    // The analyser tap receives the actual left output without changing samples or allocating a FIFO.
    {
        BlueChannelProcessor p;
        p.setPlayConfigDetails(2,2,48000,512);p.prepareToPlay(48000,512);
        juce::AudioBuffer<float> audio(2,512);juce::MidiBuffer midi;
        for(int i=0;i<512;++i) {audio.setSample(0,i,0.1f*std::sin(float(i)*0.13f));audio.setSample(1,i,0);}
        p.processBlock(audio,midi);
        bool match=p.spectrumWrite.load()==512 && p.spectrumRate.load()==48000;
        for(unsigned i=0;i<512;++i) match=match && p.spectrum[i].load()==audio.getSample(0,int(i));
        check(match,"FFT tap contains the left output at the host sample rate");
        p.prepareToPlay(96000,512);
        check(p.spectrumWrite.load()==0 && p.spectrum[100].load()==0 && p.spectrumRate.load()==96000,
              "FFT tap resets when preparing a new sample rate");
    }

    // Both presentations use every existing host parameter; switching preserves edits and layout.
    {
        BlueChannelProcessor p;
        BlueChannelPanel panel(p);
        const auto hardwareDesign = panel.designCsv();
        panel.applySkin(1);
        auto find = [](auto&& self, juce::Component& parent, const juce::String& id) -> juce::Component*
        {
            if (parent.getComponentID() == id) return &parent;
            for (auto* child : parent.getChildren())
                if (auto* found = self(self, *child, id)) return found;
            return nullptr;
        };
        auto* digital = find(find, panel, "digital-panel");
        check(digital != nullptr && digital->isVisible(), "digital skin has its own visible control layout");
        bool complete = digital != nullptr;
        for (auto* parameter : p.getParameters())
            if (auto* identified = dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter))
                complete = complete && find(find, *digital, identified->paramID) != nullptr;
        check(complete, "digital layout exposes every host parameter");
        auto* offline=dynamic_cast<juce::ComboBox*>(find(find,*digital,id::offlineOversampling));
        check(offline && offline->getNumItems()==7,"Digital Offline menu includes the extra HQ choice");
        if(offline)
        {
            offline->setSelectedItemIndex(6,juce::sendNotificationSync);
            check(getParam(p,id::hq)==1,"Digital HQ menu choice arms the shared HQ parameter");
            offline->setSelectedItemIndex(3,juce::sendNotificationSync);
            check(getParam(p,id::hq)==0 && getParam(p,id::offlineOversampling)==3,"Digital factor choice disarms HQ and sets the factor");
            setParam(p,id::hq,1);juce::MessageManager::getInstance()->runDispatchLoopUntil(10);
            check(offline->getSelectedItemIndex()==6,"Digital Offline menu follows host HQ automation");
            setParam(p,id::hq,0);
        }
        auto* attack = dynamic_cast<juce::Slider*>(find(find, *digital, id::attack));
        attack->setValue(25.0, juce::sendNotificationSync);
        check(std::abs(getParam(p, id::attack) - 25.0f) < 0.01f, "digital slider edits the shared host parameter");
        auto* source = dynamic_cast<juce::ComboBox*>(find(find, *digital, id::source));
        source->setSelectedItemIndex(0, juce::sendNotificationSync);
        panel.tick();
        auto findVisible = [](auto&& self, juce::Component& parent, const juce::String& id) -> juce::Component*
        {
            if (!parent.isVisible()) return nullptr;
            if (parent.getComponentID() == id) return &parent;
            for(auto* child : parent.getChildren()) if(auto* found=self(self,*child,id)) return found;
            return nullptr;
        };
        check(getParam(p, id::source) == 0 && findVisible(findVisible, *digital, id::micGain)
              && !findVisible(findVisible, *digital, id::lineGain), "digital source selector updates gain visibility");
        check(findVisible(findVisible,*digital,"eq-spectrum") && findVisible(findVisible,*digital,"compressor-curve"),
              "digital overview exposes EQ spectrum and compressor graph");
        auto event = [](juce::Component* c, juce::Point<float> pos)
        {
            return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), pos,
                juce::ModifierKeys::leftButtonModifier,1,0,0,0,0,c,c,juce::Time::getCurrentTime(),pos,
                juce::Time::getCurrentTime(),1,false);
        };
        auto* compGraph=find(find,*digital,"compressor-curve");
        setParam(p,id::threshold,0);setParam(p,id::ratio,3);setParam(p,id::makeup,0);
        compGraph->mouseDown(event(compGraph,{187.33f,69.45f}));
        compGraph->mouseDrag(event(compGraph,{140.13f,69.45f}));
        compGraph->mouseUp(event(compGraph,{140.13f,69.45f}));
        check(std::abs(getParam(p,id::threshold)+12)<0.1f,"compressor curve threshold handle writes the host parameter");
        compGraph->mouseDown(event(compGraph,{210.93f,80.47f}));
        compGraph->mouseDrag(event(compGraph,{210.93f,88.47f}));
        compGraph->mouseUp(event(compGraph,{210.93f,88.47f}));
        check(getParam(p,id::ratio)>3.1f,"compressor curve ratio handle writes the host parameter");
        compGraph->mouseDown(event(compGraph,{85.07f,123.13f}));
        compGraph->mouseDrag(event(compGraph,{85.07f,113.13f}));
        compGraph->mouseUp(event(compGraph,{85.07f,113.13f}));
        check(getParam(p,id::makeup)>4.0f,"compressor curve makeup handle writes the host parameter");
        setParam(p,id::hpf,80);setParam(p,id::filterRoute,2);panel.tick();
        auto* eq=find(find,*digital,"eq-spectrum");
        eq->mouseDown(event(eq,{150.81f,133.13f}));
        eq->mouseDrag(event(eq,{229.33f,133.13f}));
        eq->mouseUp(event(eq,{229.33f,133.13f}));
        check(std::abs(getParam(p,id::hpf)-200)<2 && getParam(p,id::filterRoute)==2,
              "EQ filter handle remains editable in compressor sidechain without changing its route");
        auto* highPass=findVisible(findVisible,*digital,id::hpf);
        setParam(p,id::allEq,0);panel.tick();
        check(highPass->getAlpha()>0.9f,"sidechain filters stay active when audio EQ is switched out");
        setParam(p,id::filterRoute,0);panel.tick();
        check(highPass->getAlpha()<0.5f,"unrouted filters are dimmed in the EQ field");
        setParam(p,id::filterRoute,1);setParam(p,id::allEq,1);panel.tick();
        check(highPass->getAlpha()>0.9f,"audio-routed filters are active in the EQ field");
        auto* page=dynamic_cast<juce::TextButton*>(find(find,*digital,"digital-controls"));
        page->onClick();panel.tick();
        check(findVisible(findVisible,*digital,id::gateThreshold) && !findVisible(findVisible,*digital,"eq-spectrum"),
              "digital controls page exposes gate and hides overview");
        auto* overview=dynamic_cast<juce::TextButton*>(find(find,*digital,"digital-overview"));
        overview->onClick();panel.tick();
        auto* bypass = dynamic_cast<juce::ToggleButton*>(find(find, *digital, id::bypass));
        bypass->setToggleState(true, juce::sendNotificationSync);
        check(getParam(p, id::bypass) > 0.5f, "digital toggle edits the shared host parameter");
        setParam(p, id::attack, 50.0f);
        juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
        check(std::abs(attack->getValue() - 50.0) < 0.01, "host parameter updates the digital slider");
        panel.applySkin(0);
        check(!digital->isVisible() && panel.designCsv() == hardwareDesign && getParam(p, id::bypass) > 0.5f,
              "return to hardware preserves its design and parameter edits");
        p.setSkin(1); panel.tick();
        check(digital->isVisible(), "restored editor skin refreshes the open panel");
    }

    // Offline rendering at a different factor is deterministic across repeated bounces.
    {
        BlueChannelProcessor p;
        p.setPlayConfigDetails(2, 2, 44100.0, 512);
        setParam(p, id::offlineOversampling, 4);   // 8x
        setParam(p, id::compIn, 1);
        p.prepareToPlay(44100.0, 512);
        juce::MidiBuffer midi;
        juce::AudioBuffer<float> first, second;
        for (auto* out : { &first, &second })
        {
            juce::AudioBuffer<float> rt(2, 512);
            fillNoise(rt, 11);
            p.setNonRealtime(false);
            p.processBlock(rt, midi);
            p.setNonRealtime(true);
            juce::AudioBuffer<float> buf(2, 4096);
            fillNoise(buf, 5);
            for (int s = 0; s < 4096; s += 512)
            {
                juce::AudioBuffer<float> view(buf.getArrayOfWritePointers(), 2, s, 512);
                p.processBlock(view, midi);
            }
            *out = buf;
        }
        float d = 0;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 4096; ++i) d = std::max(d, std::abs(first.getSample(ch, i) - second.getSample(ch, i)));
        check(d == 0.0f, "repeated offline bounces are identical");
        check(p.telemetry().activeFactor.load() == 8, "offline render runs at the offline factor (8x)");
    }

    std::printf("%s (%d failures)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
