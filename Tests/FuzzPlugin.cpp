#include "../Source/PluginProcessor.h"
#include "../Source/SynthEngine.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

namespace
{
struct FuzzRuntime
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    PeriodicSynthAudioProcessor processor;
    PeriodicSynth::SynthEngine engine;
    bool prepared = false;
};

FuzzRuntime& getRuntime()
{
    static FuzzRuntime runtime;
    if (! runtime.prepared)
    {
        runtime.processor.prepareToPlay (44100.0, 256);
        runtime.engine.prepare (44100.0, 256);
        runtime.prepared = true;
    }
    return runtime;
}

void writeLittleEndian32 (uint8_t* dst, uint32_t value) noexcept
{
    dst[0] = static_cast<uint8_t> (value & 0xffu);
    dst[1] = static_cast<uint8_t> ((value >> 8) & 0xffu);
    dst[2] = static_cast<uint8_t> ((value >> 16) & 0xffu);
    dst[3] = static_cast<uint8_t> ((value >> 24) & 0xffu);
}

void fuzzStateParser (FuzzRuntime& rt, const uint8_t* data, size_t size)
{
    const int cappedSize = static_cast<int> (std::min<size_t> (size, 2u * 1024u * 1024u));
    rt.processor.setStateInformation (data, cappedSize);

    // Also test XML body wrapped in JUCE's copyXmlToBinary header so deep/malformed XML is reached
    if (size > 0 && size <= 1024u * 1024u)
    {
        std::vector<uint8_t> wrapped (8 + size + 1, 0);
        writeLittleEndian32 (wrapped.data(), 0x21324356u);
        writeLittleEndian32 (wrapped.data() + 4, static_cast<uint32_t> (size));
        std::memcpy (wrapped.data() + 8, data, size);
        rt.processor.setStateInformation (wrapped.data(), static_cast<int> (wrapped.size()));
    }

    juce::MemoryBlock outState;
    rt.processor.getStateInformation (outState);
}

void fuzzSynthEngine (FuzzRuntime& rt, const uint8_t* data, size_t size)
{
    if (size < 4)
        return;

    static constexpr double kSampleRates[] = {
        44100.0,
        48000.0,
        96000.0,
        0.0,
        -44100.0,
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity()
    };

    const double sr = kSampleRates[data[0] % (sizeof (kSampleRates) / sizeof (kSampleRates[0]))];
    const int blockSize = 16 + (data[1] % 240);
    const int numChannels = data[2] % 3; // Exercise 0, 1, and 2 channels
    const int presetIndex = static_cast<int> (static_cast<int8_t> (data[3]));

    rt.engine.prepare (sr, blockSize);
    rt.engine.setElementPreset (presetIndex);
    rt.engine.setMasterGain (0.5f);
    rt.engine.setSpaceScale (0.5f);

    juce::MidiBuffer midi;
    for (size_t offset = 4; offset + 3 <= size; offset += 3)
    {
        const uint8_t status = static_cast<uint8_t> (0x80u | (data[offset] & 0x7fu));
        const uint8_t d1 = static_cast<uint8_t> (data[offset + 1] & 0x7fu);
        const uint8_t d2 = static_cast<uint8_t> (data[offset + 2] & 0x7fu);
        const int samplePos = static_cast<int> ((offset - 4) % static_cast<size_t> (blockSize));

        if ((status & 0xf0u) != 0xf0u)
            midi.addEvent (juce::MidiMessage (status, d1, d2), samplePos);
    }

    juce::AudioBuffer<float> audio (numChannels, blockSize);
    audio.clear();
    rt.engine.processBlock (audio, midi);
    rt.engine.panic();
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput (const uint8_t* data, size_t size)
{
    if (data == nullptr)
        return 0;

    auto& rt = getRuntime();
    fuzzStateParser (rt, data, size);
    fuzzSynthEngine (rt, data, size);
    return 0;
}
