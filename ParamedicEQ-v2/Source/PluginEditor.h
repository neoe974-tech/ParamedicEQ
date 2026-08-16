#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class ParamedicEQAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit ParamedicEQAudioProcessorEditor(ParamedicEQAudioProcessor&);
    ~ParamedicEQAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    ParamedicEQAudioProcessor& p;

    juce::ComboBox preset, reverbPreset;
    juce::TextButton prev{"◀"}, next{"▶"}, save{"▣"}, loadButton{"LOAD"};
    juce::TextButton addButton{"+"}, deleteButton{"▢"}, menuButton{"⋮"};
    juce::ToggleButton power{"POWER"};
    juce::TextButton specButton{"SPECTRUM"}, curveButton{"EQ CURVE"}, bothButton{"BOTH"}, analyzerGear{"⚙"};
    juce::TextButton analyzerHold{"HOLD"}, analyzerOff{"OFF"};
    juce::ToggleButton compressor{"ON"}, limiter{"ON"}, masterMix{"ON"}, masterWidth{"100%"};

    std::array<juce::Slider, 29> freq, gain, q;
    std::array<juce::ToggleButton, 29> bypass;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 87> attachments;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, 29> bypassAttachments;

    juce::Slider input, output, rmix, rpre, rdec, rsize, rdamp, rlow, rhigh, rwidth;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 10> globalAttachments;

    int selectedBand = 14;
    int draggedBand = -1;
    bool draggingGraphBand = false;
    float analyzerPhase = 0.0f;
    std::unique_ptr<juce::FileChooser> fileChooser;

    void timerCallback() override;
    void configureKnob(juce::Slider&, double lo, double hi);
    void loadPreset(int);
    void selectBand(int);
    juce::String formatFreq(float) const;

    void savePresetFile();
    void openPresetFile();
    void showPresetMenu();
    void resetCurrentPreset();
    void syncPresetSelector();
    int bandAtGraphPosition(juce::Point<float>) const;
    void moveGraphBand(int, juce::Point<float>);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParamedicEQAudioProcessorEditor)
};
