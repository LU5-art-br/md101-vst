#include "PluginEditor.h"

PeriodicSynthAudioProcessorEditor::DarkStudioLookAndFeel::DarkStudioLookAndFeel()
{
    setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff161d2b));
    setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff233047));
    setColour (juce::TextButton::textColourOffId,  juce::Colour (0xffc9d4e8));
    setColour (juce::TextButton::textColourOnId,   juce::Colours::white);
}

void PeriodicSynthAudioProcessorEditor::DarkStudioLookAndFeel::drawButtonBackground (
    juce::Graphics& g,
    juce::Button& button,
    const juce::Colour&,
    bool isHighlighted,
    bool isDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (0.8f);
    const bool toggled = button.getToggleState();

    juce::Colour bg = toggled ? accent.withAlpha (0.28f) : juce::Colour (0xff151c29);
    if (isDown)
        bg = bg.brighter (0.18f);
    else if (isHighlighted)
        bg = bg.brighter (0.10f);

    g.setColour (bg);
    g.fillRoundedRectangle (bounds, 5.0f);

    g.setColour (toggled ? accent.brighter (0.25f)
                         : (isHighlighted ? accent.withAlpha (0.75f) : juce::Colour (0xff283348)));
    g.drawRoundedRectangle (bounds, 5.0f, toggled ? 1.5f : 1.0f);
}

void PeriodicSynthAudioProcessorEditor::DarkStudioLookAndFeel::drawLinearSlider (
    juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos, float, float,
    const juce::Slider::SliderStyle,
    juce::Slider&)
{
    const float trackH = 5.0f;
    const float cy = static_cast<float> (y) + static_cast<float> (height) * 0.5f;
    const juce::Rectangle<float> track (static_cast<float> (x), cy - trackH * 0.5f, static_cast<float> (width), trackH);

    g.setColour (juce::Colour (0xff1a2232));
    g.fillRoundedRectangle (track, 2.5f);

    const float fillW = juce::jlimit (0.0f, static_cast<float> (width), sliderPos - static_cast<float> (x));
    if (fillW > 1.0f)
    {
        g.setColour (accent.brighter (0.15f));
        g.fillRoundedRectangle (track.withWidth (fillW), 2.5f);
    }

    const float thumbR = 6.0f;
    g.setColour (accent.withAlpha (0.30f));
    g.fillEllipse (sliderPos - thumbR - 2.0f, cy - thumbR - 2.0f, (thumbR + 2.0f) * 2.0f, (thumbR + 2.0f) * 2.0f);
    g.setColour (juce::Colours::white);
    g.fillEllipse (sliderPos - thumbR * 0.75f, cy - thumbR * 0.75f, thumbR * 1.5f, thumbR * 1.5f);
}

PeriodicSynthAudioProcessorEditor::PeriodicSynthAudioProcessorEditor (PeriodicSynthAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      pianoKeyboard (p.getKeyboardState())
{
    setLookAndFeel (&studioLaf);

    addAndMakeVisible (periodicTable);
    addAndMakeVisible (pianoKeyboard);

    periodicTable.onElementSelected = [this] (int idx)
    {
        audioProcessor.selectElementIndex (idx);
        syncAccentTheme (idx);
        pianoKeyboard.grabKeyboardFocus();
    };

    // Preset navigation buttons
    addAndMakeVisible (prevPresetBtn);
    addAndMakeVisible (nextPresetBtn);
    prevPresetBtn.onClick = [this]
    {
        const int cur = audioProcessor.getSelectedElementIndex();
        const int prev = (cur - 1 + PeriodicSynth::kNumElements) % PeriodicSynth::kNumElements;
        audioProcessor.selectElementIndex (prev);
        syncAccentTheme (prev);
    };
    nextPresetBtn.onClick = [this]
    {
        const int cur = audioProcessor.getSelectedElementIndex();
        const int next = (cur + 1) % PeriodicSynth::kNumElements;
        audioProcessor.selectElementIndex (next);
        syncAccentTheme (next);
    };

    // Master Gain & Space sliders
    gainSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    gainSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (gainSlider);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getApvts(), "masterGain", gainSlider);

    spaceSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    spaceSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (spaceSlider);
    spaceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getApvts(), "space", spaceSlider);

    // Piano footer controls
    addAndMakeVisible (octDownBtn);
    addAndMakeVisible (octUpBtn);
    addAndMakeVisible (holdBtn);
    addAndMakeVisible (panicBtn);

    authorLinkBtn.setFont (juce::FontOptions (10.5f, juce::Font::bold), false, juce::Justification::centredLeft);
    authorLinkBtn.setColour (juce::HyperlinkButton::textColourId, juce::Colour (0xff8fa3c4));
    addAndMakeVisible (authorLinkBtn);

    octDownBtn.onClick = [this]
    {
        pianoKeyboard.setOctaveShift (pianoKeyboard.getOctaveShift() - 1);
        updatePianoRangeLabel();
        repaint();
    };
    octUpBtn.onClick = [this]
    {
        pianoKeyboard.setOctaveShift (pianoKeyboard.getOctaveShift() + 1);
        updatePianoRangeLabel();
        repaint();
    };

    holdBtn.setClickingTogglesState (true);
    holdBtn.onClick = [this]
    {
        audioProcessor.getSynthEngine().setSustainHold (holdBtn.getToggleState());
    };

    panicBtn.onClick = [this]
    {
        holdBtn.setToggleState (false, juce::dontSendNotification);
        audioProcessor.getSynthEngine().setSustainHold (false);
        audioProcessor.getKeyboardState().allNotesOff (0);
        audioProcessor.getSynthEngine().panic();
    };

    syncAccentTheme (audioProcessor.getSelectedElementIndex());
    updatePianoRangeLabel();

    setResizable (true, true);
    setResizeLimits (940, 620, 1920, 1200);
    setSize (1160, 730);

    startTimerHz (30);
}

PeriodicSynthAudioProcessorEditor::~PeriodicSynthAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void PeriodicSynthAudioProcessorEditor::syncAccentTheme (int elementIndex)
{
    const int clamped = juce::jlimit (0, PeriodicSynth::kNumElements - 1, elementIndex);
    lastSyncedElement = clamped;
    periodicTable.setSelectedElement (clamped);

    const auto& el = PeriodicSynth::getElementByIndex (clamped);
    const auto col = PeriodicSynth::getCategoryColour (el.category);
    studioLaf.setAccent (col);
    pianoKeyboard.setAccentColour (col);
    authorLinkBtn.setColour (juce::HyperlinkButton::textColourId, col.brighter (0.25f));
    repaint();
}

void PeriodicSynthAudioProcessorEditor::updatePianoRangeLabel()
{
    const int lowOct  = (pianoKeyboard.getLowestNote() / 12) - 1;
    const int highOct = (pianoKeyboard.getHighestNote() / 12) - 1;
    pianoRangeText = "C" + juce::String (lowOct) + " - C" + juce::String (highOct);
}

void PeriodicSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto& activeEl = PeriodicSynth::getElementByIndex (audioProcessor.getSelectedElementIndex());
    const auto accent    = PeriodicSynth::getCategoryColour (activeEl.category);

    // 1. Deep Dark Studio Background with subtle top ambient glow in the active element's category color
    juce::ColourGradient bgGrad (juce::Colour (0xff101521), bounds.getCentreX(), 0.0f,
                                 juce::Colour (0xff090c13), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill (bgGrad);
    g.fillAll();

    juce::ColourGradient auraGrad (accent.withAlpha (0.09f), bounds.getCentreX(), 45.0f,
                                   juce::Colours::transparentBlack, bounds.getCentreX(), bounds.getHeight() * 0.62f, true);
    g.setGradientFill (auraGrad);
    g.fillRect (bounds);

    // 2. Top Studio Header Bar
    const juce::Rectangle<float> headerBar (0.0f, 0.0f, bounds.getWidth(), 40.0f);
    g.setColour (juce::Colour (0xff0d111a));
    g.fillRect (headerBar);
    g.setColour (juce::Colour (0xff1e2738));
    g.drawHorizontalLine (39, 0.0f, bounds.getWidth());

    // Glowing atomic orbital icon
    const juce::Point<float> iconC (24.0f, 20.0f);
    g.setColour (accent.withAlpha (0.28f));
    g.fillEllipse (iconC.x - 11.0f, iconC.y - 11.0f, 22.0f, 22.0f);
    g.setColour (accent.brighter (0.3f));
    g.drawEllipse (iconC.x - 9.0f, iconC.y - 4.5f, 18.0f, 9.0f, 1.3f);
    g.drawEllipse (iconC.x - 4.5f, iconC.y - 9.0f, 9.0f, 18.0f, 1.3f);
    g.fillEllipse (iconC.x - 2.5f, iconC.y - 2.5f, 5.0f, 5.0f);

    // Brand Title & Subtitle
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    g.drawText ("Md-101", 42, 3, 68, 19, juce::Justification::centredLeft, false);

    g.setColour (juce::Colour (0xff7c8ba3));
    g.setFont (juce::FontOptions (10.2f, juce::Font::plain));
    g.drawText (juce::String::fromUTF8 (u8"Sintetizador Periódico • 118 Elementos"),
                42, 21, 235, 15, juce::Justification::centredLeft, false);

    // Active Preset Pill between Prev & Next buttons
    const float pillX = static_cast<float> (prevPresetBtn.getRight() + 5);
    const float pillW = static_cast<float> (nextPresetBtn.getX() - prevPresetBtn.getRight() - 10);
    if (pillW > 40.0f)
    {
        const juce::Rectangle<float> presetPill (pillX, 7.0f, pillW, 26.0f);
        g.setColour (juce::Colour (0xff151c2a));
        g.fillRoundedRectangle (presetPill, 5.0f);
        g.setColour (accent.withAlpha (0.65f));
        g.drawRoundedRectangle (presetPill, 5.0f, 1.2f);

        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        const juce::String pText = "#" + juce::String (activeEl.atomicNumber).paddedLeft ('0', 3)
                                 + "  " + activeEl.symbol + " - " + juce::String::fromUTF8 (activeEl.namePt);
        g.drawText (pText, presetPill.reduced (8.0f, 0.0f), juce::Justification::centred, true);
    }

    // Labels for Volume & Space sliders
    g.setColour (juce::Colour (0xff909fb8));
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("VOLUME", gainSlider.getX() - 54, 10, 50, 20, juce::Justification::centredRight, false);
    g.drawText ("REVERB", spaceSlider.getX() - 52, 10, 48, 20, juce::Justification::centredRight, false);

    // MIDI / Polyphony Voice Status Pill (top-right)
    const int voices = audioProcessor.getSynthEngine().getActiveVoiceCount();
    const juce::Rectangle<float> voicePill (bounds.getWidth() - 112.0f, 8.0f, 96.0f, 24.0f);
    g.setColour (juce::Colour (0xff141b28));
    g.fillRoundedRectangle (voicePill, 12.0f);
    g.setColour (voices > 0 ? accent.withAlpha (0.80f) : juce::Colour (0xff253044));
    g.drawRoundedRectangle (voicePill, 12.0f, 1.1f);

    g.setColour (voices > 0 ? accent.brighter (0.35f) : juce::Colour (0xff49566e));
    g.fillEllipse (voicePill.getX() + 9.0f, voicePill.getCentreY() - 3.5f, 7.0f, 7.0f);

    g.setColour (voices > 0 ? juce::Colours::white : juce::Colour (0xff8a98b0));
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("MIDI " + juce::String (voices) + "/16",
                voicePill.withTrimmedLeft (20.0f), juce::Justification::centredLeft, false);

    // 3. Footer Piano Section Housing & Toolbar Label
    const float footerTop = static_cast<float> (pianoKeyboard.getY() - 30);
    const juce::Rectangle<float> footerPanel (10.0f, footerTop, bounds.getWidth() - 20.0f, bounds.getHeight() - footerTop - 8.0f);
    g.setColour (juce::Colour (0xff0e131d));
    g.fillRoundedRectangle (footerPanel, 8.0f);
    g.setColour (juce::Colour (0xff20293b));
    g.drawRoundedRectangle (footerPanel, 8.0f, 1.0f);

    g.setColour (juce::Colour (0xffcfd8ea));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("PIANO MIDI (" + pianoRangeText + ")",
                static_cast<int> (footerPanel.getX() + 12.0f),
                static_cast<int> (footerTop + 5.0f),
                170, 20, juce::Justification::centredLeft, false);

    g.setColour (juce::Colour (0xff6e7d96));
    g.setFont (juce::FontOptions (10.0f, juce::Font::plain));
    g.drawText (juce::String::fromUTF8 (u8"Clique/arraste nas teclas, use o teclado PC (A-L / Z-X oitava) ou envie notas de uma track MIDI da DAW"),
                static_cast<int> (footerPanel.getX() + 178.0f),
                static_cast<int> (footerTop + 5.0f),
                540, 20, juce::Justification::centredLeft, true);
}

void PeriodicSynthAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // 1. Header controls (top 40px)
    auto header = area.removeFromTop (40);
    authorLinkBtn.setBounds (112, 4, 150, 18);
    const int centerPresetW = 250;
    const int centerStart = std::max (280, (header.getWidth() - centerPresetW) / 2 - 45);
    prevPresetBtn.setBounds (centerStart, 7, 28, 26);
    nextPresetBtn.setBounds (centerStart + 28 + centerPresetW - 46, 7, 28, 26);

    const int rightAnchor = header.getWidth() - 124;
    spaceSlider.setBounds (rightAnchor - 92, 10, 88, 20);
    gainSlider.setBounds  (spaceSlider.getX() - 150, 10, 90, 20);

    // 2. Footer Piano + Toolbar (bottom ~21% of height, clamped 142..185px)
    const int footerTotalH = juce::jlimit (142, 185, static_cast<int> (getHeight() * 0.215f));
    auto footer = area.removeFromBottom (footerTotalH).reduced (16, 6);
    auto pianoToolbar = footer.removeFromTop (24);
    footer.removeFromTop (4);

    panicBtn.setBounds   (pianoToolbar.removeFromRight (58).reduced (0, 1));
    pianoToolbar.removeFromRight (6);
    holdBtn.setBounds    (pianoToolbar.removeFromRight (58).reduced (0, 1));
    pianoToolbar.removeFromRight (10);
    octUpBtn.setBounds   (pianoToolbar.removeFromRight (70).reduced (0, 1));
    pianoToolbar.removeFromRight (5);
    octDownBtn.setBounds (pianoToolbar.removeFromRight (70).reduced (0, 1));

    pianoKeyboard.setBounds (footer);

    // 3. Main Periodic Table Grid fills everything in between!
    periodicTable.setBounds (area.reduced (6, 2));
}

void PeriodicSynthAudioProcessorEditor::timerCallback()
{
    const int procIdx = audioProcessor.getSelectedElementIndex();
    if (procIdx != lastSyncedElement)
        syncAccentTheme (procIdx);

    std::array<float, PeriodicSynth::SynthEngine::kScopeSize> scope {};
    auto& eng = audioProcessor.getSynthEngine();
    eng.copyScopeData (scope);

    periodicTable.updateAudioVisuals (scope, eng.getOutputLevel(), eng.getActiveVoiceCount());
}
