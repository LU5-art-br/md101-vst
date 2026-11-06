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

    // Expose child components & refresh helper for automated verification tests
    void refreshFromProcessor() { timerCallback(); }
    PeriodicSynth::PeriodicTableGrid& getPeriodicTableGrid() noexcept { return periodicTable; }
    PeriodicSynth::PianoKeyboard& getPianoKeyboard() noexcept { return pianoKeyboard; }

private:
    class DarkStudioLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        DarkStudioLookAndFeel();
        void setAccent (juce::Colour c) noexcept { accent = c; }

        void drawButtonBackground (juce::Graphics& g,
                                   juce::Button& button,
                                   const juce::Colour& backgroundColour,
                                   bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown) override;

        void drawLinearSlider (juce::Graphics& g,
                               int x, int y, int width, int height,
                               float sliderPos, float minSliderPos, float maxSliderPos,
                               const juce::Slider::SliderStyle style,
                               juce::Slider& slider) override;

    private:
        juce::Colour accent { 0xff3d9ee5 };
    };

    void timerCallback() override;
    void syncAccentTheme (int elementIndex);
    void updatePianoRangeLabel();

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

    juce::TextButton octDownBtn { "- Oitava" };
    juce::TextButton octUpBtn   { "Oitava +" };
    juce::TextButton holdBtn    { "Hold" };
    juce::TextButton panicBtn   { "Panic" };
    juce::HyperlinkButton authorLinkBtn { "by LU5 | lu5.art.br", juce::URL ("https://lu5.art.br") };

    juce::String pianoRangeText { "C2 - C7" };
    int lastSyncedElement = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PeriodicSynthAudioProcessorEditor)
};
