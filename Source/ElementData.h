#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <array>

namespace PeriodicSynth
{

enum class ElementCategory : int
{
    NonMetal = 0,       // Não metais (#A3D246)
    AlkaliMetal,        // Metais alcalinos (#F3B202)
    Semimetal,          // Semimetais (#4DB6AC)
    OtherMetal,         // Outros metais (#A3C7D7)
    Lanthanide,         // Lantanídeos (#92E3E4)
    NobleGas,           // Gases nobres (#3D9EE5)
    AlkalineEarth,      // Metais alcalino-terrosos (#EDD805)
    Halogen,            // Halogênios (#72CBEB)
    TransitionMetal,    // Metais de transição (#EF8E8D)
    Actinide,           // Actinídios (#DCADD7)
    Count
};

enum class PhysicalPhase : int
{
    Solid = 0,
    Liquid,
    Gas
};

enum class Language : int
{
    English = 0,
    Portuguese = 1
};

struct ElementInfo
{
    int atomicNumber;               // Z = 1..118
    const char* symbol;             // "H" .. "Og"
    const char* namePt;             // UTF-8 Portuguese name matching reference table
    const char* nameEn;             // English IUPAC element name
    float atomicMass;               // Numerical atomic mass (u)
    const char* massDisplay;        // Exact display string from reference table ("1,008", "[294]", etc.)
    ElementCategory category;       // Chemical family (10 categories from reference legend)
    PhysicalPhase phase;            // Physical state at STP
    int period;                     // 1..7
    int group;                      // 1..18 (0 for f-block Lanthanides/Actinides)
    int gridRow;                    // 0..6 main table, 7 = Lanthanides, 8 = Actinides
    int gridCol;                    // 0..17
    bool isRadioactive;             // Z == 43, Z == 61, or Z >= 84
};

struct ElementSynthProfile
{
    ElementCategory category = ElementCategory::NonMetal;

    // Normalized physical variables [0..1] for UI feedback & DSP scaling
    float normZ = 0.0f;             // (Z - 1) / 117
    float normMass = 0.0f;          // Log-normalized atomic mass
    float harmonicRichness = 0.5f;  // Driven by category + Z
    float brightness = 0.5f;        // Driven by category + inverse Mass + Z
    float bodyWeight = 0.5f;        // Driven by Atomic Mass
    float sustainTail = 0.5f;       // Envelope momentum / resonance

    // Oscillator & Timbre parameters
    float sawMix = 0.0f;
    float pulseMix = 0.0f;
    float pulseWidth = 0.5f;
    float triSineMix = 0.5f;        // Pure/warm fundamental blend
    float subOscMix = 0.0f;         // -1 octave sub-oscillator weight (scaled by Mass)
    float shimmerOctaveMix = 0.0f;  // +1 / +2 octave airy/crystal overtone
    float breathNoiseMix = 0.0f;    // Gaseous breath / turbulence layer
    float fmModRatio = 2.0f;        // Metallic / Tine FM frequency ratio
    float fmSustainedIndex = 0.0f;  // Continuous FM index (scaled by Z)
    float fmStrikeIndex = 0.0f;     // Percussive transient FM index
    float fmStrikeDecay = 0.12f;    // Decay time of FM strike (s)
    float wavefoldDrive = 1.0f;     // Semimetal / harmonic saturation drive
    float unisonDetuneCents = 4.0f; // Dual/multi-oscillator detune spread
    float pitchSparkSemitones = 0.0f; // Fast attack pitch transient (Alkali spark)
    float geigerScintillation = 0.0f; // Radioactive harmonic micro-pulses

    // Filter parameters (TPT State-Variable Filter)
    float filterCutoffHz = 2200.0f; // Base cutoff at middle C (key-tracked)
    float filterResonance = 0.25f;  // 0.05 .. 0.88
    float filterEnvAmount = 0.35f;  // Envelope modulation in octaves
    float filterBandpassBlend = 0.0f; // Formant/airy bandpass mix

    // Amplitude ADSR Envelope (seconds & [0..1])
    float attackSec = 0.02f;
    float decaySec = 0.35f;
    float sustainLevel = 0.75f;
    float releaseSec = 0.45f;

    // Modulation & Stereo FX
    float lfoRateHz = 2.0f;
    float lfoPitchDepthCents = 3.0f;
    float lfoFilterDepthOct = 0.15f;
    float chorusMix = 0.25f;
    float reverbMix = 0.28f;
    float reverbRoomSize = 0.55f;
    float reverbDamping = 0.45f;

    const char* engineDescriptionPt = "";
    const char* engineDescriptionEn = "";
};

// Database accessors (1-based Z = 1..118, or 0-based index 0..117)
constexpr int kNumElements = 118;

const std::array<ElementInfo, kNumElements>& getAllElements() noexcept;
const ElementInfo& getElementByIndex (int zeroBasedIndex) noexcept;
const ElementInfo& getElementByAtomicNumber (int atomicNumber) noexcept;
int findElementAtGrid (int gridRow, int gridCol) noexcept; // returns 0..117 or -1

// Category & localized metadata (English default, Portuguese toggleable)
juce::Colour getCategoryColour (ElementCategory category) noexcept;
juce::String getElementName (const ElementInfo& element, Language lang = Language::English);
juce::String getElementMassDisplay (const ElementInfo& element, Language lang = Language::English);
juce::String getCategoryName (ElementCategory category, Language lang = Language::English);
juce::String getPhaseName (PhysicalPhase phase, Language lang = Language::English);
juce::String getEngineDescription (const ElementSynthProfile& profile, Language lang = Language::English);

juce::String getCategoryNamePt (ElementCategory category);
juce::String getPhaseNamePt (PhysicalPhase phase);

// Deterministic synthesis profile computed from Category + Atomic Number + Atomic Mass
ElementSynthProfile computeSynthProfile (const ElementInfo& element) noexcept;

} // namespace PeriodicSynth
