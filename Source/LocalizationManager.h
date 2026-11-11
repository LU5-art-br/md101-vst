#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <unordered_map>

namespace PeriodicSynth
{

enum class Language : int
{
    English = 0,
    Portuguese = 1
};

enum class ElementCategory : int;
enum class PhysicalPhase : int;

/**
 * LocalizationManager
 * Centralized, decoupled internationalization service for Md-101.
 * Loads translations from embedded BinaryData JSON resources or external files,
 * caching strings in memory for O(1) retrieval during UI rendering.
 */
class LocalizationManager
{
public:
    static LocalizationManager& getInstance() noexcept;

    void setLanguage (Language lang) noexcept;
    Language getLanguage() const noexcept { return currentLanguage; }

    juce::String getElementName (int atomicNumber, Language lang) const;
    juce::String getElementName (int atomicNumber) const { return getElementName (atomicNumber, currentLanguage); }

    juce::String getCategoryName (ElementCategory category, Language lang) const;
    juce::String getCategoryName (ElementCategory category) const { return getCategoryName (category, currentLanguage); }

    juce::String getPhaseName (PhysicalPhase phase, Language lang) const;
    juce::String getPhaseName (PhysicalPhase phase) const { return getPhaseName (phase, currentLanguage); }

    juce::String getEngineDescription (const juce::String& engineKey, Language lang) const;
    juce::String getEngineDescription (const juce::String& engineKey) const { return getEngineDescription (engineKey, currentLanguage); }

    juce::String getMeterLabel (const juce::String& meterKey, Language lang) const;
    juce::String getMeterLabel (const juce::String& meterKey) const { return getMeterLabel (meterKey, currentLanguage); }

    juce::String getUiString (const juce::String& dotPath, Language lang, const juce::String& fallback = {}) const;
    juce::String getUiString (const juce::String& dotPath, const juce::String& fallback = {}) const
    {
        return getUiString (dotPath, currentLanguage, fallback);
    }

    juce::String formatMassDisplay (const juce::String& rawMassDisplay, Language lang) const;
    juce::String formatMassDisplay (const juce::String& rawMassDisplay) const
    {
        return formatMassDisplay (rawMassDisplay, currentLanguage);
    }

    static juce::String categoryToKey (ElementCategory category);
    static juce::String phaseToKey (PhysicalPhase phase);

private:
    LocalizationManager();
    ~LocalizationManager() = default;

    struct LocaleData
    {
        juce::String locale;
        juce::String languageName;
        juce::String decimalSeparator = ".";
        std::array<juce::String, 119> elementNames; // 1-based (Z = 1..118)
        std::unordered_map<juce::String, juce::String> categories;
        std::unordered_map<juce::String, juce::String> phases;
        std::unordered_map<juce::String, juce::String> engineDescriptions;
        std::unordered_map<juce::String, juce::String> meterLabels;
        std::unordered_map<juce::String, juce::String> uiStrings;
        std::unordered_map<juce::String, juce::String> juceStrings;
    };

    void loadAllLocales();
    void loadLocaleData (Language lang, const juce::String& jsonString);
    void flattenUiJson (LocaleData& target, const juce::String& prefix, const juce::var& jsonVar);
    void applyJuceMappings (Language lang);
    const LocaleData& getLocaleData (Language lang) const noexcept;

    Language currentLanguage = Language::English;
    LocaleData englishData;
    LocaleData portugueseData;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LocalizationManager)
};

} // namespace PeriodicSynth
