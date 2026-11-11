#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <iostream>
#include <set>

namespace
{
    float computeSpectralCentroidProxy (const juce::AudioBuffer<float>& buf)
    {
        // Zero-crossing / derivative energy ratio proxy for spectral brightness verification
        const float* x = buf.getReadPointer (0);
        const int n = buf.getNumSamples();
        double diffEnergy = 0.0;
        double sigEnergy  = 0.0;
        for (int i = 1; i < n; ++i)
        {
            const double d = static_cast<double> (x[i]) - static_cast<double> (x[i - 1]);
            diffEnergy += d * d;
            sigEnergy  += static_cast<double> (x[i]) * static_cast<double> (x[i]);
        }
        return sigEnergy > 1e-9 ? static_cast<float> ( std::sqrt (diffEnergy / sigEnergy) ) : 0.0f;
    }
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::cout << "=== Md-101 Automated Verification Suite (by LU5 - https://lu5.art.br) ===" << std::endl;

    // 1. Verify all 118 elements & grid layout against reference table
    const auto& elements = PeriodicSynth::getAllElements();
    std::set<std::pair<int, int>> occupiedCells;
    std::array<int, static_cast<size_t> (PeriodicSynth::ElementCategory::Count)> categoryCounts {};

    for (int i = 0; i < PeriodicSynth::kNumElements; ++i)
    {
        const auto& el = elements[static_cast<size_t> (i)];
        if (el.atomicNumber != i + 1)
        {
            std::cerr << "FAIL: Element index " << i << " has wrong Z=" << el.atomicNumber << std::endl;
            return 1;
        }
        if (el.atomicMass <= 0.0f)
        {
            std::cerr << "FAIL: Element Z=" << el.atomicNumber << " has invalid mass" << std::endl;
            return 1;
        }
        if (el.nameEn == nullptr || el.nameEn[0] == '\0' || el.namePt == nullptr || el.namePt[0] == '\0')
        {
            std::cerr << "FAIL: Element Z=" << el.atomicNumber << " is missing English or Portuguese name" << std::endl;
            return 1;
        }
        const auto prof = PeriodicSynth::computeSynthProfile (el);
        if (prof.engineDescriptionEn == nullptr || prof.engineDescriptionEn[0] == '\0'
            || prof.engineDescriptionPt == nullptr || prof.engineDescriptionPt[0] == '\0')
        {
            std::cerr << "FAIL: Element Z=" << el.atomicNumber << " is missing bilingual engine description" << std::endl;
            return 1;
        }
        const auto pos = std::make_pair (el.gridRow, el.gridCol);
        if (! occupiedCells.insert (pos).second)
        {
            std::cerr << "FAIL: Duplicate grid cell at row=" << el.gridRow << " col=" << el.gridCol << std::endl;
            return 1;
        }
        // Ensure f-block connector cells (5,2) and (6,2) and HUD area (0..2, 2..11) are clear
        if ((el.gridRow == 5 && el.gridCol == 2) || (el.gridRow == 6 && el.gridCol == 2))
        {
            std::cerr << "FAIL: Element overlaps f-block placeholder cell" << std::endl;
            return 1;
        }
        if (el.gridRow <= 2 && el.gridCol >= 2 && el.gridCol <= 11)
        {
            std::cerr << "FAIL: Element overlaps central HUD area" << std::endl;
            return 1;
        }
        categoryCounts[static_cast<size_t> (el.category)]++;
    }

    // Expected counts matching the Toda Matéria reference table
    const std::array<int, 10> expectedCounts {{ 7, 6, 7, 11, 15, 7, 6, 6, 38, 15 }};
    for (size_t c = 0; c < expectedCounts.size(); ++c)
    {
        if (categoryCounts[c] != expectedCounts[c])
        {
            std::cerr << "FAIL: Category " << c << " count=" << categoryCounts[c]
                      << " expected=" << expectedCounts[c] << std::endl;
            return 1;
        }
    }
    std::cout << "[PASS] 118 Elements verified (exact reference categories, masses, and non-overlapping grid)." << std::endl;

    // 2. Verify Processor DAW interface & State Persistence
    PeriodicSynthAudioProcessor proc;
    proc.prepareToPlay (44100.0, 512);

    if (proc.getNumPrograms() != 118 || ! proc.acceptsMidi())
    {
        std::cerr << "FAIL: Processor program count or MIDI acceptance invalid" << std::endl;
        return 1;
    }

    proc.setCurrentProgram (78); // 79 - Au (Gold)
    proc.setLanguage (PeriodicSynth::Language::Portuguese);
    juce::MemoryBlock stateBlob;
    proc.getStateInformation (stateBlob);

    proc.setCurrentProgram (1);  // 2 - He (Helium)
    proc.setLanguage (PeriodicSynth::Language::English);
    proc.setStateInformation (stateBlob.getData(), static_cast<int> (stateBlob.getSize()));
    if (proc.getCurrentProgram() != 78 || proc.getLanguage() != PeriodicSynth::Language::Portuguese)
    {
        std::cerr << "FAIL: State save/restore did not preserve selected element or language (expected 78/PT, got "
                  << proc.getCurrentProgram() << ")" << std::endl;
        return 1;
    }
    proc.setLanguage (PeriodicSynth::Language::English);
    std::cout << "[PASS] DAW Preset Programs (118), Bilingual EN/PT Names & XML State Persistence verified." << std::endl;

    // 3. Render MIDI audio through ALL 118 elements & verify signal integrity + physical trends
    juce::AudioBuffer<float> audioBuf (2, 512);
    float brightnessHe = 0.0f, brightnessRn = 0.0f;
    float brightnessLi = 0.0f, brightnessCs = 0.0f;

    for (int idx = 0; idx < PeriodicSynth::kNumElements; ++idx)
    {
        proc.getSynthEngine().reset();
        proc.selectElementIndex (idx);

        juce::AudioBuffer<float> accumBuf (2, 4096);
        accumBuf.clear();

        float maxAbs = 0.0f;
        bool hasNan = false;

        for (int blk = 0; blk < 8; ++blk)
        {
            juce::MidiBuffer midi;
            if (blk == 0)
                midi.addEvent (juce::MidiMessage::noteOn (1, 60, static_cast<juce::uint8> (105)), 0);
            if (blk == 6)
                midi.addEvent (juce::MidiMessage::noteOff (1, 60), 0);

            audioBuf.clear();
            proc.processBlock (audioBuf, midi);

            for (int ch = 0; ch < 2; ++ch)
            {
                accumBuf.copyFrom (ch, blk * 512, audioBuf, ch, 0, 512);
                const float* p = audioBuf.getReadPointer (ch);
                for (int s = 0; s < 512; ++s)
                {
                    if (! std::isfinite (p[s]))
                        hasNan = true;
                    maxAbs = std::max (maxAbs, std::abs (p[s]));
                }
            }
        }

        if (hasNan || maxAbs < 0.005f || maxAbs > 1.001f)
        {
            std::cerr << "FAIL: Audio check failed on element Z=" << (idx + 1)
                      << " maxAbs=" << maxAbs << " hasNan=" << hasNan << std::endl;
            return 1;
        }

        if (idx + 1 == 2)  brightnessHe = computeSpectralCentroidProxy (accumBuf);
        if (idx + 1 == 86) brightnessRn = computeSpectralCentroidProxy (accumBuf);
        if (idx + 1 == 3)  brightnessLi = computeSpectralCentroidProxy (accumBuf);
        if (idx + 1 == 55) brightnessCs = computeSpectralCentroidProxy (accumBuf);
    }

    std::cout << "[PASS] All 118 Element Presets synthesized clean, finite, non-clipping stereo audio!" << std::endl;
    std::cout << "       Noble Gas Brightness Proxy: He(Z=2, M=4.0)=" << brightnessHe
              << " > Rn(Z=86, M=222)=" << brightnessRn << std::endl;
    std::cout << "       Alkali Brightness Proxy:    Li(Z=3, M=6.94)=" << brightnessLi
              << " > Cs(Z=55, M=132.9)=" << brightnessCs << std::endl;

    if (brightnessHe <= brightnessRn || brightnessLi <= brightnessCs)
    {
        std::cerr << "FAIL: Expected lighter atomic mass elements to have brighter spectral centroid than heavy homologues." << std::endl;
        return 1;
    }

    // 4. Optional UI Offscreen Snapshot Rendering (if output directory passed as argv[1])
    if (argc > 1)
    {
        const juce::File outDir (juce::String::fromUTF8 (argv[1]));
        outDir.createDirectory();

        std::unique_ptr<juce::AudioProcessorEditor> rawEd (proc.createEditor());
        auto* ed = dynamic_cast<PeriodicSynthAudioProcessorEditor*> (rawEd.get());
        if (ed != nullptr)
        {
            // Snapshot 1: Helium (Gás nobre) with Cmaj7 chord active on the footer Piano & live oscilloscope!
            proc.getSynthEngine().reset();
            proc.selectElementIndex (1); // 2 - He (Hélio)
            proc.getKeyboardState().noteOn (1, 60, 0.85f);
            proc.getKeyboardState().noteOn (1, 64, 0.80f);
            proc.getKeyboardState().noteOn (1, 67, 0.82f);
            proc.getKeyboardState().noteOn (1, 71, 0.78f);

            for (int b = 0; b < 12; ++b)
            {
                juce::MidiBuffer emptyMidi;
                audioBuf.clear();
                proc.processBlock (audioBuf, emptyMidi);
            }

            std::array<float, PeriodicSynth::SynthEngine::kScopeSize> scope {};
            proc.getSynthEngine().copyScopeData (scope);
            ed->getPeriodicTableGrid().setSelectedElement (1);
            ed->getPianoKeyboard().setAccentColour (PeriodicSynth::getCategoryColour (PeriodicSynth::ElementCategory::NobleGas));
            ed->getPeriodicTableGrid().updateAudioVisuals (scope,
                                                           proc.getSynthEngine().getOutputLevel(),
                                                           proc.getSynthEngine().getActiveVoiceCount());

            // Simulate hovering over Neon (10 Ne) to test hover + selection indicators simultaneously!
            const auto nePoint = juce::Point<float> (1095.0f, 82.0f);
            juce::MouseEvent fakeHover (juce::Desktop::getInstance().getMainMouseSource(),
                                        nePoint, juce::ModifierKeys(),
                                        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                        &ed->getPeriodicTableGrid(), &ed->getPeriodicTableGrid(),
                                        juce::Time::getCurrentTime(), nePoint, juce::Time::getCurrentTime(), 0, false);
            ed->getPeriodicTableGrid().mouseMove (fakeHover);

            auto img1 = ed->createComponentSnapshot (ed->getLocalBounds());
            juce::File snap1 = outDir.getChildFile ("snapshot_helium_chord.png");
            snap1.deleteFile();
            if (auto stream = snap1.createOutputStream())
            {
                juce::PNGImageFormat png;
                png.writeImageToStream (img1, *stream);
                std::cout << "[PASS] Saved UI snapshot 1: " << snap1.getFullPathName() << std::endl;
            }

            // Snapshot 2: Mendelevium (101 - Md, Actinide — namesake of Md-101!)
            proc.getKeyboardState().allNotesOff (0);
            proc.getSynthEngine().reset();
            proc.selectElementIndex (100); // 101 - Md (Mendelevium)
            proc.getKeyboardState().noteOn (1, 48, 0.92f);
            proc.getKeyboardState().noteOn (1, 55, 0.88f);
            proc.getKeyboardState().noteOn (1, 60, 0.90f);
            proc.getKeyboardState().noteOn (1, 63, 0.86f);
            for (int b = 0; b < 8; ++b)
            {
                juce::MidiBuffer emptyMidi;
                audioBuf.clear();
                proc.processBlock (audioBuf, emptyMidi);
            }
            ed->refreshFromProcessor();

            auto img2 = ed->createComponentSnapshot (ed->getLocalBounds());
            juce::File snap2 = outDir.getChildFile ("snapshot_md101_mendelevium.png");
            snap2.deleteFile();
            if (auto stream = snap2.createOutputStream())
            {
                juce::PNGImageFormat png;
                png.writeImageToStream (img2, *stream);
                std::cout << "[PASS] Saved UI snapshot 2 (EN): " << snap2.getFullPathName() << std::endl;
            }

            // Verify top-bar language button switches EN -> PT and back
            if (ed->getLanguage() != PeriodicSynth::Language::English)
            {
                std::cerr << "FAIL: Default editor language should be English" << std::endl;
                return 1;
            }
            if (ed->getLanguageButton().onClick)
                ed->getLanguageButton().onClick();
            if (ed->getLanguage() != PeriodicSynth::Language::Portuguese
                || proc.getLanguage() != PeriodicSynth::Language::Portuguese
                || ed->getPeriodicTableGrid().getLanguage() != PeriodicSynth::Language::Portuguese)
            {
                std::cerr << "FAIL: Language button did not switch editor/processor/grid to Portuguese" << std::endl;
                return 1;
            }

            auto img3 = ed->createComponentSnapshot (ed->getLocalBounds());
            juce::File snap3 = outDir.getChildFile ("snapshot_md101_pt.png");
            snap3.deleteFile();
            if (auto stream = snap3.createOutputStream())
            {
                juce::PNGImageFormat png;
                png.writeImageToStream (img3, *stream);
                std::cout << "[PASS] Saved UI snapshot 3 (PT): " << snap3.getFullPathName() << std::endl;
            }

            if (ed->getLanguageButton().onClick)
                ed->getLanguageButton().onClick();
            if (ed->getLanguage() != PeriodicSynth::Language::English)
            {
                std::cerr << "FAIL: Language button did not toggle back to English" << std::endl;
                return 1;
            }

            // Verify About/Sobre modal dialog opens and closes cleanly
            ed->showAboutDialog (true);
            if (! ed->isAboutDialogVisible())
            {
                std::cerr << "FAIL: About dialog did not become visible" << std::endl;
                return 1;
            }
            auto imgAbout = ed->createComponentSnapshot (ed->getLocalBounds());
            juce::File snapAbout = outDir.getChildFile ("snapshot_md101_about.png");
            snapAbout.deleteFile();
            if (auto stream = snapAbout.createOutputStream())
            {
                juce::PNGImageFormat png;
                png.writeImageToStream (imgAbout, *stream);
                std::cout << "[PASS] Saved UI snapshot 4 (About/Sobre): " << snapAbout.getFullPathName() << std::endl;
            }
            ed->showAboutDialog (false);
            if (ed->isAboutDialogVisible())
            {
                std::cerr << "FAIL: About dialog did not close" << std::endl;
                return 1;
            }
        }
    }

    std::cout << "=== ALL VERIFICATION CHECKS PASSED ===" << std::endl;
    return 0;
}
