#include "WindowUtils.h"

#if JUCE_WINDOWS
#include <windows.h>
#endif

namespace bridge::platform
{

juce::File findBridgeBaseDir()
{
    auto exeDir = juce::File::getSpecialLocation (juce::File::currentExecutableFile).getParentDirectory();
    juce::Array<juce::File> roots;
    roots.add (exeDir);
    roots.add (juce::File::getCurrentWorkingDirectory());

    auto p = exeDir;
    for (int i = 0; i < 8 && p.exists(); ++i)
    {
        roots.addIfNotAlreadyThere (p);
        p = p.getParentDirectory();
    }

    for (auto r : roots)
    {
        if (r.getChildFile ("Fonts").isDirectory()
            && r.getChildFile ("Help").isDirectory()
            && r.getChildFile ("Icons").isDirectory())
            return r;

        const juce::StringArray names { "EASYBRIDGE-JUSE", "MTC_Bridge" };
        for (const auto& name : names)
        {
            auto candidate = r.getChildFile (name);
            if (candidate.isDirectory()
                && candidate.getChildFile ("Fonts").isDirectory()
                && candidate.getChildFile ("Help").isDirectory()
                && candidate.getChildFile ("Icons").isDirectory())
                return candidate;
        }
    }

    return {};
}

juce::Image loadBridgeAppIcon()
{
    auto base = findBridgeBaseDir();
    if (! base.exists())
        return {};

#if JUCE_WINDOWS
    auto iconFile = base.getChildFile ("Icons/Icon Bridge.ico");
#elif JUCE_MAC
    auto iconFile = base.getChildFile ("Icons/Icon Bridge.icns");
#else
    auto iconFile = base.getChildFile ("Icons/App_Icon.png");
#endif
    if (! iconFile.existsAsFile())
        iconFile = base.getChildFile ("Icons/App_Icon.png");
    if (! iconFile.existsAsFile())
        return {};

    auto in = std::unique_ptr<juce::FileInputStream> (iconFile.createInputStream());
    if (in == nullptr)
        return {};

    return juce::ImageFileFormat::loadFrom (*in);
}

#if JUCE_WINDOWS
void applyNativeDarkTitleBar (juce::DocumentWindow& window)
{
    auto* peer = window.getPeer();
    if (peer == nullptr)
        return;

    auto* hwnd = static_cast<HWND> (peer->getNativeHandle());
    if (hwnd == nullptr)
        return;

    // Use dynamic loading so the app degrades gracefully on Windows versions
    // that predate the immersive dark mode DWM attribute (pre-Win10 1903).
    auto* dwm = ::LoadLibraryW (L"dwmapi.dll");
    if (dwm == nullptr)
        return;

    using DwmSetWindowAttributeFn = HRESULT (WINAPI*) (HWND, DWORD, LPCVOID, DWORD);
    auto setAttr = reinterpret_cast<DwmSetWindowAttributeFn> (
        ::GetProcAddress (dwm, "DwmSetWindowAttribute"));

    if (setAttr != nullptr)
    {
        const BOOL enabled = TRUE;
        // Attribute 20: DWMWA_USE_IMMERSIVE_DARK_MODE (Windows 10 21H1+)
        // Attribute 19: DWMWA_USE_IMMERSIVE_DARK_MODE (Windows 10 1903–20H2)
        setAttr (hwnd, 20, &enabled, sizeof (enabled));
        setAttr (hwnd, 19, &enabled, sizeof (enabled));
    }

    ::FreeLibrary (dwm);
}

void applyNativeFixedWindow (juce::DocumentWindow& window)
{
    auto* peer = window.getPeer();
    if (peer == nullptr)
        return;

    auto* hwnd = static_cast<HWND> (peer->getNativeHandle());
    if (hwnd == nullptr)
        return;

    // Strip WS_THICKFRAME (resize border) and WS_MAXIMIZEBOX so the OS-level
    // window cannot be dragged to resize, regardless of JUCE's own limits.
    constexpr int kGwlStyle = -16; // GWL_STYLE
    auto st = ::GetWindowLongPtrW (hwnd, kGwlStyle);
    st &= ~(LONG_PTR) 0x00040000L; // WS_THICKFRAME
    st &= ~(LONG_PTR) 0x00010000L; // WS_MAXIMIZEBOX
    ::SetWindowLongPtrW (hwnd, kGwlStyle, st);
    ::SetWindowPos (hwnd, nullptr, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

void removeNativeTitleBarIcon (juce::DocumentWindow& window)
{
    auto* peer = window.getPeer();
    if (peer == nullptr)
        return;

    auto* hwnd = static_cast<HWND> (peer->getNativeHandle());
    if (hwnd == nullptr)
        return;

    // Standard Win32 technique to suppress the title-bar icon (Raymond Chen):
    // Add WS_EX_DLGMODALFRAME to extended styles, clear icon handles, redraw.
    constexpr int kGwlExStyle = -20; // GWL_EXSTYLE
    auto exSt = ::GetWindowLongPtrW (hwnd, kGwlExStyle);
    ::SetWindowLongPtrW (hwnd, kGwlExStyle, exSt | (LONG_PTR) 0x00000001L); // WS_EX_DLGMODALFRAME
    ::SendMessageW (hwnd, WM_SETICON, 0 /*ICON_SMALL*/, 0);
    ::SendMessageW (hwnd, WM_SETICON, 1 /*ICON_BIG*/,   0);
    ::SetWindowPos (hwnd, nullptr, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}
#endif

} // namespace bridge::platform
