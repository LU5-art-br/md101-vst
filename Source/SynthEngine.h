#pragma once

#include "ElementData.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <vector>

namespace PeriodicSynth
{

class SynthEngine
{
public:
    static constexpr int kMaxVoices = 16;
    static constexpr int kScopeSize = 512;

    SynthEngine();

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    void setElementPreset (int elementIndex) noexcept;
    int getElementPreset() const noexcept { return currentElementIndex.load (std::memory_order_relaxed); }

    void setMasterGain (float linearGain) noexcept { masterGain.store (linearGain, std::memory_order_relaxed); }
    void setSpaceScale (float scale) noexcept { spaceScale.store (scale, std::memory_order_relaxed); }
    void setSustainHold (bool holdOn) noexcept;
    bool getSustainHold() const noexcept { return uiHoldActive.load (std::memory_order_relaxed); }
    void panic() noexcept;

    void processBlock (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midiMessages) noexcept;

    int getActiveVoiceCount() const noexcept { return activeVoicesCount.load (std::memory_order_relaxed); }
    float getOutputLevel() const noexcept { return currentOutputRms.load (std::memory_order_relaxed); }
    void copyScopeData (std::array<float, kScopeSize>& dest) const noexcept;

private:
    struct TptSvf
    {
        float ic1eq = 0.0f;
        float ic2eq = 0.0f;

        void reset() noexcept { ic1eq = 0.0f; ic2eq = 0.0f; }

        inline void process (float input, float g, float k, float& lp, float& bp, float& hp) noexcept
        {
            const float a1 = 1.0f / (1.0f + g * (g + k));
            const float a2 = g * a1;
            const float a3 = g * a2;
            const float v3 = input - ic2eq;
            const float v1 = a1 * ic1eq + a2 * v3;
            const float v2 = ic2eq + a2 * ic1eq + a3 * v3;
            ic1eq = 2.0f * v1 - ic1eq;
            ic2eq = 2.0f * v2 - ic2eq;
            lp = v2;
            bp = v1;
            hp = input - k * v1 - v2;
        }
    };

    struct Voice
    {
        bool active = false;
        bool noteHeld = false;
        bool sustainedByPedal = false;
        int midiNote = 60;
        float velocity = 0.8f;
        uint64_t ageCounter = 0;

        double phaseA = 0.0;
        double phaseB = 0.0;
        double subPhase = 0.0;
        double shimmerPhase = 0.0;
        double fmModPhase = 0.0;
        double lfoPhase = 0.0;

        float strikeEnv = 0.0f;
        float sparkEnv = 0.0f;
        float pinkState = 0.0f;
        float geigerEnv = 0.0f;
        uint32_t rngState = 0x12345678u;
        float voicePan = 0.0f;

        juce::ADSR ampAdsr;
        juce::ADSR filterAdsr;
        TptSvf svf1;
        TptSvf svf2;

        void resetDsp() noexcept
        {
            svf1.reset();
            svf2.reset();
            pinkState = 0.0f;
            geigerEnv = 0.0f;
        }
    };

    struct StereoChorus
    {
        std::vector<float> delayL;
        std::vector<float> delayR;
        int writePos = 0;
        double lfoPhase = 0.0;
        double sampleRate = 44100.0;

        void prepare (double sr);
        void reset() noexcept;
        void processSample (float& left, float& right, float mix) noexcept;
    };

    void handleMidiEvent (const juce::MidiMessage& msg) noexcept;
    void noteOn (int midiNote, float velocity) noexcept;
    void noteOff (int midiNote) noexcept;
    void releaseSustainedNotes() noexcept;
    void renderSubBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples, const ElementSynthProfile& profile) noexcept;
    void updateVoiceEnvelopes (const ElementSynthProfile& profile) noexcept;

    static float polyBlep (double t, double dt) noexcept;

    double currentSampleRate = 44100.0;
    uint64_t noteCounter = 0;

    std::array<Voice, kMaxVoices> voices;
    StereoChorus chorus;
    juce::Reverb reverb;
    juce::Reverb::Parameters reverbParams;

    std::atomic<int> currentElementIndex { 0 };
    int appliedElementIndex = -1;
    ElementSynthProfile activeProfile;

    std::atomic<float> masterGain { 0.80f };
    std::atomic<float> spaceScale { 1.0f };
    std::atomic<bool> uiHoldActive { false };
    std::atomic<bool> panicRequested { false };

    bool midiSustainPedal = false;
    float pitchBendSemitones = 0.0f;
    float modWheelValue = 0.0f;
    float smoothedGain = 0.80f;

    std::array<std::atomic<float>, kScopeSize> scopeRing {};
    std::atomic<int> scopeWritePos { 0 };
    int scopeDecimator = 0;

    std::atomic<int> activeVoicesCount { 0 };
    std::atomic<float> currentOutputRms { 0.0f };
};

} // namespace PeriodicSynth
