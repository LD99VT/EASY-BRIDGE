#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace bridge
{
// ─── Centralized colour palette ───────────────────────────────────────────────
inline const juce::Colour kBg      = juce::Colour::fromRGB (0x17, 0x17, 0x17); // UI_BG
inline const juce::Colour kRow     = juce::Colour::fromRGB (0x3a, 0x3a, 0x3a); // UI_BG_ROW
inline const juce::Colour kSection = juce::Colour::fromRGB (0x65, 0x65, 0x65); // UI_BG_SEC
inline const juce::Colour kInput   = juce::Colour::fromRGB (0x24, 0x24, 0x24); // UI_BG_INPUT
inline const juce::Colour kHeader  = juce::Colour::fromRGB (0x3a, 0x3a, 0x3a);
inline const juce::Colour kTeal    = juce::Colour::fromRGB (0x3d, 0x80, 0x70); // UI_TEAL
} // namespace bridge
