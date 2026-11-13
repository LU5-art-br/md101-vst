#include "PluginEditor.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>

#if __has_include(<BinaryData.h>)
    #include <BinaryData.h>
    #define MD101_HAS_BINARY_DATA 1
#elif __has_include("BinaryData.h")
    #include "BinaryData.h"
    #define MD101_HAS_BINARY_DATA 1
#endif

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

namespace
{
    constexpr const char* kMd101Version = "v1.1.2";
}

// ==============================================================================
// DarkStudioLookAndFeel
// ==============================================================================
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

    if (button.getComponentID() == "langSwitch")
    {
        g.setColour (juce::Colour (0xff131a27));
        g.fillRoundedRectangle (bounds, 6.0f);

        const float halfW = bounds.getWidth() * 0.5f;
        const auto activeHalf = (language == PeriodicSynth::Language::English)
                                    ? bounds.withWidth (halfW).reduced (2.0f, 2.0f)
                                    : bounds.withTrimmedLeft (halfW).reduced (2.0f, 2.0f);

        g.setColour (accent.withAlpha (isDown ? 0.48f : 0.32f));
        g.fillRoundedRectangle (activeHalf, 4.5f);
        g.setColour (accent.brighter (0.25f).withAlpha (0.85f));
        g.drawRoundedRectangle (activeHalf, 4.5f, 1.1f);

        g.setColour (isHighlighted ? accent.withAlpha (0.80f) : juce::Colour (0xff263247));
        g.drawRoundedRectangle (bounds, 6.0f, 1.0f);
        return;
    }

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

void PeriodicSynthAudioProcessorEditor::DarkStudioLookAndFeel::drawButtonText (
    juce::Graphics& g,
    juce::TextButton& button,
    bool isHighlighted,
    bool isDown)
{
    if (button.getComponentID() == "langSwitch")
    {
        const auto bounds = button.getLocalBounds().toFloat();
        const float halfW = bounds.getWidth() * 0.5f;
        const auto leftHalf  = bounds.withWidth (halfW);
        const auto rightHalf = bounds.withTrimmedLeft (halfW);
        const bool isEn = (language == PeriodicSynth::Language::English);

        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.setColour (isEn ? juce::Colours::white : juce::Colour (0xff7c8ba3));
        g.drawText ("EN", leftHalf, juce::Justification::centred, false);

        g.setColour (! isEn ? juce::Colours::white : juce::Colour (0xff7c8ba3));
        g.drawText ("PT", rightHalf, juce::Justification::centred, false);
        return;
    }

    juce::LookAndFeel_V4::drawButtonText (g, button, isHighlighted, isDown);
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

// ==============================================================================
// AboutOverlayComponent
// ==============================================================================
PeriodicSynthAudioProcessorEditor::AboutOverlayComponent::AboutOverlayComponent()
{
    loadLogo();

    authorLinkBtn.setFont (juce::FontOptions (13.5f, juce::Font::bold), false, juce::Justification::centred);
    authorLinkBtn.setColour (juce::HyperlinkButton::textColourId, juce::Colour (0xff72cbeb));
    addAndMakeVisible (authorLinkBtn);

    addAndMakeVisible (closeBtn);
    closeBtn.onClick = [this]
    {
        if (onClose)
            onClose();
    };
}

void PeriodicSynthAudioProcessorEditor::AboutOverlayComponent::loadLogo()
{
    auto sanitizeSvgFonts = [] (juce::XmlElement& element, auto& self) -> void {
        if (element.hasAttribute ("font-family"))
            element.removeAttribute ("font-family");
        for (auto* child : element.getChildIterator())
            self (*child, self);
    };

#if defined(MD101_HAS_BINARY_DATA)
    int dataSize = 0;
    const char* data = BinaryData::getNamedResource ("md101_svg", dataSize);
    if (data == nullptr)
        data = BinaryData::getNamedResource ("md_101_svg", dataSize);

    if (data != nullptr && dataSize > 0)
    {
        if (auto xml = juce::XmlDocument::parse (juce::String::createStringFromData (data, dataSize)))
        {
            sanitizeSvgFonts (*xml, sanitizeSvgFonts);
            logoDrawable = juce::Drawable::createFromSVG (*xml);
            if (logoDrawable != nullptr)
                return;
        }
    }
#endif

    const std::initializer_list<juce::File> candidateFiles = {
        juce::File::getSpecialLocation (juce::File::currentExecutableFile).getParentDirectory().getChildFile ("assets/md-101.svg"),
        juce::File::getSpecialLocation (juce::File::currentExecutableFile).getParentDirectory().getParentDirectory().getChildFile ("assets/md-101.svg")
    };

    for (const auto& file : candidateFiles)
    {
        if (file.existsAsFile())
        {
            if (auto xml = juce::XmlDocument::parse (file.loadFileAsString()))
            {
                sanitizeSvgFonts (*xml, sanitizeSvgFonts);
                logoDrawable = juce::Drawable::createFromSVG (*xml);
                if (logoDrawable != nullptr)
                    return;
            }
        }
    }

    // Direct inline SVG fallback ensures logo always renders identically even if files or resources are detached
    static constexpr const char* kMd101Svg =
        R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 256 256" width="256" height="256">
  <defs>
    <linearGradient id="bgGrad" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" stop-color="#141A24"/>
      <stop offset="100%" stop-color="#0A0E14"/>
    </linearGradient>
    <linearGradient id="borderGrad" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" stop-color="#DCADD7"/>
      <stop offset="50%" stop-color="#72CBEB"/>
      <stop offset="100%" stop-color="#3D9EE5"/>
    </linearGradient>
    <filter id="glow" x="-15%" y="-15%" width="130%" height="130%">
      <feGaussianBlur stdDeviation="4" result="blur"/>
      <feComposite in="SourceGraphic" in2="blur" operator="over"/>
    </filter>
  </defs>
  <rect x="12" y="12" width="232" height="232" rx="32" ry="32" fill="url(#bgGrad)" stroke="url(#borderGrad)" stroke-width="6"/>
  <rect x="22" y="22" width="212" height="212" rx="24" ry="24" fill="none" stroke="#DCADD7" stroke-opacity="0.25" stroke-width="2"/>
  <text x="38" y="62" font-size="28" font-weight="700" fill="#72CBEB">101</text>
  <circle cx="208" cy="48" r="8" fill="#A3D246" filter="url(#glow)"/>
  <text x="128" y="148" text-anchor="middle" font-size="92" font-weight="800" fill="#FFFFFF" letter-spacing="-2">Md</text>
  <path d="M 44 182 Q 65 182, 76 166 T 104 182 T 128 154 T 152 182 T 180 168 Q 191 182, 212 182" fill="none" stroke="#DCADD7" stroke-width="4" stroke-linecap="round"/>
  <text x="128" y="216" text-anchor="middle" font-size="20" font-weight="600" fill="#92E3E4" letter-spacing="3">SYNTH [258]</text>
</svg>)svg";

    if (auto xml = juce::XmlDocument::parse (juce::String (kMd101Svg)))
    {
        sanitizeSvgFonts (*xml, sanitizeSvgFonts);
        logoDrawable = juce::Drawable::createFromSVG (*xml);
    }
}

void PeriodicSynthAudioProcessorEditor::AboutOverlayComponent::setAccent (juce::Colour c)
{
    accent = c;
    authorLinkBtn.setColour (juce::HyperlinkButton::textColourId, c.brighter (0.35f));
    repaint();
}

void PeriodicSynthAudioProcessorEditor::AboutOverlayComponent::setLanguage (PeriodicSynth::Language l)
{
    language = l;
    closeBtn.setButtonText (PeriodicSynth::LocalizationManager::getInstance().getUiString ("about.close", language, "Close"));
    repaint();
}

juce::Rectangle<float> PeriodicSynthAudioProcessorEditor::AboutOverlayComponent::getCardBounds() const noexcept
{
    const float w = 480.0f;
    const float h = 320.0f;
    return getLocalBounds().toFloat().withSizeKeepingCentre (w, h);
}

void PeriodicSynthAudioProcessorEditor::AboutOverlayComponent::resized()
{
    const auto card = getCardBounds().toNearestInt();
    authorLinkBtn.setBounds (card.getX() + 110, card.getY() + 122, card.getWidth() - 220, 26);
    closeBtn.setBounds (card.getCentreX() - 56, card.getBottom() - 48, 112, 30);
}

void PeriodicSynthAudioProcessorEditor::AboutOverlayComponent::mouseDown (const juce::MouseEvent& e)
{
    if (! getCardBounds().contains (e.position))
    {
        if (onClose)
            onClose();
    }
}

void PeriodicSynthAudioProcessorEditor::AboutOverlayComponent::paint (juce::Graphics& g)
{
    auto& loc = PeriodicSynth::LocalizationManager::getInstance();

    // Dimmed studio backdrop
    g.fillAll (juce::Colour (0xd8060910));

    const auto card = getCardBounds();

    // Outer ambient glow
    g.setColour (accent.withAlpha (0.16f));
    g.fillRoundedRectangle (card.expanded (4.0f), 14.0f);

    // Card background
    juce::ColourGradient cardGrad (juce::Colour (0xff151c2b), card.getCentreX(), card.getY(),
                                   juce::Colour (0xff0d121c), card.getCentreX(), card.getBottom(), false);
    g.setGradientFill (cardGrad);
    g.fillRoundedRectangle (card, 11.0f);

    // Double border
    g.setColour (accent.withAlpha (0.85f));
    g.drawRoundedRectangle (card, 11.0f, 1.6f);
    g.setColour (accent.withAlpha (0.22f));
    g.drawRoundedRectangle (card.reduced (4.0f), 8.5f, 1.0f);

    // Periodic Table Element Badge [101 Md] - md-101.svg Logo
    const juce::Rectangle<float> tile (card.getX() + 24.0f, card.getY() + 22.0f, 68.0f, 68.0f);

    if (logoDrawable != nullptr)
    {
        g.setColour (accent.withAlpha (0.22f));
        g.fillRoundedRectangle (tile.expanded (2.0f), 10.0f);

        logoDrawable->drawWithin (g, tile, juce::RectanglePlacement::centred, 1.0f);
    }
    else
    {
        g.setColour (juce::Colour (0xff0c1018));
        g.fillRoundedRectangle (tile, 7.0f);
        g.setColour (accent.brighter (0.2f));
        g.drawRoundedRectangle (tile, 7.0f, 1.8f);

        g.setColour (juce::Colour (0xff72cbeb));
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText ("101", tile.withTrimmedLeft (6.0f).withTrimmedTop (4.0f).withHeight (14.0f),
                    juce::Justification::centredLeft, false);

        g.setColour (juce::Colour (0xffa3d246));
        g.fillEllipse (tile.getRight() - 13.0f, tile.getY() + 6.0f, 6.5f, 6.5f);

        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (26.0f, juce::Font::bold));
        g.drawText ("Md", tile.withTrimmedTop (14.0f), juce::Justification::centred, false);
    }

    // Title & Version Badge
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("Md-101", static_cast<int> (tile.getRight() + 16.0f), static_cast<int> (tile.getY() + 2.0f),
                110, 26, juce::Justification::centredLeft, false);

    const juce::Rectangle<float> verPill (tile.getRight() + 108.0f, tile.getY() + 5.0f, 58.0f, 21.0f);
    g.setColour (accent.withAlpha (0.24f));
    g.fillRoundedRectangle (verPill, 5.0f);
    g.setColour (accent.brighter (0.25f));
    g.drawRoundedRectangle (verPill, 5.0f, 1.1f);
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    g.drawText (kMd101Version, verPill, juce::Justification::centred, false);

    // Subtitle
    g.setColour (juce::Colour (0xffa8b8d0));
    g.setFont (juce::FontOptions (12.0f, juce::Font::plain));
    g.drawText (loc.getUiString ("about.subtitle", language, "16-Voice Polyphonic Periodic Table Synthesizer"),
                static_cast<int> (tile.getRight() + 16.0f), static_cast<int> (tile.getY() + 31.0f),
                345, 18, juce::Justification::centredLeft, false);

    g.setColour (juce::Colour (0xff6e7f99));
    g.setFont (juce::FontOptions (11.0f, juce::Font::plain));
    g.drawText (juce::String::fromUTF8 (u8"VST3 (64-bit) & Standalone  •  118 Presets  •  C++17 / JUCE 8"),
                static_cast<int> (tile.getRight() + 16.0f), static_cast<int> (tile.getY() + 50.0f),
                345, 16, juce::Justification::centredLeft, false);

    // Divider line
    g.setColour (juce::Colour (0xff222d42));
    g.drawHorizontalLine (static_cast<int> (card.getY() + 104.0f), card.getX() + 24.0f, card.getRight() - 24.0f);

    // Author pill frame behind authorLinkBtn
    const auto linkBounds = authorLinkBtn.getBounds().toFloat().expanded (8.0f, 2.0f);
    g.setColour (juce::Colour (0xff121926));
    g.fillRoundedRectangle (linkBounds, 6.0f);
    g.setColour (accent.withAlpha (0.45f));
    g.drawRoundedRectangle (linkBounds, 6.0f, 1.0f);

    // License & Copyright Panel
    const juce::Rectangle<float> licBox (card.getX() + 26.0f, card.getY() + 160.0f, card.getWidth() - 52.0f, 96.0f);
    g.setColour (juce::Colour (0xff0e131e));
    g.fillRoundedRectangle (licBox, 7.0f);
    g.setColour (juce::Colour (0xff1f2a3d));
    g.drawRoundedRectangle (licBox, 7.0f, 1.0f);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (loc.getUiString ("about.licenseHeader", language, "License: MIT License  •  Copyright © 2026 LU5"),
                licBox.withTrimmedTop (10.0f).withHeight (20.0f),
                juce::Justification::centred, false);

    g.setColour (juce::Colour (0xff8fa0b8));
    g.setFont (juce::FontOptions (11.0f, juce::Font::plain));
    const juce::String licDesc = loc.getUiString ("about.licenseBody", language,
        "This software is released under the MIT License (source code).\nBinaries compiled with the open-source JUCE 8 framework are subject to GPLv3 terms.");
    g.drawFittedText (licDesc,
                      licBox.reduced (14.0f, 8.0f).withTrimmedTop (24.0f).toNearestInt(),
                      juce::Justification::centred, 3);
}

// ==============================================================================
// PeriodicSynthAudioProcessorEditor
// ==============================================================================
PeriodicSynthAudioProcessorEditor::PeriodicSynthAudioProcessorEditor (PeriodicSynthAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      pianoKeyboard (p.getKeyboardState())
{
    setLookAndFeel (&studioLaf);
    setWantsKeyboardFocus (true);

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

    // Language selector button in top header bar (EN | PT)
    langBtn.setComponentID ("langSwitch");
    langBtn.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible (langBtn);
    langBtn.onClick = [this]
    {
        const auto nextLang = (currentLanguage == PeriodicSynth::Language::English)
                                  ? PeriodicSynth::Language::Portuguese
                                  : PeriodicSynth::Language::English;
        setLanguage (nextLang);
    };

    // About modal overlay (initially hidden; opened via Options -> About/Sobre or clicking brand header)
    addChildComponent (aboutOverlay);
    aboutOverlay.onClose = [this] { showAboutDialog (false); };

    customStandaloneOptionsBtn.setTriggeredOnMouseDown (true);
    customStandaloneOptionsBtn.onClick = [this] { openStandaloneOptionsMenu(); };

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

    setLanguage (audioProcessor.getLanguage());
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
    if (customStandaloneOptionsBtn.getParentComponent() != nullptr)
        customStandaloneOptionsBtn.getParentComponent()->removeChildComponent (&customStandaloneOptionsBtn);
    if (origStandaloneOptionsBtn != nullptr)
        origStandaloneOptionsBtn->setVisible (true);
    setLookAndFeel (nullptr);
}

void PeriodicSynthAudioProcessorEditor::parentHierarchyChanged()
{
    AudioProcessorEditor::parentHierarchyChanged();
    attachStandaloneOptionsMenu();
}

void PeriodicSynthAudioProcessorEditor::attachStandaloneOptionsMenu()
{
    auto* standaloneWin = findParentComponentOfClass<juce::StandaloneFilterWindow>();
    if (standaloneWin == nullptr)
        return;

    auto& loc = PeriodicSynth::LocalizationManager::getInstance();
    customStandaloneOptionsBtn.setButtonText (loc.getUiString ("standaloneMenu.options", currentLanguage, "Options"));

    if (hookedStandaloneWindow == standaloneWin && origStandaloneOptionsBtn != nullptr)
    {
        customStandaloneOptionsBtn.setBounds (origStandaloneOptionsBtn->getBounds());
        return;
    }

    for (int i = 0; i < standaloneWin->getNumChildComponents(); ++i)
    {
        if (auto* tb = dynamic_cast<juce::TextButton*> (standaloneWin->getChildComponent (i)))
        {
            if (tb != &customStandaloneOptionsBtn)
            {
                origStandaloneOptionsBtn = tb;
                hookedStandaloneWindow = standaloneWin;

                tb->setVisible (false);
                standaloneWin->addAndMakeVisible (customStandaloneOptionsBtn);
                customStandaloneOptionsBtn.setBounds (tb->getBounds());
                break;
            }
        }
    }
}

void PeriodicSynthAudioProcessorEditor::openStandaloneOptionsMenu()
{
    auto* standaloneWin = dynamic_cast<juce::StandaloneFilterWindow*> (hookedStandaloneWindow.getComponent());
    if (standaloneWin == nullptr)
        standaloneWin = findParentComponentOfClass<juce::StandaloneFilterWindow>();

    auto& loc = PeriodicSynth::LocalizationManager::getInstance();

    juce::PopupMenu m;
    m.addItem (1, loc.getUiString ("standaloneMenu.settings", currentLanguage, "Audio/MIDI Settings..."));
    m.addSeparator();
    m.addItem (2, loc.getUiString ("standaloneMenu.saveState", currentLanguage, "Save current state..."));
    m.addItem (3, loc.getUiString ("standaloneMenu.loadState", currentLanguage, "Load a saved state..."));
    m.addSeparator();
    m.addItem (4, loc.getUiString ("standaloneMenu.resetState", currentLanguage, "Reset to default state"));
    m.addSeparator();
    m.addItem (5, loc.getUiString ("standaloneMenu.about", currentLanguage, "About Md-101..."));

    juce::Component::SafePointer<PeriodicSynthAudioProcessorEditor> safeThis (this);
    juce::Component::SafePointer<juce::StandaloneFilterWindow> safeWin (standaloneWin);

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&customStandaloneOptionsBtn),
                     [safeThis, safeWin] (int result)
                     {
                         if (result == 5)
                         {
                             if (safeThis != nullptr)
                                 safeThis->showAboutDialog (true);
                         }
                         else if (result >= 1 && result <= 4)
                         {
                             if (safeWin != nullptr)
                                 safeWin->handleMenuResult (result);
                         }
                     });
}

void PeriodicSynthAudioProcessorEditor::showAboutDialog (bool visible)
{
    aboutOverlay.setVisible (visible);
    if (visible)
    {
        aboutOverlay.toFront (true);
    }
    else
    {
        pianoKeyboard.grabKeyboardFocus();
    }
    repaint();
}

void PeriodicSynthAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    // Clicking the Md-101 orbital logo/title in the top-left header bar also toggles the About screen
    if (e.y < 40 && e.x < 245)
    {
        showAboutDialog (! isAboutDialogVisible());
        return;
    }
    AudioProcessorEditor::mouseDown (e);
}

bool PeriodicSynthAudioProcessorEditor::keyPressed (const juce::KeyPress& key)
{
    if (aboutOverlay.isVisible() && key == juce::KeyPress::escapeKey)
    {
        showAboutDialog (false);
        return true;
    }
    return AudioProcessorEditor::keyPressed (key);
}

void PeriodicSynthAudioProcessorEditor::setLanguage (PeriodicSynth::Language lang)
{
    currentLanguage = lang;
    PeriodicSynth::LocalizationManager::getInstance().setLanguage (lang);
    audioProcessor.setLanguage (lang);
    studioLaf.setLanguage (lang);
    periodicTable.setLanguage (lang);
    aboutOverlay.setLanguage (lang);

    auto& loc = PeriodicSynth::LocalizationManager::getInstance();
    octDownBtn.setButtonText (loc.getUiString ("header.octaveDown", lang, "- Octave"));
    octUpBtn.setButtonText   (loc.getUiString ("header.octaveUp", lang, "Octave +"));
    customStandaloneOptionsBtn.setButtonText (loc.getUiString ("standaloneMenu.options", lang, "Options"));
    langBtn.repaint();
    repaint();
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
    aboutOverlay.setAccent (col);
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
    auto& loc = PeriodicSynth::LocalizationManager::getInstance();

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    g.drawText ("Md-101", 42, 3, 68, 19, juce::Justification::centredLeft, false);

    g.setColour (juce::Colour (0xff7c8ba3));
    g.setFont (juce::FontOptions (10.2f, juce::Font::plain));
    g.drawText (loc.getUiString ("header.subtitle", currentLanguage, "Periodic Synthesizer • 118 Elements"),
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
                                 + "  " + activeEl.symbol + " - " + PeriodicSynth::getElementName (activeEl, currentLanguage);
        g.drawText (pText, presetPill.reduced (8.0f, 0.0f), juce::Justification::centred, true);
    }

    // Labels for Volume & Space sliders
    g.setColour (juce::Colour (0xff909fb8));
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("VOLUME", gainSlider.getX() - 54, 10, 50, 20, juce::Justification::centredRight, false);
    g.drawText ("REVERB", spaceSlider.getX() - 52, 10, 48, 20, juce::Justification::centredRight, false);

    // 3. Footer Piano Section Housing & Toolbar Label
    const float footerTop = static_cast<float> (pianoKeyboard.getY() - 30);
    const juce::Rectangle<float> footerPanel (10.0f, footerTop, bounds.getWidth() - 20.0f, bounds.getHeight() - footerTop - 8.0f);
    g.setColour (juce::Colour (0xff0e131d));
    g.fillRoundedRectangle (footerPanel, 8.0f);
    g.setColour (juce::Colour (0xff20293b));
    g.drawRoundedRectangle (footerPanel, 8.0f, 1.0f);

    g.setColour (juce::Colour (0xffcfd8ea));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (loc.getUiString ("piano.title", currentLanguage, "MIDI PIANO") + " (" + pianoRangeText + ")",
                static_cast<int> (footerPanel.getX() + 12.0f),
                static_cast<int> (footerTop + 5.0f),
                170, 20, juce::Justification::centredLeft, false);

    // MIDI / Polyphony Voice Status Pill (in piano performance toolbar, left of octave buttons)
    if (! voicePillBounds.isEmpty())
    {
        const int voices = audioProcessor.getSynthEngine().getActiveVoiceCount();
        g.setColour (juce::Colour (0xff141b28));
        g.fillRoundedRectangle (voicePillBounds, 11.0f);
        g.setColour (voices > 0 ? accent.withAlpha (0.80f) : juce::Colour (0xff253044));
        g.drawRoundedRectangle (voicePillBounds, 11.0f, 1.1f);

        g.setColour (voices > 0 ? accent.brighter (0.35f) : juce::Colour (0xff49566e));
        g.fillEllipse (voicePillBounds.getX() + 9.0f, voicePillBounds.getCentreY() - 3.5f, 7.0f, 7.0f);

        g.setColour (voices > 0 ? juce::Colours::white : juce::Colour (0xff8a98b0));
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText ("MIDI " + juce::String (voices) + "/16",
                    voicePillBounds.withTrimmedLeft (20.0f), juce::Justification::centredLeft, false);
    }

    // Instruction Help text (bounded between title and voice pill)
    const float helpTextX = footerPanel.getX() + 180.0f;
    const float helpTextRight = voicePillBounds.isEmpty()
                                  ? static_cast<float> (octDownBtn.getX() - 10)
                                  : voicePillBounds.getX() - 12.0f;
    const float helpTextW = std::max (60.0f, helpTextRight - helpTextX);

    g.setColour (juce::Colour (0xff6e7d96));
    g.setFont (juce::FontOptions (10.0f, juce::Font::plain));
    g.drawText (loc.getUiString ("piano.instructionHelp", currentLanguage,
                    "Click/drag keys, use PC keyboard (A-L / Z-X octave) or send notes from a DAW MIDI track"),
                static_cast<int> (helpTextX),
                static_cast<int> (footerTop + 5.0f),
                static_cast<int> (helpTextW), 20, juce::Justification::centredLeft, true);
}

void PeriodicSynthAudioProcessorEditor::resized()
{
    aboutOverlay.setBounds (getLocalBounds());

    auto area = getLocalBounds();

    // 1. Header controls (top 40px)
    auto header = area.removeFromTop (40);

    langBtn.setBounds (header.getWidth() - 76, 8, 64, 24);

    const int sliderW = header.getWidth() < 1040 ? 80 : 92;
    const int labelGap = 52;
    spaceSlider.setBounds (langBtn.getX() - 14 - sliderW, 10, sliderW, 20);
    gainSlider.setBounds  (spaceSlider.getX() - labelGap - 14 - sliderW, 10, sliderW, 20);

    const int leftLimit   = 274;
    const int rightLimit  = gainSlider.getX() - labelGap - 8;
    const int availCenter = std::max (180, rightLimit - leftLimit);
    const int centerPresetW = juce::jlimit (180, 260, availCenter - 12);
    const int centerStart = leftLimit + (availCenter - centerPresetW) / 2;
    prevPresetBtn.setBounds (centerStart, 7, 28, 26);
    nextPresetBtn.setBounds (centerStart + centerPresetW - 28, 7, 28, 26);

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
    pianoToolbar.removeFromRight (10);

    voicePillBounds = pianoToolbar.removeFromRight (90).reduced (0, 1).toFloat();

    pianoKeyboard.setBounds (footer);

    // 3. Main Periodic Table Grid fills everything in between!
    periodicTable.setBounds (area.reduced (6, 2));
}

void PeriodicSynthAudioProcessorEditor::timerCallback()
{
    attachStandaloneOptionsMenu();

    if (audioProcessor.getLanguage() != currentLanguage)
        setLanguage (audioProcessor.getLanguage());

    const int procIdx = audioProcessor.getSelectedElementIndex();
    if (procIdx != lastSyncedElement)
        syncAccentTheme (procIdx);

    std::array<float, PeriodicSynth::SynthEngine::kScopeSize> scope {};
    auto& eng = audioProcessor.getSynthEngine();
    eng.copyScopeData (scope);

    const int currentVoices = eng.getActiveVoiceCount();
    if (currentVoices != lastSyncedVoices)
    {
        lastSyncedVoices = currentVoices;
        if (! voicePillBounds.isEmpty())
            repaint (voicePillBounds.toNearestInt().expanded (2));
    }

    periodicTable.updateAudioVisuals (scope, eng.getOutputLevel(), currentVoices);
}
