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
    class AfroLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                              float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;
        void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
        void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override;
    } lookAndFeel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    AfroVocalPresetsAudioProcessor& processor;
    juce::ComboBox presetBox, keyBox, scaleBox;
    juce::TextButton savePresetButton { "Save" }, bypassButton { "Bypass" }, activateAiButton { "Generate AI" };
    juce::TextEditor aiPrompt, apiKeyEditor;
    juce::Label aiStatus, titleLabel;
    juce::Slider tuneSlider, focusSlider, airSlider, spaceSlider, warmthSlider, outputSlider;
    juce::Slider hpSlider, lowMidSlider, presenceSlider, compSlider, deEssSlider, delaySlider;
    juce::Slider ratioSlider, attackSlider, releaseSlider, parallelSlider, ambientSlider, delayTimeSlider;
    std::unique_ptr<SliderAttachment> tuneA, focusA, airA, spaceA, warmthA, outputA, hpA, lowMidA, presenceA, compA, deEssA, delayA;
    std::unique_ptr<SliderAttachment> ratioA, attackA, releaseA, parallelA, ambientA, delayTimeA;
    std::unique_ptr<ButtonAttachment> bypassA;
    std::unique_ptr<ComboAttachment> keyA, scaleA;
    float meter = 0.0f;
    float gainReduction = 0.0f;

    void configureSlider(juce::Slider&, const juce::String& suffix = {});
    void timerCallback() override;
    void drawPanel(juce::Graphics&, juce::Rectangle<float>, const juce::String&, juce::Colour);
    void drawKnobLabel(juce::Graphics&, const juce::String&, juce::Rectangle<float>);
    void drawMeter(juce::Graphics&, juce::Rectangle<float>);
    void drawGraph(juce::Graphics&, juce::Rectangle<float>, bool compressor);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AfroVocalPresetsAudioProcessorEditor)
};
