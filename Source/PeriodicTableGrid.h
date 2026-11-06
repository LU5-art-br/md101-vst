#pragma once

#include "ElementData.h"
#include "SynthEngine.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace PeriodicSynth
{

class PeriodicTableGrid : public juce::Component
{
public:
    PeriodicTableGrid();

    void setSelectedElement (int elementIndex);
    int getSelectedElement() const noexcept { return selectedElementIndex; }

    void updateAudioVisuals (const std::array<float, SynthEngine::kScopeSize>& scopeSamples,
                             float outputRms,
                             int activeVoices);

    std::function<void (int elementIndex)> onElementSelected;

    void paint (juce::Graphics& g) override;
    void resized() override;

    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;

private:
    struct LayoutMetrics
    {
        juce::Rectangle<float> bounds;
        float gridLeft = 0.0f;
        float gridTop = 0.0f;
        float cellW = 50.0f;
        float cellH = 40.0f;
        float gapX = 3.0f;
        float gapY = 3.0f;
        float fBlockGapY = 10.0f;
        juce::Rectangle<float> inspectorBounds;
        juce::Rectangle<float> legendBounds;
    };

    LayoutMetrics computeMetrics() const noexcept;
    juce::Rectangle<float> getCellRect (const LayoutMetrics& m, int gridRow, int gridCol) const noexcept;
    juce::Rectangle<float> getLegendItemRect (const LayoutMetrics& m, int categoryIdx) const noexcept;
    int hitTestElement (const LayoutMetrics& m, juce::Point<float> pt) const noexcept;
    int hitTestLegendCategory (const LayoutMetrics& m, juce::Point<float> pt) const noexcept;

    void drawHeadersAndConnectors (juce::Graphics& g, const LayoutMetrics& m) const;
    void drawElementCell (juce::Graphics& g, const LayoutMetrics& m, int elementIdx) const;
    void drawInspectorHud (juce::Graphics& g, const LayoutMetrics& m) const;
    void drawLegend (juce::Graphics& g, const LayoutMetrics& m) const;

    int selectedElementIndex = 0; // Default: 1 H (Hidrogênio)
    int hoveredElementIndex = -1;
    int hoveredCategoryIndex = -1;

    std::array<float, SynthEngine::kScopeSize> scopeData {};
    float currentRms = 0.0f;
    int currentVoices = 0;
    float animPhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PeriodicTableGrid)
};

} // namespace PeriodicSynth
