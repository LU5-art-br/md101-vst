#include "PeriodicTableGrid.h"
#include <cmath>

namespace PeriodicSynth
{

PeriodicTableGrid::PeriodicTableGrid()
{
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

void PeriodicTableGrid::setSelectedElement (int elementIndex)
{
    const int clamped = juce::jlimit (0, kNumElements - 1, elementIndex);
    if (selectedElementIndex != clamped)
    {
        selectedElementIndex = clamped;
        repaint();
    }
}

void PeriodicTableGrid::setLanguage (Language lang) noexcept
{
    if (currentLanguage != lang)
    {
        currentLanguage = lang;
        repaint();
    }
}

void PeriodicTableGrid::updateAudioVisuals (const std::array<float, SynthEngine::kScopeSize>& scopeSamples,
                                            float outputRms,
                                            int activeVoices)
{
    scopeData = scopeSamples;
    currentRms = outputRms;
    currentVoices = activeVoices;
    animPhase += 0.08f;
    if (animPhase > juce::MathConstants<float>::twoPi)
        animPhase -= juce::MathConstants<float>::twoPi;
    repaint();
}

PeriodicTableGrid::LayoutMetrics PeriodicTableGrid::computeMetrics() const noexcept
{
    LayoutMetrics m;
    m.bounds = getLocalBounds().toFloat().reduced (10.0f, 4.0f);

    const float periodColW = 22.0f;
    const float groupHeaderH = 18.0f;
    const float legendH = 44.0f;
    const float legendGapTop = 8.0f;

    m.gapX = 3.0f;
    m.gapY = 3.0f;
    m.fBlockGapY = 9.0f;

    m.gridLeft = m.bounds.getX() + periodColW;
    m.gridTop  = m.bounds.getY() + groupHeaderH;

    const float availW = std::max (200.0f, m.bounds.getWidth() - periodColW);
    const float availH = std::max (180.0f, m.bounds.getHeight() - groupHeaderH - legendGapTop - legendH);

    m.cellW = (availW - 17.0f * m.gapX) / 18.0f;
    m.cellH = (availH - 8.0f * m.gapY - m.fBlockGapY) / 9.0f;

    const auto c2TopLeft  = getCellRect (m, 0, 2);
    const auto c11BotRight = getCellRect (m, 2, 11);
    m.inspectorBounds = juce::Rectangle<float> (c2TopLeft.getX() + 4.0f,
                                                c2TopLeft.getY() + 1.0f,
                                                (c11BotRight.getRight() - c2TopLeft.getX()) - 8.0f,
                                                (c11BotRight.getBottom() - c2TopLeft.getY()) - 4.0f);

    const float tableBottom = getCellRect (m, 8, 3).getBottom();
    m.legendBounds = juce::Rectangle<float> (m.gridLeft,
                                             tableBottom + legendGapTop,
                                             availW,
                                             legendH);
    return m;
}

juce::Rectangle<float> PeriodicTableGrid::getCellRect (const LayoutMetrics& m, int gridRow, int gridCol) const noexcept
{
    const float x = m.gridLeft + static_cast<float> (gridCol) * (m.cellW + m.gapX);
    float y = m.gridTop + static_cast<float> (gridRow) * (m.cellH + m.gapY);
    if (gridRow >= 7)
        y += m.fBlockGapY;
    return { x, y, m.cellW, m.cellH };
}

juce::Rectangle<float> PeriodicTableGrid::getLegendItemRect (const LayoutMetrics& m, int categoryIdx) const noexcept
{
    // 5 columns x 2 rows matching the attached Toda Matéria legend order
    const int col = categoryIdx % 5;
    const int row = categoryIdx / 5;
    const float itemW = m.legendBounds.getWidth() / 5.0f;
    const float itemH = m.legendBounds.getHeight() / 2.0f;
    return { m.legendBounds.getX() + static_cast<float> (col) * itemW + 3.0f,
             m.legendBounds.getY() + static_cast<float> (row) * itemH + 1.5f,
             itemW - 6.0f,
             itemH - 3.0f };
}

int PeriodicTableGrid::hitTestElement (const LayoutMetrics& m, juce::Point<float> pt) const noexcept
{
    const auto& all = getAllElements();
    for (int i = 0; i < kNumElements; ++i)
    {
        const auto& el = all[static_cast<size_t> (i)];
        if (getCellRect (m, el.gridRow, el.gridCol).contains (pt))
            return i;
    }
    return -1;
}

int PeriodicTableGrid::hitTestLegendCategory (const LayoutMetrics& m, juce::Point<float> pt) const noexcept
{
    if (! m.legendBounds.contains (pt))
        return -1;

    for (int c = 0; c < static_cast<int> (ElementCategory::Count); ++c)
    {
        if (getLegendItemRect (m, c).contains (pt))
            return c;
    }
    return -1;
}

void PeriodicTableGrid::paint (juce::Graphics& g)
{
    const auto m = computeMetrics();

    drawHeadersAndConnectors (g, m);
    drawInspectorHud (g, m);

    // First pass: non-selected, non-hovered elements
    for (int i = 0; i < kNumElements; ++i)
    {
        if (i != selectedElementIndex && i != hoveredElementIndex)
            drawElementCell (g, m, i);
    }

    // Second pass: hovered and selected on top so their glow/halo is never clipped by neighbors
    if (hoveredElementIndex >= 0 && hoveredElementIndex != selectedElementIndex)
        drawElementCell (g, m, hoveredElementIndex);

    if (selectedElementIndex >= 0 && selectedElementIndex < kNumElements)
        drawElementCell (g, m, selectedElementIndex);

    drawLegend (g, m);
}

void PeriodicTableGrid::drawHeadersAndConnectors (juce::Graphics& g, const LayoutMetrics& m) const
{
    // Group numbers 1..18 along top
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    for (int col = 0; col < 18; ++col)
    {
        const auto cellTop = getCellRect (m, 0, col);
        const juce::Rectangle<float> hdr (cellTop.getX(), m.bounds.getY(), m.cellW, 15.0f);
        g.setColour (juce::Colour (0xff6d7a92));
        g.drawText (juce::String (col + 1), hdr, juce::Justification::centred, false);
    }

    // Period numbers 1..7 along left
    for (int row = 0; row < 7; ++row)
    {
        const auto cellLeft = getCellRect (m, row, 0);
        const juce::Rectangle<float> pHdr (m.bounds.getX(), cellLeft.getY(), 19.0f, m.cellH);
        g.setColour (juce::Colour (0xff6d7a92));
        g.drawText (juce::String (row + 1), pHdr, juce::Justification::centred, false);
    }

    // Placeholder boxes (57-71 at row 5 col 2, and 89-103 at row 6 col 2) + dedicated connector lane
    const auto lanPlaceholder = getCellRect (m, 5, 2).withTrimmedRight (6.0f);
    const auto actPlaceholder = getCellRect (m, 6, 2).withTrimmedRight (11.0f);
    const auto laCell         = getCellRect (m, 7, 3);
    const auto acCell         = getCellRect (m, 8, 3);

    const auto lanCol = getCategoryColour (ElementCategory::Lanthanide);
    const auto actCol = getCategoryColour (ElementCategory::Actinide);

    // 57-71 placeholder box
    g.setColour (lanCol.withAlpha (0.13f));
    g.fillRoundedRectangle (lanPlaceholder, 4.0f);
    g.setColour (lanCol.withAlpha (0.58f));
    g.drawRoundedRectangle (lanPlaceholder.reduced (0.5f), 4.0f, 1.2f);
    g.setColour (lanCol.brighter (0.25f));
    g.setFont (juce::FontOptions (std::min (10.5f, m.cellH * 0.25f), juce::Font::bold));
    g.drawText ("57-71", lanPlaceholder, juce::Justification::centred, false);

    // 89-103 placeholder box
    g.setColour (actCol.withAlpha (0.13f));
    g.fillRoundedRectangle (actPlaceholder, 4.0f);
    g.setColour (actCol.withAlpha (0.58f));
    g.drawRoundedRectangle (actPlaceholder.reduced (0.5f), 4.0f, 1.2f);
    g.setColour (actCol.brighter (0.25f));
    g.drawText ("89-103", actPlaceholder, juce::Justification::centred, false);

    // Non-overlapping connector paths in dedicated right-hand lane
    const float xLanLine = getCellRect (m, 5, 2).getRight() - 3.2f;
    const float xActLine = actPlaceholder.getCentreX();

    juce::Path pLan;
    pLan.startNewSubPath (lanPlaceholder.getRight(), lanPlaceholder.getCentreY());
    pLan.lineTo (xLanLine, lanPlaceholder.getCentreY());
    pLan.lineTo (xLanLine, laCell.getCentreY());
    pLan.lineTo (laCell.getX() - 2.5f, laCell.getCentreY());
    g.setColour (lanCol.withAlpha (0.82f));
    g.strokePath (pLan, juce::PathStrokeType (1.5f));

    // Small arrowhead at La
    juce::Path arrowLan;
    arrowLan.addTriangle (laCell.getX() - 1.5f, laCell.getCentreY(),
                          laCell.getX() - 6.2f, laCell.getCentreY() - 3.0f,
                          laCell.getX() - 6.2f, laCell.getCentreY() + 3.0f);
    g.fillPath (arrowLan);

    juce::Path pAct;
    pAct.startNewSubPath (xActLine, actPlaceholder.getBottom());
    pAct.lineTo (xActLine, acCell.getCentreY());
    pAct.lineTo (acCell.getX() - 2.5f, acCell.getCentreY());
    g.setColour (actCol.withAlpha (0.82f));
    g.strokePath (pAct, juce::PathStrokeType (1.5f));

    juce::Path arrowAct;
    arrowAct.addTriangle (acCell.getX() - 1.5f, acCell.getCentreY(),
                          acCell.getX() - 6.2f, acCell.getCentreY() - 3.0f,
                          acCell.getX() - 6.2f, acCell.getCentreY() + 3.0f);
    g.fillPath (arrowAct);
}

void PeriodicTableGrid::drawElementCell (juce::Graphics& g, const LayoutMetrics& m, int elementIdx) const
{
    const auto& el = getElementByIndex (elementIdx);
    auto rect = getCellRect (m, el.gridRow, el.gridCol);

    const bool isSelected = (elementIdx == selectedElementIndex);
    const bool isHovered  = (elementIdx == hoveredElementIndex);
    const bool categoryFilteredOut = (hoveredCategoryIndex >= 0
                                      && static_cast<int> (el.category) != hoveredCategoryIndex
                                      && ! isSelected && ! isHovered);
    const bool categoryHighlighted = (hoveredCategoryIndex >= 0
                                      && static_cast<int> (el.category) == hoveredCategoryIndex);

    const juce::Colour baseCol = getCategoryColour (el.category);
    const float dimAlpha = categoryFilteredOut ? 0.28f : 1.0f;

    if (isSelected)
        rect = rect.expanded (1.6f, 1.4f);
    else if (isHovered)
        rect = rect.expanded (1.2f, 1.0f);

    // 1. Outer Neon Glow / Aura to stand out against the dark studio theme
    if (isSelected)
    {
        const float pulse = 0.5f + 0.5f * std::sin (animPhase) + juce::jlimit (0.0f, 0.8f, currentRms * 2.5f);
        g.setColour (juce::Colours::white.withAlpha (0.22f + 0.10f * pulse));
        g.fillRoundedRectangle (rect.expanded (7.5f), 8.5f);
        g.setColour (baseCol.brighter (0.45f).withAlpha (0.55f));
        g.fillRoundedRectangle (rect.expanded (4.2f), 6.5f);
    }
    else if (isHovered || categoryHighlighted)
    {
        g.setColour (baseCol.brighter (0.25f).withAlpha (isHovered ? 0.34f : 0.22f));
        g.fillRoundedRectangle (rect.expanded (4.0f), 6.5f);
    }
    else if (! categoryFilteredOut)
    {
        // Subtle ambient bloom under every element tile
        g.setColour (baseCol.withAlpha (0.11f));
        g.fillRoundedRectangle (rect.expanded (1.4f), 5.0f);
    }

    // 2. Main Tile Satin Gradient Fill (faithful to reference category color)
    juce::Colour topFill = baseCol.brighter (isHovered ? 0.22f : (isSelected ? 0.16f : 0.05f)).withMultipliedAlpha (dimAlpha);
    juce::Colour botFill = baseCol.darker   (isHovered ? 0.04f : 0.18f).withMultipliedAlpha (dimAlpha);

    juce::ColourGradient grad (topFill, rect.getX(), rect.getY(),
                               botFill, rect.getX(), rect.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (rect, 4.2f);

    // Top specular glass highlight
    g.setColour (juce::Colours::white.withAlpha ((isSelected || isHovered) ? 0.34f : (0.18f * dimAlpha)));
    g.fillRoundedRectangle (rect.withHeight (rect.getHeight() * 0.35f).reduced (1.0f, 0.8f), 3.5f);

    // 3. Borders: Distinct Selection & Hover Indicators
    if (isSelected)
    {
        // High-contrast double ring: dark inner separator + bright white outer frame
        g.setColour (juce::Colour (0xff0a0e16));
        g.drawRoundedRectangle (rect.reduced (1.6f), 3.5f, 1.5f);
        g.setColour (juce::Colours::white);
        g.drawRoundedRectangle (rect.expanded (0.4f), 4.5f, 2.3f);

        // Active LED indicator dot in top-right corner
        const float dotR = 5.2f;
        const juce::Rectangle<float> dotRect (rect.getRight() - dotR - 3.5f, rect.getY() + 3.2f, dotR, dotR);
        g.setColour (juce::Colour (0xff0a0e16));
        g.fillEllipse (dotRect.expanded (1.2f));
        g.setColour (juce::Colours::white);
        g.fillEllipse (dotRect);
    }
    else if (isHovered)
    {
        g.setColour (juce::Colours::white.withAlpha (0.92f));
        g.drawRoundedRectangle (rect, 4.2f, 1.8f);
    }
    else
    {
        g.setColour (baseCol.brighter (0.28f).withAlpha (0.45f * dimAlpha));
        g.drawRoundedRectangle (rect.reduced (0.4f), 4.0f, 0.9f);
    }

    // 4. Crisp Element Typography (Atomic Number, Symbol, Atomic Mass)
    const juce::Colour inkMain = juce::Colour (0xff090d14).withMultipliedAlpha (dimAlpha);
    const juce::Colour inkSub  = juce::Colour (0xff141c28).withMultipliedAlpha (0.82f * dimAlpha);

    const float numFontH  = juce::jlimit (8.0f, 10.5f, rect.getHeight() * 0.22f);
    const float symFontH  = juce::jlimit (12.0f, 18.0f, rect.getHeight() * 0.40f);
    const float massFontH = juce::jlimit (7.2f, 9.2f, rect.getHeight() * 0.19f);

    // Atomic Number (top-left)
    g.setColour (inkMain);
    g.setFont (juce::FontOptions (numFontH, juce::Font::bold));
    g.drawText (juce::String (el.atomicNumber),
                rect.withTrimmedLeft (3.5f).withHeight (numFontH + 2.0f).translated (0.0f, 1.5f),
                juce::Justification::centredLeft, false);

    // Chemical Symbol (center)
    g.setFont (juce::FontOptions (symFontH, juce::Font::bold));
    g.drawText (juce::String (el.symbol),
                rect.withY (rect.getY() + rect.getHeight() * 0.22f).withHeight (rect.getHeight() * 0.48f),
                juce::Justification::centred, false);

    // Atomic Mass (bottom)
    if (rect.getHeight() >= 34.0f)
    {
        g.setColour (inkSub);
        g.setFont (juce::FontOptions (massFontH, juce::Font::plain));
        g.drawText (getElementMassDisplay (el, currentLanguage),
                    rect.withTop (rect.getBottom() - massFontH - 2.5f).reduced (1.5f, 0.0f),
                    juce::Justification::centred, false);
    }
}

void PeriodicTableGrid::drawInspectorHud (juce::Graphics& g, const LayoutMetrics& m) const
{
    const auto hud = m.inspectorBounds;
    if (hud.getWidth() < 180.0f || hud.getHeight() < 70.0f)
        return;

    const auto& activeEl = getElementByIndex (selectedElementIndex);
    const auto profile   = computeSynthProfile (activeEl);
    const auto accent    = getCategoryColour (activeEl.category);

    // Dark glass panel background with subtle category glow border
    juce::ColourGradient panelGrad (juce::Colour (0xff141a26), hud.getX(), hud.getY(),
                                    juce::Colour (0xff0e121b), hud.getX(), hud.getBottom(), false);
    g.setGradientFill (panelGrad);
    g.fillRoundedRectangle (hud, 8.0f);

    g.setColour (accent.withAlpha (0.40f));
    g.drawRoundedRectangle (hud.reduced (0.6f), 8.0f, 1.3f);

    auto inner = hud.reduced (8.0f, 7.0f);

    // --- Column 1: Zoomed Element Preset Card (inspired by reference PDF callout) ---
    const float cardW = std::min (126.0f, inner.getWidth() * 0.24f);
    const auto cardRect = inner.removeFromLeft (cardW);

    juce::ColourGradient cardGrad (accent.withAlpha (0.24f), cardRect.getX(), cardRect.getY(),
                                   accent.withAlpha (0.08f), cardRect.getX(), cardRect.getBottom(), false);
    g.setGradientFill (cardGrad);
    g.fillRoundedRectangle (cardRect, 6.5f);
    g.setColour (accent.withAlpha (0.85f));
    g.drawRoundedRectangle (cardRect.reduced (0.5f), 6.5f, 1.6f);

    // Atomic number + Phase badge inside card
    g.setColour (accent.brighter (0.35f));
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("Z = " + juce::String (activeEl.atomicNumber),
                cardRect.reduced (7.0f, 5.0f).withHeight (15.0f),
                juce::Justification::centredLeft, false);

    g.setColour (juce::Colour (0xffb8c4d9));
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    g.drawText (getPhaseName (activeEl.phase, currentLanguage).toUpperCase(),
                cardRect.reduced (7.0f, 5.0f).withHeight (15.0f),
                juce::Justification::centredRight, false);

    // Large Symbol
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (33.0f, juce::Font::bold));
    g.drawText (juce::String (activeEl.symbol),
                cardRect.withY (cardRect.getY() + 20.0f).withHeight (cardRect.getHeight() * 0.42f),
                juce::Justification::centred, false);

    // Full Element Name
    g.setColour (accent.brighter (0.30f));
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawText (getElementName (activeEl, currentLanguage),
                cardRect.withTop (cardRect.getBottom() - 36.0f).withHeight (17.0f).reduced (4.0f, 0.0f),
                juce::Justification::centred, true);

    // Full Atomic Mass
    g.setColour (juce::Colour (0xff9aa7bd));
    g.setFont (juce::FontOptions (10.5f, juce::Font::plain));
    g.drawText (getElementMassDisplay (activeEl, currentLanguage) + " u",
                cardRect.withTop (cardRect.getBottom() - 18.0f).withHeight (14.0f),
                juce::Justification::centred, false);

    inner.removeFromLeft (10.0f);

    // --- Column 3 (Right): Live Stereo Oscilloscope ---
    const float scopeW = std::min (185.0f, inner.getWidth() * 0.38f);
    const auto scopeRect = inner.removeFromRight (scopeW);
    inner.removeFromRight (10.0f);

    g.setColour (juce::Colour (0xff090c12));
    g.fillRoundedRectangle (scopeRect, 6.0f);
    g.setColour (juce::Colour (0xff232c3d));
    g.drawRoundedRectangle (scopeRect, 6.0f, 1.0f);

    // Subtle oscilloscope grid lines
    g.setColour (juce::Colour (0xff161d2a));
    g.drawHorizontalLine (static_cast<int> (scopeRect.getCentreY()), scopeRect.getX() + 4.0f, scopeRect.getRight() - 4.0f);
    g.drawVerticalLine   (static_cast<int> (scopeRect.getCentreX()), scopeRect.getY() + 4.0f, scopeRect.getBottom() - 4.0f);

    g.setColour (juce::Colour (0xff7e8ca6));
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    auto& loc = LocalizationManager::getInstance();
    const auto scopeTitle = currentVoices > 0
        ? loc.getUiString ("inspector.oscilloscopeLive", currentLanguage, "OSCILLOSCOPE • LIVE")
        : loc.getUiString ("inspector.harmonicSignature", currentLanguage, "HARMONIC SIGNATURE");
    g.drawText (scopeTitle,
                scopeRect.reduced (6.0f, 4.0f).withHeight (12.0f),
                juce::Justification::centredLeft, false);

    // Waveform path (live audio if voices playing, or characteristic preview wave of preset)
    const auto waveArea = scopeRect.reduced (6.0f, 18.0f).translated (0.0f, 4.0f);
    juce::Path wavePath;
    const int numPts = 128;

    for (int i = 0; i < numPts; ++i)
    {
        const float t = static_cast<float> (i) / static_cast<float> (numPts - 1);
        const float x = waveArea.getX() + t * waveArea.getWidth();
        float sampleVal = 0.0f;

        if (currentVoices > 0 || currentRms > 0.003f)
        {
            const int srcIdx = (i * SynthEngine::kScopeSize) / numPts;
            sampleVal = juce::jlimit (-1.0f, 1.0f, scopeData[static_cast<size_t> (srcIdx)] * 1.65f);
        }
        else
        {
            // Characteristic static/gentle preview of this element's timbre
            const float ph = t * juce::MathConstants<float>::twoPi * 2.0f + animPhase * 0.35f;
            const float fm = profile.fmSustainedIndex * std::sin (ph * profile.fmModRatio);
            sampleVal = profile.triSineMix * std::sin (ph + fm)
                      + profile.sawMix * (2.0f * std::fmod (t * 2.0f, 1.0f) - 1.0f) * 0.6f
                      + profile.subOscMix * std::sin (ph * 0.5f) * 0.5f;
            sampleVal = std::tanh (sampleVal * profile.wavefoldDrive * 0.7f) * 0.65f;
        }

        const float y = waveArea.getCentreY() - sampleVal * (waveArea.getHeight() * 0.46f);
        if (i == 0)
            wavePath.startNewSubPath (x, y);
        else
            wavePath.lineTo (x, y);
    }

    g.setColour (accent.withAlpha (0.28f));
    g.strokePath (wavePath, juce::PathStrokeType (3.8f));
    g.setColour (accent.brighter (0.35f));
    g.strokePath (wavePath, juce::PathStrokeType (1.6f));

    // --- Column 2 (Center): Chemical Family & Synth Variables Breakdown ---
    auto mid = inner;

    // Category badge + Engine description
    g.setColour (accent);
    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
    g.drawText (getCategoryName (activeEl.category, currentLanguage).toUpperCase(),
                mid.removeFromTop (16.0f), juce::Justification::centredLeft, true);

    g.setColour (juce::Colour (0xffc5d0e3));
    g.setFont (juce::FontOptions (10.5f, juce::Font::plain));
    g.drawText (getEngineDescription (profile, currentLanguage),
                mid.removeFromTop (15.0f), juce::Justification::centredLeft, true);

    mid.removeFromTop (4.0f);

    // 4 physical-to-sonic parameter meters
    struct MeterRow { juce::String label; float normVal; };
    const std::array<MeterRow, 4> meters {{
        { loc.getMeterLabel ("atomicNumber", currentLanguage),       profile.harmonicRichness },
        { loc.getMeterLabel ("atomicMass", currentLanguage),         profile.bodyWeight },
        { loc.getMeterLabel ("spectralBrightness", currentLanguage),  profile.brightness },
        { loc.getMeterLabel ("resonanceRelease", currentLanguage),    profile.sustainTail }
    }};

    const float rowH = std::min (16.5f, (mid.getHeight() - 18.0f) / 4.0f);
    for (const auto& mr : meters)
    {
        auto r = mid.removeFromTop (rowH);
        const auto labelArea = r.removeFromLeft (mid.getWidth() * 0.56f);
        const auto barArea   = r.reduced (2.0f, 3.8f);

        g.setColour (juce::Colour (0xff8c9ab3));
        g.setFont (juce::FontOptions (9.8f, juce::Font::plain));
        g.drawText (mr.label, labelArea, juce::Justification::centredLeft, true);

        g.setColour (juce::Colour (0xff1d2536));
        g.fillRoundedRectangle (barArea, 3.0f);

        const auto fillArea = barArea.withWidth (barArea.getWidth() * juce::jlimit (0.05f, 1.0f, mr.normVal));
        g.setColour (accent.brighter (0.15f));
        g.fillRoundedRectangle (fillArea, 3.0f);
    }

    // Hover Preview Status Line at bottom of Center HUD
    auto footerLine = mid.withTop (mid.getBottom() - 14.0f);
    if (hoveredElementIndex >= 0 && hoveredElementIndex != selectedElementIndex)
    {
        const auto& hov = getElementByIndex (hoveredElementIndex);
        const auto hovCol = getCategoryColour (hov.category);
        g.setColour (hovCol.brighter (0.3f));
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        const juce::String msg = "Hover: [" + juce::String (hov.atomicNumber) + " " + hov.symbol + "] "
                               + getElementName (hov, currentLanguage)
                               + " (" + getElementMassDisplay (hov, currentLanguage) + " u) - "
                               + getCategoryName (hov.category, currentLanguage);
        g.drawText (msg, footerLine, juce::Justification::centredLeft, true);
    }
    else
    {
        g.setColour (juce::Colour (0xff5f6d85));
        g.setFont (juce::FontOptions (9.6f, juce::Font::plain));
        g.drawText (loc.getUiString ("inspector.emptyHoverHint", currentLanguage,
                                    "Click any element to load its sound preset"),
                    footerLine, juce::Justification::centredLeft, true);
    }
}

void PeriodicTableGrid::drawLegend (juce::Graphics& g, const LayoutMetrics& m) const
{
    for (int c = 0; c < static_cast<int> (ElementCategory::Count); ++c)
    {
        const auto cat = static_cast<ElementCategory> (c);
        const auto itemRect = getLegendItemRect (m, c);
        const auto col = getCategoryColour (cat);
        const bool isHov = (hoveredCategoryIndex == c);

        if (isHov)
        {
            g.setColour (col.withAlpha (0.16f));
            g.fillRoundedRectangle (itemRect, 4.0f);
        }

        const float swSize = 12.0f;
        const juce::Rectangle<float> swatch (itemRect.getX() + 4.0f,
                                             itemRect.getCentreY() - swSize * 0.5f,
                                             swSize, swSize);
        g.setColour (col.withAlpha (0.30f));
        g.fillRoundedRectangle (swatch.expanded (1.8f), 3.5f);
        g.setColour (col);
        g.fillRoundedRectangle (swatch, 2.5f);

        g.setColour (isHov ? juce::Colours::white : juce::Colour (0xffb8c4d8));
        g.setFont (juce::FontOptions (11.0f, isHov ? juce::Font::bold : juce::Font::plain));
        g.drawText (getCategoryName (cat, currentLanguage),
                    itemRect.withTrimmedLeft (swSize + 10.0f),
                    juce::Justification::centredLeft, true);
    }
}

void PeriodicTableGrid::resized()
{
    repaint();
}

void PeriodicTableGrid::mouseMove (const juce::MouseEvent& e)
{
    const auto m = computeMetrics();
    const int elIdx  = hitTestElement (m, e.position);
    const int catIdx = (elIdx < 0) ? hitTestLegendCategory (m, e.position) : -1;

    setMouseCursor ((elIdx >= 0 || catIdx >= 0) ? juce::MouseCursor::PointingHandCursor
                                                : juce::MouseCursor::NormalCursor);

    if (elIdx != hoveredElementIndex || catIdx != hoveredCategoryIndex)
    {
        hoveredElementIndex  = elIdx;
        hoveredCategoryIndex = catIdx;
        repaint();
    }
}

void PeriodicTableGrid::mouseExit (const juce::MouseEvent&)
{
    if (hoveredElementIndex != -1 || hoveredCategoryIndex != -1)
    {
        hoveredElementIndex = -1;
        hoveredCategoryIndex = -1;
        repaint();
    }
}

void PeriodicTableGrid::mouseDown (const juce::MouseEvent& e)
{
    const auto m = computeMetrics();
    const int elIdx = hitTestElement (m, e.position);
    if (elIdx >= 0)
    {
        setSelectedElement (elIdx);
        if (onElementSelected)
            onElementSelected (elIdx);
        return;
    }

    // Clicking a category in the legend selects the first element of that category as a quick family audition!
    const int catIdx = hitTestLegendCategory (m, e.position);
    if (catIdx >= 0)
    {
        const auto& all = getAllElements();
        for (int i = 0; i < kNumElements; ++i)
        {
            if (static_cast<int> (all[static_cast<size_t> (i)].category) == catIdx)
            {
                setSelectedElement (i);
                if (onElementSelected)
                    onElementSelected (i);
                break;
            }
        }
    }
}

} // namespace PeriodicSynth
