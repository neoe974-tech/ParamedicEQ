#pragma once
#include <JuceHeader.h>

class ParamedicEQAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int bands = 29;
    ParamedicEQAudioProcessor();

    void prepareToPlay(double, int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Paramedic EQ"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState& state() { return apvts; }
    void loadPreset(int);
    juce::String presetName() const { return currentPreset; }

    static float bandFrequency(int index);

private:
    juce::AudioProcessorValueTreeState apvts;
    std::array<juce::dsp::IIR::Filter<float>, bands> eq{};
    juce::dsp::Reverb reverb;
    juce::dsp::Gain<float> inGain, outGain;
    double sr = 44100.0;
    int program = 0;
    juce::String currentPreset = "Vocal Clean";

    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    void updateEQ();
    float val(const juce::String&, float) const;
    bool boolVal(const juce::String&, bool) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParamedicEQAudioProcessor)
};
