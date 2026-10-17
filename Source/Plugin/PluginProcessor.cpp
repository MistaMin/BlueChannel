// SPDX-License-Identifier: MIT
// Copyright (c) 2026 OpenGrid

#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace
{
constexpr const char* kStateTag = "BlueChannel";

// Factory programs set sound parameters only; oversampling and offline settings are left alone.
struct Program
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

const std::vector<Program>& programs()
{
    using namespace bluechannel::params;
    static const std::vector<Program> list = {
        { "Init", {} },
        { "Vocal",
          { { id::filterRoute, 1 }, { id::hpf, 80 }, { id::paramRoute, 1 }, { id::lmfGain, -2.5f }, { id::lmfFreq, 300 },
            { id::hmfGain, 2.0f }, { id::hmfFreq, 3000 }, { id::shelfRoute, 1 }, { id::hfGain, 3.0f }, { id::hfFreq, 2 },
            { id::hfHigh, 1 }, { id::compIn, 1 }, { id::ratio, 3.0f }, { id::threshold, -8.0f }, { id::attack, 5.0f },
            { id::release, 0.25f }, { id::autoRelease, 1 }, { id::makeup, 4.0f }, { id::deessIn, 1 },
            { id::deessThreshold, -12.0f }, { id::deessFreq, 6000 } } },
        { "Drum Bus",
          { { id::shelfRoute, 1 }, { id::lfGain, 3.0f }, { id::lfFreq, 1 }, { id::hfGain, 2.5f }, { id::hfFreq, 2 },
            { id::compIn, 1 }, { id::ratio, 4.0f }, { id::threshold, -6.0f }, { id::attack, 20.0f }, { id::release, 0.15f },
            { id::makeup, 3.0f }, { id::blend, 1 }, { id::mix, 60.0f } } },
        { "Bass DI",
          { { id::source, 2 }, { id::trim, 6.0f }, { id::filterRoute, 1 }, { id::hpf, 35 }, { id::lpf, 6000 },
            { id::paramRoute, 1 }, { id::lmfGain, 3.0f }, { id::lmfFreq, 90 }, { id::compIn, 1 }, { id::compType, 1 },
            { id::ratio, 4.0f }, { id::threshold, -10.0f }, { id::makeup, 4.0f } } },
        { "Gated Toms",
          { { id::filterRoute, 3 }, { id::hpf, 80 }, { id::lpf, 2000 }, { id::gateIn, 1 }, { id::gateThreshold, -20.0f },
            { id::range, 40.0f }, { id::hold, 0.15f }, { id::gateRelease, 0.3f }, { id::fastAttack, 1 } } },
        { "Mix Polish",
          { { id::shelfRoute, 1 }, { id::lfGain, 1.5f }, { id::lfFreq, 0 }, { id::lfHigh, 1 }, { id::hfGain, 1.5f },
            { id::hfFreq, 3 }, { id::hfHigh, 1 }, { id::compIn, 1 }, { id::ratio, 1.8f }, { id::threshold, -4.0f },
            { id::attack, 30.0f }, { id::autoRelease, 1 }, { id::makeup, 1.0f }, { id::softLimit, 1 } } },
    };
    return list;
}
} // namespace

BlueChannelProcessor::BlueChannelProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                         .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts(*this, &undo, kStateTag, bluechannel::params::createLayout()),
      reader(apvts)
{
    apvts.state.setProperty("schema", bluechannel::params::kSchemaVersion, nullptr);
}

bool BlueChannelProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& in = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (in != out || (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()))
        return false;
    if (layouts.inputBuses.size() > 1)
    {
        const auto& sc = layouts.inputBuses.getReference(1);
        if (!sc.isDisabled() && sc != juce::AudioChannelSet::mono() && sc != juce::AudioChannelSet::stereo())
            return false;
    }
    return true;
}

void BlueChannelProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sampleRate_ = sampleRate;
    spectrumRate.store(sampleRate, std::memory_order_relaxed);
    for (auto& sample : spectrum) sample.store(0.0f, std::memory_order_relaxed);
    spectrumWrite.store(0, std::memory_order_release);
    maxBlock_ = std::max(1, samplesPerBlock);
    const int channels = std::clamp(getMainBusNumInputChannels(), 1, 2);
    channelsActive = channels;
    engine.setOversampling(reader.playbackStages(), reader.offlineStages());
    engine.prepare(sampleRate, maxBlock_, channels);   // allocates every bank; engine stays real-time safe after this
    hqRenderer.prepare(sampleRate, maxBlock_, channels);
    for (auto& b : hqAudio) b.assign(size_t(maxBlock_), 0);
    for (auto& b : hqKey) b.assign(size_t(maxBlock_), 0);
    previousHQ = previousOffline = false;
    setLatencySamples(engine.latencySamples());
}

void BlueChannelProcessor::reset() { engine.reset(); hqRenderer.restart(); }

juce::String BlueChannelProcessor::hqStatusText() const
{
    using H = bluechannel::hq::Renderer;
    switch (hqStatus.load(std::memory_order_relaxed))
    {
        case H::Armed: return "Standard / full HQ on export";
        case H::Solving: return "HQ: component solve";
        case H::Failed: return "HQ failed: render muted";
        case H::Unavailable: return "Offline HQ unavailable";
        default: return "Standard";
    }
}

template <typename Sample>
void BlueChannelProcessor::process(juce::AudioBuffer<Sample>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    auto main = getBusBuffer(buffer, true, 0);
    const int channels = std::min(engine.numChannels(), main.getNumChannels());
    if (channels <= 0 || n <= 0) return;

    const Sample* keyPtrs[2] = { nullptr, nullptr };
    int keyChannels = 0;
    if (getBusCount(true) > 1 && getBus(true, 1)->isEnabled())
    {
        auto key = getBusBuffer(buffer, true, 1);
        keyChannels = std::min(2, key.getNumChannels());
        for (int ch = 0; ch < keyChannels; ++ch) keyPtrs[ch] = key.getReadPointer(ch);
    }
    sidechainActive.store(keyChannels > 0, std::memory_order_relaxed);

    reader.read(controls);
    engine.setOversampling(reader.playbackStages(), reader.offlineStages());
    Sample* io[2] = { main.getWritePointer(0), channels > 1 ? main.getWritePointer(1) : nullptr };

    const bool offline = isNonRealtime();
    const bool wantHQ = reader.hqEnabled();
    if (wantHQ != previousHQ || (offline && !previousOffline)) hqRenderer.restart();
    previousHQ = wantHQ;
    previousOffline = offline;
    const bool solve = wantHQ && offline;
    hqStatus.store(wantHQ ? (bluechannel::hq::Renderer::compiled() ? bluechannel::hq::Renderer::Armed : bluechannel::hq::Renderer::Unavailable)
                         : bluechannel::hq::Renderer::Idle, std::memory_order_relaxed);

    // Hosts may call with more frames than announced: process in prepared-size chunks.
    for (int start = 0; start < n; start += maxBlock_)
    {
        const int len = std::min(maxBlock_, n - start);
        Sample* chunk[2] = { io[0] + start, io[1] ? io[1] + start : nullptr };
        const Sample* keys[2] = { keyPtrs[0] ? keyPtrs[0] + start : nullptr, keyPtrs[1] ? keyPtrs[1] + start : nullptr };
        if (solve)
        {
            double* audio[2] = { hqAudio[0].data(), hqAudio[1].data() };
            const double* hkeys[2] = { hqKey[0].data(), hqKey[1].data() };
            for (int ch=0; ch<channels; ++ch)
                for (int i=0; i<len; ++i)
                {
                    audio[ch][i] = double(chunk[ch][i]);
                    hqKey[size_t(ch)][size_t(i)] = keyChannels ? double(keys[keyChannels>1 ? ch:0][i]) : 0;
                }
            const int stages = reader.offlineStages() < 0 ? reader.playbackStages() : reader.offlineStages();
            if (bluechannel::hq::Renderer::compiled()) hqStatus.store(bluechannel::hq::Renderer::Solving,std::memory_order_relaxed);
            const bool ok = hqRenderer.render(audio, len, hkeys, keyChannels, controls, 1<<stages, engine.telemetry);
            hqStatus.store(ok ? bluechannel::hq::Renderer::Solving :
                (bluechannel::hq::Renderer::compiled() ? bluechannel::hq::Renderer::Failed : bluechannel::hq::Renderer::Unavailable), std::memory_order_relaxed);
            for (int ch=0; ch<channels; ++ch)
                for (int i=0; i<len; ++i) chunk[ch][i] = ok ? Sample(audio[ch][i]) : Sample(0);
        }
        else engine.process<Sample>(chunk, channels, len, keyChannels ? keys : nullptr, keyChannels, controls, offline);
    }
    unsigned write = spectrumWrite.load(std::memory_order_relaxed);
    for (int i = 0; i < n; ++i)
        spectrum[write++ & (kSpectrumSize - 1)].store(float(io[0][i]), std::memory_order_relaxed);
    spectrumWrite.store(write, std::memory_order_release);
    for (int ch = channels; ch < getTotalNumOutputChannels(); ++ch) buffer.clear(ch, 0, n);
}

void BlueChannelProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) { process(buffer); }
void BlueChannelProcessor::processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer&) { process(buffer); }

juce::AudioProcessorEditor* BlueChannelProcessor::createEditor() { return new BlueChannelEditor(*this); }

int BlueChannelProcessor::getNumPrograms() { return int(programs().size()); }
const juce::String BlueChannelProcessor::getProgramName(int index)
{
    return juce::isPositiveAndBelow(index, getNumPrograms()) ? programs()[size_t(index)].name : "";
}

void BlueChannelProcessor::setCurrentProgram(int index)
{
    if (!juce::isPositiveAndBelow(index, getNumPrograms())) return;
    currentProgram = index;
    using namespace bluechannel::params;
    // Reset every sound parameter to its default, then apply the program's values.
    for (auto* p : getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(p))
        {
            const auto pid = r->getParameterID();
            if (pid == id::oversampling || pid == id::offlineOversampling || pid == id::hq) continue;
            r->setValueNotifyingHost(r->getDefaultValue());
        }
    for (auto& [name, value] : programs()[size_t(index)].values)
        if (auto* r = apvts.getParameter(name)) r->setValueNotifyingHost(r->convertTo0to1(value));
}

void BlueChannelProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty("schema", bluechannel::params::kSchemaVersion, nullptr);
    state.setProperty("version", BLUECHANNEL_VERSION, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, dest);
}

void BlueChannelProcessor::setStateInformation(const void* data, int size)
{
    // Malformed or foreign state is ignored and the current state kept (docs/PARAMETERS.md).
    auto xml = getXmlFromBinary(data, size);
    if (xml == nullptr || !xml->hasTagName(apvts.state.getType())) return;
    auto incoming = juce::ValueTree::fromXml(*xml);
    if (!incoming.isValid()) return;
    // Parameters missing from older or partial states return to their defaults, so restoring into a modified
    // instance gives the same result as restoring into a fresh one.
    for (auto* p : getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*>(p))
        {
            bool present = false;
            for (const auto& child : incoming)
                if (child.hasType("PARAM") && child.getProperty("id").toString() == r->getParameterID()) { present = true; break; }
            if (!present)
            {
                juce::ValueTree param("PARAM");
                param.setProperty("id", r->getParameterID(), nullptr);
                param.setProperty("value", r->convertFrom0to1(r->getDefaultValue()), nullptr);
                incoming.appendChild(param, nullptr);
            }
        }
    apvts.replaceState(incoming);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BlueChannelProcessor(); }
