#pragma once

#include "ElementData.h"
#include "SynthEngine.h"
#include <juce_audio_processors/juce_audio_processors.h>

class PeriodicSynthAudioProcessor : public juce::AudioProcessor
{
public:
    PeriodicSynthAudioProcessor();
    ~PeriodicSynthAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Md-101"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.5; }

    // Expose all 118 chemical elements as DAW factory presets
    int getNumPrograms() override { return PeriodicSynth::kNumElements; }
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // UI <-> Processor accessors
    void selectElementIndex (int zeroBasedIndex);
    int getSelectedElementIndex() const noexcept;

    juce::MidiKeyboardState& getKeyboardState() noexcept { return keyboardState; }
    PeriodicSynth::SynthEngine& getSynthEngine() noexcept { return synthEngine; }
    juce::AudioProcessorValueTreeState& getApvts() noexcept { return apvts; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    PeriodicSynth::SynthEngine synthEngine;

    std::atomic<float>* elementParam = nullptr;
    std::atomic<float>* gainParam    = nullptr;
    std::atomic<float>* spaceParam   = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PeriodicSynthAudioProcessor)
};
