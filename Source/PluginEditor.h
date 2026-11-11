#pragma once

#include "PluginProcessor.h"
#include "PeriodicTableGrid.h"
#include "PianoKeyboard.h"

class PeriodicSynthAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit PeriodicSynthAudioProcessorEditor (PeriodicSynthAudioProcessor&);
    ~PeriodicSynthAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

    // Expose child components & refresh helper for automated verification tests
    void refreshFromProcessor() { timerCallback(); }
    void setLanguage (PeriodicSynth::Language lang);
    PeriodicSynth::Language getLanguage() const noexcept { return currentLanguage; }
    juce::TextButton& getLanguageButton() noexcept { return langBtn; }
    PeriodicSynth::PeriodicTableGrid& getPeriodicTableGrid() noexcept { return periodicTable; }
    PeriodicSynth::PianoKeyboard& getPianoKeyboard() noexcept { return pianoKeyboard; }

    void showAboutDialog (bool visible);
    bool isAboutDialogVisible() const noexcept { return aboutOverlay.isVisible(); }

private:
    class DarkStudioLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        DarkStudioLookAndFeel();
        void setAccent (juce::Colour c) noexcept { accent = c; }
        void setLanguage (PeriodicSynth::Language l) noexcept { language = l; }

        void drawButtonBackground (juce::Graphics& g,
                                   juce::Button& button,
                                   const juce::Colour& backgroundColour,
                                   bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown) override;

        void drawButtonText (juce::Graphics& g,
                             juce::TextButton& button,
                             bool shouldDrawButtonAsHighlighted,
                             bool shouldDrawButtonAsDown) override;

        void drawLinearSlider (juce::Graphics& g,
                               int x, int y, int width, int height,
                               float sliderPos, float minSliderPos, float maxSliderPos,
                               const juce::Slider::SliderStyle style,
                               juce::Slider& slider) override;

    private:
        juce::Colour accent { 0xff3d9ee5 };
        PeriodicSynth::Language language = PeriodicSynth::Language::English;
    };

    class AboutOverlayComponent : public juce::Component
    {
    public:
        AboutOverlayComponent();

        void setAccent (juce::Colour c);
        void setLanguage (PeriodicSynth::Language l);

        void paint (juce::Graphics& g) override;
        void resized() override;
        void mouseDown (const juce::MouseEvent& e) override;

        std::function<void()> onClose;

    private:
        juce::Rectangle<float> getCardBounds() const noexcept;
        void loadLogo();

        juce::Colour accent { 0xffdcadd7 };
        PeriodicSynth::Language language = PeriodicSynth::Language::English;
        std::unique_ptr<juce::Drawable> logoDrawable;
        juce::HyperlinkButton authorLinkBtn { "by LU5 | lu5.art.br", juce::URL ("https://lu5.art.br") };
        juce::TextButton closeBtn { "Close" };
    };

    void timerCallback() override;
    void syncAccentTheme (int elementIndex);
    void updatePianoRangeLabel();
    void attachStandaloneOptionsMenu();
    void openStandaloneOptionsMenu();

    PeriodicSynthAudioProcessor& audioProcessor;
    DarkStudioLookAndFeel studioLaf;

    PeriodicSynth::PeriodicTableGrid periodicTable;
    PeriodicSynth::PianoKeyboard pianoKeyboard;

    juce::TextButton prevPresetBtn { "<" };
    juce::TextButton nextPresetBtn { ">" };

    juce::Slider gainSlider;
    juce::Slider spaceSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> spaceAttachment;

    juce::TextButton octDownBtn { "- Octave" };
    juce::TextButton octUpBtn   { "Octave +" };
    juce::TextButton holdBtn    { "Hold" };
    juce::TextButton panicBtn   { "Panic" };
    juce::TextButton langBtn    { "EN | PT" };

    AboutOverlayComponent aboutOverlay;

    juce::Component::SafePointer<juce::Component> hookedStandaloneWindow;
    juce::Component::SafePointer<juce::Button> origStandaloneOptionsBtn;
    juce::TextButton customStandaloneOptionsBtn { "Options" };

    juce::String pianoRangeText { "C2 - C7" };
    int lastSyncedElement = -1;
    PeriodicSynth::Language currentLanguage = PeriodicSynth::Language::English;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PeriodicSynthAudioProcessorEditor)
};
