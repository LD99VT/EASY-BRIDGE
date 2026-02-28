#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

namespace bridge::platform
{
// ─── Locate project base directory (Fonts/, Help/, Icons/ siblings) ───────────
// Searches from the executable upward to find the project root.
juce::File findBridgeBaseDir();

// ─── Load App_Icon.png from project Icons/ folder ─────────────────────────────
juce::Image loadBridgeAppIcon();

#if JUCE_WINDOWS
// ─── Apply native Windows dark title bar (DWM, Win10 1903+) ──────────────────
// Uses runtime GetProcAddress so the app degrades gracefully on older Windows.
void applyNativeDarkTitleBar (juce::DocumentWindow& window);

// ─── Remove WS_THICKFRAME + WS_MAXIMIZEBOX ───────────────────────────────────
// Makes the window truly non-resizable at the OS level (JUCE's setResizable
// alone does not strip the Win32 resize border when using native title bars).
void applyNativeFixedWindow (juce::DocumentWindow& window);

// ─── Remove icon from native title bar ───────────────────────────────────────
// Uses WS_EX_DLGMODALFRAME + WM_SETICON(null) — the canonical Win32 approach.
void removeNativeTitleBarIcon (juce::DocumentWindow& window);
#endif
} // namespace bridge::platform
