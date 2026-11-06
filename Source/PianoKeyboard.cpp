#include "PianoKeyboard.h"

namespace PeriodicSynth
{

PianoKeyboard::PianoKeyboard (juce::MidiKeyboardState& state)
    : keyboardState (state)
{
    setWantsKeyboardFocus (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    keyboardState.addListener (this);
    startTimerHz (60);
}

PianoKeyboard::~PianoKeyboard()
{
    stopTimer();
    keyboardState.removeListener (this);
    releaseMouseNote();
}

void PianoKeyboard::setAccentColour (juce::Colour colour) noexcept
{
    if (accentColour != colour)
    {
        accentColour = colour;
        repaint();
    }
}

void PianoKeyboard::setOctaveShift (int shiftOctaves)
{
    const int clamped = juce::jlimit (-2, 2, shiftOctaves);
    if (octaveShift != clamped)
    {
        releaseMouseNote();
        for (int n = 0; n < 128; ++n)
        {
            if (pcKeyHeldNotes[static_cast<size_t> (n)])
            {
                keyboardState.noteOff (midiChannel, n, 0.0f);
                pcKeyHeldNotes[static_cast<size_t> (n)] = false;
            }
        }
        octaveShift = clamped;
        repaint();
    }
}

bool PianoKeyboard::isBlackKey (int midiNote) noexcept
{
    switch (midiNote % 12)
    {
        case 1: case 3: case 6: case 8: case 10:
            return true;
        default:
            return false;
    }
}

juce::Rectangle<float> PianoKeyboard::getKeyBounds (int midiNote) const noexcept
{
    const int low  = getLowestNote();
    const int high = getHighestNote();
    if (midiNote < low || midiNote > high)
        return {};

    const auto area = getLocalBounds().toFloat().reduced (2.0f, 2.0f);
    if (area.getWidth() <= 10.0f || area.getHeight() <= 10.0f)
        return {};

    // Count total white keys in [low, high]
    int totalWhite = 0;
    int whiteBefore = 0;
    for (int n = low; n <= high; ++n)
    {
        if (! isBlackKey (n))
        {
            if (n < midiNote)
                ++whiteBefore;
            ++totalWhite;
        }
    }

    if (totalWhite <= 0)
        return {};

    const float whiteW = area.getWidth() / static_cast<float> (totalWhite);
    const float whiteH = area.getHeight();

    if (! isBlackKey (midiNote))
    {
        return { area.getX() + static_cast<float> (whiteBefore) * whiteW,
                 area.getY(),
                 whiteW,
                 whiteH };
    }

    // Black key sits between the preceding white key and the next white key with realistic piano offsets
    const float blackW = whiteW * 0.60f;
    const float blackH = whiteH * 0.62f;
    float centerOffset = 0.0f;
    switch (midiNote % 12)
    {
        case 1:  centerOffset = -0.06f * whiteW; break; // C#
        case 3:  centerOffset =  0.06f * whiteW; break; // D#
        case 6:  centerOffset = -0.07f * whiteW; break; // F#
        case 8:  centerOffset =  0.00f * whiteW; break; // G#
        case 10: centerOffset =  0.07f * whiteW; break; // A#
        default: break;
    }

    const float centerX = area.getX() + static_cast<float> (whiteBefore) * whiteW + centerOffset;
    return { centerX - blackW * 0.5f, area.getY(), blackW, blackH };
}

int PianoKeyboard::getNoteAtPosition (juce::Point<float> pos, float& outVelocity) const noexcept
{
    const int low  = getLowestNote();
    const int high = getHighestNote();

    // Check black keys first (they sit on top of white keys)
    for (int n = low; n <= high; ++n)
    {
        if (isBlackKey (n))
        {
            const auto r = getKeyBounds (n);
            if (r.contains (pos))
            {
                const float relY = juce::jlimit (0.0f, 1.0f, (pos.y - r.getY()) / std::max (1.0f, r.getHeight()));
                outVelocity = 0.42f + 0.54f * relY;
                return n;
            }
        }
    }

    // Check white keys
    for (int n = low; n <= high; ++n)
    {
        if (! isBlackKey (n))
        {
            const auto r = getKeyBounds (n);
            if (r.contains (pos))
            {
                const float relY = juce::jlimit (0.0f, 1.0f, (pos.y - r.getY()) / std::max (1.0f, r.getHeight()));
                outVelocity = 0.40f + 0.58f * relY;
                return n;
            }
        }
    }

    outVelocity = 0.8f;
    return -1;
}

void PianoKeyboard::paint (juce::Graphics& g)
{
    const auto outer = getLocalBounds().toFloat();

    // Piano housing / dark felt rail
    g.setColour (juce::Colour (0xff090c12));
    g.fillRoundedRectangle (outer, 6.0f);

    const int low  = getLowestNote();
    const int high = getHighestNote();

    // 1. Draw all traditional White Keys
    for (int n = low; n <= high; ++n)
    {
        if (isBlackKey (n))
            continue;

        auto keyRect = getKeyBounds (n).reduced (0.65f, 0.0f);
        const bool isDown  = keyboardState.isNoteOnForChannels (0xffff, n);
        const bool isHover = (n == hoveredNote);

        // Base ivory/white key fill
        juce::Colour topCol = juce::Colour (0xfff7f9fc);
        juce::Colour botCol = juce::Colour (0xffe6ebf2);

        if (isDown)
        {
            topCol = juce::Colour (0xffe2e8f2).interpolatedWith (accentColour, 0.22f);
            botCol = juce::Colour (0xffd5deeb).interpolatedWith (accentColour, 0.36f);
            keyRect.removeFromTop (1.5f);
        }
        else if (isHover)
        {
            topCol = juce::Colour (0xffffffff);
            botCol = juce::Colour (0xfff0f4fa).interpolatedWith (accentColour, 0.10f);
        }

        juce::ColourGradient wkGrad (topCol, keyRect.getX(), keyRect.getY(),
                                     botCol, keyRect.getX(), keyRect.getBottom(), false);
        g.setGradientFill (wkGrad);
        g.fillRoundedRectangle (keyRect, 3.5f);

        // Bottom 3D lip of white key
        const float lipH = isDown ? 3.0f : 6.0f;
        auto lipRect = keyRect.withTop (keyRect.getBottom() - lipH);
        g.setColour (isDown ? accentColour.withAlpha (0.85f) : juce::Colour (0xffcbd3e0));
        g.fillRoundedRectangle (lipRect, 3.0f);

        // Active key glow indicator at bottom edge
        if (isDown)
        {
            g.setColour (accentColour.brighter (0.3f).withAlpha (0.85f));
            g.drawRoundedRectangle (keyRect.reduced (0.8f), 3.0f, 1.8f);
        }
        else if (isHover)
        {
            g.setColour (accentColour.withAlpha (0.55f));
            g.drawRoundedRectangle (keyRect.reduced (0.8f), 3.0f, 1.3f);
        }

        // C-note octave label (C1, C2, C3, C4...)
        if ((n % 12) == 0)
        {
            const int oct = (n / 12) - 1;
            g.setColour (isDown ? juce::Colour (0xff0c1018) : juce::Colour (0xff5c6678));
            g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
            auto labelArea = keyRect.withTop (keyRect.getBottom() - 22.0f).withBottom (keyRect.getBottom() - 6.0f);
            g.drawText ("C" + juce::String (oct), labelArea, juce::Justification::centred, false);
        }
    }

    // Classic dark red/crimson studio felt strip along top of keys
    const auto keysArea = outer.reduced (2.0f, 2.0f);
    juce::ColourGradient feltGrad (accentColour.darker (0.6f).withAlpha (0.85f), keysArea.getX(), keysArea.getY(),
                                   juce::Colour (0xff121722), keysArea.getX(), keysArea.getY() + 4.0f, false);
    g.setGradientFill (feltGrad);
    g.fillRect (keysArea.getX(), keysArea.getY(), keysArea.getWidth(), 3.0f);

    // 2. Draw all traditional Black Keys on top
    for (int n = low; n <= high; ++n)
    {
        if (! isBlackKey (n))
            continue;

        const auto keyRect = getKeyBounds (n);
        const bool isDown  = keyboardState.isNoteOnForChannels (0xffff, n);
        const bool isHover = (n == hoveredNote);

        // Subtle shadow cast onto neighboring white keys
        if (! isDown)
        {
            g.setColour (juce::Colours::black.withAlpha (0.32f));
            g.fillRoundedRectangle (keyRect.translated (1.2f, 2.5f), 2.8f);
        }

        // Main ebony body
        juce::Colour blkTop = juce::Colour (0xff2a303d);
        juce::Colour blkBot = juce::Colour (0xff10131a);

        if (isDown)
        {
            blkTop = juce::Colour (0xff252b38).interpolatedWith (accentColour, 0.35f);
            blkBot = juce::Colour (0xff171c26).interpolatedWith (accentColour, 0.50f);
        }
        else if (isHover)
        {
            blkTop = juce::Colour (0xff384052);
            blkBot = juce::Colour (0xff181d28);
        }

        juce::ColourGradient bkGrad (blkTop, keyRect.getX(), keyRect.getY(),
                                     blkBot, keyRect.getX(), keyRect.getBottom(), false);
        g.setGradientFill (bkGrad);
        g.fillRoundedRectangle (keyRect, 2.8f);

        // 3D satin bevel highlight on inner face of black key
        auto innerFace = keyRect.reduced (2.2f, 1.5f).withTrimmedBottom (isDown ? 2.5f : 6.5f);
        juce::ColourGradient bevelGrad (isDown ? accentColour.withAlpha (0.45f) : juce::Colour (0xff3c4456).withAlpha (0.55f),
                                        innerFace.getX(), innerFace.getY(),
                                        juce::Colour (0xff1c212c).withAlpha (0.35f),
                                        innerFace.getX(), innerFace.getBottom(), false);
        g.setGradientFill (bevelGrad);
        g.fillRoundedRectangle (innerFace, 1.8f);

        if (isDown)
        {
            g.setColour (accentColour.brighter (0.35f));
            g.drawRoundedRectangle (keyRect.reduced (0.6f), 2.6f, 1.5f);
        }
        else if (isHover)
        {
            g.setColour (accentColour.withAlpha (0.65f));
            g.drawRoundedRectangle (keyRect.reduced (0.6f), 2.6f, 1.1f);
        }
        else
        {
            g.setColour (juce::Colour (0xff07090d));
            g.drawRoundedRectangle (keyRect, 2.8f, 0.9f);
        }
    }
}

void PianoKeyboard::resized()
{
    repaint();
}

void PianoKeyboard::mouseMove (const juce::MouseEvent& e)
{
    float vel = 0.0f;
    const int n = getNoteAtPosition (e.position, vel);
    if (n != hoveredNote)
    {
        hoveredNote = n;
        repaint();
    }
}

void PianoKeyboard::mouseExit (const juce::MouseEvent&)
{
    if (hoveredNote != -1)
    {
        hoveredNote = -1;
        repaint();
    }
}

void PianoKeyboard::mouseDown (const juce::MouseEvent& e)
{
    grabKeyboardFocus();
    float vel = 0.8f;
    const int n = getNoteAtPosition (e.position, vel);
    releaseMouseNote();
    if (n >= 0)
    {
        mouseDownNote = n;
        hoveredNote = n;
        keyboardState.noteOn (midiChannel, n, vel);
        repaint();
    }
}

void PianoKeyboard::mouseDrag (const juce::MouseEvent& e)
{
    float vel = 0.8f;
    const int n = getNoteAtPosition (e.position, vel);
    hoveredNote = n;

    if (n != mouseDownNote)
    {
        releaseMouseNote();
        if (n >= 0)
        {
            mouseDownNote = n;
            keyboardState.noteOn (midiChannel, n, vel);
        }
        repaint();
    }
}

void PianoKeyboard::mouseUp (const juce::MouseEvent&)
{
    releaseMouseNote();
    repaint();
}

void PianoKeyboard::releaseMouseNote()
{
    if (mouseDownNote >= 0)
    {
        keyboardState.noteOff (midiChannel, mouseDownNote, 0.0f);
        mouseDownNote = -1;
    }
}

bool PianoKeyboard::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress ('z') || key == juce::KeyPress ('Z'))
    {
        setOctaveShift (octaveShift - 1);
        return true;
    }
    if (key == juce::KeyPress ('x') || key == juce::KeyPress ('X'))
    {
        setOctaveShift (octaveShift + 1);
        return true;
    }
    updateComputerKeyboardNotes();
    return false;
}

bool PianoKeyboard::keyStateChanged (bool)
{
    updateComputerKeyboardNotes();
    return false;
}

void PianoKeyboard::focusLost (FocusChangeType)
{
    for (int n = 0; n < 128; ++n)
    {
        if (pcKeyHeldNotes[static_cast<size_t> (n)])
        {
            keyboardState.noteOff (midiChannel, n, 0.0f);
            pcKeyHeldNotes[static_cast<size_t> (n)] = false;
        }
    }
    repaint();
}

void PianoKeyboard::updateComputerKeyboardNotes()
{
    // Standard DAW QWERTY musical typing layout starting at C4 (shifted by octaveShift)
    static constexpr const char* kKeyMap = "awsedftgyhujkolp;";
    const int baseNote = juce::jlimit (12, 96, 60 + octaveShift * 12);

    for (int i = 0; kKeyMap[i] != '\0'; ++i)
    {
        const int midiNote = baseNote + i;
        if (midiNote < 0 || midiNote > 127)
            continue;

        const bool down = juce::KeyPress::isKeyCurrentlyDown (kKeyMap[i]);
        if (down && ! pcKeyHeldNotes[static_cast<size_t> (midiNote)])
        {
            pcKeyHeldNotes[static_cast<size_t> (midiNote)] = true;
            keyboardState.noteOn (midiChannel, midiNote, 0.82f);
            stateDirty.store (true, std::memory_order_relaxed);
        }
        else if (! down && pcKeyHeldNotes[static_cast<size_t> (midiNote)])
        {
            pcKeyHeldNotes[static_cast<size_t> (midiNote)] = false;
            keyboardState.noteOff (midiChannel, midiNote, 0.0f);
            stateDirty.store (true, std::memory_order_relaxed);
        }
    }
}

void PianoKeyboard::handleNoteOn (juce::MidiKeyboardState*, int, int, float)
{
    stateDirty.store (true, std::memory_order_relaxed);
}

void PianoKeyboard::handleNoteOff (juce::MidiKeyboardState*, int, int, float)
{
    stateDirty.store (true, std::memory_order_relaxed);
}

void PianoKeyboard::timerCallback()
{
    if (hasKeyboardFocus (false))
        updateComputerKeyboardNotes();

    if (stateDirty.exchange (false, std::memory_order_relaxed))
        repaint();
}

} // namespace PeriodicSynth
