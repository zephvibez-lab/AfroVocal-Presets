#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class AfroVocalPresetsAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit AfroVocalPresetsAudioProcessorEditor(AfroVocalPresetsAudioProcessor&);
    ~AfroVocalPresetsAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    AfroVocalPresetsAudioProcessor& processor;
    juce::ComboBox presetBox, keyBox, scaleBox;
    juce::TextButton savePresetButton { "SAVE" }, bypassButton { "BYPASS" }, activateAiButton { "GENERATE" };
    juce::TextEditor aiPrompt, apiKeyEditor;
    juce::Label aiStatus, titleLabel;
    juce::Slider tuneSlider, airSlider, spaceSlider, warmthSlider, outputSlider;
    juce::Slider hpSlider, lowMidSlider, presenceSlider, compSlider, deEssSlider, delaySlider;
    juce::Slider ratioSlider, attackSlider, releaseSlider, parallelSlider, ambientSlider, delayTimeSlider;
    std::unique_ptr<SliderAttachment> tuneA, airA, spaceA, warmthA, outputA, hpA, lowMidA, presenceA, compA, deEssA, delayA;
    std::unique_ptr<SliderAttachment> ratioA, attackA, releaseA, parallelA, ambientA, delayTimeA;
    std::unique_ptr<ButtonAttachment> bypassA;
    std::unique_ptr<ComboAttachment> keyA, scaleA;
    float meter = 0.0f;
    float gainReduction = 0.0f;

    void configureSlider(juce::Slider&, const juce::String& suffix = {});
    void timerCallback() override;
    void drawModule(juce::Graphics&, juce::Rectangle<int>, const juce::String&, juce::Colour, bool enabled = true);
    void drawMeter(juce::Graphics&, juce::Rectangle<float>);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AfroVocalPresetsAudioProcessorEditor)
};
