#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "BridgeColours.h"

namespace bridge
{
// ─── Custom LookAndFeel: rounded controls, dark combo/editor/button ───────────
class BridgeLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawPopupMenuBackground (juce::Graphics& g, int /*width*/, int /*height*/) override
    {
        g.fillAll (findColour (juce::PopupMenu::backgroundColourId));
    }

    void drawButtonBackground (juce::Graphics& g,
                               juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool isMouseOverButton,
                               bool isButtonDown) override
    {
        juce::ignoreUnused (backgroundColour);
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        auto c = button.findColour (juce::TextButton::buttonColourId);
        const auto text = button.getButtonText().trim();
        const bool isPlusMinus = (text == "+" || text == "-");
        if (isPlusMinus)
        {
            c = kInput;
            if (isMouseOverButton || isButtonDown)
                c = isButtonDown ? kTeal.darker (0.15f) : kTeal;

            g.setColour (c);
            g.fillRoundedRectangle (bounds, 5.0f);
            g.setColour (juce::Colour::fromRGB (0x4a, 0x4a, 0x4a));
            g.drawRoundedRectangle (bounds, 5.0f, 1.0f);
            return;
        }

        if (isButtonDown)
            c = c.darker (0.15f);
        else if (isMouseOverButton)
            c = c.brighter (0.08f);

        g.setColour (c);
        g.fillRoundedRectangle (bounds, 5.0f);
        g.setColour (juce::Colour::fromRGB (0x2f, 0x2f, 0x2f));
        g.drawRoundedRectangle (bounds, 5.0f, 1.0f);
    }

    void drawButtonText (juce::Graphics& g,
                         juce::TextButton& button,
                         bool isMouseOverButton,
                         bool isButtonDown) override
    {
        const auto text = button.getButtonText().trim();
        const bool isPlusMinus = (text == "+" || text == "-");
        if (! isPlusMinus)
        {
            juce::LookAndFeel_V4::drawButtonText (g, button, isMouseOverButton, isButtonDown);
            return;
        }

        const auto iconColour = (isMouseOverButton || isButtonDown) ? kBg : juce::Colour::fromRGB (0x90, 0x90, 0x90);
        const auto b = button.getLocalBounds().toFloat();
        const float cx = b.getCentreX();
        const float cy = b.getCentreY();

        g.setColour (iconColour);
        g.fillRect (juce::Rectangle<float> (cx - 5.5f, cy - 1.0f, 11.0f, 2.0f));
        if (text == "+")
            g.fillRect (juce::Rectangle<float> (cx - 1.0f, cy - 5.5f, 2.0f, 11.0f));
    }

    void drawComboBox (juce::Graphics& g,
                       int width,
                       int height,
                       bool,
                       int, int, int, int,
                       juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
        const auto bg      = box.findColour (juce::ComboBox::backgroundColourId);
        const auto outline = box.findColour (juce::ComboBox::outlineColourId);
        const auto arrow   = box.findColour (juce::ComboBox::arrowColourId);

        g.setColour (bg);
        g.fillRoundedRectangle (bounds, 5.0f);
        g.setColour (outline);
        g.drawRoundedRectangle (bounds, 5.0f, 1.0f);

        juce::Path p;
        const float cx = (float) width - 14.0f;
        const float cy = (float) height * 0.5f;
        p.startNewSubPath (cx - 5.0f, cy - 2.0f);
        p.lineTo (cx, cy + 3.0f);
        p.lineTo (cx + 5.0f, cy - 2.0f);
        g.setColour (arrow);
        g.strokePath (p, juce::PathStrokeType (1.5f));
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (box.getLocalBounds().reduced (8, 1));
        label.setFont (getComboBoxFont (box));
        label.setJustificationType (juce::Justification::centredLeft);
    }

    void fillTextEditorBackground (juce::Graphics& g, int width, int height, juce::TextEditor& editor) override
    {
        auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
        g.setColour (editor.findColour (juce::TextEditor::backgroundColourId));
        g.fillRoundedRectangle (bounds, 5.0f);
    }

    void drawTextEditorOutline (juce::Graphics& g, int width, int height, juce::TextEditor& editor) override
    {
        auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
        const auto c = editor.hasKeyboardFocus (true)
                           ? editor.findColour (juce::TextEditor::focusedOutlineColourId)
                           : editor.findColour (juce::TextEditor::outlineColourId);
        g.setColour (c);
        g.drawRoundedRectangle (bounds, 5.0f, 1.0f);
    }

    // Rounded text-box background for Slider labels.
    void drawLabel (juce::Graphics& g, juce::Label& label) override
    {
        if (dynamic_cast<juce::Slider*> (label.getParentComponent()) != nullptr)
        {
            auto b = label.getLocalBounds().toFloat().reduced (0.5f);
            g.setColour (label.findColour (juce::Label::backgroundColourId));
            g.fillRoundedRectangle (b, 4.0f);
            g.setColour (label.findColour (juce::Label::backgroundColourId)
                             .contrasting (0.3f)
                             .withAlpha (0.6f));
            g.drawRoundedRectangle (b, 4.0f, 0.8f);

            if (! label.isBeingEdited())
            {
                g.setColour (label.findColour (juce::Label::textColourId)
                                 .withMultipliedAlpha (label.isEnabled() ? 1.0f : 0.5f));
                g.setFont (label.getFont());
                g.drawFittedText (label.getText(),
                                  label.getLocalBounds().reduced (3, 1),
                                  label.getJustificationType(), 1, 1.0f);
            }
            return;
        }
        juce::LookAndFeel_V4::drawLabel (g, label);
    }
};
} // namespace bridge
