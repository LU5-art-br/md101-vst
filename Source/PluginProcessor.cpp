#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "LocalizationManager.h"
#include <utility>
#include <vector>

PeriodicSynth::Language PeriodicSynthAudioProcessor::detectInitialLanguage() noexcept
{
    const auto sysLang = juce::SystemStats::getUserLanguage();
    if (sysLang.startsWithIgnoreCase ("pt"))
        return PeriodicSynth::Language::Portuguese;
    return PeriodicSynth::Language::English;
}

PeriodicSynthAudioProcessor::PeriodicSynthAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Md101Parameters", createParameterLayout())
{
    const auto initialLang = detectInitialLanguage();
    setLanguage (initialLang);

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
            return juce::String (el.atomicNumber) + " - " + el.symbol + " (" + PeriodicSynth::getElementName (el, PeriodicSynth::Language::English) + ")";
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
    PeriodicSynth::LocalizationManager::getInstance().setLanguage (lang);
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

namespace
{
constexpr int kMaxStateSizeBytes = 1024 * 1024; // 1 MiB
constexpr int kMaxXmlDepth       = 64;
constexpr int kMaxXmlElements    = 8192;
constexpr juce::uint32 kJuceMagicXmlNumber = 0x21324356u;

bool isAsciiSpace (unsigned char ch) noexcept
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

bool matchBytesAt (const char* data, int len, int pos, const char* pat, int patLen) noexcept
{
    if (pos + patLen > len)
        return false;

    for (int i = 0; i < patLen; ++i)
    {
        if (data[pos + i] != pat[i])
            return false;
    }

    return true;
}

bool isXmlPayloadWithinLimits (const void* data, int sizeInBytes, int maxDepth, int maxElements) noexcept
{
    if (data == nullptr || sizeInBytes <= 8)
        return false;

    if (juce::ByteOrder::littleEndianInt (data) != kJuceMagicXmlNumber)
        return false;

    const int declaredLen = static_cast<int> (juce::ByteOrder::littleEndianInt (juce::addBytesToPointer (data, 4)));
    if (declaredLen <= 0)
        return false;

    const int xmlLen = juce::jmin (sizeInBytes - 8, declaredLen);
    const char* ptr  = static_cast<const char*> (data) + 8;

    int depth        = 0;
    int elementCount = 0;
    int i            = 0;

    while (i < xmlLen && ptr[i] != '\0')
    {
        if (ptr[i] != '<')
        {
            ++i;
            continue;
        }

        if (i + 1 >= xmlLen || ptr[i + 1] == '\0')
            break;

        const char next = ptr[i + 1];

        if (next == '?')
        {
            i += 2;
            while (i < xmlLen && ptr[i] != '\0')
            {
                if (matchBytesAt (ptr, xmlLen, i, "?>", 2))
                {
                    i += 2;
                    break;
                }
                ++i;
            }
            continue;
        }

        if (next == '!')
        {
            if (matchBytesAt (ptr, xmlLen, i, "<!--", 4))
            {
                i += 4;
                while (i < xmlLen && ptr[i] != '\0')
                {
                    if (matchBytesAt (ptr, xmlLen, i, "-->", 3))
                    {
                        i += 3;
                        break;
                    }
                    ++i;
                }
            }
            else if (matchBytesAt (ptr, xmlLen, i, "<![CDATA[", 9))
            {
                i += 9;
                while (i < xmlLen && ptr[i] != '\0')
                {
                    if (matchBytesAt (ptr, xmlLen, i, "]]>", 3))
                    {
                        i += 3;
                        break;
                    }
                    ++i;
                }
            }
            else
            {
                i += 2;
                while (i < xmlLen && ptr[i] != '\0' && ptr[i] != '>')
                    ++i;
                if (i < xmlLen && ptr[i] == '>')
                    ++i;
            }
            continue;
        }

        if (next == '/')
        {
            i += 2;
            while (i < xmlLen && ptr[i] != '\0' && ptr[i] != '>')
                ++i;
            if (i < xmlLen && ptr[i] == '>')
                ++i;
            if (depth > 0)
                --depth;
            continue;
        }

        // Opening or self-closing tag
        ++elementCount;
        if (elementCount > maxElements)
            return false;

        ++i;
        char quote       = '\0';
        char lastNonWs   = '\0';

        while (i < xmlLen && ptr[i] != '\0')
        {
            const char ch = ptr[i];
            if (quote != '\0')
            {
                if (ch == quote)
                    quote = '\0';
                ++i;
            }
            else if (ch == '"' || ch == '\'')
            {
                quote     = ch;
                lastNonWs = ch;
                ++i;
            }
            else if (ch == '>')
            {
                ++i;
                break;
            }
            else
            {
                if (! isAsciiSpace (static_cast<unsigned char> (ch)))
                    lastNonWs = ch;
                ++i;
            }
        }

        if (lastNonWs == '/')
        {
            if (depth + 1 > maxDepth)
                return false;
        }
        else
        {
            ++depth;
            if (depth > maxDepth)
                return false;
        }
    }

    return true;
}

bool isXmlTreeWithinLimits (const juce::XmlElement& root, int maxDepth, int maxElements)
{
    std::vector<std::pair<const juce::XmlElement*, int>> stack;
    stack.reserve (64);
    stack.emplace_back (&root, 1);

    int elementCount = 0;

    while (! stack.empty())
    {
        const auto current = stack.back();
        stack.pop_back();

        if (current.second > maxDepth)
            return false;

        ++elementCount;
        if (elementCount > maxElements)
            return false;

        for (const auto* child = current.first->getFirstChildElement();
             child != nullptr;
             child = child->getNextElement())
        {
            stack.emplace_back (child, current.second + 1);
        }
    }

    return true;
}
} // namespace

void PeriodicSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0 || sizeInBytes > kMaxStateSizeBytes)
        return;

    if (! isXmlPayloadWithinLimits (data, sizeInBytes, kMaxXmlDepth, kMaxXmlElements))
        return;

    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (! isXmlTreeWithinLimits (*xml, kMaxXmlDepth, kMaxXmlElements))
            return;

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
