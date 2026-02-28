#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace bridge
{
// ─── Horizontal level meter with colour zones ─────────────────────────────────
class LevelMeter final : public juce::Component
{
public:
    LevelMeter() { setOpaque (false); }

    void setLevel (float newLevel)
    {
        newLevel = juce::jlimit (0.0f, 2.0f, newLevel);
        if (std::abs (currentLevel_ - newLevel) > 0.001f)
        {
            currentLevel_ = newLevel;
            repaint();
        }
    }

    void setMeterColour (juce::Colour c) { meterColour_ = c; }

    void paint (juce::Graphics& g) override
    {
        auto intBounds = getLocalBounds();
        constexpr float corner = 2.0f;

        g.setColour (juce::Colour (0xFF0D0E12));
        g.fillRoundedRectangle (intBounds.toFloat(), corner);

        auto bounds = intBounds.toFloat().reduced (1.0f);
        if (currentLevel_ > 0.001f)
        {
            const float displayLevel = juce::jmin (1.0f, currentLevel_);
            const auto fillBounds = bounds.withWidth (bounds.getWidth() * displayLevel);

            juce::Colour barColour;
            if (currentLevel_ < 0.6f)
                barColour = meterColour_.withAlpha (0.7f);
            else if (currentLevel_ < 0.85f)
                barColour = juce::Colour (0xFFFFAB00).withAlpha (0.8f);
            else
                barColour = juce::Colour (0xFFC62828).withAlpha (0.9f);

            g.setColour (barColour);
            g.fillRoundedRectangle (fillBounds, corner);
            g.setColour (juce::Colours::white.withAlpha (0.08f));
            g.fillRoundedRectangle (fillBounds.withHeight (fillBounds.getHeight() * 0.4f), corner);

            if (currentLevel_ > 1.0f)
            {
                g.setColour (juce::Colour (0xFFC62828).withAlpha (0.3f));
                g.fillRoundedRectangle (bounds, corner);
            }
        }

        g.setColour (juce::Colour (0xFF2A2D35));
        g.drawRoundedRectangle (bounds, corner, 0.5f);

        g.setColour (juce::Colour (0xFF2A2D35).withAlpha (0.6f));
        for (auto tp : { 0.25f, 0.5f, 0.75f })
        {
            const float x = bounds.getX() + bounds.getWidth() * tp;
            g.drawLine (x, bounds.getY(), x, bounds.getBottom(), 0.5f);
        }
    }

private:
    float currentLevel_ { 0.0f };
    juce::Colour meterColour_ { juce::Colour (0xFF2E7D32) };
};
} // namespace bridge
