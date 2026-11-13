#include "ElementData.h"
#include <cmath>
#include <algorithm>

namespace PeriodicSynth
{

namespace
{
    constexpr ElementCategory NM = ElementCategory::NonMetal;
    constexpr ElementCategory AM = ElementCategory::AlkaliMetal;
    constexpr ElementCategory SM = ElementCategory::Semimetal;
    constexpr ElementCategory OM = ElementCategory::OtherMetal;
    constexpr ElementCategory LA = ElementCategory::Lanthanide;
    constexpr ElementCategory NG = ElementCategory::NobleGas;
    constexpr ElementCategory AE = ElementCategory::AlkalineEarth;
    constexpr ElementCategory HA = ElementCategory::Halogen;
    constexpr ElementCategory TM = ElementCategory::TransitionMetal;
    constexpr ElementCategory AC = ElementCategory::Actinide;

    constexpr PhysicalPhase Sol = PhysicalPhase::Solid;
    constexpr PhysicalPhase Liq = PhysicalPhase::Liquid;
    constexpr PhysicalPhase Gas = PhysicalPhase::Gas;

    // Complete 118-element Periodic Table matching the reference Toda Matéria table
    const std::array<ElementInfo, kNumElements> kElements {{
        // Period 1 (row 0)
        { 1, "H", 1.008f, "1,008", NM, Gas, 1, 1, 0, 0, false },
        { 2, "He", 4.0026f, "4,0026", NG, Gas, 1, 18, 0, 17, false },

        // Period 2 (row 1)
        { 3, "Li", 6.94f, "6,94", AM, Sol, 2, 1, 1, 0, false },
        { 4, "Be", 9.0122f, "9,0122", AE, Sol, 2, 2, 1, 1, false },
        { 5, "B", 10.81f, "10,81", SM, Sol, 2, 13, 1, 12, false },
        { 6, "C", 12.011f, "12,011", NM, Sol, 2, 14, 1, 13, false },
        { 7, "N", 14.007f, "14,007", NM, Gas, 2, 15, 1, 14, false },
        { 8, "O", 15.999f, "15,999", NM, Gas, 2, 16, 1, 15, false },
        { 9, "F", 18.998f, "18,998", HA, Gas, 2, 17, 1, 16, false },
        { 10, "Ne", 20.180f, "20,180", NG, Gas, 2, 18, 1, 17, false },

        // Period 3 (row 2)
        { 11, "Na", 22.990f, "22,990", AM, Sol, 3, 1, 2, 0, false },
        { 12, "Mg", 24.305f, "24,305", AE, Sol, 3, 2, 2, 1, false },
        { 13, "Al", 26.982f, "26,982", OM, Sol, 3, 13, 2, 12, false },
        { 14, "Si", 28.085f, "28,085", SM, Sol, 3, 14, 2, 13, false },
        { 15, "P", 30.974f, "30,974", NM, Sol, 3, 15, 2, 14, false },
        { 16, "S", 32.06f, "32,06", NM, Sol, 3, 16, 2, 15, false },
        { 17, "Cl", 35.45f, "35,45", HA, Gas, 3, 17, 2, 16, false },
        { 18, "Ar", 39.948f, "39,948", NG, Gas, 3, 18, 2, 17, false },

        // Period 4 (row 3)
        { 19, "K", 39.098f, "39,098", AM, Sol, 4, 1, 3, 0, false },
        { 20, "Ca", 40.078f, "40,078", AE, Sol, 4, 2, 3, 1, false },
        { 21, "Sc", 44.956f, "44,956", TM, Sol, 4, 3, 3, 2, false },
        { 22, "Ti", 47.867f, "47,867", TM, Sol, 4, 4, 3, 3, false },
        { 23, "V", 50.942f, "50,942", TM, Sol, 4, 5, 3, 4, false },
        { 24, "Cr", 51.996f, "51,996", TM, Sol, 4, 6, 3, 5, false },
        { 25, "Mn", 54.938f, "54,938", TM, Sol, 4, 7, 3, 6, false },
        { 26, "Fe", 55.845f, "55,845", TM, Sol, 4, 8, 3, 7, false },
        { 27, "Co", 58.933f, "58,933", TM, Sol, 4, 9, 3, 8, false },
        { 28, "Ni", 58.693f, "58,693", TM, Sol, 4, 10, 3, 9, false },
        { 29, "Cu", 63.546f, "63,546", TM, Sol, 4, 11, 3, 10, false },
        { 30, "Zn", 65.38f, "65,38", TM, Sol, 4, 12, 3, 11, false },
        { 31, "Ga", 69.723f, "69,723", OM, Sol, 4, 13, 3, 12, false },
        { 32, "Ge", 72.630f, "72,630", SM, Sol, 4, 14, 3, 13, false },
        { 33, "As", 74.922f, "74,922", SM, Sol, 4, 15, 3, 14, false },
        { 34, "Se", 78.971f, "78,971", NM, Sol, 4, 16, 3, 15, false },
        { 35, "Br", 79.904f, "79,904", HA, Liq, 4, 17, 3, 16, false },
        { 36, "Kr", 83.798f, "83,798", NG, Gas, 4, 18, 3, 17, false },

        // Period 5 (row 4)
        { 37, "Rb", 85.468f, "85,468", AM, Sol, 5, 1, 4, 0, false },
        { 38, "Sr", 87.62f, "87,62", AE, Sol, 5, 2, 4, 1, false },
        { 39, "Y", 88.906f, "88,906", TM, Sol, 5, 3, 4, 2, false },
        { 40, "Zr", 91.224f, "91,224", TM, Sol, 5, 4, 4, 3, false },
        { 41, "Nb", 92.906f, "92,906", TM, Sol, 5, 5, 4, 4, false },
        { 42, "Mo", 95.95f, "95,95", TM, Sol, 5, 6, 4, 5, false },
        { 43, "Tc", 98.0f, "[98]", TM, Sol, 5, 7, 4, 6, true },
        { 44, "Ru", 101.07f, "101,07", TM, Sol, 5, 8, 4, 7, false },
        { 45, "Rh", 102.91f, "102,91", TM, Sol, 5, 9, 4, 8, false },
        { 46, "Pd", 106.42f, "106,42", TM, Sol, 5, 10, 4, 9, false },
        { 47, "Ag", 107.87f, "107,87", TM, Sol, 5, 11, 4, 10, false },
        { 48, "Cd", 112.41f, "112,41", TM, Sol, 5, 12, 4, 11, false },
        { 49, "In", 114.82f, "114,82", OM, Sol, 5, 13, 4, 12, false },
        { 50, "Sn", 118.71f, "118,71", OM, Sol, 5, 14, 4, 13, false },
        { 51, "Sb", 121.76f, "121,76", SM, Sol, 5, 15, 4, 14, false },
        { 52, "Te", 127.60f, "127,60", SM, Sol, 5, 16, 4, 15, false },
        { 53, "I", 126.90f, "126,90", HA, Sol, 5, 17, 4, 16, false },
        { 54, "Xe", 131.29f, "131,29", NG, Gas, 5, 18, 4, 17, false },

        // Period 6 (row 5) + Lanthanides (row 7, cols 3..17)
        { 55, "Cs", 132.91f, "132,91", AM, Sol, 6, 1, 5, 0, false },
        { 56, "Ba", 137.33f, "137,33", AE, Sol, 6, 2, 5, 1, false },
        { 57, "La", 138.91f, "138,91", LA, Sol, 6, 0, 7, 3, false },
        { 58, "Ce", 140.12f, "140,12", LA, Sol, 6, 0, 7, 4, false },
        { 59, "Pr", 140.91f, "140,91", LA, Sol, 6, 0, 7, 5, false },
        { 60, "Nd", 144.24f, "144,24", LA, Sol, 6, 0, 7, 6, false },
        { 61, "Pm", 145.0f, "[145]", LA, Sol, 6, 0, 7, 7, true },
        { 62, "Sm", 150.36f, "150,36", LA, Sol, 6, 0, 7, 8, false },
        { 63, "Eu", 151.96f, "151,96", LA, Sol, 6, 0, 7, 9, false },
        { 64, "Gd", 157.25f, "157,25", LA, Sol, 6, 0, 7, 10, false },
        { 65, "Tb", 158.93f, "158,93", LA, Sol, 6, 0, 7, 11, false },
        { 66, "Dy", 162.50f, "162,50", LA, Sol, 6, 0, 7, 12, false },
        { 67, "Ho", 164.93f, "164,93", LA, Sol, 6, 0, 7, 13, false },
        { 68, "Er", 167.26f, "167,26", LA, Sol, 6, 0, 7, 14, false },
        { 69, "Tm", 168.93f, "168,93", LA, Sol, 6, 0, 7, 15, false },
        { 70, "Yb", 173.05f, "173,05", LA, Sol, 6, 0, 7, 16, false },
        { 71, "Lu", 174.97f, "174,97", LA, Sol, 6, 0, 7, 17, false },
        { 72, "Hf", 178.49f, "178,49", TM, Sol, 6, 4, 5, 3, false },
        { 73, "Ta", 180.95f, "180,95", TM, Sol, 6, 5, 5, 4, false },
        { 74, "W", 183.84f, "183,84", TM, Sol, 6, 6, 5, 5, false },
        { 75, "Re", 186.21f, "186,21", TM, Sol, 6, 7, 5, 6, false },
        { 76, "Os", 190.23f, "190,23", TM, Sol, 6, 8, 5, 7, false },
        { 77, "Ir", 192.22f, "192,22", TM, Sol, 6, 9, 5, 8, false },
        { 78, "Pt", 195.08f, "195,08", TM, Sol, 6, 10, 5, 9, false },
        { 79, "Au", 196.97f, "196,97", TM, Sol, 6, 11, 5, 10, false },
        { 80, "Hg", 200.59f, "200,59", TM, Liq, 6, 12, 5, 11, false },
        { 81, "Tl", 204.38f, "204,38", OM, Sol, 6, 13, 5, 12, false },
        { 82, "Pb", 207.2f, "207,2", OM, Sol, 6, 14, 5, 13, false },
        { 83, "Bi", 208.98f, "208,98", OM, Sol, 6, 15, 5, 14, false },
        { 84, "Po", 209.0f, "[209]", SM, Sol, 6, 16, 5, 15, true },
        { 85, "At", 210.0f, "[210]", HA, Sol, 6, 17, 5, 16, true },
        { 86, "Rn", 222.0f, "[222]", NG, Gas, 6, 18, 5, 17, true },

        // Period 7 (row 6) + Actinides (row 8, cols 3..17)
        { 87, "Fr", 223.0f, "[223]", AM, Sol, 7, 1, 6, 0, true },
        { 88, "Ra", 226.0f, "[226]", AE, Sol, 7, 2, 6, 1, true },
        { 89, "Ac", 227.0f, "[227]", AC, Sol, 7, 0, 8, 3, true },
        { 90, "Th", 232.04f, "232,04", AC, Sol, 7, 0, 8, 4, true },
        { 91, "Pa", 231.04f, "231,04", AC, Sol, 7, 0, 8, 5, true },
        { 92, "U", 238.03f, "238,03", AC, Sol, 7, 0, 8, 6, true },
        { 93, "Np", 237.0f, "[237]", AC, Sol, 7, 0, 8, 7, true },
        { 94, "Pu", 244.0f, "[244]", AC, Sol, 7, 0, 8, 8, true },
        { 95, "Am", 243.0f, "[243]", AC, Sol, 7, 0, 8, 9, true },
        { 96, "Cm", 247.0f, "[247]", AC, Sol, 7, 0, 8, 10, true },
        { 97, "Bk", 247.0f, "[247]", AC, Sol, 7, 0, 8, 11, true },
        { 98, "Cf", 251.0f, "[251]", AC, Sol, 7, 0, 8, 12, true },
        { 99, "Es", 252.0f, "[252]", AC, Sol, 7, 0, 8, 13, true },
        { 100, "Fm", 257.0f, "[257]", AC, Sol, 7, 0, 8, 14, true },
        { 101, "Md", 258.0f, "[258]", AC, Sol, 7, 0, 8, 15, true },
        { 102, "No", 259.0f, "[259]", AC, Sol, 7, 0, 8, 16, true },
        { 103, "Lr", 262.0f, "[262]", AC, Sol, 7, 0, 8, 17, true },
        { 104, "Rf", 267.0f, "[267]", TM, Sol, 7, 4, 6, 3, true },
        { 105, "Db", 268.0f, "[268]", TM, Sol, 7, 5, 6, 4, true },
        { 106, "Sg", 269.0f, "[269]", TM, Sol, 7, 6, 6, 5, true },
        { 107, "Bh", 270.0f, "[270]", TM, Sol, 7, 7, 6, 6, true },
        { 108, "Hs", 269.0f, "[269]", TM, Sol, 7, 8, 6, 7, true },
        { 109, "Mt", 278.0f, "[278]", TM, Sol, 7, 9, 6, 8, true },
        { 110, "Ds", 281.0f, "[281]", TM, Sol, 7, 10, 6, 9, true },
        { 111, "Rg", 281.0f, "[281]", TM, Sol, 7, 11, 6, 10, true },
        { 112, "Cn", 285.0f, "[285]", TM, Sol, 7, 12, 6, 11, true },
        { 113, "Nh", 286.0f, "[286]", OM, Sol, 7, 13, 6, 12, true },
        { 114, "Fl", 289.0f, "[289]", OM, Sol, 7, 14, 6, 13, true },
        { 115, "Mc", 288.0f, "[288]", OM, Sol, 7, 15, 6, 14, true },
        { 116, "Lv", 293.0f, "[293]", OM, Sol, 7, 16, 6, 15, true },
        { 117, "Ts", 294.0f, "[294]", HA, Sol, 7, 17, 6, 16, true },
        { 118, "Og", 294.0f, "[294]", NG, Sol, 7, 18, 6, 17, true }
    }};
} // namespace

const std::array<ElementInfo, kNumElements>& getAllElements() noexcept
{
    return kElements;
}

const ElementInfo& getElementByIndex (int zeroBasedIndex) noexcept
{
    const int clamped = juce::jlimit (0, kNumElements - 1, zeroBasedIndex);
    return kElements[static_cast<size_t> (clamped)];
}

const ElementInfo& getElementByAtomicNumber (int atomicNumber) noexcept
{
    const int clamped = juce::jlimit (1, kNumElements, atomicNumber);
    return getElementByIndex (clamped - 1);
}

int findElementAtGrid (int gridRow, int gridCol) noexcept
{
    for (int i = 0; i < kNumElements; ++i)
    {
        const auto& el = kElements[static_cast<size_t> (i)];
        if (el.gridRow == gridRow && el.gridCol == gridCol)
            return i;
    }
    return -1;
}

juce::Colour getCategoryColour (ElementCategory category) noexcept
{
    // Exact hex palette sampled directly from the user's reference Toda Matéria table
    switch (category)
    {
        case ElementCategory::NonMetal:        return juce::Colour (0xffa3d246); // Não metais
        case ElementCategory::AlkaliMetal:     return juce::Colour (0xfff3b202); // Metais alcalinos
        case ElementCategory::Semimetal:       return juce::Colour (0xff4db6ac); // Semimetais
        case ElementCategory::OtherMetal:      return juce::Colour (0xffa3c7d7); // Outros metais
        case ElementCategory::Lanthanide:      return juce::Colour (0xff92e3e4); // Lantanídeos
        case ElementCategory::NobleGas:        return juce::Colour (0xff3d9ee5); // Gases nobres
        case ElementCategory::AlkalineEarth:   return juce::Colour (0xffedd805); // Metais alcalino-terrosos
        case ElementCategory::Halogen:         return juce::Colour (0xff72cbeb); // Halogênios
        case ElementCategory::TransitionMetal: return juce::Colour (0xffef8e8d); // Metais de transição
        case ElementCategory::Actinide:        return juce::Colour (0xffdcadd7); // Actinídios
        case ElementCategory::Count:           break;
    }
    return juce::Colour (0xff3d9ee5);
}

juce::String getElementName (const ElementInfo& element, Language lang)
{
    return LocalizationManager::getInstance().getElementName (element.atomicNumber, lang);
}

juce::String getElementMassDisplay (const ElementInfo& element, Language lang)
{
    return LocalizationManager::getInstance().formatMassDisplay (element.massDisplay, lang);
}

juce::String getCategoryName (ElementCategory category, Language lang)
{
    return LocalizationManager::getInstance().getCategoryName (category, lang);
}

juce::String getPhaseName (PhysicalPhase phase, Language lang)
{
    return LocalizationManager::getInstance().getPhaseName (phase, lang);
}

juce::String getEngineDescription (const ElementSynthProfile& profile, Language lang)
{
    return LocalizationManager::getInstance().getEngineDescription (profile.engineDescriptionKey, lang);
}

juce::String getCategoryNamePt (ElementCategory category)
{
    return getCategoryName (category, Language::Portuguese);
}

juce::String getPhaseNamePt (PhysicalPhase phase)
{
    return getPhaseName (phase, Language::Portuguese);
}

ElementSynthProfile computeSynthProfile (const ElementInfo& el) noexcept
{
    ElementSynthProfile p;
    p.category = el.category;

    // 1. Normalized Atomic Number Z in [0, 1] -> drives electronic shell complexity & harmonic richness
    const float z = juce::jlimit (0.0f, 1.0f, static_cast<float> (el.atomicNumber - 1) / 117.0f);

    // 2. Log-normalized Atomic Mass A in [0, 1] -> drives physical inertia, envelope momentum & low-end weight
    const float minLogM = std::log (1.008f);
    const float maxLogM = std::log (294.0f);
    const float m = juce::jlimit (0.0f, 1.0f, (std::log ( std::max (1.008f, el.atomicMass) ) - minLogM) / (maxLogM - minLogM));
    const float mLin = juce::jlimit (0.0f, 1.0f, el.atomicMass / 294.0f);

    // 3. Neutron excess ratio -> subtle nuclear beating / detune instability on heavy elements
    const float neutronExcess = juce::jlimit (0.0f, 0.40f,
        (el.atomicMass - 2.0f * static_cast<float> (el.atomicNumber)) / std::max (1.0f, el.atomicMass));

    p.normZ = z;
    p.normMass = m;

    // 4. Category-defined synthesis architecture modulated continuously by Z and Mass
    switch (el.category)
    {
        case ElementCategory::NobleGas:
        {
            // Ethereal, airy, floating Ambient Breath Pad — all Noble Gases sound unmistakably gaseous & luminous
            p.engineDescriptionKey = "nobleGas";
            p.triSineMix        = 0.85f;
            p.sawMix            = 0.12f + 0.16f * z;
            p.pulseMix          = 0.0f;
            p.subOscMix         = 0.12f + 0.42f * mLin;
            p.shimmerOctaveMix  = 0.36f * (1.0f - 0.60f * m);
            p.breathNoiseMix    = 0.18f - 0.05f * mLin;
            p.wavefoldDrive     = 1.0f + 0.35f * z;
            p.unisonDetuneCents = 7.0f + 11.0f * z + 10.0f * neutronExcess;

            p.filterCutoffHz      = 4600.0f * std::pow (0.30f, m) + 450.0f;
            p.filterResonance     = 0.28f + 0.18f * z;
            p.filterEnvAmount     = 0.28f;
            p.filterBandpassBlend = 0.24f;

            p.attackSec    = 0.14f + 0.48f * m;
            p.decaySec     = 0.65f + 0.85f * m;
            p.sustainLevel = 0.86f;
            p.releaseSec   = 0.75f + 1.65f * m;

            p.lfoRateHz          = 2.6f - 1.7f * m;
            p.lfoPitchDepthCents = 4.5f + 5.5f * z;
            p.lfoFilterDepthOct  = 0.25f + 0.20f * z;
            p.chorusMix          = 0.48f;
            p.reverbMix          = 0.48f + 0.14f * m;
            p.reverbRoomSize     = 0.78f + 0.16f * m;
            p.reverbDamping      = 0.25f + 0.35f * m;
            break;
        }

        case ElementCategory::NonMetal:
        {
            // Organic Warm Analog / Wind-Reed Synth — gaseous non-metals (H, N, O) share airy breathiness with gases!
            const bool isGas = (el.phase == PhysicalPhase::Gas);
            p.engineDescriptionKey = isGas ? "nonMetalGas" : "nonMetalSolid";

            p.triSineMix        = isGas ? 0.72f : 0.52f;
            p.sawMix            = (isGas ? 0.18f : 0.34f) + 0.25f * z;
            p.pulseMix          = 0.20f + 0.15f * z;
            p.pulseWidth        = 0.44f - 0.12f * z;
            p.subOscMix         = 0.10f + 0.32f * m;
            p.shimmerOctaveMix  = isGas ? (0.20f * (1.0f - m)) : 0.06f;
            p.breathNoiseMix    = isGas ? 0.14f : 0.03f;
            p.wavefoldDrive     = 1.08f + 0.45f * z;
            p.unisonDetuneCents = 4.5f + 7.5f * z;

            p.filterCutoffHz      = (isGas ? 3900.0f : 2900.0f) * std::pow (0.42f, m) + 380.0f;
            p.filterResonance     = 0.22f + 0.22f * z;
            p.filterEnvAmount     = isGas ? 0.30f : 0.46f;
            p.filterBandpassBlend = isGas ? 0.16f : 0.04f;

            p.attackSec    = isGas ? (0.055f + 0.12f * m) : (0.014f + 0.055f * m);
            p.decaySec     = 0.35f + 0.45f * m;
            p.sustainLevel = isGas ? 0.82f : 0.68f;
            p.releaseSec   = (isGas ? 0.52f : 0.32f) + 0.55f * m;

            p.lfoRateHz          = 3.4f - 1.6f * m;
            p.lfoPitchDepthCents = 3.0f + 4.0f * z;
            p.lfoFilterDepthOct  = 0.14f + 0.14f * z;
            p.chorusMix          = 0.30f;
            p.reverbMix          = isGas ? 0.38f : 0.26f;
            p.reverbRoomSize     = 0.60f + 0.15f * m;
            p.reverbDamping      = 0.38f + 0.25f * m;
            break;
        }

        case ElementCategory::Halogen:
        {
            // Reactive Acid / Ionized Resonant Synth — biting filter resonance, electric sizzle, gaseous fizz on F/Cl
            const bool isGas = (el.phase == PhysicalPhase::Gas);
            const bool isLiq = (el.phase == PhysicalPhase::Liquid);
            p.engineDescriptionKey = "halogen";

            p.triSineMix        = 0.18f;
            p.sawMix            = 0.58f + 0.20f * z;
            p.pulseMix          = 0.48f;
            p.pulseWidth        = 0.22f + 0.14f * m;
            p.subOscMix         = 0.15f + 0.40f * mLin;
            p.shimmerOctaveMix  = 0.12f * (1.0f - 0.5f * m);
            p.breathNoiseMix    = isGas ? 0.13f : 0.05f;
            p.wavefoldDrive     = 1.65f + 1.35f * z;
            p.unisonDetuneCents = 6.0f + 11.0f * z + (isLiq ? 5.0f : 0.0f);

            p.filterCutoffHz      = 3500.0f * std::pow (0.34f, m) + 320.0f;
            p.filterResonance     = 0.60f + 0.22f * z;
            p.filterEnvAmount     = 0.78f;
            p.filterBandpassBlend = 0.12f;

            p.attackSec    = isGas ? 0.025f : (0.008f + 0.035f * m);
            p.decaySec     = 0.28f + 0.50f * m;
            p.sustainLevel = 0.62f + 0.15f * m;
            p.releaseSec   = 0.30f + 0.65f * m;

            p.lfoRateHz          = isLiq ? 4.8f : (4.2f - 2.0f * m);
            p.lfoPitchDepthCents = isLiq ? 9.0f : (4.0f + 5.0f * z);
            p.lfoFilterDepthOct  = 0.26f + 0.22f * z;
            p.chorusMix          = 0.34f;
            p.reverbMix          = 0.30f;
            p.reverbRoomSize     = 0.58f + 0.18f * m;
            p.reverbDamping      = 0.32f + 0.28f * m;
            break;
        }

        case ElementCategory::AlkaliMetal:
        {
            // Energetic Spark Pluck / Electric Keys — fast percussive transient with pitch chirp ("water reaction spark")
            p.engineDescriptionKey = "alkaliMetal";

            p.triSineMix          = 0.38f;
            p.sawMix              = 0.45f + 0.25f * z;
            p.pulseMix            = 0.38f;
            p.pulseWidth          = 0.35f;
            p.subOscMix           = 0.22f + 0.45f * m;
            p.fmModRatio          = 2.0f;
            p.fmStrikeIndex       = 1.6f + 1.4f * z;
            p.fmStrikeDecay       = 0.045f + 0.06f * m;
            p.pitchSparkSemitones = 9.0f * (1.0f - 0.35f * m);
            p.wavefoldDrive       = 1.35f + 0.85f * z;
            p.unisonDetuneCents   = 4.0f + 7.0f * z;

            p.filterCutoffHz  = 3400.0f * std::pow (0.36f, m) + 360.0f;
            p.filterResonance = 0.36f + 0.20f * z;
            p.filterEnvAmount = 0.85f;

            p.attackSec    = 0.0025f + 0.008f * m;
            p.decaySec     = 0.22f + 0.48f * m;
            p.sustainLevel = 0.26f + 0.20f * m;
            p.releaseSec   = 0.20f + 0.45f * m;

            p.lfoRateHz          = 4.5f - 2.0f * m;
            p.lfoPitchDepthCents = 2.5f + 3.5f * z;
            p.lfoFilterDepthOct  = 0.10f;
            p.chorusMix          = 0.28f;
            p.reverbMix          = 0.24f + 0.10f * m;
            p.reverbRoomSize     = 0.50f + 0.20f * m;
            p.reverbDamping      = 0.42f;
            break;
        }

        case ElementCategory::AlkalineEarth:
        {
            // Grounded Warm Brass-Poly & Sub-Harmonic Synth — solid structural foundation
            p.engineDescriptionKey = "alkalineEarth";

            p.triSineMix        = 0.30f;
            p.sawMix            = 0.68f;
            p.pulseMix          = 0.25f + 0.20f * z;
            p.pulseWidth        = 0.48f;
            p.subOscMix         = 0.40f + 0.38f * m;
            p.wavefoldDrive     = 1.25f + 0.65f * z;
            p.unisonDetuneCents = 6.5f + 8.0f * z;

            p.filterCutoffHz  = 2600.0f * std::pow (0.40f, m) + 300.0f;
            p.filterResonance = 0.28f + 0.18f * z;
            p.filterEnvAmount = 0.62f;

            p.attackSec    = 0.022f + 0.075f * m;
            p.decaySec     = 0.38f + 0.45f * m;
            p.sustainLevel = 0.76f;
            p.releaseSec   = 0.32f + 0.58f * m;

            p.lfoRateHz          = 3.0f - 1.4f * m;
            p.lfoPitchDepthCents = 3.5f + 3.5f * z;
            p.lfoFilterDepthOct  = 0.12f;
            p.chorusMix          = 0.32f;
            p.reverbMix          = 0.26f + 0.12f * m;
            p.reverbRoomSize     = 0.58f + 0.18f * m;
            p.reverbDamping      = 0.48f;
            break;
        }

        case ElementCategory::TransitionMetal:
        {
            // Lustrous Metallic FM Bell-Resonator — d-block column subtly selects harmonic/bell ratio!
            static constexpr std::array<float, 10> kDBlockRatios {{
                2.000f, // Group 3 (Sc, Y)        - hollow tubular metal
                2.500f, // Group 4 (Ti, Zr, Hf)   - bright structural chime
                3.000f, // Group 5 (V, Nb, Ta)    - clear twelfth bell
                3.500f, // Group 6 (Cr, Mo, W)    - hard steel overtone
                2.756f, // Group 7 (Mn, Tc, Re)   - classic chime bar mode
                3.142f, // Group 8 (Fe, Ru, Os)   - forged iron/anvil resonance
                4.000f, // Group 9 (Co, Rh, Ir)   - bright cobalt harmonic
                2.004f, // Group 10 (Ni, Pd, Pt)  - shimmering noble alloy
                3.002f, // Group 11 (Cu, Ag, Au)  - pure singing coinage bell
                1.996f  // Group 12 (Zn, Cd, Hg)  - beating liquid/amalgam metal
            }};
            const int dIdx = juce::jlimit (0, 9, el.group - 3);
            const bool isMercury = (el.phase == PhysicalPhase::Liquid);
            p.engineDescriptionKey = isMercury ? "transitionMetalLiquid" : "transitionMetalSolid";

            p.triSineMix        = 0.82f;
            p.sawMix            = 0.14f + 0.15f * z;
            p.subOscMix         = 0.16f + 0.36f * mLin;
            p.fmModRatio        = kDBlockRatios[static_cast<size_t> (dIdx)];
            p.fmSustainedIndex  = 0.42f + 1.05f * z;
            p.fmStrikeIndex     = 1.85f + 1.55f * (1.0f - 0.35f * m);
            p.fmStrikeDecay     = 0.08f + 0.24f * m;
            p.wavefoldDrive     = 1.12f + 0.40f * z;
            p.unisonDetuneCents = (isMercury ? 11.0f : 3.5f) + 6.0f * z + 8.0f * neutronExcess;

            p.filterCutoffHz  = 5200.0f * std::pow (0.38f, m) + 550.0f;
            p.filterResonance = 0.24f + 0.18f * z;
            p.filterEnvAmount = 0.55f;

            p.attackSec    = isMercury ? 0.035f : (0.003f + 0.014f * mLin);
            p.decaySec     = 0.45f + 1.35f * m;
            p.sustainLevel = 0.42f + 0.22f * m;
            p.releaseSec   = 0.50f + 1.45f * m;

            p.lfoRateHz          = isMercury ? 4.4f : (2.8f - 1.3f * m);
            p.lfoPitchDepthCents = isMercury ? 10.0f : (2.0f + 3.0f * z);
            p.lfoFilterDepthOct  = 0.12f;
            p.chorusMix          = isMercury ? 0.46f : 0.24f;
            p.reverbMix          = 0.34f + 0.14f * m;
            p.reverbRoomSize     = 0.65f + 0.22f * m;
            p.reverbDamping      = 0.30f + 0.30f * m;
            break;
        }

        case ElementCategory::Semimetal:
        {
            // Crystalline Wavefold / Semiconductor Digital-Hybrid Synth
            p.engineDescriptionKey = "semimetal";

            p.triSineMix        = 0.78f;
            p.sawMix            = 0.15f;
            p.pulseMix          = 0.22f;
            p.pulseWidth        = 0.28f;
            p.subOscMix         = 0.14f + 0.26f * m;
            p.shimmerOctaveMix  = 0.28f * (1.0f - 0.35f * m);
            p.fmModRatio        = 1.50f; // Crystalline fifth harmonic
            p.fmSustainedIndex  = 0.30f + 0.65f * z;
            p.fmStrikeIndex     = 0.90f;
            p.fmStrikeDecay     = 0.09f;
            p.wavefoldDrive     = 1.75f + 1.85f * z;
            p.unisonDetuneCents = 5.0f + 7.0f * z;

            p.filterCutoffHz      = 4400.0f * std::pow (0.42f, m) + 500.0f;
            p.filterResonance     = 0.38f + 0.20f * z;
            p.filterEnvAmount     = 0.52f;
            p.filterBandpassBlend = 0.15f;

            p.attackSec    = 0.008f + 0.030f * m;
            p.decaySec     = 0.36f + 0.55f * m;
            p.sustainLevel = 0.60f;
            p.releaseSec   = 0.42f + 0.68f * m;

            p.lfoRateHz          = 3.8f - 1.5f * m;
            p.lfoPitchDepthCents = 3.0f + 4.0f * z;
            p.lfoFilterDepthOct  = 0.22f;
            p.chorusMix          = 0.38f;
            p.reverbMix          = 0.35f;
            p.reverbRoomSize     = 0.66f;
            p.reverbDamping      = 0.30f;
            break;
        }

        case ElementCategory::OtherMetal:
        {
            // Malleable Warm Electric Tine / Tube-Saturated Post-Transition Metal (Al, Sn, Pb, Bi...)
            p.engineDescriptionKey = "otherMetal";

            p.triSineMix        = 0.74f;
            p.sawMix            = 0.24f + 0.18f * z;
            p.subOscMix         = 0.28f + 0.45f * mLin;
            p.fmModRatio        = 4.0f; // Soft electric piano tine harmonic
            p.fmSustainedIndex  = 0.18f + 0.35f * (1.0f - mLin);
            p.fmStrikeIndex     = 1.25f * (1.0f - 0.30f * mLin);
            p.fmStrikeDecay     = 0.06f + 0.08f * m;
            p.wavefoldDrive     = 1.30f + 0.75f * m;
            p.unisonDetuneCents = 5.5f + 8.5f * z;

            p.filterCutoffHz  = 2800.0f * std::pow (0.36f, m) + 280.0f;
            p.filterResonance = 0.22f + 0.16f * z;
            p.filterEnvAmount = 0.48f;

            p.attackSec    = 0.006f + 0.025f * mLin;
            p.decaySec     = 0.42f + 0.75f * m;
            p.sustainLevel = 0.56f + 0.20f * m;
            p.releaseSec   = 0.38f + 0.80f * m;

            p.lfoRateHz          = 2.6f - 1.1f * m;
            p.lfoPitchDepthCents = 5.0f + 5.0f * z; // Malleable pitch warmth
            p.lfoFilterDepthOct  = 0.14f;
            p.chorusMix          = 0.36f;
            p.reverbMix          = 0.28f + 0.12f * m;
            p.reverbRoomSize     = 0.58f + 0.20f * m;
            p.reverbDamping      = 0.48f + 0.22f * m;
            break;
        }

        case ElementCategory::Lanthanide:
        {
            // Luminous Rare-Earth Shimmer / Magnetic Detuned Ensemble Synth
            const float fBlockPos = juce::jlimit (0.0f, 1.0f, static_cast<float> (el.atomicNumber - 57) / 14.0f);
            p.engineDescriptionKey = "lanthanide";

            p.triSineMix        = 0.42f;
            p.sawMix            = 0.65f;
            p.pulseMix          = 0.28f;
            p.pulseWidth        = 0.38f + 0.15f * fBlockPos;
            p.subOscMix         = 0.22f + 0.25f * fBlockPos;
            p.shimmerOctaveMix  = 0.34f - 0.12f * fBlockPos;
            p.fmModRatio        = 2.0f;
            p.fmSustainedIndex  = 0.25f + 0.45f * fBlockPos;
            p.wavefoldDrive     = 1.25f + 0.55f * fBlockPos;
            p.unisonDetuneCents = 10.0f + 12.0f * fBlockPos;

            p.filterCutoffHz      = 3800.0f * std::pow (0.58f, fBlockPos) + 520.0f;
            p.filterResonance     = 0.35f + 0.22f * fBlockPos;
            p.filterEnvAmount     = 0.50f;
            p.filterBandpassBlend = 0.14f;

            p.attackSec    = 0.045f + 0.11f * fBlockPos;
            p.decaySec     = 0.50f + 0.40f * fBlockPos;
            p.sustainLevel = 0.82f;
            p.releaseSec   = 0.58f + 0.65f * fBlockPos;

            p.lfoRateHz          = 2.2f + 1.4f * fBlockPos;
            p.lfoPitchDepthCents = 5.5f + 4.5f * fBlockPos;
            p.lfoFilterDepthOct  = 0.28f;
            p.chorusMix          = 0.46f;
            p.reverbMix          = 0.42f;
            p.reverbRoomSize     = 0.74f;
            p.reverbDamping      = 0.28f;
            break;
        }

        case ElementCategory::Actinide:
        {
            // Heavy Radioactive Drone / Unstable Sub-Pulse Synth (Ac..Lr: U, Pu, Cf...)
            const float actPos = juce::jlimit (0.0f, 1.0f, static_cast<float> (el.atomicNumber - 89) / 14.0f);
            p.engineDescriptionKey = "actinide";

            p.triSineMix          = 0.36f;
            p.sawMix              = 0.56f + 0.20f * actPos;
            p.pulseMix            = 0.44f;
            p.pulseWidth          = 0.26f - 0.08f * actPos;
            p.subOscMix           = 0.58f + 0.28f * actPos;
            p.fmModRatio          = 1.414f; // Inharmonic nuclear tension ratio (sqrt(2))
            p.fmSustainedIndex    = 0.45f + 0.85f * actPos;
            p.wavefoldDrive       = 1.65f + 1.10f * actPos;
            p.unisonDetuneCents   = 11.0f + 14.0f * actPos + 12.0f * neutronExcess;
            p.geigerScintillation = 0.07f + 0.08f * actPos;

            p.filterCutoffHz  = 1650.0f * std::pow (0.52f, actPos) + 220.0f;
            p.filterResonance = 0.48f + 0.24f * actPos;
            p.filterEnvAmount = 0.42f;

            p.attackSec    = 0.085f + 0.24f * actPos;
            p.decaySec     = 0.75f + 0.75f * actPos;
            p.sustainLevel = 0.85f;
            p.releaseSec   = 0.85f + 1.35f * actPos;

            p.lfoRateHz          = 1.35f + 1.8f * actPos;
            p.lfoPitchDepthCents = 8.5f + 9.5f * actPos;
            p.lfoFilterDepthOct  = 0.34f;
            p.chorusMix          = 0.40f;
            p.reverbMix          = 0.46f;
            p.reverbRoomSize     = 0.84f;
            p.reverbDamping      = 0.54f;
            break;
        }

        case ElementCategory::Count:
            break;
    }

    // Subtle radioactive instability nuance for non-actinide radioactive isotopes (Tc, Pm, Po, At, Rn, Fr, Ra, Rf..Og)
    if (el.isRadioactive && el.category != ElementCategory::Actinide)
    {
        p.geigerScintillation = std::max (p.geigerScintillation, 0.035f);
        p.lfoPitchDepthCents += 2.5f;
    }

    // Normalized visual summary meters [0..1] for the UI Inspector
    p.harmonicRichness = juce::jlimit (0.08f, 1.0f,
        0.30f * z + 0.25f * p.sawMix + 0.20f * (p.fmSustainedIndex / 1.5f) + 0.25f * ((p.wavefoldDrive - 1.0f) / 2.5f));
    p.brightness = juce::jlimit (0.08f, 1.0f,
        (std::log (std::max (200.0f, p.filterCutoffHz)) - std::log (200.0f)) / (std::log (5800.0f) - std::log (200.0f)));
    p.bodyWeight = juce::jlimit (0.08f, 1.0f, 0.55f * m + 0.45f * p.subOscMix);
    p.sustainTail = juce::jlimit (0.08f, 1.0f,
        0.45f * p.sustainLevel + 0.55f * juce::jlimit (0.0f, 1.0f, p.releaseSec / 2.4f));

    return p;
}

} // namespace PeriodicSynth
