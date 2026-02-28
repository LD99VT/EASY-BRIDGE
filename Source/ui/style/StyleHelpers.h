#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "BridgeColours.h"

namespace bridge
{
// ─── Shared control styling functions ─────────────────────────────────────────
// These are free functions so any component can style controls consistently.

inline void styleCombo (juce::ComboBox& c)
{
    c.setColour (juce::ComboBox::backgroundColourId, kInput);
    c.setColour (juce::ComboBox::outlineColourId,    kRow);
    c.setColour (juce::ComboBox::textColourId,       juce::Colour::fromRGB (0xc0, 0xc0, 0xc0));
}

inline void styleEditor (juce::TextEditor& e)
{
    e.setColour (juce::TextEditor::backgroundColourId,     kInput);
    e.setColour (juce::TextEditor::outlineColourId,        kRow);
    e.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour::fromRGB (0x56, 0x5f, 0x6b));
    e.setColour (juce::TextEditor::textColourId,           juce::Colour::fromRGB (0xc0, 0xc0, 0xc0));
    e.setJustification (juce::Justification::centredLeft);
    e.setIndents (8, 2);
}

inline void styleSlider (juce::Slider& s, bool dbStyle)
{
    s.setColour (juce::Slider::backgroundColourId,      juce::Colour::fromRGB (0x20, 0x20, 0x20));
    s.setColour (juce::Slider::trackColourId,           dbStyle ? kTeal : juce::Colour::fromRGB (0x1f, 0x3b, 0x45));
    s.setColour (juce::Slider::thumbColourId,           juce::Colours::white);
    s.setColour (juce::Slider::textBoxTextColourId,     juce::Colour::fromRGB (0xc0, 0xc0, 0xc0));
    s.setColour (juce::Slider::textBoxOutlineColourId,  kRow);
    s.setColour (juce::Slider::textBoxBackgroundColourId, kInput);
}
} // namespace bridge
