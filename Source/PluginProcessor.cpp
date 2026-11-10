#include "PluginProcessor.h"
#include "PluginEditor.h"

PeriodicSynthAudioProcessor::PeriodicSynthAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Md101Parameters", createParameterLayout())
{
    elementParam = apvts.getRawParameterValue ("element");
    gainParam    = apvts.getRawParameterValue ("masterGain");
    spaceParam   = apvts.getRawParameterValue ("space");
}

PeriodicSynthAudioProcessor::~PeriodicSynthAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout PeriodicSynthAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto elementAttr = juce::AudioParameterIntAttributes()
        .withStringFromValueFunction ([] (int value, int)
        {
            const auto& el = PeriodicSynth::getElementByAtomicNumber (value);
            return juce::String (el.atomicNumber) + " - " + el.symbol + " (" + juce::String::fromUTF8 (el.nameEn) + ")";
        });

    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "element", 1 },
        "Chemical Element (Preset)",
        1,
        PeriodicSynth::kNumElements,
        1,
        elementAttr));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "masterGain", 1 },
        "Master Volume",
        juce::NormalisableRange<float> (0.0f, 1.5f, 0.01f),
        0.82f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "space", 1 },
        "Ambience / Reverb",
        juce::NormalisableRange<float> (0.0f, 2.0f, 0.01f),
        1.00f));

    return { params.begin(), params.end() };
}

void PeriodicSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    keyboardState.reset();
    synthEngine.prepare (sampleRate, samplesPerBlock);
    synthEngine.setElementPreset (getSelectedElementIndex());
}

void PeriodicSynthAudioProcessor::releaseResources()
{
    keyboardState.reset();
    synthEngine.reset();
}

bool PeriodicSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet().isDisabled();
}

void PeriodicSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Sync UI piano <-> incoming DAW MIDI track
    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

    if (elementParam != nullptr)
        synthEngine.setElementPreset (static_cast<int> (std::round (elementParam->load())) - 1);

    if (gainParam != nullptr)
        synthEngine.setMasterGain (gainParam->load());

    if (spaceParam != nullptr)
        synthEngine.setSpaceScale (spaceParam->load());

    synthEngine.processBlock (buffer, midiMessages);
}

int PeriodicSynthAudioProcessor::getSelectedElementIndex() const noexcept
{
    if (elementParam == nullptr)
        return 0;

    return juce::jlimit (0, PeriodicSynth::kNumElements - 1,
                         static_cast<int> (std::round (elementParam->load())) - 1);
}

void PeriodicSynthAudioProcessor::selectElementIndex (int zeroBasedIndex)
{
    const int atomicNum = juce::jlimit (1, PeriodicSynth::kNumElements, zeroBasedIndex + 1);
    if (auto* param = apvts.getParameter ("element"))
    {
        const float norm = param->convertTo0to1 (static_cast<float> (atomicNum));
        param->beginChangeGesture();
        param->setValueNotifyingHost (norm);
        param->endChangeGesture();
    }
    synthEngine.setElementPreset (atomicNum - 1);
}

void PeriodicSynthAudioProcessor::setLanguage (PeriodicSynth::Language lang) noexcept
{
    currentLanguage.store (static_cast<int> (lang), std::memory_order_relaxed);
}

PeriodicSynth::Language PeriodicSynthAudioProcessor::getLanguage() const noexcept
{
    const int raw = currentLanguage.load (std::memory_order_relaxed);
    return (raw == static_cast<int> (PeriodicSynth::Language::Portuguese))
               ? PeriodicSynth::Language::Portuguese
               : PeriodicSynth::Language::English;
}

int PeriodicSynthAudioProcessor::getCurrentProgram()
{
    return getSelectedElementIndex();
}

void PeriodicSynthAudioProcessor::setCurrentProgram (int index)
{
    selectElementIndex (index);
}

const juce::String PeriodicSynthAudioProcessor::getProgramName (int index)
{
    const auto& el = PeriodicSynth::getElementByIndex (index);
    return juce::String (el.atomicNumber).paddedLeft ('0', 3) + " - " + el.symbol
         + " (" + PeriodicSynth::getElementName (el, getLanguage()) + ")";
}

void PeriodicSynthAudioProcessor::changeProgramName (int, const juce::String&)
{
}

void PeriodicSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const auto state = apvts.copyState();
    if (auto xml = state.createXml())
    {
        xml->setAttribute ("uiLanguage", static_cast<int> (getLanguage()));
        copyXmlToBinary (*xml, destData);
    }
}

void PeriodicSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            if (xml->hasAttribute ("uiLanguage"))
            {
                const int langVal = xml->getIntAttribute ("uiLanguage", 0);
                setLanguage (langVal == static_cast<int> (PeriodicSynth::Language::Portuguese)
                                 ? PeriodicSynth::Language::Portuguese
                                 : PeriodicSynth::Language::English);
            }
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            synthEngine.setElementPreset (getSelectedElementIndex());
        }
    }
}

juce::AudioProcessorEditor* PeriodicSynthAudioProcessor::createEditor()
{
    return new PeriodicSynthAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PeriodicSynthAudioProcessor();
}
