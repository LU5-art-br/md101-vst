#include "SynthEngine.h"
#include <cmath>
#include <algorithm>

namespace PeriodicSynth
{

namespace
{
    constexpr double kTwoPi = juce::MathConstants<double>::twoPi;

    inline float nextRandomBipolar (uint32_t& state) noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float> (static_cast<int32_t> (state)) * (1.0f / 2147483648.0f);
    }
} // namespace

void SynthEngine::StereoChorus::prepare (double sr)
{
    sampleRate = std::isfinite (sr) ? juce::jlimit (8000.0, 768000.0, sr) : 44100.0;
    const int bufLen = static_cast<int> (sampleRate * 0.060) + 16; // 60 ms max delay
    delayL.assign (static_cast<size_t> (bufLen), 0.0f);
    delayR.assign (static_cast<size_t> (bufLen), 0.0f);
    writePos = 0;
    lfoPhase = 0.0;
}

void SynthEngine::StereoChorus::reset() noexcept
{
    std::fill (delayL.begin(), delayL.end(), 0.0f);
    std::fill (delayR.begin(), delayR.end(), 0.0f);
    writePos = 0;
}

void SynthEngine::StereoChorus::processSample (float& left, float& right, float mix) noexcept
{
    if (delayL.empty() || mix <= 0.001f)
        return;

    const int bufSize = static_cast<int> (delayL.size());
    delayL[static_cast<size_t> (writePos)] = left;
    delayR[static_cast<size_t> (writePos)] = right;

    lfoPhase += 0.65 / sampleRate;
    if (lfoPhase >= 1.0)
        lfoPhase -= 1.0;

    const float modL = static_cast<float> (std::sin (lfoPhase * kTwoPi));
    const float modR = static_cast<float> (std::cos (lfoPhase * kTwoPi));

    const float delaySamplesL = static_cast<float> (sampleRate) * (0.014f + 0.0045f * modL);
    const float delaySamplesR = static_cast<float> (sampleRate) * (0.016f + 0.0045f * modR);

    auto readInterp = [bufSize, this] (const std::vector<float>& buf, float delaySamps) noexcept -> float
    {
        float readPos = static_cast<float> (writePos) - delaySamps;
        while (readPos < 0.0f)
            readPos += static_cast<float> (bufSize);

        const int idx0 = static_cast<int> (readPos) % bufSize;
        const int idx1 = (idx0 + 1) % bufSize;
        const float frac = readPos - std::floor (readPos);
        return buf[static_cast<size_t> (idx0)] * (1.0f - frac) + buf[static_cast<size_t> (idx1)] * frac;
    };

    const float wetL = readInterp (delayL, delaySamplesL);
    const float wetR = readInterp (delayR, delaySamplesR);

    writePos = (writePos + 1) % bufSize;

    const float dry = 1.0f - 0.45f * mix;
    left  = left  * dry + wetL * (0.65f * mix);
    right = right * dry + wetR * (0.65f * mix);
}

SynthEngine::SynthEngine()
{
    for (size_t i = 0; i < voices.size(); ++i)
        voices[i].rngState = 0x9E3779B9u ^ static_cast<uint32_t> ((i + 1) * 7919u);

    for (auto& s : scopeRing)
        s.store (0.0f, std::memory_order_relaxed);

    activeProfile = computeSynthProfile (getElementByIndex (0));
}

void SynthEngine::prepare (double sampleRate, int /*maxBlockSize*/)
{
    currentSampleRate = std::isfinite (sampleRate) ? juce::jlimit (8000.0, 768000.0, sampleRate) : 44100.0;

    for (auto& v : voices)
    {
        v.ampAdsr.setSampleRate (currentSampleRate);
        v.filterAdsr.setSampleRate (currentSampleRate);
        v.ampAdsr.reset();
        v.filterAdsr.reset();
        v.resetDsp();
        v.active = false;
    }

    chorus.prepare (currentSampleRate);
    reverb.setSampleRate (currentSampleRate);
    reverb.reset();

    appliedElementIndex = -1;
    const int targetIdx = currentElementIndex.load (std::memory_order_relaxed);
    activeProfile = computeSynthProfile (getElementByIndex (targetIdx));
    updateVoiceEnvelopes (activeProfile);
    appliedElementIndex = targetIdx;
}

void SynthEngine::reset()
{
    for (auto& v : voices)
    {
        v.active = false;
        v.noteHeld = false;
        v.sustainedByPedal = false;
        v.ampAdsr.reset();
        v.filterAdsr.reset();
        v.resetDsp();
    }
    chorus.reset();
    reverb.reset();
    activeVoicesCount.store (0, std::memory_order_relaxed);
}

void SynthEngine::setElementPreset (int elementIndex) noexcept
{
    const int clamped = juce::jlimit (0, kNumElements - 1, elementIndex);
    currentElementIndex.store (clamped, std::memory_order_relaxed);
}

void SynthEngine::setSustainHold (bool holdOn) noexcept
{
    uiHoldActive.store (holdOn, std::memory_order_relaxed);
}

void SynthEngine::panic() noexcept
{
    panicRequested.store (true, std::memory_order_relaxed);
}

void SynthEngine::copyScopeData (std::array<float, kScopeSize>& dest) const noexcept
{
    const int wp = scopeWritePos.load (std::memory_order_relaxed);
    for (int i = 0; i < kScopeSize; ++i)
    {
        const int idx = (wp + i) % kScopeSize;
        dest[static_cast<size_t> (i)] = scopeRing[static_cast<size_t> (idx)].load (std::memory_order_relaxed);
    }
}

float SynthEngine::polyBlep (double t, double dt) noexcept
{
    if (t < dt)
    {
        t /= dt;
        return static_cast<float> (t + t - t * t - 1.0);
    }
    if (t > 1.0 - dt)
    {
        t = (t - 1.0) / dt;
        return static_cast<float> (t * t + t + t + 1.0);
    }
    return 0.0f;
}

void SynthEngine::updateVoiceEnvelopes (const ElementSynthProfile& profile) noexcept
{
    juce::ADSR::Parameters ampP;
    ampP.attack  = std::max (0.002f, profile.attackSec);
    ampP.decay   = std::max (0.015f, profile.decaySec);
    ampP.sustain = juce::jlimit (0.0f, 1.0f, profile.sustainLevel);
    ampP.release = std::max (0.020f, profile.releaseSec);

    juce::ADSR::Parameters fltP;
    fltP.attack  = std::max (0.002f, profile.attackSec * 0.75f);
    fltP.decay   = std::max (0.040f, profile.decaySec * 1.15f);
    fltP.sustain = juce::jlimit (0.10f, 0.85f, profile.sustainLevel * 0.55f);
    fltP.release = std::max (0.030f, profile.releaseSec);

    for (auto& v : voices)
    {
        v.ampAdsr.setParameters (ampP);
        v.filterAdsr.setParameters (fltP);
    }

    const float space = juce::jlimit (0.0f, 2.0f, spaceScale.load (std::memory_order_relaxed));
    const float effectiveWet = juce::jlimit (0.0f, 0.75f, profile.reverbMix * space);
    reverbParams.roomSize   = juce::jlimit (0.1f, 0.96f, profile.reverbRoomSize);
    reverbParams.damping    = juce::jlimit (0.05f, 0.90f, profile.reverbDamping);
    reverbParams.wetLevel   = effectiveWet * 0.45f;
    reverbParams.dryLevel   = 1.0f - effectiveWet * 0.22f;
    reverbParams.width      = 0.92f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters (reverbParams);
}

void SynthEngine::processBlock (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midiMessages) noexcept
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();
    const int totalSamples = buffer.getNumSamples();
    if (totalSamples <= 0 || buffer.getNumChannels() <= 0)
        return;

    if (panicRequested.exchange (false, std::memory_order_relaxed))
    {
        midiSustainPedal = false;
        reset();
    }

    // Check if element preset or reverb space changed
    const int targetIdx = currentElementIndex.load (std::memory_order_relaxed);
    if (targetIdx != appliedElementIndex)
    {
        activeProfile = computeSynthProfile (getElementByIndex (targetIdx));
        appliedElementIndex = targetIdx;
    }
    updateVoiceEnvelopes (activeProfile);

    // Release sustained notes if both MIDI pedal and UI Hold are now off
    const bool sustainOn = midiSustainPedal || uiHoldActive.load (std::memory_order_relaxed);
    if (! sustainOn)
        releaseSustainedNotes();

    // Sample-accurate MIDI event slicing
    int currentSample = 0;
    for (const auto metadata : midiMessages)
    {
        const int eventSample = juce::jlimit (0, totalSamples, metadata.samplePosition);
        if (eventSample > currentSample)
        {
            renderSubBlock (buffer, currentSample, eventSample - currentSample, activeProfile);
            currentSample = eventSample;
        }
        handleMidiEvent (metadata.getMessage());
    }

    if (currentSample < totalSamples)
        renderSubBlock (buffer, currentSample, totalSamples - currentSample, activeProfile);

    // Stereo Reverb pass
    if (buffer.getNumChannels() >= 2)
    {
        reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), totalSamples);
    }
    else if (buffer.getNumChannels() == 1)
    {
        reverb.processMono (buffer.getWritePointer (0), totalSamples);
    }

    // Master Gain, Soft Limiter, RMS metering & Oscilloscope feed
    const float targetGain = masterGain.load (std::memory_order_relaxed);
    float sumSq = 0.0f;
    int wp = scopeWritePos.load (std::memory_order_relaxed);

    float* leftOut  = buffer.getWritePointer (0);
    float* rightOut = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < totalSamples; ++i)
    {
        smoothedGain += 0.004f * (targetGain - smoothedGain);

        const float l = std::tanh (leftOut[i] * smoothedGain);
        leftOut[i] = l;

        float mono = l;
        if (rightOut != nullptr)
        {
            const float r = std::tanh (rightOut[i] * smoothedGain);
            rightOut[i] = r;
            mono = 0.5f * (l + r);
        }

        sumSq += mono * mono;

        // Decimate by 4 so 512 samples cover ~46ms at 44.1kHz (shows low & mid notes clearly!)
        if ((++scopeDecimator & 3) == 0)
        {
            scopeRing[static_cast<size_t> (wp)].store (mono, std::memory_order_relaxed);
            wp = (wp + 1) % kScopeSize;
        }
    }

    scopeWritePos.store (wp, std::memory_order_relaxed);

    const float blockRms = std::sqrt (sumSq / static_cast<float> (totalSamples));
    const float prevRms = currentOutputRms.load (std::memory_order_relaxed);
    currentOutputRms.store (blockRms > prevRms ? blockRms : (prevRms * 0.88f + blockRms * 0.12f),
                            std::memory_order_relaxed);

    int activeCount = 0;
    for (const auto& v : voices)
        if (v.active)
            ++activeCount;
    activeVoicesCount.store (activeCount, std::memory_order_relaxed);
}

void SynthEngine::handleMidiEvent (const juce::MidiMessage& msg) noexcept
{
    if (msg.isNoteOn())
    {
        noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
    }
    else if (msg.isNoteOff())
    {
        noteOff (msg.getNoteNumber());
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
    {
        midiSustainPedal = false;
        for (auto& v : voices)
        {
            v.noteHeld = false;
            v.sustainedByPedal = false;
            v.ampAdsr.noteOff();
            v.filterAdsr.noteOff();
        }
    }
    else if (msg.isSustainPedalOn())
    {
        midiSustainPedal = true;
    }
    else if (msg.isSustainPedalOff())
    {
        midiSustainPedal = false;
        if (! uiHoldActive.load (std::memory_order_relaxed))
            releaseSustainedNotes();
    }
    else if (msg.isPitchWheel())
    {
        const int pw = msg.getPitchWheelValue(); // 0..16383, center 8192
        pitchBendSemitones = (static_cast<float> (pw - 8192) / 8192.0f) * 2.0f;
    }
    else if (msg.isController() && msg.getControllerNumber() == 1)
    {
        modWheelValue = static_cast<float> (msg.getControllerValue()) / 127.0f;
    }
}

void SynthEngine::noteOn (int midiNote, float velocity) noexcept
{
    Voice* chosen = nullptr;

    // 1. Retrigger existing voice on same MIDI note if already active
    for (auto& v : voices)
    {
        if (v.active && v.midiNote == midiNote)
        {
            chosen = &v;
            break;
        }
    }

    // 2. Otherwise pick free voice
    if (chosen == nullptr)
    {
        for (auto& v : voices)
        {
            if (! v.active)
            {
                chosen = &v;
                break;
            }
        }
    }

    // 3. Voice stealing: prefer a releasing voice, else oldest voice
    if (chosen == nullptr)
    {
        Voice* oldestReleasing = nullptr;
        Voice* oldestAny = &voices[0];

        for (auto& v : voices)
        {
            if (v.ageCounter < oldestAny->ageCounter)
                oldestAny = &v;

            if (! v.noteHeld && ! v.sustainedByPedal)
            {
                if (oldestReleasing == nullptr || v.ageCounter < oldestReleasing->ageCounter)
                    oldestReleasing = &v;
            }
        }
        chosen = (oldestReleasing != nullptr) ? oldestReleasing : oldestAny;
        chosen->resetDsp();
    }

    const bool wasActive = chosen->active;
    chosen->active = true;
    chosen->noteHeld = true;
    chosen->sustainedByPedal = false;
    chosen->midiNote = midiNote;
    chosen->velocity = juce::jlimit (0.08f, 1.0f, velocity);
    chosen->ageCounter = ++noteCounter;

    if (! wasActive)
    {
        chosen->phaseA = 0.0;
        chosen->phaseB = 0.37;
        chosen->subPhase = 0.0;
        chosen->shimmerPhase = 0.19;
        chosen->fmModPhase = 0.0;
        chosen->lfoPhase = static_cast<double> (chosen->ageCounter % 7u) / 7.0;
        chosen->resetDsp();
    }

    chosen->strikeEnv = 1.0f;
    chosen->sparkEnv = 1.0f;
    chosen->voicePan = ((midiNote % 2 == 0) ? -1.0f : 1.0f) * (0.14f + 0.06f * static_cast<float> (midiNote % 5));

    chosen->ampAdsr.noteOn();
    chosen->filterAdsr.noteOn();
}

void SynthEngine::noteOff (int midiNote) noexcept
{
    const bool sustainOn = midiSustainPedal || uiHoldActive.load (std::memory_order_relaxed);

    for (auto& v : voices)
    {
        if (v.active && v.midiNote == midiNote && v.noteHeld)
        {
            v.noteHeld = false;
            if (sustainOn)
            {
                v.sustainedByPedal = true;
            }
            else
            {
                v.ampAdsr.noteOff();
                v.filterAdsr.noteOff();
            }
        }
    }
}

void SynthEngine::releaseSustainedNotes() noexcept
{
    for (auto& v : voices)
    {
        if (v.active && v.sustainedByPedal && ! v.noteHeld)
        {
            v.sustainedByPedal = false;
            v.ampAdsr.noteOff();
            v.filterAdsr.noteOff();
        }
    }
}

void SynthEngine::renderSubBlock (juce::AudioBuffer<float>& buffer, int startSample, int numSamples, const ElementSynthProfile& p) noexcept
{
    if (numSamples <= 0 || buffer.getNumChannels() <= 0)
        return;

    float* outL = buffer.getWritePointer (0);
    float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    const double invSr = 1.0 / currentSampleRate;
    const float strikeDecayCoeff = std::exp (-1.0f / std::max (1.0f, p.fmStrikeDecay * static_cast<float> (currentSampleRate)));
    const float sparkDecayCoeff  = std::exp (-1.0f / std::max (1.0f, 0.014f * static_cast<float> (currentSampleRate)));
    const float geigerDecayCoeff = std::exp (-1.0f / std::max (1.0f, 0.0045f * static_cast<float> (currentSampleRate)));

    const float detuneRatio = std::pow (2.0f, p.unisonDetuneCents / 1200.0f);
    const float invDetuneRatio = 1.0f / detuneRatio;
    const float space = juce::jlimit (0.0f, 2.0f, spaceScale.load (std::memory_order_relaxed));
    const float chorusMix = juce::jlimit (0.0f, 0.85f, p.chorusMix * (0.55f + 0.45f * space));

    for (int s = 0; s < numSamples; ++s)
    {
        float sumL = 0.0f;
        float sumR = 0.0f;

        for (auto& v : voices)
        {
            if (! v.active)
                continue;

            const float ampEnv = v.ampAdsr.getNextSample();
            const float fltEnv = v.filterAdsr.getNextSample();

            if (! v.ampAdsr.isActive())
            {
                v.active = false;
                v.noteHeld = false;
                v.sustainedByPedal = false;
                continue;
            }

            // Per-voice LFO
            v.lfoPhase += static_cast<double> (p.lfoRateHz) * invSr;
            if (v.lfoPhase >= 1.0)
                v.lfoPhase -= 1.0;
            const float lfoSin = static_cast<float> (std::sin (v.lfoPhase * kTwoPi));

            //Transient envelopes (FM strike & Alkali pitch spark)
            v.strikeEnv *= strikeDecayCoeff;
            v.sparkEnv  *= sparkDecayCoeff;

            // Pitch calculation (MIDI note + pitch bend + LFO + mod wheel + spark transient)
            const float lfoPitchSt = ((p.lfoPitchDepthCents + modWheelValue * 28.0f) / 100.0f) * lfoSin;
            const float sparkSt    = p.pitchSparkSemitones * v.sparkEnv;
            const float noteFloat  = static_cast<float> (v.midiNote) + pitchBendSemitones + lfoPitchSt + sparkSt;
            const float baseFreq   = 440.0f * std::pow (2.0f, (noteFloat - 69.0f) / 12.0f);

            const double dtBase = juce::jlimit (0.00001, 0.48, static_cast<double> (baseFreq) * invSr);
            const double dtA    = juce::jlimit (0.00001, 0.48, dtBase * static_cast<double> (detuneRatio));
            const double dtB    = juce::jlimit (0.00001, 0.48, dtBase * static_cast<double> (invDetuneRatio));

            // FM Modulator operator
            float fmPhaseMod = 0.0f;
            const float totalFmIndex = p.fmSustainedIndex + p.fmStrikeIndex * v.strikeEnv * (0.5f + 0.5f * v.velocity);
            if (totalFmIndex > 0.001f)
            {
                const double dtMod = juce::jlimit (0.00001, 0.48, dtBase * static_cast<double> (p.fmModRatio));
                v.fmModPhase += dtMod;
                if (v.fmModPhase >= 1.0)
                    v.fmModPhase -= 1.0;
                fmPhaseMod = (totalFmIndex / static_cast<float> (kTwoPi)) * static_cast<float> (std::sin (v.fmModPhase * kTwoPi));
            }

            // Advance Osc A & Osc B
            v.phaseA += dtA;
            if (v.phaseA >= 1.0) v.phaseA -= 1.0;
            v.phaseB += dtB;
            if (v.phaseB >= 1.0) v.phaseB -= 1.0;

            double modPhaseA = v.phaseA + static_cast<double> (fmPhaseMod);
            modPhaseA -= std::floor (modPhaseA);

            float oscMix = 0.0f;

            // 1. Pure Sine / Warm Triangle core (modulated by FM when active)
            if (p.triSineMix > 0.001f)
            {
                const float sineA = static_cast<float> (std::sin (modPhaseA * kTwoPi));
                const float triB  = static_cast<float> (1.0 - 4.0 * std::abs (v.phaseB - 0.5));
                oscMix += p.triSineMix * (0.65f * sineA + 0.35f * triB);
            }

            // 2. Band-limited PolyBLEP Dual Saw
            if (p.sawMix > 0.001f)
            {
                float sawA = static_cast<float> (2.0 * v.phaseA - 1.0) - polyBlep (v.phaseA, dtA);
                float sawB = static_cast<float> (2.0 * v.phaseB - 1.0) - polyBlep (v.phaseB, dtB);
                oscMix += p.sawMix * 0.5f * (sawA + sawB);
            }

            // 3. Band-limited PolyBLEP Pulse (with subtle LFO PWM)
            if (p.pulseMix > 0.001f)
            {
                const double pw = juce::jlimit (0.12, 0.88, static_cast<double> (p.pulseWidth + 0.08f * lfoSin));
                float pulse = (v.phaseA < pw) ? 1.0f : -1.0f;
                pulse += polyBlep (v.phaseA, dtA);
                double t2 = v.phaseA - pw;
                if (t2 < 0.0) t2 += 1.0;
                pulse -= polyBlep (t2, dtA);
                oscMix += p.pulseMix * pulse * 0.75f;
            }

            // 4. Sub-Oscillator (-1 octave, weighted by Atomic Mass)
            if (p.subOscMix > 0.001f)
            {
                v.subPhase += 0.5 * dtBase;
                if (v.subPhase >= 1.0) v.subPhase -= 1.0;
                const float subSample = static_cast<float> (std::sin (v.subPhase * kTwoPi));
                oscMix += p.subOscMix * subSample;
            }

            // 5. Shimmer Overtone (+1 octave crystalline harmonic)
            if (p.shimmerOctaveMix > 0.001f)
            {
                const double dtShim = std::min (0.48, 2.0 * dtBase);
                v.shimmerPhase += dtShim;
                if (v.shimmerPhase >= 1.0) v.shimmerPhase -= 1.0;
                const float shimSample = static_cast<float> (std::sin (v.shimmerPhase * kTwoPi));
                oscMix += p.shimmerOctaveMix * shimSample * 0.65f;
            }

            // 6. Gaseous Breath Turbulence & Radioactive Scintillation
            if (p.breathNoiseMix > 0.001f || p.geigerScintillation > 0.001f)
            {
                const float white = nextRandomBipolar (v.rngState);
                v.pinkState = 0.88f * v.pinkState + 0.12f * white;

                if (p.breathNoiseMix > 0.001f)
                {
                    const float air = white * 0.35f + v.pinkState * 0.65f;
                    oscMix += p.breathNoiseMix * air;
                }

                if (p.geigerScintillation > 0.001f)
                {
                    if (std::abs (white) > 0.9965f)
                        v.geigerEnv = 1.0f;
                    v.geigerEnv *= geigerDecayCoeff;
                    oscMix += p.geigerScintillation * v.geigerEnv * static_cast<float> (std::sin (v.phaseB * 7.0 * kTwoPi));
                }
            }

            // 7. Wavefolder / Warm Saturation stage
            float shaped = oscMix * 0.55f;
            if (p.wavefoldDrive > 1.05f)
            {
                const float driven = shaped * p.wavefoldDrive;
                if (p.category == ElementCategory::Semimetal)
                    shaped = 0.65f * std::sin (driven * 1.45f) + 0.35f * std::tanh (driven);
                else
                    shaped = std::tanh (driven);
            }

            // 8. Key-tracked + Envelope + LFO Cytomic TPT SVF Filter
            const float keyTrackOct = (noteFloat - 60.0f) / 12.0f * 0.65f;
            const float envOct      = p.filterEnvAmount * fltEnv * (0.55f + 0.45f * v.velocity) * 2.4f;
            const float lfoOct      = (p.lfoFilterDepthOct + modWheelValue * 0.35f) * lfoSin;
            const float velOct      = (v.velocity - 0.5f) * 0.55f;

            const float cutoffHz = juce::jlimit (65.0f,
                                                 static_cast<float> (currentSampleRate * 0.44),
                                                 p.filterCutoffHz * std::pow (2.0f, keyTrackOct + envOct + lfoOct + velOct));

            const float g = std::tan (juce::MathConstants<float>::pi * cutoffHz * static_cast<float> (invSr));
            const float k = 2.0f - 1.82f * juce::jlimit (0.05f, 0.90f, p.filterResonance);

            float lp1 = 0.0f, bp1 = 0.0f, hp1 = 0.0f;
            v.svf1.process (shaped, g, k, lp1, bp1, hp1);

            // Second pole stage for lush 24dB slope blended with airy bandpass when desired
            float lp2 = 0.0f, bp2 = 0.0f, hp2 = 0.0f;
            v.svf2.process (lp1, g, 1.4f, lp2, bp2, hp2);

            const float filtered = lp2 * (1.0f - p.filterBandpassBlend) + bp1 * (p.filterBandpassBlend * 1.35f);

            // 9. Amplitude + Velocity + Stereo Pan
            const float velGain = 0.32f + 0.68f * v.velocity;
            const float voiceSample = filtered * ampEnv * velGain * 0.34f;

            const float pan = juce::jlimit (-0.85f, 0.85f, v.voicePan + 0.18f * lfoSin);
            const float gainL = 0.7071f * (1.0f - pan);
            const float gainR = 0.7071f * (1.0f + pan);

            sumL += voiceSample * gainL;
            sumR += voiceSample * gainR;
        }

        chorus.processSample (sumL, sumR, chorusMix);

        outL[startSample + s] += sumL;
        if (outR != nullptr)
            outR[startSample + s] += sumR;
    }
}

} // namespace PeriodicSynth
