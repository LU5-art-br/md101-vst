#include "LocalizationManager.h"
#include "ElementData.h"

#if __has_include(<BinaryData.h>)
    #include <BinaryData.h>
    #define MD101_HAS_BINARY_DATA 1
#elif __has_include("BinaryData.h")
    #include "BinaryData.h"
    #define MD101_HAS_BINARY_DATA 1
#endif

namespace PeriodicSynth
{

LocalizationManager& LocalizationManager::getInstance() noexcept
{
    static LocalizationManager instance;
    return instance;
}

LocalizationManager::LocalizationManager()
{
    loadAllLocales();
}

void LocalizationManager::setLanguage (Language lang) noexcept
{
    currentLanguage = lang;
}

void LocalizationManager::loadAllLocales()
{
    juce::String enJson;
    juce::String ptJson;

#if defined(MD101_HAS_BINARY_DATA)
    if (BinaryData::enUS_jsonSize > 0)
        enJson = juce::String::createStringFromData (BinaryData::enUS_json, BinaryData::enUS_jsonSize);

    if (BinaryData::ptBR_jsonSize > 0)
        ptJson = juce::String::createStringFromData (BinaryData::ptBR_json, BinaryData::ptBR_jsonSize);
#endif

    // Fallback or external override check on disk
    auto checkFileOnDisk = [] (const juce::String& filename) -> juce::String
    {
        // Check relative to current working directory (e.g. repo root)
        juce::File directFile (filename);
        if (directFile.existsAsFile())
            return directFile.loadFileAsString();

        // Check relative to executable folder
        auto exeFile = juce::File::getSpecialLocation (juce::File::currentExecutableFile)
                           .getParentDirectory()
                           .getChildFile (filename);
        if (exeFile.existsAsFile())
            return exeFile.loadFileAsString();

        return {};
    };

    if (enJson.isEmpty())
        enJson = checkFileOnDisk ("assets/translations/en-US.json");

    if (ptJson.isEmpty())
        ptJson = checkFileOnDisk ("assets/translations/pt-BR.json");

    if (enJson.isNotEmpty())
        loadLocaleData (Language::English, enJson);

    if (ptJson.isNotEmpty())
        loadLocaleData (Language::Portuguese, ptJson);
}

void LocalizationManager::loadLocaleData (Language lang, const juce::String& jsonString)
{
    auto& target = (lang == Language::Portuguese) ? portugueseData : englishData;

    juce::var parsed = juce::JSON::parse (jsonString);
    auto* root = parsed.getDynamicObject();
    if (root == nullptr)
        return;

    // 1. Meta
    if (auto* meta = root->getProperty ("meta").getDynamicObject())
    {
        target.locale = meta->getProperty ("locale").toString();
        target.languageName = meta->getProperty ("name").toString();
        const auto sep = meta->getProperty ("decimalSeparator").toString();
        if (sep.isNotEmpty())
            target.decimalSeparator = sep;
    }

    // 2. Elements (Z = 1..118)
    if (auto* elems = root->getProperty ("elements").getDynamicObject())
    {
        for (const auto& prop : elems->getProperties())
        {
            const int z = prop.name.toString().getIntValue();
            if (z >= 1 && z <= 118)
            {
                if (auto* elObj = prop.value.getDynamicObject())
                    target.elementNames[static_cast<size_t> (z)] = elObj->getProperty ("name").toString();
                else if (prop.value.isString())
                    target.elementNames[static_cast<size_t> (z)] = prop.value.toString();
            }
        }
    }

    // 3. Categories
    if (auto* cats = root->getProperty ("categories").getDynamicObject())
    {
        for (const auto& prop : cats->getProperties())
            target.categories[prop.name.toString()] = prop.value.toString();
    }

    // 4. Phases
    if (auto* phs = root->getProperty ("phases").getDynamicObject())
    {
        for (const auto& prop : phs->getProperties())
            target.phases[prop.name.toString()] = prop.value.toString();
    }

    // 5. Engine Descriptions
    if (auto* engs = root->getProperty ("engineDescriptions").getDynamicObject())
    {
        for (const auto& prop : engs->getProperties())
            target.engineDescriptions[prop.name.toString()] = prop.value.toString();
    }

    // 6. Meters
    if (auto* mtrs = root->getProperty ("meters").getDynamicObject())
    {
        for (const auto& prop : mtrs->getProperties())
            target.meterLabels[prop.name.toString()] = prop.value.toString();
    }

    // 7. UI strings
    if (auto* ui = root->getProperty ("ui").getDynamicObject())
        flattenUiJson (target, {}, root->getProperty ("ui"));
}

void LocalizationManager::flattenUiJson (LocaleData& target, const juce::String& prefix, const juce::var& jsonVar)
{
    if (auto* dyn = jsonVar.getDynamicObject())
    {
        for (const auto& prop : dyn->getProperties())
        {
            const juce::String key = prefix.isEmpty() ? prop.name.toString()
                                                      : prefix + "." + prop.name.toString();
            if (prop.value.isObject())
                flattenUiJson (target, key, prop.value);
            else
                target.uiStrings[key] = prop.value.toString();
        }
    }
}

const LocalizationManager::LocaleData& LocalizationManager::getLocaleData (Language lang) const noexcept
{
    return (lang == Language::Portuguese) ? portugueseData : englishData;
}

juce::String LocalizationManager::getElementName (int atomicNumber, Language lang) const
{
    if (atomicNumber >= 1 && atomicNumber <= 118)
    {
        const auto& data = getLocaleData (lang);
        const auto& name = data.elementNames[static_cast<size_t> (atomicNumber)];
        if (name.isNotEmpty())
            return name;

        // Fallback to English
        const auto& enName = englishData.elementNames[static_cast<size_t> (atomicNumber)];
        if (enName.isNotEmpty())
            return enName;
    }
    return {};
}

juce::String LocalizationManager::categoryToKey (ElementCategory category)
{
    switch (category)
    {
        case ElementCategory::NonMetal:        return "nonMetal";
        case ElementCategory::AlkaliMetal:     return "alkaliMetal";
        case ElementCategory::Semimetal:       return "semimetal";
        case ElementCategory::OtherMetal:      return "otherMetal";
        case ElementCategory::Lanthanide:      return "lanthanide";
        case ElementCategory::NobleGas:        return "nobleGas";
        case ElementCategory::AlkalineEarth:   return "alkalineEarth";
        case ElementCategory::Halogen:         return "halogen";
        case ElementCategory::TransitionMetal: return "transitionMetal";
        case ElementCategory::Actinide:        return "actinide";
        case ElementCategory::Count:           break;
    }
    return {};
}

juce::String LocalizationManager::phaseToKey (PhysicalPhase phase)
{
    switch (phase)
    {
        case PhysicalPhase::Gas:    return "gas";
        case PhysicalPhase::Liquid: return "liquid";
        case PhysicalPhase::Solid:  return "solid";
    }
    return {};
}

juce::String LocalizationManager::getCategoryName (ElementCategory category, Language lang) const
{
    const auto key = categoryToKey (category);
    const auto& data = getLocaleData (lang);
    auto it = data.categories.find (key);
    if (it != data.categories.end() && it->second.isNotEmpty())
        return it->second;

    auto enIt = englishData.categories.find (key);
    if (enIt != englishData.categories.end())
        return enIt->second;

    return {};
}

juce::String LocalizationManager::getPhaseName (PhysicalPhase phase, Language lang) const
{
    const auto key = phaseToKey (phase);
    const auto& data = getLocaleData (lang);
    auto it = data.phases.find (key);
    if (it != data.phases.end() && it->second.isNotEmpty())
        return it->second;

    auto enIt = englishData.phases.find (key);
    if (enIt != englishData.phases.end())
        return enIt->second;

    return {};
}

juce::String LocalizationManager::getEngineDescription (const juce::String& engineKey, Language lang) const
{
    const auto& data = getLocaleData (lang);
    auto it = data.engineDescriptions.find (engineKey);
    if (it != data.engineDescriptions.end() && it->second.isNotEmpty())
        return it->second;

    auto enIt = englishData.engineDescriptions.find (engineKey);
    if (enIt != englishData.engineDescriptions.end())
        return enIt->second;

    return {};
}

juce::String LocalizationManager::getMeterLabel (const juce::String& meterKey, Language lang) const
{
    const auto& data = getLocaleData (lang);
    auto it = data.meterLabels.find (meterKey);
    if (it != data.meterLabels.end() && it->second.isNotEmpty())
        return it->second;

    auto enIt = englishData.meterLabels.find (meterKey);
    if (enIt != englishData.meterLabels.end())
        return enIt->second;

    return {};
}

juce::String LocalizationManager::getUiString (const juce::String& dotPath, Language lang, const juce::String& fallback) const
{
    const auto& data = getLocaleData (lang);
    auto it = data.uiStrings.find (dotPath);
    if (it != data.uiStrings.end() && it->second.isNotEmpty())
        return it->second;

    auto enIt = englishData.uiStrings.find (dotPath);
    if (enIt != englishData.uiStrings.end() && enIt->second.isNotEmpty())
        return enIt->second;

    return fallback;
}

juce::String LocalizationManager::formatMassDisplay (const juce::String& rawMassDisplay, Language lang) const
{
    const auto& data = getLocaleData (lang);
    if (data.decimalSeparator == ",")
        return rawMassDisplay.replaceCharacter ('.', ',');
    if (data.decimalSeparator == ".")
        return rawMassDisplay.replaceCharacter (',', '.');

    return rawMassDisplay;
}

} // namespace PeriodicSynth
