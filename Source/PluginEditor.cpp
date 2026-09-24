#include "PluginEditor.h"

namespace
{
    const juce::Colour bg { 0xff0c1116 }, frame { 0xff18232c }, panel { 0xff202d36 }, panel2 { 0xff18242d }, edge { 0xff52616a }, text { 0xffe2edf0 }, subtext { 0xffaab8bd }, teal { 0xff43d4c5 }, cyan { 0xff50c7ec }, purple { 0xff9e7bd7 }, gold { 0xffd4b166 }, green { 0xff55d5bb };
    const juce::StringArray factoryPresets { "Afrobeat Lead - Clear Bounce", "Afrobeat Lead - Warm Pocket", "Afropiano Lead - Gloss", "Afropiano Lead - Soft Air", "Emotional Ballad - Intimate", "Emotional Ballad - Wide", "Background Harmony - Tucked", "Background Harmony - Airy Stack", "Hard-Tuned Lead - Modern", "Hard-Tuned Lead - Dry" };
}

void AfroVocalPresetsAudioProcessorEditor::AfroLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
    float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height).reduced(8.0f);
    const float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight() - 19.0f);
    const auto knob = juce::Rectangle<float>(bounds.getCentreX() - diameter * 0.42f, bounds.getY() + 2.0f, diameter * 0.84f, diameter * 0.84f);
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto accent = slider.findColour(juce::Slider::rotarySliderFillColourId);
    g.setColour(juce::Colour(0x50000000)); g.fillEllipse(knob.translated(2.5f, 3.0f));
    g.setColour(juce::Colour(0xff0b151c)); g.fillEllipse(knob);
    g.setColour(juce::Colour(0xff7f929b)); g.drawEllipse(knob, 1.2f);
    auto arcBounds = knob.reduced(4.0f); juce::Path arc; arc.addArc(arcBounds.getX(), arcBounds.getY(), arcBounds.getWidth(), arcBounds.getHeight(), rotaryStartAngle, angle, true); g.setColour(accent); g.strokePath(arc, juce::PathStrokeType(3.0f));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffd8e2e4), knob.getTopLeft(), juce::Colour(0xff61747d), knob.getBottomRight(), false));
    g.fillEllipse(knob.reduced(9.0f));
    g.setColour(juce::Colour(0xffe8f1f2)); g.drawLine(knob.getCentreX(), knob.getCentreY(), knob.getCentreX() + std::cos(angle - juce::MathConstants<float>::halfPi) * knob.getWidth() * 0.30f, knob.getCentreY() + std::sin(angle - juce::MathConstants<float>::halfPi) * knob.getHeight() * 0.30f, 2.0f);
}

void AfroVocalPresetsAudioProcessorEditor::AfroLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced(1.0f); auto c = down ? juce::Colour(0xff2b8794) : (over ? juce::Colour(0xff2c5c6b) : juce::Colour(0xff253946));
    g.setColour(c); g.fillRoundedRectangle(r, 5.0f); g.setColour(juce::Colour(0xff83a9b2)); g.drawRoundedRectangle(r, 5.0f, 1.0f);
}

void AfroVocalPresetsAudioProcessorEditor::AfroLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setColour(juce::Colour(0xffe9f7f7)); g.setFont(juce::Font(15.0f, juce::Font::bold)); g.drawText(b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

AfroVocalPresetsAudioProcessorEditor::AfroVocalPresetsAudioProcessorEditor(AfroVocalPresetsAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&lookAndFeel); setSize(1408, 768); setResizable(true, true); setResizeLimits(1180, 650, 2100, 1150);
    titleLabel.setText({}, juce::dontSendNotification); titleLabel.setVisible(false);
    for (int i = 0; i < factoryPresets.size(); ++i) presetBox.addItem(factoryPresets[i], i + 1);
    for (int i = 0; i < processor.getGeneratedPresetCount(); ++i) presetBox.addItem("AI Vocal Preset " + juce::String::formatted("%04d", i + 1), factoryPresets.size() + i + 1);
    presetBox.setSelectedId(1); presetBox.setJustificationType(juce::Justification::centred); presetBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff0d1921)); presetBox.setColour(juce::ComboBox::textColourId, teal); presetBox.onChange = [this] { const int id = presetBox.getSelectedId(); if (id > 0 && id <= factoryPresets.size()) processor.applyFactoryPreset(presetBox.getText()); else if (id > factoryPresets.size()) processor.generateNextPreset(); }; addAndMakeVisible(presetBox);
    savePresetButton.onClick = [this] { aiStatus.setText("Preset state stored with the DAW project.", juce::dontSendNotification); }; addAndMakeVisible(savePresetButton); bypassButton.setClickingTogglesState(true); addAndMakeVisible(bypassButton);
    configureSlider(tuneSlider, "%"); configureSlider(focusSlider, "%"); configureSlider(airSlider, "%"); configureSlider(spaceSlider, "%"); configureSlider(warmthSlider, "%"); configureSlider(outputSlider, " dB"); configureSlider(hpSlider, " Hz"); configureSlider(lowMidSlider, " dB"); configureSlider(presenceSlider, " dB"); configureSlider(compSlider, " dB"); configureSlider(deEssSlider, "%"); configureSlider(delaySlider, "%"); configureSlider(ratioSlider, ":1"); configureSlider(attackSlider, " ms"); configureSlider(releaseSlider, " ms"); configureSlider(parallelSlider, "%"); configureSlider(ambientSlider, "%"); configureSlider(delayTimeSlider, " ms");
    for (auto* s : { &tuneSlider, &focusSlider, &airSlider, &spaceSlider, &warmthSlider, &outputSlider, &hpSlider, &lowMidSlider, &presenceSlider, &compSlider, &deEssSlider, &delaySlider, &ratioSlider, &attackSlider, &releaseSlider, &parallelSlider, &ambientSlider, &delayTimeSlider }) addAndMakeVisible(*s);
    tuneA = std::make_unique<SliderAttachment>(processor.parameters, "tuneAmount", tuneSlider); focusA = std::make_unique<SliderAttachment>(processor.parameters, "vocalFocus", focusSlider); airA = std::make_unique<SliderAttachment>(processor.parameters, "air", airSlider); spaceA = std::make_unique<SliderAttachment>(processor.parameters, "plate", spaceSlider); warmthA = std::make_unique<SliderAttachment>(processor.parameters, "warmth", warmthSlider); outputA = std::make_unique<SliderAttachment>(processor.parameters, "outputGain", outputSlider); hpA = std::make_unique<SliderAttachment>(processor.parameters, "highPass", hpSlider); lowMidA = std::make_unique<SliderAttachment>(processor.parameters, "lowMidCut", lowMidSlider); presenceA = std::make_unique<SliderAttachment>(processor.parameters, "presence", presenceSlider); compA = std::make_unique<SliderAttachment>(processor.parameters, "compThreshold", compSlider); deEssA = std::make_unique<SliderAttachment>(processor.parameters, "deEss", deEssSlider); delayA = std::make_unique<SliderAttachment>(processor.parameters, "delayMix", delaySlider); ratioA = std::make_unique<SliderAttachment>(processor.parameters, "compRatio", ratioSlider); attackA = std::make_unique<SliderAttachment>(processor.parameters, "compAttack", attackSlider); releaseA = std::make_unique<SliderAttachment>(processor.parameters, "compRelease", releaseSlider); parallelA = std::make_unique<SliderAttachment>(processor.parameters, "parallelComp", parallelSlider); ambientA = std::make_unique<SliderAttachment>(processor.parameters, "ambient", ambientSlider); delayTimeA = std::make_unique<SliderAttachment>(processor.parameters, "delayMs", delayTimeSlider);
    bypassA = std::make_unique<ButtonAttachment>(processor.parameters, "bypass", bypassButton);
    keyBox.addItemList({ "Chromatic", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 1); scaleBox.addItemList({ "Major", "Minor", "Dorian", "Pentatonic" }, 1); addAndMakeVisible(keyBox); addAndMakeVisible(scaleBox); keyA = std::make_unique<ComboAttachment>(processor.parameters, "key", keyBox); scaleA = std::make_unique<ComboAttachment>(processor.parameters, "scale", scaleBox);
    aiPrompt.setTextToShowWhenEmpty("Afrobeat vibe, clear voice, add a bit of space…", juce::Colour(0xff82949c)); aiPrompt.setMultiLine(true); addAndMakeVisible(aiPrompt); apiKeyEditor.setTextToShowWhenEmpty("Optional API key for online AI", juce::Colour(0xff82949c)); apiKeyEditor.setPasswordCharacter('*'); addAndMakeVisible(apiKeyEditor);
    activateAiButton.onClick = [this] { const int generated = processor.generateNextPreset(); presetBox.setSelectedId(factoryPresets.size() + generated + 1); aiStatus.setText("New offline AI preset generated and loaded.", juce::dontSendNotification); if (aiPrompt.getText().trim().isNotEmpty() && apiKeyEditor.getText().trim().isNotEmpty()) processor.requestAiPreset(aiPrompt.getText(), apiKeyEditor.getText()); }; addAndMakeVisible(activateAiButton);
    aiStatus.setText(processor.getAiStatus(), juce::dontSendNotification); aiStatus.setColour(juce::Label::textColourId, subtext); aiStatus.setFont(juce::Font(12.0f)); addAndMakeVisible(aiStatus); startTimerHz(12);
}

AfroVocalPresetsAudioProcessorEditor::~AfroVocalPresetsAudioProcessorEditor() { setLookAndFeel(nullptr); }

void AfroVocalPresetsAudioProcessorEditor::configureSlider(juce::Slider& s, const juce::String& suffix)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag); s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 82, 25); s.setTextValueSuffix(suffix); s.setColour(juce::Slider::rotarySliderFillColourId, teal); s.setColour(juce::Slider::rotarySliderOutlineColourId, cyan); s.setColour(juce::Slider::thumbColourId, text); s.setColour(juce::Slider::textBoxTextColourId, teal); s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff12222a)); s.setColour(juce::Slider::textBoxOutlineColourId, edge); s.setNumDecimalPlacesToDisplay(1);
}

void AfroVocalPresetsAudioProcessorEditor::timerCallback()
{
    processor.applyPendingAiPreset(); aiStatus.setText(processor.getAiStatus(), juce::dontSendNotification); meter = juce::jmax(0.01f, processor.getMeterPeak()); gainReduction = processor.getGainReduction(); repaint();
}

void AfroVocalPresetsAudioProcessorEditor::drawPanel(juce::Graphics& g, juce::Rectangle<float> r, const juce::String& name, juce::Colour accent)
{
    g.setColour(panel); g.fillRoundedRectangle(r, 6.0f); g.setColour(edge); g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, 1.0f); g.setColour(text); g.setFont(juce::Font(18.0f, juce::Font::bold)); g.drawText(name, r.getX() + 15.0f, r.getY() + 8.0f, r.getWidth() - 30.0f, 24.0f, juce::Justification::centred); g.setColour(accent); g.fillEllipse(r.getRight() - 20.0f, r.getY() + 14.0f, 7.0f, 7.0f); g.setColour(juce::Colour(0x556b7a82)); g.drawLine(r.getX() + 12.0f, r.getY() + 38.0f, r.getRight() - 12.0f, r.getY() + 38.0f, 1.0f);
}

void AfroVocalPresetsAudioProcessorEditor::drawKnobLabel(juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r)
{
    g.setColour(text); g.setFont(juce::Font(13.0f, juce::Font::bold)); g.drawText(s, r, juce::Justification::centred);
}

void AfroVocalPresetsAudioProcessorEditor::drawMeter(juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour(juce::Colour(0xff071117)); g.fillRoundedRectangle(r, 5.0f); g.setColour(edge); g.drawRoundedRectangle(r, 5.0f, 1.0f); const float gap = 2.0f; const float h = (r.getHeight() - 28.0f) / 36.0f;
    for (int channel = 0; channel < 2; ++channel) { const float x = r.getX() + 18.0f + channel * (r.getWidth() * 0.46f); g.setColour(subtext); g.setFont(juce::Font(12.0f, juce::Font::bold)); g.drawText(channel == 0 ? "L" : "R", x, r.getY() + 7.0f, 22.0f, 16.0f, juce::Justification::centred); for (int i = 0; i < 30; ++i) { const float y = r.getBottom() - 24.0f - (float) i * (h + gap); const bool lit = (float) i / 30.0f < meter * (channel == 0 ? 1.0f : 0.92f); g.setColour(lit ? (i > 24 ? gold : green) : juce::Colour(0xff26343b)); g.fillRect(x, y, 18.0f, h); } }
    g.setColour(subtext); g.setFont(juce::Font(10.0f)); g.drawText("0", r.getX() + 50.0f, r.getY() + 33.0f, 28.0f, 14.0f, juce::Justification::centred); g.drawText("-12", r.getX() + 50.0f, r.getCentreY() - 8.0f, 28.0f, 14.0f, juce::Justification::centred); g.drawText("-35", r.getX() + 50.0f, r.getBottom() - 30.0f, 28.0f, 14.0f, juce::Justification::centred);
}

void AfroVocalPresetsAudioProcessorEditor::drawGraph(juce::Graphics& g, juce::Rectangle<float> r, bool compressor)
{
    g.setColour(juce::Colour(0xff0a121a)); g.fillRoundedRectangle(r, 5.0f); g.setColour(juce::Colour(0xff435560)); g.drawRoundedRectangle(r, 5.0f, 1.0f); for (int i = 1; i < 5; ++i) { g.setColour(juce::Colour(0x334d6b76)); g.drawHorizontalLine((int)(r.getY() + i * r.getHeight() / 5.0f), r.getX(), r.getRight()); }
    juce::Path p; p.startNewSubPath(r.getX() + 6.0f, r.getBottom() - 16.0f); for (int i = 0; i <= 30; ++i) { const float x = r.getX() + 6.0f + i * (r.getWidth() - 12.0f) / 30.0f; const float wave = compressor ? std::sin(i * 0.16f) * 0.04f : std::sin(i * 0.84f) * 0.16f + std::sin(i * 0.22f) * 0.1f; p.lineTo(x, r.getCentreY() - wave * r.getHeight() - (compressor ? (i * r.getHeight() / 38.0f) : 0.0f)); } g.setColour(compressor ? cyan : teal); g.strokePath(p, juce::PathStrokeType(2.0f));
}

void AfroVocalPresetsAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg); const auto r = getLocalBounds().toFloat().reduced(8.0f); g.setColour(frame); g.fillRoundedRectangle(r, 8.0f); g.setColour(juce::Colour(0xff6b7b83)); g.drawRoundedRectangle(r, 8.0f, 1.0f);
    g.setColour(juce::Colour(0xff111a21)); g.fillRect(r.getX(), r.getY(), r.getWidth(), 56.0f); g.setColour(edge); g.drawHorizontalLine((int) r.getY() + 56, r.getX(), r.getRight());
    g.setColour(subtext); g.setFont(juce::Font(15.0f, juce::Font::bold)); g.drawText("Options ▼", 22, 18, 90, 24, juce::Justification::centred); g.drawText("A", 140, 18, 28, 24, juce::Justification::centred); g.drawText("B", 176, 18, 28, 24, juce::Justification::centred); g.drawText("C", 212, 18, 28, 24, juce::Justification::centred); g.drawText("D", 248, 18, 28, 24, juce::Justification::centred); g.drawText("Copy", 310, 18, 52, 24, juce::Justification::centred); g.drawText("Paste", 370, 18, 58, 24, juce::Justification::centred); g.drawText("Input", 482, 18, 58, 24, juce::Justification::centred); g.drawText("Settings", r.getRight() - 172, 18, 82, 24, juce::Justification::centred); g.drawText("—  ×", r.getRight() - 68, 18, 52, 24, juce::Justification::centred);
    const auto input = juce::Rectangle<float>(14, 65, 294, 300); const auto leftPreset = juce::Rectangle<float>(14, 376, 294, 316); const auto comp = juce::Rectangle<float>(325, 65, 280, 300); const auto tone = juce::Rectangle<float>(614, 65, 333, 300); const auto fx = juce::Rectangle<float>(614, 376, 333, 316); const auto meterArea = juce::Rectangle<float>(956, 65, 130, 627); const auto rightPreset = juce::Rectangle<float>(1108, 65, 286, 405); const auto ai = juce::Rectangle<float>(1108, 480, 286, 212);
    drawPanel(g, input, "Input / Gate", cyan); drawPanel(g, comp, "Compressor", teal); drawPanel(g, tone, "Tone / THD", teal); drawPanel(g, fx, "FX / Space", purple); drawPanel(g, meterArea, "Voice Meter", teal);
    drawPanel(g, leftPreset, "Presets", cyan); drawPanel(g, rightPreset, "Presets", teal); drawPanel(g, ai, "AI", cyan);
    g.setColour(subtext); g.setFont(juce::Font(12.0f, juce::Font::bold)); g.drawText("Key/scale", 115, 253, 95, 18, juce::Justification::centred); g.drawText("Input", 32, 320, 60, 18, juce::Justification::left); g.drawText("-dB", 265, 320, 34, 18, juce::Justification::right); g.setColour(teal); g.fillRect(juce::Rectangle<float>(36.0f, 341.0f, 245.0f * juce::jlimit(0.08f, 1.0f, meter), 8.0f)); g.setColour(juce::Colour(0xff2e3d43)); g.drawRect(36.0f, 341.0f, 245.0f, 8.0f, 1.0f);
    drawKnobLabel(g, "HPF", { 34, 103, 78, 25 }); drawKnobLabel(g, "Tune", { 117, 103, 78, 25 }); drawKnobLabel(g, "Focus", { 200, 103, 78, 25 }); drawKnobLabel(g, "Threshold", { 340, 103, 78, 25 }); drawKnobLabel(g, "Ratio", { 430, 103, 78, 25 }); drawKnobLabel(g, "Attack", { 520, 103, 78, 25 }); drawKnobLabel(g, "Low Mid", { 630, 103, 88, 25 }); drawKnobLabel(g, "Presence", { 730, 103, 88, 25 }); drawKnobLabel(g, "Output", { 830, 103, 88, 25 });
    drawKnobLabel(g, "Attack", { 340, 230, 78, 25 }); drawKnobLabel(g, "Parallel", { 430, 230, 78, 25 }); drawKnobLabel(g, "Release", { 520, 230, 78, 25 }); drawKnobLabel(g, "Air", { 665, 230, 88, 25 }); drawKnobLabel(g, "Warmth", { 775, 230, 88, 25 }); drawKnobLabel(g, "Output", { 865, 230, 70, 25 }); drawKnobLabel(g, "De-Ess", { 630, 415, 88, 25 }); drawKnobLabel(g, "Plate", { 730, 415, 88, 25 }); drawKnobLabel(g, "Delay", { 830, 415, 88, 25 }); drawKnobLabel(g, "Ambient", { 630, 555, 88, 25 }); drawKnobLabel(g, "Time", { 730, 555, 88, 25 });
    drawGraph(g, { 340, 265, 252, 84 }, true); drawGraph(g, { 630, 160, 288, 175 }, false); drawGraph(g, { 730, 545, 188, 120 }, false); drawMeter(g, { 970, 113, 102, 560 });
    g.setColour(juce::Colour(0xff0e171d)); g.fillRoundedRectangle(leftPreset.reduced(14.0f, 52.0f), 5.0f); g.fillRoundedRectangle(rightPreset.reduced(14.0f, 52.0f), 5.0f); g.setColour(text); g.setFont(juce::Font(14.0f, juce::Font::bold)); g.drawText("Factory + AI Generated", rightPreset.getX() + 16, rightPreset.getY() + 58, rightPreset.getWidth() - 32, 22, juce::Justification::left); for (int i = 0; i < factoryPresets.size(); ++i) { const float y = leftPreset.getY() + 65.0f + i * 23.0f; if (i == 0) { g.setColour(juce::Colour(0xff2c7a86)); g.fillRoundedRectangle(leftPreset.getX() + 8.0f, y - 4.0f, leftPreset.getWidth() - 16.0f, 22.0f, 4.0f); } g.setColour(text); g.setFont(juce::Font(13.0f, i == 0 ? juce::Font::bold : juce::Font::plain)); g.drawText(factoryPresets[i], leftPreset.getX() + 20.0f, y, leftPreset.getWidth() - 30.0f, 18.0f, juce::Justification::left); }
    for (int i = 0; i < factoryPresets.size(); ++i) { const float y = rightPreset.getY() + 96.0f + i * 27.0f; if (i == 0) { g.setColour(juce::Colour(0xff2c7a86)); g.fillRoundedRectangle(rightPreset.getX() + 8.0f, y - 4.0f, rightPreset.getWidth() - 16.0f, 24.0f, 4.0f); } g.setColour(text); g.setFont(juce::Font(13.0f, i == 0 ? juce::Font::bold : juce::Font::plain)); g.drawText(factoryPresets[i], rightPreset.getX() + 20.0f, y, rightPreset.getWidth() - 30.0f, 18.0f, juce::Justification::left); }
    g.setColour(subtext); g.setFont(juce::Font(13.0f)); g.drawText("Example/Prompt:", ai.getX() + 16, ai.getY() + 112, ai.getWidth() - 32, 18, juce::Justification::left); g.drawText("API/AI status", ai.getX() + 16, ai.getBottom() - 28, ai.getWidth() - 32, 18, juce::Justification::left);
    g.setColour(juce::Colour(0xff101a21)); g.fillRect(r.getX(), r.getBottom() - 50.0f, r.getWidth(), 50.0f); g.setColour(edge); g.drawHorizontalLine((int) r.getBottom() - 50, r.getX(), r.getRight()); g.setColour(teal); g.setFont(juce::Font(19.0f, juce::Font::bold)); g.drawText("◈ AfroVocal", 22, (int) r.getBottom() - 37, 190, 25, juce::Justification::left); g.setColour(cyan); g.drawText("Preset", 204, (int) r.getBottom() - 37, 80, 25, juce::Justification::left); g.setColour(subtext); g.setFont(juce::Font(13.0f)); g.drawText("AI", r.getRight() - 250, (int) r.getBottom() - 35, 36, 20, juce::Justification::centred); g.drawText("Stereo ▼", r.getRight() - 190, (int) r.getBottom() - 35, 75, 20, juce::Justification::centred); g.drawText("Version 7.5.1", r.getRight() - 105, (int) r.getBottom() - 35, 98, 20, juce::Justification::centred);
}

void AfroVocalPresetsAudioProcessorEditor::resized()
{
    const auto r = getLocalBounds().reduced(8); titleLabel.setBounds(22, 16, 210, 30); presetBox.setBounds(548, 12, 342, 34); savePresetButton.setBounds(904, 12, 68, 34); bypassButton.setBounds(982, 12, 92, 34); keyBox.setBounds(35, 274, 126, 32); scaleBox.setBounds(163, 274, 126, 32);
    auto place3 = [](juce::Slider& a, juce::Slider& b, juce::Slider& c, int x, int y, int w) { const int cell = w / 3; a.setBounds(x + 2, y, cell - 4, 126); b.setBounds(x + cell + 2, y, cell - 4, 126); c.setBounds(x + 2 * cell + 2, y, cell - 4, 126); };
    place3(hpSlider, tuneSlider, focusSlider, 28, 125, 264); place3(compSlider, ratioSlider, attackSlider, 337, 125, 256); place3(attackSlider, parallelSlider, releaseSlider, 337, 247, 256); place3(lowMidSlider, presenceSlider, outputSlider, 628, 125, 306); place3(deEssSlider, spaceSlider, delaySlider, 628, 437, 306); place3(ambientSlider, delayTimeSlider, warmthSlider, 628, 563, 306); airSlider.setBounds(720, 220, 88, 126); outputSlider.setBounds(824, 125, 88, 126); warmthSlider.setBounds(824, 563, 88, 126); lowMidSlider.setBounds(628, 125, 88, 126); presenceSlider.setBounds(730, 125, 88, 126); compSlider.setBounds(337, 125, 78, 126); ratioSlider.setBounds(425, 125, 78, 126); attackSlider.setBounds(513, 125, 78, 126); parallelSlider.setBounds(425, 247, 78, 126); releaseSlider.setBounds(513, 247, 78, 126); hpSlider.setBounds(28, 125, 78, 126); tuneSlider.setBounds(116, 125, 78, 126); focusSlider.setBounds(204, 125, 78, 126); deEssSlider.setBounds(628, 437, 88, 126); spaceSlider.setBounds(730, 437, 88, 126); delaySlider.setBounds(832, 437, 88, 126); ambientSlider.setBounds(628, 563, 88, 126); delayTimeSlider.setBounds(730, 563, 88, 126);
    aiPrompt.setBounds(1122, 590, 258, 50); apiKeyEditor.setBounds(1122, 646, 258, 22); activateAiButton.setBounds(1122, 528, 258, 42); aiStatus.setBounds(1122, 673, 258, 14);
}
