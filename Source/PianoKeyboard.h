#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>

namespace PeriodicSynth
{

class PianoKeyboard : public juce::Component,
                      private juce::MidiKeyboardState::Listener,
                      private juce::Timer
{
public:
    explicit PianoKeyboard (juce::MidiKeyboardState& state);
    ~PianoKeyboard() override;

    void setAccentColour (juce::Colour colour) noexcept;
    void setOctaveShift (int shiftOctaves); // -2 .. +2 around C2..C7
    int getOctaveShift() const noexcept { return octaveShift; }

    int getLowestNote() const noexcept  { return juce::jlimit (0, 108, 36 + octaveShift * 12); }
    int getHighestNote() const noexcept { return juce::jmin (127, getLowestNote() + 60); } // 61 keys (5 octaves + C)

    void paint (juce::Graphics& g) override;
    void resized() override;

    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

    bool keyPressed (const juce::KeyPress& key) override;
    bool keyStateChanged (bool isKeyDown) override;
    void focusLost (FocusChangeType cause) override;

private:
    void handleNoteOn (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff (juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void timerCallback() override;

    static bool isBlackKey (int midiNote) noexcept;
    juce::Rectangle<float> getKeyBounds (int midiNote) const noexcept;
    int getNoteAtPosition (juce::Point<float> pos, float& outVelocity) const noexcept;
    void updateComputerKeyboardNotes();
    void releaseMouseNote();

    juce::MidiKeyboardState& keyboardState;
    juce::Colour accentColour { 0xff3d9ee5 };

    int octaveShift = 0;
    int hoveredNote = -1;
    int mouseDownNote = -1;
    int midiChannel = 1;

    std::array<bool, 128> pcKeyHeldNotes {};
    std::atomic<bool> stateDirty { true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoKeyboard)
};

} // namespace PeriodicSynth
