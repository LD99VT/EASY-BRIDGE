#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "../style/BridgeColours.h"

namespace bridge
{

// ─── Single coloured text segment for the status bar ─────────────────────────
struct StatusSegment
{
    juce::String text;
    juce::Colour colour;
};

// ─── Status bar widget — shows connection / error state at the bottom ─────────
class StatusBarComponent final : public juce::Component
{
public:
    std::function<void()> onClick;

    void setText (juce::String text, juce::Colour colour)
    {
        text_ = std::move (text);
        segments_.clearQuick();
        segments_.add ({ text_, colour });
        repaint();
    }

    void setSegments (juce::Array<StatusSegment> segments)
    {
        segments_ = std::move (segments);
        text_.clear();
        for (int i = 0; i < segments_.size(); ++i)
            text_ += segments_.getReference (i).text;
        repaint();
    }

    juce::String getText() const { return text_; }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds();
        if (area.isEmpty())
            return;

        g.setColour (kRow);
        g.fillRoundedRectangle (area.toFloat(), 6.0f);

        if (segments_.isEmpty())
            return;

        auto fontOptions = juce::FontOptions (15.0f);
        fontOptions = fontOptions.withStyle ("Medium");
        juce::Font font (fontOptions);
        g.setFont (font);

        float totalWidth = 0.0f;
        for (const auto& segment : segments_)
            totalWidth += font.getStringWidthFloat (segment.text);

        float x = juce::jmax (8.0f, (area.getWidth() - totalWidth) * 0.5f);

        for (const auto& segment : segments_)
        {
            const float w = font.getStringWidthFloat (segment.text);
            g.setColour (segment.colour);
            g.drawFittedText (segment.text,
                              juce::Rectangle<int> ((int) std::round (x), area.getY(),
                                                    (int) std::ceil (w), area.getHeight()),
                              juce::Justification::centredLeft, 1);
            x += w;
        }
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (onClick)
            onClick();
    }

private:
    juce::String text_;
    juce::Array<StatusSegment> segments_;
};

} // namespace bridge
