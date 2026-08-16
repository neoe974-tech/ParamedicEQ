#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
const std::array<float, 29> F = {
    20.f, 25.f, 31.5f, 40.f, 50.f, 63.f, 80.f, 100.f, 125.f, 160.f,
    200.f, 250.f, 315.f, 400.f, 500.f, 630.f, 800.f, 1000.f, 1250.f,
    1600.f, 2000.f, 2500.f, 3150.f, 4000.f, 5000.f, 6300.f, 8000.f,
    12000.f
};

const std::vector<juce::String> P = {
    "Vocal Clean", "Vocal Presence", "Vocal Air", "Vocal Warm", "Vocal Bright",
    "Vocal Radio", "Vocal Power", "Vocal Rap", "Vocal Trap", "Vocal RnB",
    "Vocal Smooth", "Vocal Intimate", "Guitar Clean", "Guitar Heavy", "Piano",
    "Kick Punch", "Kick Deep", "Snare Crack", "Drum Bus", "Bass Tight",
    "Bass Sub", "Master Clean", "Master Glue", "Loud Master", "Dark Mix",
    "Bright Mix", "Trap", "Amapiano", "House", "Reggae", "Acoustic",
    "Podcast", "Radio Voice"
};
}

float ParamedicEQAudioProcessor::bandFrequency(int index)
{
    return F[(size_t) juce::jlimit(0, bands - 1, index)];
}

ParamedicEQAudioProcessor::ParamedicEQAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMEDIC", layout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout ParamedicEQAudioProcessor::layout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> v;

    for (int i = 0; i < bands; ++i)
    {
        const auto n = juce::String(i + 1);
        v.push_back(std::make_unique<juce::AudioParameterFloat>(
            "F" + n, "Band " + n + " Frequency",
            juce::NormalisableRange<float>(20.f, 20000.f, 0.01f, 0.35f), F[(size_t)i]));
        v.push_back(std::make_unique<juce::AudioParameterFloat>(
            "G" + n, "Band " + n + " Gain",
            juce::NormalisableRange<float>(-18.f, 18.f, 0.01f), 0.f));
        v.push_back(std::make_unique<juce::AudioParameterFloat>(
            "Q" + n, "Band " + n + " Q",
            juce::NormalisableRange<float>(0.1f, 10.f, 0.01f, 0.5f), 1.f));
        v.push_back(std::make_unique<juce::AudioParameterBool>("B" + n, "Band " + n + " Bypass", false));
    }

    auto addFloat = [&v](const char* id, float lo, float hi, float d)
    {
        v.push_back(std::make_unique<juce::AudioParameterFloat>(
            id, id, juce::NormalisableRange<float>(lo, hi, 0.01f), d));
    };

    addFloat("INPUT", -24.f, 24.f, 0.f);
    addFloat("OUTPUT", -24.f, 12.f, 0.f);
    addFloat("RMIX", 0.f, 100.f, 20.f);
    addFloat("RPRE", 0.f, 200.f, 25.f);
    addFloat("RDEC", 0.1f, 4.f, 1.2f);
    addFloat("RSIZE", 0.f, 100.f, 30.f);
    addFloat("RDAMP", 0.f, 100.f, 40.f);
    addFloat("RLOW", 20.f, 1000.f, 120.f);
    addFloat("RHIGH", 2000.f, 20000.f, 8000.f);
    addFloat("RWIDTH", 0.f, 100.f, 100.f);

    return { v.begin(), v.end() };
}

float ParamedicEQAudioProcessor::val(const juce::String& id, float d) const
{
    if (auto* p = apvts.getRawParameterValue(id))
        return p->load();
    return d;
}

bool ParamedicEQAudioProcessor::boolVal(const juce::String& id, bool d) const
{
    if (auto* p = apvts.getRawParameterValue(id))
        return p->load() > 0.5f;
    return d;
}

void ParamedicEQAudioProcessor::prepareToPlay(double s, int n)
{
    sr = s;
    const juce::dsp::ProcessSpec sp { s, (juce::uint32)n, (juce::uint32)getTotalNumOutputChannels() };
    for (auto& f : eq) f.prepare(sp);
    reverb.prepare(sp);
    inGain.prepare(sp);
    outGain.prepare(sp);
    updateEQ();
}

void ParamedicEQAudioProcessor::releaseResources()
{
}

bool ParamedicEQAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    const auto main = l.getMainOutputChannelSet();
    return l.getMainInputChannelSet() == main
        && (main == juce::AudioChannelSet::mono() || main == juce::AudioChannelSet::stereo());
}

void ParamedicEQAudioProcessor::updateEQ()
{
    for (int i = 0; i < bands; ++i)
    {
        const auto n = juce::String(i + 1);
        const auto freq = juce::jlimit(20.f, 20000.f, val("F" + n, F[(size_t)i]));
        const auto q = juce::jmax(0.1f, val("Q" + n, 1.f));
        const auto gain = juce::Decibels::decibelsToGain(val("G" + n, 0.f));
        *eq[(size_t)i].coefficients = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, freq, q, gain);
    }
}

void ParamedicEQAudioProcessor::processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    updateEQ();

    juce::dsp::AudioBlock<float> block(b);
    juce::dsp::ProcessContextReplacing<float> context(block);

    inGain.setGainDecibels(val("INPUT", 0.f));
    outGain.setGainDecibels(val("OUTPUT", 0.f));
    inGain.process(context);

    for (int i = 0; i < bands; ++i)
        if (!boolVal("B" + juce::String(i + 1), false))
            eq[(size_t)i].process(context);

    juce::dsp::Reverb::Parameters r;
    r.wetLevel = val("RMIX", 20.f) / 100.f;
    r.dryLevel = 1.f - r.wetLevel;
    r.roomSize = val("RSIZE", 30.f) / 100.f;
    r.damping = val("RDAMP", 40.f) / 100.f;
    r.width = val("RWIDTH", 100.f) / 100.f;
    reverb.setParameters(r);
    reverb.process(context); // JUCE 8 API; replaces the removed processStereo() call.

    outGain.process(context);
}

int ParamedicEQAudioProcessor::getNumPrograms() { return (int) P.size(); }
int ParamedicEQAudioProcessor::getCurrentProgram() { return program; }
void ParamedicEQAudioProcessor::setCurrentProgram(int i) { if (i >= 0 && i < (int)P.size()) loadPreset(i); }
const juce::String ParamedicEQAudioProcessor::getProgramName(int i) { return i >= 0 && i < (int)P.size() ? P[(size_t)i] : juce::String(); }

void ParamedicEQAudioProcessor::loadPreset(int i)
{
    if (i < 0 || i >= (int)P.size()) return;

    program = i;
    currentPreset = P[(size_t)i];
    const auto name = currentPreset.toLowerCase();

    auto set = [this](const juce::String& id, float x)
    {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(x));
    };

    for (int b = 0; b < bands; ++b)
    {
        set("F" + juce::String(b + 1), F[(size_t)b]);
        set("Q" + juce::String(b + 1), 1.f);
        set("B" + juce::String(b + 1), 0.f);

        float g = 0.f;
        if (name.contains("vocal"))
        {
            if (b >= 5 && b <= 8) g = 2.f;
            if (b >= 18 && b <= 25) g = (float)(b - 17) * 0.3f;
            if (name.contains("warm") && b >= 14) g = -1.f;
            if (name.contains("air") && b >= 23) g = 3.f;
            if (name.contains("radio") && b >= 0 && b < 3) g = -6.f;
        }
        else if (name.contains("bass"))
        {
            if (b < 5) g = 2.f;
            if (b >= 17 && b <= 21) g = -1.f;
        }
        else if (name.contains("kick"))
        {
            if (b < 4) g = 3.f;
            if (b >= 8 && b < 12) g = -1.f;
        }
        else if (name.contains("bright"))
        {
            if (b > 20) g = 2.f;
        }
        else if (name.contains("dark"))
        {
            if (b > 17) g = -2.f;
        }
        else if (name.contains("master"))
        {
            if (b >= 7 && b <= 20) g = 0.5f;
        }
        else if (name.contains("trap"))
        {
            if (b < 5) g = 2.f;
            if (b >= 20 && b <= 25) g = 1.5f;
        }

        set("G" + juce::String(b + 1), g);
    }

    const bool vocal = name.contains("vocal");
    const bool air = name.contains("air");
    set("RMIX", vocal ? 20.f : 6.f);
    set("RPRE", vocal ? 25.f : 12.f);
    set("RDEC", vocal ? 1.2f : 0.6f);
    set("RSIZE", vocal ? 30.f : 15.f);
    set("RDAMP", 40.f);
    set("RLOW", 120.f);
    set("RHIGH", air ? 12000.f : 8000.f);
    set("RWIDTH", 100.f);
    set("INPUT", 0.f);
    set("OUTPUT", 0.f);
}

void ParamedicEQAudioProcessor::getStateInformation(juce::MemoryBlock& d)
{
    auto s = apvts.copyState();
    s.setProperty("preset", currentPreset, nullptr);
    if (auto x = s.createXml())
        copyXmlToBinary(*x, d);
}

void ParamedicEQAudioProcessor::setStateInformation(const void* d, int n)
{
    if (auto x = getXmlFromBinary(d, n))
    {
        if (x->hasTagName(apvts.state.getType()))
        {
            apvts.replaceState(juce::ValueTree::fromXml(*x));
            currentPreset = apvts.state.getProperty("preset", "Vocal Clean").toString();
        }
    }
}

juce::AudioProcessorEditor* ParamedicEQAudioProcessor::createEditor()
{
    return new ParamedicEQAudioProcessorEditor(*this);
}


juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ParamedicEQAudioProcessor();
}
