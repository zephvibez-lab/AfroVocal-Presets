#include "PluginEditor.h"

namespace
{
    const juce::Colour canvas { 0xff0d1013 }, chrome { 0xff191e22 }, metal { 0xffd0d0c8 }, metalDark { 0xffb8b9b2 }, ink { 0xff1d2529 }, mutedInk { 0xff4d5659 }, white { 0xfff2f4ef }, cyan { 0xff72c9c8 }, teal { 0xff289899 }, blue { 0xff3179c8 }, red { 0xffd93842 }, amber { 0xffd4a14e };
    const juce::StringArray factoryPresets { "Afrobeat Lead - Clear Bounce", "Afrobeat Lead - Warm Pocket", "Amapiano Lead - Gloss", "Amapiano Lead - Soft Air", "Emotional Ballad - Intimate", "Emotional Ballad - Wide", "Background Harmony - Tucked", "Background Harmony - Airy Stack", "Hard-Tuned Lead - Modern", "Hard-Tuned Lead - Dry" };
}

AfroVocalPresetsAudioProcessorEditor::AfroVocalPresetsAudioProcessorEditor(AfroVocalPresetsAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setSize(1540, 900); setResizable(true, true); setResizeLimits(1240, 760, 2200, 1300);
    titleLabel.setText("AFROVOCAL PRESETS", juce::dontSendNotification); titleLabel.setFont(juce::Font(22.0f, juce::Font::bold)); titleLabel.setColour(juce::Label::textColourId, white); addAndMakeVisible(titleLabel);
    for (int i = 0; i < factoryPresets.size(); ++i) presetBox.addItem(factoryPresets[i], i + 1);
    presetBox.setSelectedId(1); presetBox.onChange = [this] { processor.applyFactoryPreset(presetBox.getText()); }; addAndMakeVisible(presetBox);
    savePresetButton.onClick = [this] { aiStatus.setText("Preset state is stored with the DAW project.", juce::dontSendNotification); }; addAndMakeVisible(savePresetButton);
    bypassButton.setClickingTogglesState(true); addAndMakeVisible(bypassButton);

    configureSlider(tuneSlider, "%"); configureSlider(focusSlider, "%"); configureSlider(airSlider, "%"); configureSlider(spaceSlider, "%"); configureSlider(warmthSlider, "%"); configureSlider(outputSlider, " dB");
    configureSlider(hpSlider, " Hz"); configureSlider(lowMidSlider, " dB"); configureSlider(presenceSlider, " dB"); configureSlider(compSlider, " dB"); configureSlider(deEssSlider, "%"); configureSlider(delaySlider, "%");
    configureSlider(ratioSlider, ":1"); configureSlider(attackSlider, " ms"); configureSlider(releaseSlider, " ms"); configureSlider(parallelSlider, "%"); configureSlider(ambientSlider, "%"); configureSlider(delayTimeSlider, " ms");
    for (auto* s : { &tuneSlider, &focusSlider, &airSlider, &spaceSlider, &warmthSlider, &outputSlider, &hpSlider, &lowMidSlider, &presenceSlider, &compSlider, &deEssSlider, &delaySlider, &ratioSlider, &attackSlider, &releaseSlider, &parallelSlider, &ambientSlider, &delayTimeSlider }) addAndMakeVisible(*s);
    tuneA = std::make_unique<SliderAttachment>(processor.parameters, "tuneAmount", tuneSlider); focusA = std::make_unique<SliderAttachment>(processor.parameters, "vocalFocus", focusSlider); airA = std::make_unique<SliderAttachment>(processor.parameters, "air", airSlider); spaceA = std::make_unique<SliderAttachment>(processor.parameters, "plate", spaceSlider); warmthA = std::make_unique<SliderAttachment>(processor.parameters, "warmth", warmthSlider); outputA = std::make_unique<SliderAttachment>(processor.parameters, "outputGain", outputSlider);
    hpA = std::make_unique<SliderAttachment>(processor.parameters, "highPass", hpSlider); lowMidA = std::make_unique<SliderAttachment>(processor.parameters, "lowMidCut", lowMidSlider); presenceA = std::make_unique<SliderAttachment>(processor.parameters, "presence", presenceSlider); compA = std::make_unique<SliderAttachment>(processor.parameters, "compThreshold", compSlider); deEssA = std::make_unique<SliderAttachment>(processor.parameters, "deEss", deEssSlider); delayA = std::make_unique<SliderAttachment>(processor.parameters, "delayMix", delaySlider);
    ratioA = std::make_unique<SliderAttachment>(processor.parameters, "compRatio", ratioSlider); attackA = std::make_unique<SliderAttachment>(processor.parameters, "compAttack", attackSlider); releaseA = std::make_unique<SliderAttachment>(processor.parameters, "compRelease", releaseSlider); parallelA = std::make_unique<SliderAttachment>(processor.parameters, "parallelComp", parallelSlider); ambientA = std::make_unique<SliderAttachment>(processor.parameters, "ambient", ambientSlider); delayTimeA = std::make_unique<SliderAttachment>(processor.parameters, "delayMs", delayTimeSlider);
    bypassA = std::make_unique<ButtonAttachment>(processor.parameters, "bypass", bypassButton);

    keyBox.addItemList({ "Chromatic", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 1); scaleBox.addItemList({ "Major", "Minor", "Dorian", "Pentatonic" }, 1); addAndMakeVisible(keyBox); addAndMakeVisible(scaleBox); keyA = std::make_unique<ComboAttachment>(processor.parameters, "key", keyBox); scaleA = std::make_unique<ComboAttachment>(processor.parameters, "scale", scaleBox);
    aiPrompt.setTextToShowWhenEmpty("Example: intimate Amapiano lead, soft plate, fast tune…", juce::Colour(0xff7b8587)); aiPrompt.setMultiLine(false); addAndMakeVisible(aiPrompt);
    apiKeyEditor.setTextToShowWhenEmpty("Optional API key for online assistance", juce::Colour(0xff7b8587)); apiKeyEditor.setPasswordCharacter('*'); addAndMakeVisible(apiKeyEditor);
    activateAiButton.onClick = [this] { processor.requestAiPreset(aiPrompt.getText(), apiKeyEditor.getText()); }; addAndMakeVisible(activateAiButton);
    aiStatus.setText(processor.getAiStatus(), juce::dontSendNotification); aiStatus.setColour(juce::Label::textColourId, juce::Colour(0xff9ba8aa)); aiStatus.setFont(juce::Font(11.0f)); addAndMakeVisible(aiStatus);
    startTimerHz(12);
}

AfroVocalPresetsAudioProcessorEditor::~AfroVocalPresetsAudioProcessorEditor() = default;

void AfroVocalPresetsAudioProcessorEditor::configureSlider(juce::Slider& s, const juce::String& suffix)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag); s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 62, 18); s.setTextValueSuffix(suffix);
    s.setColour(juce::Slider::rotarySliderFillColourId, teal); s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff7f8883)); s.setColour(juce::Slider::thumbColourId, white); s.setColour(juce::Slider::textBoxTextColourId, ink); s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xffe0e0d9)); s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack); s.setNumDecimalPlacesToDisplay(1);
}

void AfroVocalPresetsAudioProcessorEditor::timerCallback()
{
    processor.applyPendingAiPreset(); aiStatus.setText(processor.getAiStatus(), juce::dontSendNotification); meter = juce::jmax(0.02f, processor.getMeterPeak()); gainReduction = processor.getGainReduction(); repaint();
}

void AfroVocalPresetsAudioProcessorEditor::drawModule(juce::Graphics& g, juce::Rectangle<int> area, const juce::String& name, juce::Colour accentColour, bool enabled)
{
    g.setColour(metal); g.fillRoundedRectangle(area.toFloat(), 5.0f); g.setColour(juce::Colour(0x44000000)); g.drawRoundedRectangle(area.toFloat().reduced(0.5f), 5.0f, 1.0f);
    g.setColour(ink); g.setFont(juce::Font(13.0f, juce::Font::bold)); g.drawText(name.toUpperCase(), area.getX() + 13, area.getY() + 8, area.getWidth() - 42, 20, juce::Justification::left);
    g.setColour(enabled ? accentColour : mutedInk); g.fillEllipse(static_cast<float>(area.getRight() - 21), static_cast<float>(area.getY() + 10), 7.0f, 7.0f);
    g.setColour(juce::Colour(0x22000000)); g.drawLine(static_cast<float>(area.getX() + 8), static_cast<float>(area.getY() + 28), static_cast<float>(area.getRight() - 8), static_cast<float>(area.getY() + 28), 1.0f);
}

void AfroVocalPresetsAudioProcessorEditor::drawMeter(juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour(juce::Colour(0xff11181b)); g.fillRoundedRectangle(area, 5.0f); g.setColour(juce::Colour(0xff465459)); g.drawRoundedRectangle(area, 5.0f, 1.0f);
    const int bars = 20; const float gap = 3.0f; const float h = (area.getHeight() - gap * (bars + 1)) / static_cast<float>(bars);
    for (int i = 0; i < bars; ++i)
    {
        const float y = area.getBottom() - gap - (i + 1) * (h + gap); const bool lit = static_cast<float>(i) / static_cast<float>(bars) < meter;
        g.setColour(lit ? (i > 16 ? red : (i > 12 ? amber : cyan)) : juce::Colour(0xff263337)); g.fillRoundedRectangle(area.getX() + 9.0f, y, area.getWidth() - 18.0f, h, 2.0f);
    }
    g.setColour(juce::Colour(0xffb9c6c8)); g.setFont(juce::Font(11.0f)); for (int i = 0; i < 5; ++i) g.drawText(juce::String(-i * 6) + " dB", static_cast<int>(area.getRight() + 6), static_cast<int>(area.getY() + 14 + i * area.getHeight() / 5), 55, 15, juce::Justification::left);
    g.setColour(cyan); g.setFont(juce::Font(12.0f, juce::Font::bold)); g.drawText("PEAK", static_cast<int>(area.getX() + 9), static_cast<int>(area.getBottom() - 27), static_cast<int>(area.getWidth() - 18), 16, juce::Justification::centred);
    g.setColour(amber); g.drawText("GR  " + juce::String(gainReduction * 12.0f, 1) + " dB", static_cast<int>(area.getX() + 9), static_cast<int>(area.getBottom() - 42), static_cast<int>(area.getWidth() - 18), 14, juce::Justification::centred);
}

void AfroVocalPresetsAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(canvas); const auto r = getLocalBounds().reduced(12); g.setColour(chrome); g.fillRoundedRectangle(r.toFloat(), 8.0f);
    g.setColour(juce::Colour(0xff3d4546)); g.drawRoundedRectangle(r.toFloat(), 8.0f, 1.0f);
    g.setColour(juce::Colour(0xff101416)); g.fillRect(r.getX(), r.getY(), r.getWidth(), 49); g.setColour(juce::Colour(0xff363e40)); g.drawHorizontalLine(r.getY() + 49, static_cast<float>(r.getX()), static_cast<float>(r.getRight()));
    g.setColour(mutedInk); g.setFont(juce::Font(12.0f, juce::Font::bold)); g.drawText("A", 285, 29, 24, 18, juce::Justification::centred); g.drawText("B", 317, 29, 24, 18, juce::Justification::centred); g.drawText("COPY", 350, 29, 52, 18, juce::Justification::centred); g.drawText("PASTE", 410, 29, 58, 18, juce::Justification::centred); g.drawText("INPUT", 480, 29, 64, 18, juce::Justification::centred);

    const auto body = r.withTop(r.getY() + 58).withBottom(r.getBottom() - 58); const int browserWidth = juce::jmax(190, static_cast<int>(body.getWidth() * 0.205f)); const int meterWidth = juce::jmax(118, static_cast<int>(body.getWidth() * 0.135f));
    const int left = body.getX(), right = body.getRight() - browserWidth, gap = 7; const int laneWidth = (right - left - meterWidth - gap * 5) / 4; int x = left;
    auto lane = [&](const juce::String& name, juce::Colour c, int width) { auto a = juce::Rectangle<int>(x, body.getY(), width, body.getHeight()); drawModule(g, a, name, c); x += width + gap; return a; };
    const auto inputArea = lane("INPUT / GATE", blue, laneWidth); const auto dynArea = lane("COMPRESSOR", red, laneWidth); const auto toneArea = lane("TONE / THD", cyan, laneWidth); const auto fxArea = lane("FX / SPACE", amber, laneWidth);
    auto meterArea = juce::Rectangle<int>(x, body.getY(), meterWidth, body.getHeight()); drawModule(g, meterArea, "VOICE METER", cyan); drawMeter(g, meterArea.reduced(17, 42).toFloat()); x += meterWidth + gap;
    auto browserArea = juce::Rectangle<int>(x, body.getY(), right + browserWidth - x, body.getHeight()); g.setColour(juce::Colour(0xff101417)); g.fillRoundedRectangle(browserArea.toFloat(), 5.0f); g.setColour(juce::Colour(0xff495355)); g.drawRoundedRectangle(browserArea.toFloat(), 5.0f, 1.0f); g.setColour(white); g.setFont(juce::Font(12.0f, juce::Font::bold)); g.drawText("PRESETS", browserArea.getX() + 13, browserArea.getY() + 14, browserArea.getWidth() - 26, 17, juce::Justification::left); g.setColour(juce::Colour(0xff798589)); g.setFont(juce::Font(10.0f)); g.drawText("AFROBEAT VOCALS", browserArea.getX() + 13, browserArea.getY() + 36, browserArea.getWidth() - 26, 15, juce::Justification::left);
    g.setColour(juce::Colour(0xff252d31)); g.fillRect(browserArea.getX() + 10, browserArea.getY() + 61, browserArea.getWidth() - 20, 1);
    for (int i = 0; i < factoryPresets.size(); ++i) { const int yy = browserArea.getY() + 73 + i * 27; if (presetBox.getSelectedId() == i + 1) { g.setColour(juce::Colour(0xff294d52)); g.fillRoundedRectangle(browserArea.getX() + 7.0f, static_cast<float>(yy - 4), browserArea.getWidth() - 14.0f, 22.0f, 3.0f); } g.setColour(i == presetBox.getSelectedId() - 1 ? cyan : juce::Colour(0xffb8c0c0)); g.setFont(juce::Font(10.0f, i == presetBox.getSelectedId() - 1 ? juce::Font::bold : juce::Font::plain)); g.drawText(factoryPresets[i], browserArea.getX() + 15, yy, browserArea.getWidth() - 26, 14, juce::Justification::left); }

    auto labels = [&](juce::Rectangle<int> area, const juce::StringArray& names) { const int cell = area.getWidth() / names.size(); for (int i = 0; i < names.size(); ++i) { g.setColour(ink); g.setFont(juce::Font(9.0f, juce::Font::bold)); g.drawText(names[i], area.getX() + i * cell, area.getY(), cell, 16, juce::Justification::centred); } };
    labels(inputArea.withTop(inputArea.getY() + 42).withHeight(16), { "HPF", "TUNE", "FOCUS" }); labels(dynArea.withTop(dynArea.getY() + 42).withHeight(16), { "THRESH", "RATIO", "ATTACK" }); labels(toneArea.withTop(toneArea.getY() + 42).withHeight(16), { "LOW MID", "PRESENCE", "AIR" }); labels(fxArea.withTop(fxArea.getY() + 42).withHeight(16), { "DE-ESS", "PLATE", "DELAY" });
    labels(dynArea.withTop(dynArea.getY() + 242).withHeight(16), { "PARALLEL", "RELEASE", "" }); labels(toneArea.withTop(toneArea.getY() + 242).withHeight(16), { "WARMTH", "OUTPUT", "" }); labels(fxArea.withTop(fxArea.getY() + 242).withHeight(16), { "AMBIENT", "TIME", "" });
    g.setColour(metal); g.fillRect(r.getX(), r.getBottom() - 49, r.getWidth(), 49); g.setColour(mutedInk); g.setFont(juce::Font(9.0f)); g.drawText("THD / TONE", r.getX() + 16, r.getBottom() - 31, 100, 15, juce::Justification::left); g.drawText("STEREO MODE", r.getRight() - 265, r.getBottom() - 31, 100, 15, juce::Justification::left); g.drawText("AI AUDIO ASSISTANCE", r.getRight() - 500, r.getBottom() - 31, 140, 15, juce::Justification::left);
}

void AfroVocalPresetsAudioProcessorEditor::resized()
{
    const auto r = getLocalBounds().reduced(14); const auto body = r.withTop(r.getY() + 66).withBottom(r.getBottom() - 68); const int browserWidth = juce::jmax(240, static_cast<int>(body.getWidth() * 0.20f)); const int meterWidth = juce::jmax(150, static_cast<int>(body.getWidth() * 0.13f)); const int left = body.getX(), right = body.getRight() - browserWidth, gap = 10, laneWidth = (right - left - meterWidth - gap * 5) / 4; int x = left;
    titleLabel.setBounds(r.getX() + 20, r.getY() + 14, 270, 30); presetBox.setBounds(r.getX() + 760, r.getY() + 14, 300, 34); savePresetButton.setBounds(r.getX() + 1080, r.getY() + 14, 82, 34); bypassButton.setBounds(r.getRight() - 126, r.getY() + 14, 106, 34);
    auto lane = [&](int width) { auto a = juce::Rectangle<int>(x, body.getY(), width, body.getHeight()); x += width + gap; return a; }; const auto input = lane(laneWidth), dyn = lane(laneWidth), tone = lane(laneWidth), fx = lane(laneWidth); x += meterWidth + gap;
    const int top = body.getY() + 65, rowH = 154, rowGap = 32;
    auto three = [&](juce::Slider& a, juce::Slider& b, juce::Slider& c, juce::Rectangle<int> area) { const int w = area.getWidth() / 3; a.setBounds(area.getX() + 8, area.getY(), w - 13, rowH); b.setBounds(area.getX() + w + 3, area.getY(), w - 13, rowH); c.setBounds(area.getX() + 2 * w - 2, area.getY(), w - 10, rowH); };
    three(hpSlider, tuneSlider, focusSlider, input.withTop(top)); three(compSlider, ratioSlider, attackSlider, dyn.withTop(top)); three(lowMidSlider, presenceSlider, airSlider, tone.withTop(top)); three(deEssSlider, spaceSlider, delaySlider, fx.withTop(top));
    keyBox.setBounds(input.getX() + 14, top + rowH + 28, input.getWidth() - 28, 36); scaleBox.setBounds(input.getX() + 14, top + rowH + 74, input.getWidth() - 28, 36);
    auto two = [&](juce::Slider& a, juce::Slider& b, juce::Rectangle<int> area) { const int w = area.getWidth() / 2; a.setBounds(area.getX() + 20, area.getY(), w - 28, rowH); b.setBounds(area.getX() + w + 8, area.getY(), w - 28, rowH); };
    two(parallelSlider, releaseSlider, dyn.withTop(top + rowH + rowGap)); two(warmthSlider, outputSlider, tone.withTop(top + rowH + rowGap)); two(ambientSlider, delayTimeSlider, fx.withTop(top + rowH + rowGap));
    aiPrompt.setBounds(r.getX() + 20, r.getBottom() - 49, 280, 32); apiKeyEditor.setBounds(r.getX() + 310, r.getBottom() - 49, 280, 32); activateAiButton.setBounds(r.getX() + 600, r.getBottom() - 49, 105, 32); aiStatus.setBounds(r.getX() + 720, r.getBottom() - 49, 430, 32);
}
