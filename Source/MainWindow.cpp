#include "MainWindow.h"
#include "core/BridgeVersion.h"
#include "platform/WindowUtils.h"
#include "ui/StatusMonitorWindow.h"
#include <cmath>
// BUG-3 fix: dwmapi loaded dynamically in platform/WindowUtils.cpp — no static link here

namespace bridge
{
void FpsIndicatorStrip::setActiveFps (std::optional<FrameRate> fps)
{
    if (activeFps_ == fps)
        return;

    activeFps_ = fps;
    repaint();
}

juce::String FpsIndicatorStrip::getActiveFpsText() const
{
    return activeFps_.has_value() ? frameRateToString (*activeFps_) : "--";
}

FpsConvertStrip::FpsConvertStrip (std::initializer_list<FrameRate> availableRates)
{
    for (auto fps : availableRates)
        availableRates_.add (fps);
}

void FpsConvertStrip::setSelectedFps (std::optional<FrameRate> fps)
{
    if (selectedFps_ == fps)
        return;

    selectedFps_ = fps;
    repaint();
}

void FpsIndicatorStrip::paint (juce::Graphics& g)
{
    auto area = getLocalBounds();
    if (area.isEmpty())
        return;

    const juce::String labels[] = { "23.976 FPS", "24 FPS", "25 FPS", "29.97 FPS", "30 FPS" };
    const FrameRate values[] = { FrameRate::FPS_2398, FrameRate::FPS_24, FrameRate::FPS_25, FrameRate::FPS_2997, FrameRate::FPS_30 };

    constexpr int gap = 6;
    auto buttonsArea = area;
    const int buttonWidth = juce::jmax (1, (buttonsArea.getWidth() - gap * 4) / 5);
    const auto inactiveBg = juce::Colour::fromRGB (0x1a, 0x1a, 0x1a);
    const auto inactiveText = juce::Colour::fromRGB (0xca, 0xca, 0xca);
    const auto activeBg = kSection;
    const auto activeText = juce::Colour::fromRGB (0xf0, 0xf0, 0xf0);
    const auto border = juce::Colour::fromRGB (0x54, 0x54, 0x54);
    auto font = juce::FontOptions (13.5f);
    font = font.withStyle ("Medium");

    for (int i = 0; i < 5; ++i)
    {
        auto cell = buttonsArea.removeFromLeft (buttonWidth);
        if (i < 4)
            buttonsArea.removeFromLeft (gap);

        const bool isActive = activeFps_.has_value() && *activeFps_ == values[i];
        g.setColour (isActive ? activeBg : inactiveBg);
            g.fillRoundedRectangle (cell.toFloat(), 6.0f);
            g.setColour (isActive ? activeBg : border);
            g.drawRoundedRectangle (cell.toFloat(), 6.0f, 1.0f);
            g.setColour (isActive ? activeText : inactiveText);
            g.setFont (font);
            g.drawFittedText (labels[i], cell, juce::Justification::centred, 1);
        }
}

void FpsConvertStrip::paint (juce::Graphics& g)
{
    auto area = getLocalBounds();
    if (area.isEmpty() || availableRates_.isEmpty())
        return;

    constexpr int gap = 6;
    const int buttonCount = availableRates_.size();
    const int buttonWidth = juce::jmax (1, (area.getWidth() - gap * (buttonCount - 1)) / buttonCount);
    auto buttonsArea = area;
    const auto inactiveBg = kInput;
    const auto inactiveText = juce::Colour::fromRGB (0xca, 0xca, 0xca);
    const auto activeBg = kTeal;
    const auto activeText = kBg;
    auto font = juce::FontOptions (14.0f);
    font = font.withStyle ("Medium");

    for (int i = 0; i < buttonCount; ++i)
    {
        auto cell = buttonsArea.removeFromLeft (buttonWidth);
        if (i < buttonCount - 1)
            buttonsArea.removeFromLeft (gap);

        const bool isActive = selectedFps_.has_value() && *selectedFps_ == availableRates_[i];
        g.setColour (isActive ? activeBg : inactiveBg);
        g.fillRoundedRectangle (cell.toFloat(), 6.0f);
        g.setColour (isActive ? activeText : inactiveText);
        g.setFont (font);
        g.drawFittedText (frameRateToString (availableRates_[i]), cell, juce::Justification::centred, 1);
    }
}

void FpsConvertStrip::mouseUp (const juce::MouseEvent& event)
{
    auto area = getLocalBounds();
    if (area.isEmpty() || availableRates_.isEmpty())
        return;

    constexpr int gap = 6;
    const int buttonCount = availableRates_.size();
    const int buttonWidth = juce::jmax (1, (area.getWidth() - gap * (buttonCount - 1)) / buttonCount);
    auto buttonsArea = area;

    for (int i = 0; i < buttonCount; ++i)
    {
        auto cell = buttonsArea.removeFromLeft (buttonWidth);
        if (i < buttonCount - 1)
            buttonsArea.removeFromLeft (gap);

        if (! cell.contains (event.getPosition()))
            continue;

        const auto clicked = availableRates_[i];
        const auto next = (selectedFps_.has_value() && *selectedFps_ == clicked)
            ? std::optional<FrameRate> {}
            : std::optional<FrameRate> { clicked };
        setSelectedFps (next);
        if (onChange)
            onChange (next);
        return;
    }
}

void StatusBarComponent::setText (juce::String text, juce::Colour colour)
{
    text_ = std::move (text);
    segments_.clearQuick();
    segments_.add ({ text_, colour });
    repaint();
}

void StatusBarComponent::setSegments (juce::Array<StatusSegment> segments)
{
    segments_ = std::move (segments);
    text_.clear();
    for (int i = 0; i < segments_.size(); ++i)
        text_ += segments_.getReference (i).text;
    repaint();
}

void StatusBarComponent::paint (juce::Graphics& g)
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
                          juce::Rectangle<int> ((int) std::round (x), area.getY(), (int) std::ceil (w), area.getHeight()),
                          juce::Justification::centredLeft, 1);
        x += w;
    }
}

void StatusBarComponent::mouseUp (const juce::MouseEvent&)
{
    if (onClick)
        onClick();
}

namespace
{
// BUG-7 fix: named constant instead of magic literal 256
constexpr int kLtcOutputBufferSize = 256;
constexpr int kPlaceholderItemId   = 10000;
constexpr int kCompactBarHeight    = 24;
constexpr int kStartupHeightExtra  = 8;
constexpr int kOuterMargin         = 8;
constexpr int kHeaderHeight        = 40;
constexpr int kTimerHeight         = 90;
constexpr int kTopSectionGap       = 4;
constexpr int kBottomButtonsHeight = 40;
// Colours live in BridgeColours.h (included via MainWindow.h)

juce::String parseBindIpFromAdapterLabel (juce::String text)
{
    text = text.trim();
    if (text.isEmpty())
        return "0.0.0.0";

    const auto lower = text.toLowerCase();
    if (lower.contains ("loopback"))
        return "127.0.0.1";
    if (lower.contains ("all interfaces"))
        return "0.0.0.0";

    const int open = text.lastIndexOfChar ('(');
    const int close = text.lastIndexOfChar (')');
    if (open >= 0 && close > open)
    {
        const auto ip = text.substring (open + 1, close).trim();
        if (ip.isNotEmpty())
            return ip;
    }

    return "0.0.0.0";
}

void setupOffsetSlider (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::LinearHorizontal);
    s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 20);
    s.setRange (-30, 30, 1);
    s.setValue (0);
}

void setupDbSlider (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::LinearHorizontal);
    s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 20);
    s.setRange (-24, 24, 0.1);
    s.setValue (0.0);
    s.setTextValueSuffix (" dB");
    s.setDoubleClickReturnValue (true, 0.0);
}

void fillRateCombo (juce::ComboBox& combo)
{
    combo.clear();
    combo.addItem ("Default", 1);
    combo.addItem ("44100", 2);
    combo.addItem ("48000", 3);
    combo.addItem ("88200", 4);
    combo.addItem ("96000", 5);
    combo.addItem ("176400", 6);
    combo.addItem ("192000", 7);
    combo.setSelectedId (1, juce::dontSendNotification);
}

void fillChannelCombo (juce::ComboBox& combo, int channelCount)
{
    combo.clear();
    if (channelCount <= 0)
    {
        combo.addItem ("(No channels)", kPlaceholderItemId);
        combo.setSelectedId (kPlaceholderItemId, juce::dontSendNotification);
        return;
    }

    for (int i = 1; i <= channelCount; ++i)
        combo.addItem (juce::String (i), i);

    for (int start = 0; start + 1 < channelCount; start += 2)
        combo.addItem (juce::String (start + 1) + "+" + juce::String (start + 2), 1000 + start);

    combo.setSelectedId (1, juce::dontSendNotification);
}

juce::String normalizeDriverKey (juce::String s)
{
    s = s.toLowerCase().trim();
    if (s.contains ("asio")) return "asio";
    if (s.contains ("directsound")) return "directsound";
    if (s.contains ("coreaudio")) return "coreaudio";
    if (s.contains ("alsa")) return "alsa";
    if (s.contains ("wasapi") || s.contains ("windows audio")) return "windowsaudio";
    return s;
}

bool matchesDriverFilter (const juce::String& driverUi, const juce::String& typeName)
{
    const auto d = normalizeDriverKey (driverUi);
    if (d.startsWith ("default"))
        return true;
    const auto t = normalizeDriverKey (typeName);
    return t == d;
}

bool varsEqualConfig (const juce::var& a, const juce::var& b)
{
    if (a.isArray() || b.isArray())
    {
        auto* aa = a.getArray();
        auto* bb = b.getArray();
        if (aa == nullptr || bb == nullptr || aa->size() != bb->size())
            return false;
        for (int i = 0; i < aa->size(); ++i)
            if (! varsEqualConfig ((*aa)[i], (*bb)[i]))
                return false;
        return true;
    }

    if (a.isBool() || b.isBool())
        return (bool) a == (bool) b;

    if (a.isInt() || a.isInt64() || a.isDouble() || b.isInt() || b.isInt64() || b.isDouble())
        return juce::approximatelyEqual ((double) a, (double) b);

    return a.toString().trim() == b.toString().trim();
}

void fillDriverCombo (juce::ComboBox& combo, const juce::Array<engine::AudioChoice>& choices, const juce::String& previousText)
{
    combo.clear();
    combo.addItem ("Default (all devices)", 1);

    juce::StringArray seen;
    for (const auto& c : choices)
    {
        if (c.typeName.isNotEmpty() && ! seen.contains (c.typeName))
            seen.add (c.typeName);
    }

    seen.sortNatural();
    for (int i = 0; i < seen.size(); ++i)
        combo.addItem (seen[i], i + 2);

    if (previousText.isNotEmpty())
    {
        for (int i = 0; i < combo.getNumItems(); ++i)
        {
            if (combo.getItemText (i) == previousText)
            {
                combo.setSelectedItemIndex (i, juce::dontSendNotification);
                return;
            }
        }
    }

    combo.setSelectedId (1, juce::dontSendNotification);
}

float dbToLinearGain (double db)
{
    return (float) std::pow (10.0, db / 20.0);
}

juce::String formatDisplayedInputFps (bool hasLatchedTc, FrameRate latchedFps, FpsIndicatorStrip* strip)
{
    if (strip != nullptr)
        return strip->getActiveFpsText();

    return hasLatchedTc ? frameRateToString (latchedFps) : "--";
}

juce::String outputStatusWithFps (const juce::String& state, std::optional<FrameRate> outputFps)
{
    if (state.equalsIgnoreCase ("OFF") || state.equalsIgnoreCase ("THRU"))
        return state;

    return outputFps.has_value() ? (state + " " + frameRateToString (*outputFps)) : state;
}

bool isConvertActive (const juce::String& state, std::optional<FrameRate> outputFps, std::optional<FrameRate> inputFps)
{
    if (state.equalsIgnoreCase ("OFF") || state.equalsIgnoreCase ("THRU"))
        return false;

    return outputFps.has_value() && inputFps.has_value() && *outputFps != *inputFps;
}

std::optional<FrameRate> fpsFromString (juce::String text)
{
    text = text.trim();
    if (text.isEmpty())
        return std::nullopt;
    if (text == "23.976")
        return FrameRate::FPS_2398;
    if (text == "24")
        return FrameRate::FPS_24;
    if (text == "25")
        return FrameRate::FPS_25;
    if (text == "29.97")
        return FrameRate::FPS_2997;
    if (text == "30")
        return FrameRate::FPS_30;
    return std::nullopt;
}
// findBridgeBaseDirFromExe, loadBridgeAppIcon, applyNativeDarkTitleBar
// → moved to platform/WindowUtils.cpp (BUG-1, BUG-3 fix)
// StatusMonitorWindow → moved to ui/StatusMonitorWindow.h (BUG-10 fix)

// ─── Fake block to keep compiler happy: remove the old StatusMonitorWindow ──
// (nothing to declare here — it's now a standalone class in StatusMonitorWindow.h)

// Temporary placeholder so the removal of the original StatusMonitorWindow
// constructor body below compiles cleanly.
// The block originally started here; we patch up to the first non-StatusMonitorWindow
// class (BridgeTrayIcon) below.

// Placeholder: original StatusMonitorWindow init block that called applyNativeDarkTitleBar
// is gone; the real StatusMonitorWindow ctor is in ui/StatusMonitorWindow.h.

// ─── Tray icon (stays here — circular dep with MainWindow) ───────────────────
class BridgeTrayIcon final : public juce::SystemTrayIconComponent
{
public:
    explicit BridgeTrayIcon (MainWindow& owner) : owner_ (owner) {}

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown())
        {
            juce::PopupMenu menu;
            menu.addItem (1, "Show");
            menu.addSeparator();
            menu.addItem (2, "Quit");
            menu.showMenuAsync (juce::PopupMenu::Options(),
                                [safeOwner = juce::Component::SafePointer<MainWindow> (&owner_)] (int result)
                                {
                                    if (safeOwner == nullptr)
                                        return;
                                    if (result == 1)
                                        safeOwner->showFromTray();
                                    else if (result == 2)
                                        safeOwner->quitFromTray();
                                });
            return;
        }

        owner_.showFromTray();
    }

private:
    MainWindow& owner_;
};

class ExitSavePromptDialog final : public juce::DocumentWindow
{
public:
    static void show (juce::Component* relativeTo,
                      std::function<void(int)> onDone)
    {
        new ExitSavePromptDialog (relativeTo, std::move (onDone));
    }

    void closeButtonPressed() override
    {
        finishAndClose (0);
    }

private:
    ExitSavePromptDialog (juce::Component* relativeTo,
                          std::function<void(int)> onDone)
        : juce::DocumentWindow ("Close Easy Bridge",
                                juce::Colour::fromRGB (0x1e, 0x1e, 0x1e),
                                juce::DocumentWindow::closeButton),
          onDone_ (std::move (onDone))
    {
        setUsingNativeTitleBar (true);
        setResizable (false, false);
        setContentOwned (new Content (*this), true);
        centreWithSize (450, 190);
        if (relativeTo != nullptr)
        {
            const auto rc = relativeTo->getScreenBounds();
            setBounds (rc.getCentreX() - 225, rc.getCentreY() - 95, 450, 190);
        }
        setVisible (true);
#if JUCE_WINDOWS
        platform::applyNativeDarkTitleBar (*this);
        platform::applyNativeFixedWindow (*this);
        platform::removeNativeTitleBarIcon (*this);
#endif
        toFront (true);
    }

    void finishAndClose (int result)
    {
        if (onDone_ != nullptr)
            onDone_ (result);

        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<ExitSavePromptDialog> (this)]
        {
            if (auto* w = safe.getComponent())
                delete w;
        });
    }

    struct Content final : juce::Component
    {
        explicit Content (ExitSavePromptDialog& owner) : owner_ (owner)
        {
            msg_.setText ("Configuration has unsaved changes.\nSave config before exit?",
                          juce::dontSendNotification);
            msg_.setColour (juce::Label::textColourId, juce::Colour::fromRGB (0xe0, 0xe0, 0xe0));
            msg_.setJustificationType (juce::Justification::centred);
            addAndMakeVisible (msg_);

            setupButton (saveBtn_, "Save", [this] { owner_.finishAndClose (1); });
            setupButton (exitBtn_, "Exit", [this] { owner_.finishAndClose (2); });
            setupButton (cancelBtn_, "Cancel", [this] { owner_.finishAndClose (0); });
            addAndMakeVisible (saveBtn_);
            addAndMakeVisible (exitBtn_);
            addAndMakeVisible (cancelBtn_);
        }

        void resized() override
        {
            constexpr int kPad = 20;
            msg_.setBounds (kPad, kPad, getWidth() - kPad * 2, 90);
            const int y = getHeight() - 50;
            const int w = 120;
            const int gap = 10;
            const int total = w * 3 + gap * 2;
            int x = (getWidth() - total) / 2;
            saveBtn_.setBounds (x, y, w, 32); x += w + gap;
            exitBtn_.setBounds (x, y, w, 32); x += w + gap;
            cancelBtn_.setBounds (x, y, w, 32);
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (juce::Colour::fromRGB (0x17, 0x17, 0x17));
        }

        static void setupButton (juce::TextButton& btn,
                                 const juce::String& text,
                                 std::function<void()> onClick)
        {
            btn.setButtonText (text);
            btn.setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (0x4a, 0x4a, 0x4a));
            btn.setColour (juce::TextButton::buttonOnColourId, juce::Colour::fromRGB (0x4a, 0x4a, 0x4a));
            btn.setColour (juce::TextButton::textColourOffId, juce::Colour::fromRGB (0xe1, 0xe6, 0xef));
            btn.setColour (juce::TextButton::textColourOnId, juce::Colour::fromRGB (0xe1, 0xe6, 0xef));
            btn.onClick = std::move (onClick);
        }

        ExitSavePromptDialog& owner_;
        juce::Label msg_;
        juce::TextButton saveBtn_, exitBtn_, cancelBtn_;
    };

    std::function<void(int)> onDone_;
};

static int findFilteredIndex (const juce::Array<int>& filteredIndices,
                              const juce::Array<engine::AudioChoice>& entries,
                              const juce::String& typeName,
                              const juce::String& deviceName)
{
    if (deviceName.isEmpty())
        return -1;

    if (typeName.isNotEmpty())
    {
        for (int i = 0; i < filteredIndices.size(); ++i)
        {
            const int realIdx = filteredIndices[i];
            if (juce::isPositiveAndBelow (realIdx, entries.size())
                && entries[realIdx].typeName == typeName
                && entries[realIdx].deviceName == deviceName)
                return i;
        }
    }

    for (int i = 0; i < filteredIndices.size(); ++i)
    {
        const int realIdx = filteredIndices[i];
        if (juce::isPositiveAndBelow (realIdx, entries.size())
            && entries[realIdx].deviceName == deviceName)
            return i;
    }
    return -1;
}

void selectComboItemByText (juce::ComboBox& combo, const juce::String& text)
{
    for (int i = 0; i < combo.getNumItems(); ++i)
    {
        if (combo.getItemText (i) == text)
        {
            combo.setSelectedItemIndex (i, juce::dontSendNotification);
            return;
        }
    }
}

}

class BridgeAudioScanThread final : public juce::Thread
{
public:
    explicit BridgeAudioScanThread (MainContentComponent* owner)
        : juce::Thread ("BridgeAudioDeviceScan"), safeOwner_ (owner)
    {
    }

    void run() override
    {
        engine::BridgeEngine probeEngine;
        auto inputs = probeEngine.scanAudioInputs();

        if (threadShouldExit())
            return;

        auto outputs = probeEngine.scanAudioOutputs();

        if (threadShouldExit())
            return;

        auto safe = safeOwner_;
        juce::MessageManager::callAsync ([safe, inputs, outputs]()
        {
            if (auto* owner = safe.getComponent())
                owner->onAudioScanComplete (inputs, outputs);
        });
    }

private:
    juce::Component::SafePointer<MainContentComponent> safeOwner_;
};

// ─── RowsPanel ────────────────────────────────────────────────────────────────
// Scrollable content panel that holds all parameter section rows.
// MainContentComponent places this inside a juce::Viewport so that when the
// total row height exceeds the available screen area the user can scroll
// instead of rows getting squished.
class RowsPanel final : public juce::Component
{
public:
    RowsPanel() { setOpaque (false); }

    // Filled by MainContentComponent::resized() and read back in paint().
    juce::Array<juce::Rectangle<int>> paramRowRects;
    juce::Array<juce::Rectangle<int>> sectionRowRects;

    void paint (juce::Graphics& g) override
    {
        g.setColour (kSection);
        for (auto r : sectionRowRects)
            g.fillRoundedRectangle (r.toFloat(), 5.0f);

        g.setColour (kRow);
        for (auto r : paramRowRects)
            g.fillRoundedRectangle (r.toFloat(), 5.0f);

        g.setColour (juce::Colour::fromRGB (0x2f, 0x2f, 0x2f));
        for (auto r : sectionRowRects)
            g.drawRoundedRectangle (r.toFloat(), 5.0f, 1.0f);
        for (auto r : paramRowRects)
            g.drawRoundedRectangle (r.toFloat(), 5.0f, 1.0f);
    }
};

MainContentComponent::MainContentComponent()
{
    ltcOutputApplyThread_ = std::thread ([this] { ltcOutputApplyLoop(); });
    rowsPanel_ = std::make_unique<RowsPanel>();
    setOpaque (true);

    loadFontsAndIcon();
    applyLookAndFeel();

    for (int i = 0; i < (int) artSendLbls_.size(); ++i)
    {
        artSendLbls_[(size_t) i].setText ("Send " + juce::String (i + 1) + ":", juce::dontSendNotification);
        artAdapterLbls_[(size_t) i].setText ("Adapter:", juce::dontSendNotification);
        if (i < (int) artnetDestRemoveButtons_.size())
            artnetDestRemoveButtons_[(size_t) i].setButtonText ("-");
    }

    titleEasyLabel_.setText ("EASY ", juce::dontSendNotification);
    titleEasyLabel_.setJustificationType (juce::Justification::centredLeft);
    titleEasyLabel_.setColour (juce::Label::textColourId, juce::Colour::fromRGB (0xce, 0xce, 0xce));
    titleEasyLabel_.setFont ((titleEasyFont_.getHeight() > 0.0f ? titleEasyFont_ : juce::Font()).withHeight (32.0f));
    addAndMakeVisible (titleEasyLabel_);

    titleBridgeLabel_.setText ("BRIDGE", juce::dontSendNotification);
    titleBridgeLabel_.setJustificationType (juce::Justification::centredLeft);
    titleBridgeLabel_.setColour (juce::Label::textColourId, juce::Colour::fromRGB (0x8a, 0x8a, 0x8a));
    titleBridgeLabel_.setFont ((titleBridgeFont_.getHeight() > 0.0f ? titleBridgeFont_ : juce::Font()).withHeight (32.0f));
    addAndMakeVisible (titleBridgeLabel_);

    titleVersionLabel_.setText ("v" + juce::String (version::kAppVersion), juce::dontSendNotification);
    titleVersionLabel_.setJustificationType (juce::Justification::centredLeft);
    titleVersionLabel_.setColour (juce::Label::textColourId, juce::Colour::fromRGB (0x8a, 0x8a, 0x8a));
    titleVersionLabel_.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (titleVersionLabel_);

    helpButton_.onClick = [this] { openHelpPage(); };
    addAndMakeVisible (helpButton_);

    tcLabel_.setText ("00:00:00:00", juce::dontSendNotification);
    tcLabel_.setJustificationType (juce::Justification::centred);
    tcLabel_.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    tcLabel_.setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    tcLabel_.setColour (juce::Label::textColourId, juce::Colours::white);
    if (monoFont_.getHeight() > 0.0f)
        tcLabel_.setFont (monoFont_.withHeight (68.0f));
    addAndMakeVisible (tcLabel_);

    fpsIndicatorStrip_ = std::make_unique<FpsIndicatorStrip>();
    fpsIndicatorStrip_->setActiveFps (std::nullopt);
    addAndMakeVisible (*fpsIndicatorStrip_);

    statusButton_.setText ("STOPPED", juce::Colour::fromRGB (0xec, 0x48, 0x3c));
    statusButton_.onClick = [this] { openStatusMonitorWindow(); };
    addAndMakeVisible (statusButton_);

    settingsButton_.setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (0x4a, 0x4a, 0x4a));
    settingsButton_.setColour (juce::TextButton::buttonOnColourId, juce::Colour::fromRGB (0x4a, 0x4a, 0x4a));
    settingsButton_.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    settingsButton_.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    settingsButton_.onClick = [this] { openSettingsMenu(); };
    addAndMakeVisible (settingsButton_);

    quitButton_.setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (0xb6, 0x45, 0x40));
    quitButton_.setColour (juce::TextButton::buttonOnColourId, juce::Colour::fromRGB (0xb6, 0x45, 0x40));
    quitButton_.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    quitButton_.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    quitButton_.onClick = [this]
    {
        if (auto* window = findParentComponentOfClass<MainWindow>())
            window->quitFromTray();
        else
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
    };
    addAndMakeVisible (quitButton_);

    sourceCombo_.addItem ("LTC", 1);
    sourceCombo_.addItem ("MTC", 2);
    sourceCombo_.addItem ("ArtNet", 3);
    sourceCombo_.addItem ("OSC", 4);
    sourceCombo_.addItem ("System Time", 5);
    sourceCombo_.setSelectedId (1, juce::dontSendNotification);
    styleCombo (sourceCombo_);
    rowsPanel_->addAndMakeVisible (sourceHeaderLabel_);
    sourceExpandBtn_.setExpanded (true);
    sourceExpandBtn_.onClick = [this]
    {
        sourceExpanded_ = ! sourceExpanded_;
        sourceExpandBtn_.setExpanded (sourceExpanded_);
        updateWindowHeight();
        resized();
    };
    rowsPanel_->addAndMakeVisible (sourceExpandBtn_);
    rowsPanel_->addAndMakeVisible (sourceCombo_);
    sourceHeaderLabel_.setColour (juce::Label::textColourId, juce::Colour::fromRGB (0xe4, 0xe4, 0xe4));
    sourceHeaderLabel_.setFont (juce::FontOptions (14.0f));
    sourceHeaderLabel_.setJustificationType (juce::Justification::centredLeft);
    sourceHeaderLabel_.setBorderSize (juce::BorderSize<int> (0, 6, 0, 0));
    juce::Label* rowLabels[] = {
        &inDriverLbl_, &inDeviceLbl_, &inChannelLbl_, &inRateLbl_, &inLevelLbl_, &inGainLbl_,
        &mtcInLbl_, &artInLbl_, &artInListenIpLbl_, &oscAdapterLbl_, &oscIpLbl_, &oscPortLbl_, &oscFpsLbl_, &systemTimeFpsLbl_, &oscStrLbl_, &oscFloatLbl_, &oscFloatTypeLbl_, &oscFloatMaxLbl_,
        &outDriverLbl_, &outDeviceLbl_, &outChannelLbl_, &outRateLbl_, &outConvertLbl_, &outOffsetLbl_, &outLevelLbl_,
        &mtcOutLbl_, &mtcConvertLbl_, &mtcOffsetLbl_, &artConvertLbl_, &artOffsetLbl_,
        &artSendLbls_[0], &artSendLbls_[1], &artSendLbls_[2], &artSendLbls_[3], &artSendLbls_[4],
        &artAdapterLbls_[0], &artAdapterLbls_[1], &artAdapterLbls_[2], &artAdapterLbls_[3], &artAdapterLbls_[4]
    };
    for (auto* l : rowLabels)
    {
        l->setColour (juce::Label::textColourId, juce::Colour::fromRGB (0xca, 0xca, 0xca));
        l->setJustificationType (juce::Justification::centredLeft);
        rowsPanel_->addAndMakeVisible (*l);
    }

    for (auto* c : { &ltcInDriverCombo_, &ltcOutDriverCombo_ })
    {
        c->addItem ("Default (all devices)", 1);
        c->setSelectedId (1, juce::dontSendNotification);
        styleCombo (*c);
    }

    styleCombo (ltcInDeviceCombo_);
    styleCombo (ltcInChannelCombo_);
    styleCombo (ltcInSampleRateCombo_);
    styleCombo (mtcInCombo_);
    styleCombo (artnetInCombo_);
    styleCombo (oscAdapterCombo_);
    styleCombo (oscFpsCombo_);
    styleCombo (systemTimeFpsCombo_);
    styleCombo (ltcOutDeviceCombo_);
    styleCombo (ltcOutChannelCombo_);
    styleCombo (ltcOutSampleRateCombo_);
    styleCombo (mtcOutCombo_);
    for (auto& combo : artnetTargetAdapterCombos_)
        styleCombo (combo);
    rowsPanel_->addAndMakeVisible (ltcInDriverCombo_);
    rowsPanel_->addAndMakeVisible (ltcOutDriverCombo_);

    fillChannelCombo (ltcInChannelCombo_, 0);
    fillChannelCombo (ltcOutChannelCombo_, 0);
    fillRateCombo (ltcInSampleRateCombo_);
    fillRateCombo (ltcOutSampleRateCombo_);

    oscPortEditor_.setText ("9000");
    oscIpEditor_.setText ("0.0.0.0");
    oscPortEditor_.setInputRestrictions (5, "0123456789");
    oscAddrStrEditor_.setText ("/frames/str");
    oscAddrFloatEditor_.setText ("/time");
    oscFloatTypeCombo_.addItem ("Seconds",    1);
    oscFloatTypeCombo_.addItem ("Frames",     2);
    oscFloatTypeCombo_.addItem ("Normalized", 3);
    oscFloatTypeCombo_.setSelectedId (1, juce::dontSendNotification);
    oscFloatMaxEditor_.setText ("3600");
    // Allow both dot and comma as decimal separators (e.g. 7.15 or 7,15).
    oscFloatMaxEditor_.setInputRestrictions (10, "0123456789.,");
    styleEditor (oscIpEditor_);
    styleEditor (oscPortEditor_);
    styleEditor (oscAddrStrEditor_);
    styleEditor (oscAddrFloatEditor_);
    styleCombo  (oscFloatTypeCombo_);
    styleEditor (oscFloatMaxEditor_);
    styleEditor (artnetListenIpEditor_);
    artnetListenIpEditor_.setText ("0.0.0.0");
    for (auto& e : artnetDestIpEditors_)
        styleEditor (e);
    artnetDestIpEditors_[0].setText ("255.255.255.255");
    for (int i = 1; i < (int) artnetDestIpEditors_.size(); ++i)
        artnetDestIpEditors_[i].setText ("");

    oscFpsCombo_.addItem ("24", 1);
    oscFpsCombo_.addItem ("25", 2);
    oscFpsCombo_.addItem ("29.97", 3);
    oscFpsCombo_.addItem ("30", 4);
    oscFpsCombo_.setSelectedId (2, juce::dontSendNotification);
    systemTimeFpsCombo_.addItem ("23.976", 1);
    systemTimeFpsCombo_.addItem ("24", 2);
    systemTimeFpsCombo_.addItem ("25", 3);
    systemTimeFpsCombo_.addItem ("29.97", 4);
    systemTimeFpsCombo_.addItem ("30", 5);
    systemTimeFpsCombo_.setSelectedId (3, juce::dontSendNotification);
    setupDbSlider (ltcInGainSlider_);
    ltcInLevelBar_.setMeterColour (juce::Colour::fromRGB (0x3d, 0x80, 0x70));
    setupDbSlider (ltcOutLevelSlider_);
    ltcOffsetEditor_.setInputRestrictions (4, "-0123456789");
    mtcOffsetEditor_.setInputRestrictions (4, "-0123456789");
    artnetOffsetEditor_.setInputRestrictions (4, "-0123456789");
    ltcOffsetEditor_.setText ("0");
    mtcOffsetEditor_.setText ("0");
    artnetOffsetEditor_.setText ("0");
    styleEditor (ltcOffsetEditor_);
    styleEditor (mtcOffsetEditor_);
    styleEditor (artnetOffsetEditor_);
    styleSlider (ltcInGainSlider_, true);
    styleSlider (ltcOutLevelSlider_, true);

    for (auto* h : { &outLtcHeaderLabel_, &outMtcHeaderLabel_, &outArtHeaderLabel_ })
    {
        h->setColour (juce::Label::backgroundColourId, kSection);
        h->setColour (juce::Label::textColourId, juce::Colour::fromRGB (0xe4, 0xe4, 0xe4));
        h->setFont (juce::FontOptions (14.0f));
        h->setJustificationType (juce::Justification::centredLeft);
        h->setBorderSize (juce::BorderSize<int> (0, 42, 0, 0));
        rowsPanel_->addAndMakeVisible (*h);
    }

    for (auto* b : { &outLtcExpandBtn_, &outMtcExpandBtn_, &outArtExpandBtn_ })
        rowsPanel_->addAndMakeVisible (*b);
    outLtcExpandBtn_.setExpanded (false);
    outMtcExpandBtn_.setExpanded (false);
    outArtExpandBtn_.setExpanded (false);
    outLtcExpandBtn_.onClick = [this] { outLtcExpanded_ = ! outLtcExpanded_; outLtcExpandBtn_.setExpanded (outLtcExpanded_); updateWindowHeight(); };
    outMtcExpandBtn_.onClick = [this] { outMtcExpanded_ = ! outMtcExpanded_; outMtcExpandBtn_.setExpanded (outMtcExpanded_); updateWindowHeight(); };
    outArtExpandBtn_.onClick = [this] { outArtExpanded_ = ! outArtExpanded_; outArtExpandBtn_.setExpanded (outArtExpanded_); updateWindowHeight(); };

    for (auto* c : { &ltcOutSwitch_, &mtcOutSwitch_, &artnetOutSwitch_ })
        rowsPanel_->addAndMakeVisible (*c);
    rowsPanel_->addAndMakeVisible (ltcThruDot_);
    ltcThruDot_.onToggle = [this] (bool) { queueLtcOutputApply(); refreshConfigDirtyState(); };
    ltcThruLbl_.setColour (juce::Label::textColourId, juce::Colour::fromRGB (0xe4, 0xe4, 0xe4));
    ltcThruLbl_.setJustificationType (juce::Justification::centredLeft);
    rowsPanel_->addAndMakeVisible (ltcThruLbl_);

    for (auto* c : {
            &ltcInDeviceCombo_, &ltcInChannelCombo_, &ltcInSampleRateCombo_,
            &ltcOutDeviceCombo_, &ltcOutChannelCombo_, &ltcOutSampleRateCombo_,
            &oscAdapterCombo_,
            &mtcInCombo_, &artnetInCombo_, &mtcOutCombo_, &oscFpsCombo_ })
    {
        rowsPanel_->addAndMakeVisible (*c);
        c->onChange = [this] { onInputSettingsChanged(); refreshConfigDirtyState(); };
    }
    for (auto& combo : artnetTargetAdapterCombos_)
    {
        rowsPanel_->addAndMakeVisible (combo);
        combo.onChange = [this] { onOutputSettingsChanged(); refreshConfigDirtyState(); };
    }
    artnetListenIpEditor_.onTextChange = [this] { onInputSettingsChanged(); refreshConfigDirtyState(); };
    oscIpEditor_.onTextChange = [this] { onInputSettingsChanged(); refreshConfigDirtyState(); };
    oscPortEditor_.onTextChange = [this] { onInputSettingsChanged(); refreshConfigDirtyState(); };
    oscAddrStrEditor_.onTextChange = [this] { onInputSettingsChanged(); refreshConfigDirtyState(); };
    oscAddrFloatEditor_.onTextChange = [this] { onInputSettingsChanged(); refreshConfigDirtyState(); };
    oscFloatTypeCombo_.onChange = [this] { resized(); onInputSettingsChanged(); refreshConfigDirtyState(); };
    oscFloatMaxEditor_.onTextChange = [this] { onInputSettingsChanged(); refreshConfigDirtyState(); };
    ltcOffsetEditor_.onTextChange = [this] { refreshConfigDirtyState(); };
    mtcOffsetEditor_.onTextChange = [this] { refreshConfigDirtyState(); };
    artnetOffsetEditor_.onTextChange = [this] { refreshConfigDirtyState(); };
    ltcConvertStrip_.onChange = [this] (std::optional<FrameRate> fps)
    {
        bridgeEngine_.setLtcOutputConvertFps (fps);
        refreshConfigDirtyState();
    };
    mtcConvertStrip_.onChange = [this] (std::optional<FrameRate> fps)
    {
        bridgeEngine_.setMtcOutputConvertFps (fps);
        refreshConfigDirtyState();
    };
    artnetConvertStrip_.onChange = [this] (std::optional<FrameRate> fps)
    {
        bridgeEngine_.setArtnetOutputConvertFps (fps);
        refreshConfigDirtyState();
    };
    for (auto& e : artnetDestIpEditors_)
        e.onTextChange = [this] { onOutputSettingsChanged(); refreshConfigDirtyState(); };
    for (int i = 0; i < (int) artnetTargetExpandButtons_.size(); ++i)
    {
        artnetTargetExpandButtons_[(size_t) i].setColours (kInput, juce::Colour::fromRGB (0x4a, 0x4a, 0x4a), juce::Colour::fromRGB (0xf2, 0xf2, 0xf2));
        artnetTargetExpandButtons_[(size_t) i].setExpanded (artnetTargetAdapterExpanded_[(size_t) i]);
        artnetTargetExpandButtons_[(size_t) i].onClick = [this, i]
        {
            if (i >= artnetDestVisibleCount_)
                return;
            artnetTargetAdapterExpanded_[(size_t) i] = ! artnetTargetAdapterExpanded_[(size_t) i];
            artnetTargetExpandButtons_[(size_t) i].setExpanded (artnetTargetAdapterExpanded_[(size_t) i]);
            updateWindowHeight();
            resized();
            refreshConfigDirtyState();
        };
        rowsPanel_->addAndMakeVisible (artnetTargetExpandButtons_[(size_t) i]);
    }
    for (int i = 0; i < (int) artnetDestRemoveButtons_.size(); ++i)
    {
        artnetDestRemoveButtons_[i].setColour (juce::TextButton::buttonColourId, kInput);
        artnetDestRemoveButtons_[i].setColour (juce::TextButton::buttonOnColourId, kInput);
        artnetDestRemoveButtons_[i].setColour (juce::TextButton::textColourOffId, juce::Colour::fromRGB (0xca, 0xca, 0xca));
        artnetDestRemoveButtons_[i].setColour (juce::TextButton::textColourOnId, juce::Colour::fromRGB (0xca, 0xca, 0xca));
        artnetDestRemoveButtons_[i].onClick = [this, i]
        {
            const int idx = i + 1;
            if (idx < artnetDestVisibleCount_)
            {
                artnetDestIpEditors_[idx].setText ("", juce::dontSendNotification);
                artnetTargetAdapterCombos_[(size_t) idx].setSelectedItemIndex (0, juce::dontSendNotification);
                artnetTargetAdapterExpanded_[(size_t) idx] = false;
                for (int j = idx; j < artnetDestVisibleCount_ - 1; ++j)
                {
                    artnetDestIpEditors_[j].setText (artnetDestIpEditors_[j + 1].getText(), juce::dontSendNotification);
                    artnetTargetAdapterCombos_[(size_t) j].setText (artnetTargetAdapterCombos_[(size_t) (j + 1)].getText(), juce::dontSendNotification);
                    artnetTargetAdapterExpanded_[(size_t) j] = artnetTargetAdapterExpanded_[(size_t) (j + 1)];
                }
                artnetDestIpEditors_[artnetDestVisibleCount_ - 1].setText ("", juce::dontSendNotification);
                artnetTargetAdapterCombos_[(size_t) (artnetDestVisibleCount_ - 1)].setSelectedItemIndex (0, juce::dontSendNotification);
                artnetTargetAdapterExpanded_[(size_t) (artnetDestVisibleCount_ - 1)] = false;
                --artnetDestVisibleCount_;
                updateArtnetIpControls();
                updateWindowHeight();
                resized();
                onOutputSettingsChanged();
                refreshConfigDirtyState();
            }
        };
        rowsPanel_->addAndMakeVisible (artnetDestRemoveButtons_[i]);
    }
    artnetAddIpButton_.setColour (juce::TextButton::buttonColourId, kInput);
    artnetAddIpButton_.setColour (juce::TextButton::buttonOnColourId, kInput);
    artnetAddIpButton_.setColour (juce::TextButton::textColourOffId, juce::Colour::fromRGB (0xca, 0xca, 0xca));
    artnetAddIpButton_.setColour (juce::TextButton::textColourOnId, juce::Colour::fromRGB (0xca, 0xca, 0xca));
    artnetAddIpButton_.onClick = [this]
    {
        if (artnetDestVisibleCount_ < (int) artnetDestIpEditors_.size())
        {
            artnetDestIpEditors_[(size_t) artnetDestVisibleCount_].setText ("255.255.255.255", juce::dontSendNotification);
            artnetTargetAdapterExpanded_[(size_t) artnetDestVisibleCount_] = false;
            artnetTargetAdapterCombos_[(size_t) artnetDestVisibleCount_].setSelectedItemIndex (0, juce::dontSendNotification);
            ++artnetDestVisibleCount_;
            updateArtnetIpControls();
            updateWindowHeight();
            resized();
            refreshConfigDirtyState();
        }
    };
    rowsPanel_->addAndMakeVisible (artnetAddIpButton_);
    ltcInDriverCombo_.onChange = [this]
    {
        refreshLtcDeviceListsByDriver();
        refreshLtcChannelCombos();
        onInputSettingsChanged();
        refreshConfigDirtyState();
    };
    ltcOutDriverCombo_.onChange = [this]
    {
        refreshLtcDeviceListsByDriver();
        refreshLtcChannelCombos();
        onOutputSettingsChanged();
        refreshConfigDirtyState();
    };
    ltcInDeviceCombo_.onChange = [this]
    {
        refreshLtcChannelCombos();
        onInputSettingsChanged();
        refreshConfigDirtyState();
    };
    ltcOutDeviceCombo_.onChange = [this]
    {
        refreshLtcChannelCombos();
        onOutputSettingsChanged();
        refreshConfigDirtyState();
    };
    oscAdapterCombo_.onChange = [this]
    {
        syncOscIpWithAdapter();
        onInputSettingsChanged();
    };

    for (auto* c : { &sourceCombo_ })
        c->onChange = [this]
        {
            restartSelectedSource();
            updateWindowHeight();
            resized();
            // MTC loop guard: re-check when source switches TO MTC so that a
            // pre-configured Out port that matches In is caught immediately.
            if (sourceCombo_.getSelectedId() == 2)
                onOutputSettingsChanged();
            refreshConfigDirtyState();
        };

    ltcOutSwitch_.onToggle = [this] (bool) { onOutputToggleChanged(); };
    mtcOutSwitch_.onToggle = [this] (bool) { onOutputToggleChanged(); };
    artnetOutSwitch_.onToggle = [this] (bool) { onOutputToggleChanged(); };

    for (auto* c : { &ltcOutDeviceCombo_, &ltcOutChannelCombo_, &ltcOutSampleRateCombo_, &mtcOutCombo_ })
        c->onChange = [this] { onOutputSettingsChanged(); refreshConfigDirtyState(); };

    // MTC loop guard: re-check when MTC In device changes while source is MTC.
    mtcInCombo_.onChange = [this]
    {
        onInputSettingsChanged();
        if (sourceCombo_.getSelectedId() == 2)
            onOutputSettingsChanged();
        refreshConfigDirtyState();
    };
    systemTimeFpsCombo_.onChange = [this]
    {
        onInputSettingsChanged();
        refreshConfigDirtyState();
    };
    rowsPanel_->addAndMakeVisible (ltcInGainSlider_);
    rowsPanel_->addAndMakeVisible (ltcInLevelBar_);
    rowsPanel_->addAndMakeVisible (ltcOutLevelSlider_);
    rowsPanel_->addAndMakeVisible (ltcOffsetEditor_);
    rowsPanel_->addAndMakeVisible (mtcOffsetEditor_);
    rowsPanel_->addAndMakeVisible (artnetOffsetEditor_);
    rowsPanel_->addAndMakeVisible (ltcConvertStrip_);
    rowsPanel_->addAndMakeVisible (mtcConvertStrip_);
    rowsPanel_->addAndMakeVisible (artnetConvertStrip_);
    rowsPanel_->addAndMakeVisible (oscIpEditor_);
    rowsPanel_->addAndMakeVisible (oscPortEditor_);
    rowsPanel_->addAndMakeVisible (systemTimeFpsCombo_);
    rowsPanel_->addAndMakeVisible (oscAddrStrEditor_);
    rowsPanel_->addAndMakeVisible (oscAddrFloatEditor_);
    rowsPanel_->addAndMakeVisible (oscFloatTypeCombo_);
    rowsPanel_->addAndMakeVisible (oscFloatMaxEditor_);
    rowsPanel_->addAndMakeVisible (artnetListenIpEditor_);
    for (auto& e : artnetDestIpEditors_)
        rowsPanel_->addAndMakeVisible (e);

    updateArtnetIpControls();

    // Viewport wraps the scrollable rows panel so the window never needs to
    // exceed the usable screen height — rows scroll instead of squishing.
    rowsViewport_.setScrollBarsShown (true, false);
    rowsViewport_.setScrollBarThickness (8);
    rowsViewport_.setViewedComponent (rowsPanel_.get(), false);
    addAndMakeVisible (rowsViewport_);

    setSize (430, calcPreferredHeight() + kStartupHeightExtra);
    resized();
    startTimerHz (60);
}

MainContentComponent::~MainContentComponent()
{
    // Must be cleared before the LookAndFeel instance is destroyed.
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
    {
        const std::lock_guard<std::mutex> lock (ltcOutputApplyMutex_);
        ltcOutputApplyExit_ = true;
        ltcOutputApplyPending_ = false;
    }
    ltcOutputApplyCv_.notify_all();
    if (ltcOutputApplyThread_.joinable())
        ltcOutputApplyThread_.join();

    if (scanThread_ != nullptr && scanThread_->isThreadRunning())
        scanThread_->stopThread (2000);
}

int MainContentComponent::calcPreferredHeight() const
{
    return calcHeightForState (
        sourceExpanded_,
        sourceCombo_.getSelectedId(),
        outLtcExpanded_,
        outMtcExpanded_,
        outArtExpanded_);
}

int MainContentComponent::calcHeightForState (bool sourceExpanded, int sourceId, bool outLtcExpanded, bool outMtcExpanded, bool outArtExpanded) const
{
    int h = 16; // outer margins
    h += 40 + 4;  // title
    h += 90;     // tc
    h += kCompactBarHeight + 4;  // tc fps

    auto addRows = [&h] (int count)
    {
        h += count * (40 + 4);
    };

    addRows (1); // source header
    if (sourceExpanded)
    {
        if (sourceId == 1) addRows (6);
        else if (sourceId == 2) addRows (1);
        else if (sourceId == 3) addRows (2);
        else if (sourceId == 4)
        {
            addRows (7); // adapter, ip, port, fps, str, float, floatType
            if (oscFloatTypeCombo_.getSelectedId() == 3)
                addRows (1); // floatMax (Normalized only)
        }
        else if (sourceId == 5) addRows (1);
    }

    addRows (1); // out LTC header
    if (outLtcExpanded) addRows (7);
    addRows (1); // out MTC header
    if (outMtcExpanded) addRows (3);
    addRows (1); // out ArtNet header
    if (outArtExpanded)
    {
        int artRows = 2; // convert + offset
        for (int i = 0; i < artnetDestVisibleCount_; ++i)
            artRows += 1 + (artnetTargetAdapterExpanded_[(size_t) i] ? 1 : 0);
        addRows (artRows);
    }

    h += 4; // gap before status
    h += kCompactBarHeight; // status
    h += 4; // gap
    h += 40; // settings/quit buttons
    return juce::jmax (420, h);  // no upper cap — height = sum of rows + gaps
}

void MainContentComponent::updateWindowHeight()
{
    const int currentContent = calcPreferredHeight();

    if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
    {
        if (window->isMinimised())
            return;

        auto* content = window->getContentComponent();
        const int chrome = content != nullptr ? (window->getHeight() - content->getHeight()) : 0;

        const int collapsedContent = calcHeightForState (false, sourceCombo_.getSelectedId(), false, false, false);
        const int expandedContent = calcHeightForState (true, sourceCombo_.getSelectedId(), true, true, true);

        const int minTotal   = collapsedContent + chrome;
        const int maxTotal   = expandedContent  + chrome; // used for auto-resize target only
        const int targetTotal = juce::jlimit (minTotal, maxTotal, currentContent + chrome);

        // Upper resize limit is left open (8192) so the user can always drag the
        // window taller than the auto-sized target; macOS/JUCE clamp to screen edge.
        window->setResizeLimits (430, minTotal, 430, 8192);
        if (window->getHeight() != targetTotal)
            window->setSize (window->getWidth(), targetTotal);
    }
    // On macOS with a native title bar, window->setSize() may not immediately
    // propagate updated bounds to the content component. Calling resized() here
    // guarantees the layout always reflects the current component size, regardless
    // of whether the OS has applied the window resize synchronously.
    resized();
}

void MainContentComponent::loadFontsAndIcon()
{
    // BUG-1 fix: use platform::findBridgeBaseDir() which searches for both
    // "EASYBRIDGE-JUSE" and "MTC_Bridge" — old code only searched "MTC_Bridge".
    const auto base = platform::findBridgeBaseDir();
    if (! base.exists())
        return;

    auto loadFont = [] (const juce::File& f) -> juce::Font
    {
        if (! f.existsAsFile())
            return {};
        juce::MemoryBlock data;
        f.loadFileAsData (data);
        auto tf = juce::Typeface::createSystemTypefaceFor (data.getData(), data.getSize());
        if (tf == nullptr)
            return {};
        return juce::Font (juce::FontOptions (tf));
    };

    titleEasyFont_ = loadFont (base.getChildFile ("Fonts/Thunder-SemiBoldLC.ttf"));
    titleBridgeFont_ = loadFont (base.getChildFile ("Fonts/Thunder-LightLC.ttf"));
    monoFont_ = loadFont (base.getChildFile ("Fonts/JetBrainsMonoNL-Bold.ttf"));

    auto iconFile = base.getChildFile ("Icons/App_Icon.png");
    if (iconFile.existsAsFile())
    {
        auto in = std::unique_ptr<juce::FileInputStream> (iconFile.createInputStream());
        if (in != nullptr)
            appIcon_ = juce::ImageFileFormat::loadFrom (*in);
    }
}

void MainContentComponent::applyLookAndFeel()
{
    lookAndFeel_ = std::make_unique<BridgeLookAndFeel>();
    lookAndFeel_->setColour (juce::ComboBox::backgroundColourId, kInput);
    lookAndFeel_->setColour (juce::ComboBox::textColourId, juce::Colour::fromRGB (0xca, 0xca, 0xca));
    lookAndFeel_->setColour (juce::ComboBox::outlineColourId, kRow);
    lookAndFeel_->setColour (juce::ComboBox::arrowColourId, juce::Colour::fromRGB (0x9a, 0xa1, 0xac));

    // Dropdown list style (close to PySide theme).
    lookAndFeel_->setColour (juce::PopupMenu::backgroundColourId, kInput);
    lookAndFeel_->setColour (juce::PopupMenu::textColourId, juce::Colour::fromRGB (0xca, 0xca, 0xca));
    lookAndFeel_->setColour (juce::PopupMenu::highlightedBackgroundColourId, kTeal);
    lookAndFeel_->setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    lookAndFeel_->setColour (juce::PopupMenu::headerTextColourId, juce::Colour::fromRGB (0xe4, 0xe4, 0xe4));

    // Scrollbar inside popup menu.
    lookAndFeel_->setColour (juce::ScrollBar::backgroundColourId, juce::Colour::fromRGB (0x1a, 0x1a, 0x1a));
    lookAndFeel_->setColour (juce::ScrollBar::thumbColourId, juce::Colour::fromRGB (0x5a, 0x5a, 0x5a));
    lookAndFeel_->setColour (juce::ScrollBar::trackColourId, juce::Colour::fromRGB (0x1a, 0x1a, 0x1a));

    lookAndFeel_->setColour (juce::TextEditor::backgroundColourId, kInput);
    lookAndFeel_->setColour (juce::TextEditor::textColourId, juce::Colour::fromRGB (0xca, 0xca, 0xca));
    lookAndFeel_->setColour (juce::TextEditor::outlineColourId, kRow);
    setLookAndFeel (lookAndFeel_.get());
    // Set as application-wide default so that PopupMenus (which create their own
    // top-level windows) also inherit the dark theme instead of falling back to
    // the system default LookAndFeel.
    juce::LookAndFeel::setDefaultLookAndFeel (lookAndFeel_.get());
}

void MainContentComponent::paint (juce::Graphics& g)
{
    g.fillAll (kBg);

    if (! headerRect_.isEmpty())
    {
        g.setColour (kHeader);
        g.fillRoundedRectangle (headerRect_.toFloat(), 5.0f);
        g.setColour (juce::Colour::fromRGB (0x3c, 0x3e, 0x42));
        g.drawRoundedRectangle (headerRect_.toFloat(), 5.0f, 1.0f);
    }

    if (! tcLabel_.getBounds().isEmpty())
    {
        const auto tcRect = tcLabel_.getBounds().toFloat();
        g.setColour (juce::Colour::fromRGB (0x1a, 0x1a, 0x1a));
        g.fillRoundedRectangle (tcRect, 5.0f);
        g.setColour (juce::Colour::fromRGB (0x33, 0x33, 0x33));
        g.drawRoundedRectangle (tcRect, 5.0f, 1.0f);
    }

    // Section and param row backgrounds are painted by RowsPanel::paint()
    // (which lives inside rowsViewport_).

    if (! buttonRowRect_.isEmpty())
    {
        g.setColour (juce::Colour::fromRGB (0x2f, 0x2f, 0x2f));
        g.drawRoundedRectangle (buttonRowRect_.toFloat(), 4.0f, 1.0f);
    }
}

void MainContentComponent::resized()
{
    constexpr int kBottomStripH = kTopSectionGap + kCompactBarHeight + kTopSectionGap + kBottomButtonsHeight;
    constexpr int kFixedTopH = kHeaderHeight + kTopSectionGap + kTimerHeight + kTopSectionGap
                             + kCompactBarHeight + kTopSectionGap;
    constexpr int kFixedChromeH = kOuterMargin * 2 + kFixedTopH + kBottomStripH;

    // Use ACTUAL window bounds — viewport handles any overflow by scrolling.
    auto a = getLocalBounds().reduced (kOuterMargin);
    auto bottomStrip = a.removeFromBottom (kBottomStripH);

    headerRect_ = a.removeFromTop (40);
    auto header = headerRect_.reduced (6, 0);
    auto help = header.removeFromRight (28);
    helpButton_.setBounds (help.withSizeKeepingCentre (28, 28));

    auto titleArea = header;
    const int easyW = juce::jmax (46, titleEasyLabel_.getFont().getStringWidth ("EASY ") + 6);
    const int bridgeW = juce::jmax (74, titleBridgeLabel_.getFont().getStringWidth ("BRIDGE") + 6);
    const int versionW = juce::jmax (58, titleVersionLabel_.getFont().getStringWidth (titleVersionLabel_.getText()) + 4);
    const int startX = titleArea.getX() + 2;
    const int titleYOffset = 6;
    const int titleH = juce::jmax (1, titleArea.getHeight() - titleYOffset);
    const int easyX = startX;
    const int bridgeX = easyX + easyW;
    const int versionX = bridgeX + bridgeW + 2;
    titleEasyLabel_.setBounds (easyX, titleArea.getY() + titleYOffset, easyW, titleH);
    titleBridgeLabel_.setBounds (bridgeX, titleArea.getY() + titleYOffset, bridgeW, titleH);
    titleVersionLabel_.setBounds (versionX, titleArea.getY() + titleYOffset, versionW, titleH);
    a.removeFromTop (4);
    tcLabel_.setBounds (a.removeFromTop (90));
    a.removeFromTop (4);
    if (fpsIndicatorStrip_ != nullptr)
        fpsIndicatorStrip_->setBounds (a.removeFromTop (kCompactBarHeight));
    a.removeFromTop (4);

    // ── Scrollable rows viewport ─────────────────────────────────────────────
    // `a` now holds the middle area between the fixed top and fixed bottom.
    // The RowsPanel is always sized to its preferred (full) height; the viewport
    // clips to the available screen area and lets the user scroll if needed.
    rowsViewport_.setBounds (a);
    const int rowsContentH = juce::jmax (0, calcPreferredHeight() - kFixedChromeH);
    // When the content is taller than the viewport a vertical scrollbar appears.
    // Reserve space for it so it does not overlap row widgets; when there is
    // no overflow the panel uses the full viewport width.
    const bool needsScrollbar = (rowsContentH > a.getHeight());
    const int panelW = needsScrollbar
                           ? a.getWidth() - rowsViewport_.getScrollBarThickness()
                           : a.getWidth();
    rowsPanel_->setSize (panelW, rowsContentH);

    // Row layout is now in RowsPanel-local coordinates (origin = top-left of panel).
    rowsPanel_->paramRowRects.clear();
    rowsPanel_->sectionRowRects.clear();
    auto rb = juce::Rectangle<int> (0, 0, rowsPanel_->getWidth(), rowsPanel_->getHeight());

    auto row = [&rb] (int h = 40)
    {
        auto r = rb.removeFromTop (h);
        rb.removeFromTop (4);
        return r;
    };
    auto fieldRow = [&] (juce::Label& lbl, juce::Component& editor)
    {
        auto r = row();
        rowsPanel_->paramRowRects.add (r);
        lbl.setVisible (true);
        editor.setVisible (true);
        auto labelArea = r.removeFromLeft (112);
        auto controlArea = r.reduced (0, 3);
        lbl.setBounds (labelArea.reduced (10, 0));
        if (&editor == &ltcInLevelBar_)
        {
            auto bar = controlArea.reduced (6, 0);
            const int h = 8; // thinner meter like the original UI
            bar = juce::Rectangle<int> (bar.getX(), bar.getCentreY() - h / 2, bar.getWidth(), h);
            editor.setBounds (bar);
        }
        else
        {
            editor.setBounds (controlArea.reduced (2, 0));
        }
    };
    auto hideRowLabels = [this]
    {
        juce::Label* labels[] = {
            &inDriverLbl_, &inDeviceLbl_, &inChannelLbl_, &inRateLbl_, &inLevelLbl_, &inGainLbl_,
            &mtcInLbl_, &artInLbl_, &artInListenIpLbl_, &oscAdapterLbl_, &oscIpLbl_, &oscPortLbl_, &oscFpsLbl_, &systemTimeFpsLbl_, &oscStrLbl_, &oscFloatLbl_, &oscFloatTypeLbl_, &oscFloatMaxLbl_,
            &outDriverLbl_, &outDeviceLbl_, &outChannelLbl_, &outRateLbl_, &outConvertLbl_, &outOffsetLbl_, &outLevelLbl_,
            &mtcOutLbl_, &mtcConvertLbl_, &mtcOffsetLbl_, &artConvertLbl_, &artOffsetLbl_,
            &artSendLbls_[0], &artSendLbls_[1], &artSendLbls_[2], &artSendLbls_[3], &artSendLbls_[4],
            &artAdapterLbls_[0], &artAdapterLbls_[1], &artAdapterLbls_[2], &artAdapterLbls_[3], &artAdapterLbls_[4]
        };
        for (auto* l : labels)
            l->setVisible (false);
    };
    hideRowLabels();

    auto sourceRow = row();
    rowsPanel_->sectionRowRects.add (sourceRow);
    auto sourceLabelZone = sourceRow.removeFromLeft (112);
    {
        auto btnHost = sourceLabelZone.removeFromLeft (36);
        const int d = 28;
        sourceExpandBtn_.setBounds (
            btnHost.getX() + 3 + (btnHost.getWidth() - d) / 2,
            btnHost.getY() + (btnHost.getHeight() - d) / 2,
            d, d);
    }
    sourceHeaderLabel_.setBounds (sourceLabelZone);
    sourceCombo_.setBounds (sourceRow.reduced (2, 3));
    sourceHeaderLabel_.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    sourceExpandBtn_.setExpanded (sourceExpanded_);

    const auto src = sourceCombo_.getSelectedId();
    if (sourceExpanded_ && src == 1)
    {
        fieldRow (inDriverLbl_, ltcInDriverCombo_);
        fieldRow (inDeviceLbl_, ltcInDeviceCombo_);
        fieldRow (inChannelLbl_, ltcInChannelCombo_);
        fieldRow (inRateLbl_, ltcInSampleRateCombo_);
        fieldRow (inLevelLbl_, ltcInLevelBar_);
        fieldRow (inGainLbl_, ltcInGainSlider_);
    }
    else if (sourceExpanded_ && src == 2)
    {
        fieldRow (mtcInLbl_, mtcInCombo_);
    }
    else if (sourceExpanded_ && src == 3)
    {
        fieldRow (artInLbl_, artnetInCombo_);
        fieldRow (artInListenIpLbl_, artnetListenIpEditor_);
    }
    else if (sourceExpanded_ && src == 4)
    {
        fieldRow (oscAdapterLbl_, oscAdapterCombo_);
        fieldRow (oscIpLbl_, oscIpEditor_);
        fieldRow (oscPortLbl_, oscPortEditor_);
        fieldRow (oscFpsLbl_, oscFpsCombo_);
        fieldRow (oscStrLbl_, oscAddrStrEditor_);
        fieldRow (oscFloatLbl_, oscAddrFloatEditor_);
        fieldRow (oscFloatTypeLbl_, oscFloatTypeCombo_);
        if (oscFloatTypeCombo_.getSelectedId() == 3)
            fieldRow (oscFloatMaxLbl_, oscFloatMaxEditor_);
        else
        {
            oscFloatMaxLbl_.setVisible (false);
            oscFloatMaxEditor_.setVisible (false);
        }
    }
    else if (sourceExpanded_ && src == 5)
        fieldRow (systemTimeFpsLbl_, systemTimeFpsCombo_);

    auto ltcHeader = row();
    rowsPanel_->sectionRowRects.add (ltcHeader);
    auto ltcHeaderCopy = ltcHeader;
    outLtcHeaderLabel_.setBounds (ltcHeaderCopy);
    {
        auto btnHost = ltcHeader.removeFromLeft (36);
        const int d = 28;
        outLtcExpandBtn_.setBounds (
            btnHost.getX() + 3 + (btnHost.getWidth() - d) / 2,
            btnHost.getY() + (btnHost.getHeight() - d) / 2,
            d, d);
    }
    ltcHeader.removeFromLeft (110);
    ltcOutSwitch_.setBounds (ltcHeader.removeFromRight (54).reduced (0, 6));
    {
        auto dotHost = ltcHeader.removeFromRight (22);
        const int d = 18;
        ltcThruDot_.setBounds (dotHost.getCentreX() - d / 2, dotHost.getCentreY() - d / 2, d, d);
    }
    ltcThruLbl_.setBounds (ltcHeader.removeFromRight (40));
    outLtcHeaderLabel_.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    outLtcExpandBtn_.setExpanded (outLtcExpanded_);

    if (outLtcExpanded_)
    {
        fieldRow (outDriverLbl_, ltcOutDriverCombo_);
        fieldRow (outDeviceLbl_, ltcOutDeviceCombo_);
        fieldRow (outChannelLbl_, ltcOutChannelCombo_);
        fieldRow (outRateLbl_, ltcOutSampleRateCombo_);
        fieldRow (outConvertLbl_, ltcConvertStrip_);
        fieldRow (outOffsetLbl_, ltcOffsetEditor_);
        fieldRow (outLevelLbl_, ltcOutLevelSlider_);
    }

    auto mtcHeader = row();
    rowsPanel_->sectionRowRects.add (mtcHeader);
    auto mtcHeaderCopy = mtcHeader;
    outMtcHeaderLabel_.setBounds (mtcHeaderCopy);
    {
        auto btnHost = mtcHeader.removeFromLeft (36);
        const int d = 28;
        outMtcExpandBtn_.setBounds (
            btnHost.getX() + 3 + (btnHost.getWidth() - d) / 2,
            btnHost.getY() + (btnHost.getHeight() - d) / 2,
            d, d);
    }
    mtcHeader.removeFromLeft (110);
    mtcOutSwitch_.setBounds (mtcHeader.removeFromRight (54).reduced (0, 6));
    outMtcHeaderLabel_.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    outMtcExpandBtn_.setExpanded (outMtcExpanded_);

    if (outMtcExpanded_)
    {
        fieldRow (mtcOutLbl_, mtcOutCombo_);
        fieldRow (mtcConvertLbl_, mtcConvertStrip_);
        fieldRow (mtcOffsetLbl_, mtcOffsetEditor_);
    }

    auto artHeader = row();
    rowsPanel_->sectionRowRects.add (artHeader);
    auto artHeaderCopy = artHeader;
    outArtHeaderLabel_.setBounds (artHeaderCopy);
    {
        auto btnHost = artHeader.removeFromLeft (36);
        const int d = 28;
        outArtExpandBtn_.setBounds (
            btnHost.getX() + 3 + (btnHost.getWidth() - d) / 2,
            btnHost.getY() + (btnHost.getHeight() - d) / 2,
            d, d);
    }
    artHeader.removeFromLeft (110);
    artnetOutSwitch_.setBounds (artHeader.removeFromRight (54).reduced (0, 6));
    outArtHeaderLabel_.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    outArtExpandBtn_.setExpanded (outArtExpanded_);
    if (outArtExpanded_)
    {
        for (int i = 0; i < artnetDestVisibleCount_; ++i)
        {
            auto targetRow = row();
            rowsPanel_->paramRowRects.add (targetRow);

            auto arrowHost = targetRow.removeFromLeft (36);
            const int arrowSize = 28;
            artnetTargetExpandButtons_[(size_t) i].setBounds (
                arrowHost.getX() + 3 + (arrowHost.getWidth() - arrowSize) / 2,
                arrowHost.getY() + (arrowHost.getHeight() - arrowSize) / 2,
                arrowSize, arrowSize);
            artnetTargetExpandButtons_[(size_t) i].setExpanded (artnetTargetAdapterExpanded_[(size_t) i]);

            auto labelArea = targetRow.removeFromLeft (76);
            artSendLbls_[(size_t) i].setBounds (labelArea.reduced (6, 0));

            auto controlArea = targetRow.reduced (2, 3);
            auto actionArea = controlArea.removeFromRight (40);
            auto portArea = controlArea.removeFromRight (0);
            juce::ignoreUnused (portArea);
            artnetDestIpEditors_[(size_t) i].setBounds (controlArea);

            if (i == 0)
                artnetAddIpButton_.setBounds (actionArea.reduced (2, 0));
            else
                artnetDestRemoveButtons_[(size_t) (i - 1)].setBounds (actionArea.reduced (2, 0));

            if (artnetTargetAdapterExpanded_[(size_t) i])
            {
                auto adapterRow = row();
                rowsPanel_->paramRowRects.add (adapterRow);
                auto spacer = adapterRow.removeFromLeft (36);
                juce::ignoreUnused (spacer);
                auto adapterLabelArea = adapterRow.removeFromLeft (76);
                artAdapterLbls_[(size_t) i].setBounds (adapterLabelArea.reduced (6, 0));
                artnetTargetAdapterCombos_[(size_t) i].setBounds (adapterRow.reduced (2, 3));
            }
        }
        fieldRow (artConvertLbl_, artnetConvertStrip_);
        fieldRow (artOffsetLbl_, artnetOffsetEditor_);
    }

    // Redraw the rows panel so updated background rects are visible.
    rowsPanel_->repaint();

    // Status and buttons: laid out in bottom strip (pinned to actual window bottom).
    bottomStrip.removeFromTop (4);
    statusRect_ = bottomStrip.removeFromTop (kCompactBarHeight);
    bottomStrip.removeFromTop (4);
    buttonRowRect_ = bottomStrip.removeFromTop (40);

    statusButton_.setBounds (statusRect_);
    auto buttons = buttonRowRect_;
    settingsButton_.setBounds (buttons.removeFromLeft (buttons.getWidth() / 2).reduced (1, 0));
    quitButton_.setBounds (buttons.reduced (1, 0));

    // BUG-6 fix: removed the redundant setVisible block here that was immediately
    // overridden by the sourceExpanded_ && src == N block that follows.
    auto hideAll = [this]
    {
        juce::Component* comps[] = {
            &ltcInDriverCombo_, &ltcInDeviceCombo_, &ltcInChannelCombo_, &ltcInSampleRateCombo_, &ltcInLevelBar_, &ltcInGainSlider_,
            &mtcInCombo_, &artnetInCombo_, &artnetListenIpEditor_, &oscAdapterCombo_, &oscIpEditor_, &oscPortEditor_, &oscFpsCombo_, &systemTimeFpsCombo_, &oscAddrStrEditor_, &oscAddrFloatEditor_, &oscFloatTypeCombo_, &oscFloatMaxEditor_,
            &ltcConvertStrip_, &mtcConvertStrip_, &artnetConvertStrip_
        };
        for (auto* c : comps)
            if (c != nullptr)
                c->setVisible (false);
    };
    hideAll();
    ltcInDriverCombo_.setVisible (sourceExpanded_ && src == 1);
    ltcInDeviceCombo_.setVisible (sourceExpanded_ && src == 1);
    ltcInChannelCombo_.setVisible (sourceExpanded_ && src == 1);
    ltcInSampleRateCombo_.setVisible (sourceExpanded_ && src == 1);
    ltcInLevelBar_.setVisible (sourceExpanded_ && src == 1);
    ltcInGainSlider_.setVisible (sourceExpanded_ && src == 1);
    mtcInCombo_.setVisible (sourceExpanded_ && src == 2);
    artnetInCombo_.setVisible (sourceExpanded_ && src == 3);
    artnetListenIpEditor_.setVisible (sourceExpanded_ && src == 3);
    oscAdapterCombo_.setVisible (sourceExpanded_ && src == 4);
    oscIpEditor_.setVisible (sourceExpanded_ && src == 4);
    oscPortEditor_.setVisible (sourceExpanded_ && src == 4);
    oscFpsCombo_.setVisible (sourceExpanded_ && src == 4);
    systemTimeFpsCombo_.setVisible (sourceExpanded_ && src == 5);
    oscAddrStrEditor_.setVisible (sourceExpanded_ && src == 4);
    oscAddrFloatEditor_.setVisible (sourceExpanded_ && src == 4);
    oscFloatTypeCombo_.setVisible (sourceExpanded_ && src == 4);
    oscFloatMaxEditor_.setVisible (sourceExpanded_ && src == 4 && oscFloatTypeCombo_.getSelectedId() == 3);

    ltcOutDriverCombo_.setVisible (outLtcExpanded_);
    ltcOutDeviceCombo_.setVisible (outLtcExpanded_);
    ltcOutChannelCombo_.setVisible (outLtcExpanded_);
    ltcOutSampleRateCombo_.setVisible (outLtcExpanded_);
    ltcConvertStrip_.setVisible (outLtcExpanded_);
    ltcOffsetEditor_.setVisible (outLtcExpanded_);
    ltcOutLevelSlider_.setVisible (outLtcExpanded_);

    mtcOutCombo_.setVisible (outMtcExpanded_);
    mtcConvertStrip_.setVisible (outMtcExpanded_);
    mtcOffsetEditor_.setVisible (outMtcExpanded_);

    artnetAddIpButton_.setVisible (outArtExpanded_);
    artnetConvertStrip_.setVisible (outArtExpanded_);
    for (int i = 0; i < (int) artnetDestIpEditors_.size(); ++i)
    {
        const bool rowVisible = outArtExpanded_ && i < artnetDestVisibleCount_;
        artnetTargetExpandButtons_[(size_t) i].setVisible (rowVisible);
        artSendLbls_[(size_t) i].setVisible (rowVisible);
        artnetDestIpEditors_[(size_t) i].setVisible (rowVisible);
        artAdapterLbls_[(size_t) i].setVisible (rowVisible && artnetTargetAdapterExpanded_[(size_t) i]);
        artnetTargetAdapterCombos_[(size_t) i].setVisible (rowVisible && artnetTargetAdapterExpanded_[(size_t) i]);
    }
    for (int i = 0; i < (int) artnetDestRemoveButtons_.size(); ++i)
        artnetDestRemoveButtons_[(size_t) i].setVisible (outArtExpanded_ && (i + 1) < artnetDestVisibleCount_);
    artnetOffsetEditor_.setVisible (outArtExpanded_);
}

void MainContentComponent::restartSelectedSource()
{
    const int src = sourceCombo_.getSelectedId();
    if (src == 1)
        bridgeEngine_.setInputSource (engine::InputSource::LTC);
    else if (src == 2)
        bridgeEngine_.setInputSource (engine::InputSource::MTC);
    else if (src == 3)
        bridgeEngine_.setInputSource (engine::InputSource::ArtNet);
    else if (src == 4)
        bridgeEngine_.setInputSource (engine::InputSource::OSC);
    else
        bridgeEngine_.setInputSource (engine::InputSource::SystemTime);
}

void MainContentComponent::queueLtcOutputApply()
{
    const bool enabled = ltcOutSwitch_.getState();

    const int selectedId = ltcOutDeviceCombo_.getSelectedId();
    const int idx = selectedId > 0 ? selectedId - 1 : -1;
    if (! juce::isPositiveAndBelow (idx, filteredOutputIndices_.size()))
    {
        if (! enabled)
            bridgeEngine_.setLtcOutputEnabled (false);
        return;
    }

    {
        const std::lock_guard<std::mutex> lock (ltcOutputApplyMutex_);
        pendingLtcOutputChoice_ = outputChoices_[filteredOutputIndices_[idx]];
        pendingLtcOutputChannel_ = comboChannelIndex (ltcOutChannelCombo_);
        pendingLtcOutputSampleRate_ = comboSampleRate (ltcOutSampleRateCombo_);
        pendingLtcOutputBufferSize_ = kLtcOutputBufferSize; // BUG-7 fix: named constant
        pendingLtcOutputEnabled_ = enabled;
        pendingLtcThruMode_ = ltcThruDot_.getState();
        ltcOutputApplyPending_ = true;
    }

    ltcOutputApplyCv_.notify_one();
}

void MainContentComponent::ltcOutputApplyLoop()
{
    auto safeThis = juce::Component::SafePointer<MainContentComponent> (this);

    for (;;)
    {
        engine::AudioChoice choice;
        int channel = 0;
        double sampleRate = 0.0;
        int bufferSize = 0;
        bool enabled = false;
        bool thruMode = false;

        {
            std::unique_lock<std::mutex> lock (ltcOutputApplyMutex_);
            ltcOutputApplyCv_.wait (lock, [this] { return ltcOutputApplyExit_ || ltcOutputApplyPending_; });
            if (ltcOutputApplyExit_)
                break;

            choice = pendingLtcOutputChoice_;
            channel = pendingLtcOutputChannel_;
            sampleRate = pendingLtcOutputSampleRate_;
            bufferSize = pendingLtcOutputBufferSize_;
            enabled = pendingLtcOutputEnabled_;
            thruMode = pendingLtcThruMode_;
            ltcOutputApplyPending_ = false;
        }

        juce::String err;
        if (thruMode && enabled)
        {
            // Thru ON + switch ON → stop normal output, run passthrough
            bridgeEngine_.stopLtcOutput();
            bridgeEngine_.startLtcThru (choice, channel, sampleRate, bufferSize, err);
            if (err.isNotEmpty())
                juce::MessageManager::callAsync ([safeThis, err]
                {
                    if (safeThis != nullptr)
                    {
                        safeThis->setStatusText (err, juce::Colour::fromRGB (0xde, 0x9b, 0x3c));
                        safeThis->ltcThruDot_.setState (false);
                    }
                });
        }
        else if (thruMode && !enabled)
        {
            // Thru ON + switch OFF → stop both, nothing runs
            bridgeEngine_.stopLtcThru();
            bridgeEngine_.stopLtcOutput();
        }
        else
        {
            // Normal mode (Thru OFF) → stop thru, run normal output
            bridgeEngine_.stopLtcThru();
            bridgeEngine_.startLtcOutput (choice, channel, sampleRate, bufferSize, err);
            bridgeEngine_.setLtcOutputEnabled (enabled);
            if (err.isNotEmpty())
                juce::MessageManager::callAsync ([safeThis, err]
                {
                    if (safeThis != nullptr)
                        safeThis->setStatusText (err, juce::Colour::fromRGB (0xde, 0x9b, 0x3c));
                });
        }
    }
}

void MainContentComponent::onOutputToggleChanged()
{
    bridgeEngine_.setLtcOutputEnabled (ltcOutSwitch_.getState());
    queueLtcOutputApply();

    bridgeEngine_.setMtcOutputEnabled (mtcOutSwitch_.getState());
    bridgeEngine_.setArtnetOutputEnabled (artnetOutSwitch_.getState());
    refreshConfigDirtyState();
}

void MainContentComponent::onOutputSettingsChanged()
{
    juce::String err;
    bridgeEngine_.setLtcOutputConvertFps (ltcConvertStrip_.getSelectedFps());
    bridgeEngine_.setMtcOutputConvertFps (mtcConvertStrip_.getSelectedFps());
    bridgeEngine_.setArtnetOutputConvertFps (artnetConvertStrip_.getSelectedFps());
    queueLtcOutputApply();

    if (mtcOutCombo_.getNumItems() > 0)
    {
        // MTC loop guard: Out device cannot equal In device
        const auto mtcInName  = mtcInCombo_.getText();
        const auto mtcOutName = mtcOutCombo_.getText();
        bool skipMtcStart = false;
        if (sourceCombo_.getSelectedId() == 2  // only guard when MTC is the active source
            && mtcInCombo_.getNumItems() > 0
            && mtcInName == mtcOutName
            && mtcInName.isNotEmpty())
        {
            bool switched = false;
            for (int i = 0; i < mtcOutCombo_.getNumItems(); ++i)
            {
                if (mtcOutCombo_.getItemText (i) != mtcInName)
                {
                    mtcOutCombo_.setSelectedItemIndex (i, juce::dontSendNotification);
                    setStatusText ("MTC Out: auto-switched (same device as MTC In)",
                                   juce::Colour::fromRGB (0xde, 0x9b, 0x3c));
                    switched = true;
                    break;
                }
            }
            if (! switched)
            {
                mtcOutSwitch_.setState (false);
                bridgeEngine_.setMtcOutputEnabled (false);
                DarkDialog::show ("MTC Output",
                    "MTC In and MTC Out cannot use the same device.\n"
                    "No other MIDI device is available.\nMTC Output has been disabled.",
                    this);
                skipMtcStart = true;
            }
        }
        if (! skipMtcStart)
            bridgeEngine_.startMtcOutput (mtcOutCombo_.getSelectedItemIndex(), err);
    }

    bridgeEngine_.startArtnetOutput (collectArtnetTargets(), err);

    bridgeEngine_.setMtcOutputEnabled (mtcOutSwitch_.getState());
    bridgeEngine_.setArtnetOutputEnabled (artnetOutSwitch_.getState());

    if (err.isNotEmpty())
        setStatusText (err, juce::Colour::fromRGB (0xde, 0x9b, 0x3c));
}

void MainContentComponent::onInputSettingsChanged()
{
    // BUG-4 fix: all inputs are kept running (so source switching is instant),
    // but errors are only reported for the currently active source.
    const int activeSrc = sourceCombo_.getSelectedId();
    juce::String ltcErr, mtcErr, artErr, oscErr;

    const int ltcSelectedId = ltcInDeviceCombo_.getSelectedId();
    const int ltcIdx = ltcSelectedId > 0 ? ltcSelectedId - 1 : -1;
    if (juce::isPositiveAndBelow (ltcIdx, filteredInputIndices_.size()))
        bridgeEngine_.startLtcInput (inputChoices_[filteredInputIndices_[ltcIdx]],
                                     comboChannelIndex (ltcInChannelCombo_),
                                     comboSampleRate (ltcInSampleRateCombo_),
                                     0,
                                     ltcErr);

    if (mtcInCombo_.getNumItems() > 0)
        bridgeEngine_.startMtcInput (mtcInCombo_.getSelectedItemIndex(), mtcErr);

    if (artnetInCombo_.getNumItems() > 0)
    {
        const auto artnetListenIp = artnetListenIpEditor_.getText().trim();
        bridgeEngine_.startArtnetInput (artnetInCombo_.getSelectedItemIndex(),
                                        (artnetListenIp == "0.0.0.0" ? juce::String() : artnetListenIp),
                                        artErr);
    }

    FrameRate fps = FrameRate::FPS_25;
    if (oscFpsCombo_.getSelectedId() == 1) fps = FrameRate::FPS_24;
    if (oscFpsCombo_.getSelectedId() == 3) fps = FrameRate::FPS_2997;
    if (oscFpsCombo_.getSelectedId() == 4) fps = FrameRate::FPS_30;
    const auto bindIp = (oscIpEditor_.getText().trim().isNotEmpty() ? oscIpEditor_.getText().trim() : parseBindIpFromAdapterLabel (oscAdapterCombo_.getText()));
    const auto floatVt  = static_cast<bridge::engine::OscValueType> (oscFloatTypeCombo_.getSelectedId() - 1);
    // Accept both \"7.15\" and \"7,15\" by normalising comma to dot before parsing.
    auto floatMaxRaw = oscFloatMaxEditor_.getText().trim();
    juce::String floatMaxText;
    for (int i = 0; i < floatMaxRaw.length(); ++i)
    {
        auto ch = floatMaxRaw[i];
        if (ch == ',')
            ch = '.';
        floatMaxText += ch;
    }
    const double floatMax = juce::jmax (1.0, floatMaxText.getDoubleValue());
    bridgeEngine_.startOscInput (juce::jlimit (1, 65535, oscPortEditor_.getText().getIntValue()),
                                 bindIp,
                                 fps,
                                 oscAddrStrEditor_.getText(),
                                 oscAddrFloatEditor_.getText(),
                                 floatVt,
                                 floatMax,
                                 oscErr);

    FrameRate systemTimeFps = FrameRate::FPS_25;
    if (systemTimeFpsCombo_.getSelectedId() == 1) systemTimeFps = FrameRate::FPS_2398;
    if (systemTimeFpsCombo_.getSelectedId() == 2) systemTimeFps = FrameRate::FPS_24;
    if (systemTimeFpsCombo_.getSelectedId() == 4) systemTimeFps = FrameRate::FPS_2997;
    if (systemTimeFpsCombo_.getSelectedId() == 5) systemTimeFps = FrameRate::FPS_30;
    bridgeEngine_.setSystemTimeFps (systemTimeFps);

    restartSelectedSource();

    // Only surface the error for whichever source is currently selected.
    const juce::String activeErr = (activeSrc == 1) ? ltcErr
                                 : (activeSrc == 2) ? mtcErr
                                 : (activeSrc == 3) ? artErr
                                 :                    oscErr;
    if (activeErr.isNotEmpty())
        setStatusText (activeErr, juce::Colour::fromRGB (0xde, 0x9b, 0x3c));
}

void MainContentComponent::timerCallback()
{
    bridgeEngine_.setLtcInputGain (dbToLinearGain (ltcInGainSlider_.getValue()));
    bridgeEngine_.setLtcOutputGain (dbToLinearGain (ltcOutLevelSlider_.getValue()));
    bridgeEngine_.setOffsets (
        offsetFromEditor (ltcOffsetEditor_),
        offsetFromEditor (mtcOffsetEditor_),
        offsetFromEditor (artnetOffsetEditor_));
    auto st = bridgeEngine_.tick();

    const float peak = bridgeEngine_.getLtcInputPeakLevel();
    // Instant attack with exponential decay for responsive level metering.
    ltcInLevelSmoothed_ = (peak > ltcInLevelSmoothed_) ? peak : (ltcInLevelSmoothed_ * 0.85f);
    ltcInLevelBar_.setLevel (ltcInLevelSmoothed_);

    if (st.hasInputTc)
    {
        hasLatchedTc_ = true;
        latchedTc_ = st.inputTc;
        latchedFps_ = st.inputFps;
        tcLabel_.setText (st.inputTc.toDisplayString (st.inputFps).replaceCharacter ('.', ':'), juce::dontSendNotification);
        if (fpsIndicatorStrip_ != nullptr)
            fpsIndicatorStrip_->setActiveFps (st.inputFps);
        juce::Array<StatusSegment> segments;
        const auto runningColour = juce::Colour::fromRGB (0x51, 0xc8, 0x7b);
        segments.add ({ "RUNNING | ", runningColour });
        segments.add ({ "LTC " + outputStatusWithFps (st.ltcOutStatus, st.ltcOutFps), isConvertActive (st.ltcOutStatus, st.ltcOutFps, st.inputFps) ? kOrange : runningColour });
        segments.add ({ " | ", runningColour });
        segments.add ({ "MTC " + outputStatusWithFps (st.mtcOutStatus, st.mtcOutFps), isConvertActive (st.mtcOutStatus, st.mtcOutFps, st.inputFps) ? kOrange : runningColour });
        segments.add ({ " | ", runningColour });
        segments.add ({ "ArtNet " + outputStatusWithFps (st.artnetOutStatus, st.artnetOutFps), isConvertActive (st.artnetOutStatus, st.artnetOutFps, st.inputFps) ? kOrange : runningColour });
        setStatusSegments (std::move (segments));
    }
    else
    {
        if (hasLatchedTc_)
        {
            tcLabel_.setText (latchedTc_.toDisplayString (latchedFps_).replaceCharacter ('.', ':'), juce::dontSendNotification);
            if (fpsIndicatorStrip_ != nullptr)
                fpsIndicatorStrip_->setActiveFps (latchedFps_);
        }
        else
        {
            tcLabel_.setText ("00:00:00:00", juce::dontSendNotification);
            if (fpsIndicatorStrip_ != nullptr)
                fpsIndicatorStrip_->setActiveFps (std::nullopt);
        }

        if (sourceCombo_.getSelectedId() == 6 && st.inputStatus.startsWithIgnoreCase ("LINK"))
            setStatusText (st.inputStatus, juce::Colour::fromRGB (0xde, 0x9b, 0x3c));
        else if (statusButton_.getText().startsWithIgnoreCase ("RUNNING"))
            setStatusText ("STOPPED - no timecode", juce::Colour::fromRGB (0xec, 0x48, 0x3c));
    }
}

void MainContentComponent::refreshDeviceLists()
{
    startAudioDeviceScan();
}

void MainContentComponent::startAudioDeviceScan()
{
    if (scanThread_ != nullptr && scanThread_->isThreadRunning())
    {
        if (! scanThread_->stopThread (2000))
            return;
    }

    scanThread_ = std::make_unique<BridgeAudioScanThread> (this);
    scanThread_->startThread();
}

void MainContentComponent::onAudioScanComplete (const juce::Array<engine::AudioChoice>& inputs,
                                                const juce::Array<engine::AudioChoice>& outputs)
{
    const auto prevInDriver = ltcInDriverCombo_.getText();
    const auto prevOutDriver = ltcOutDriverCombo_.getText();

    inputChoices_ = inputs;
    outputChoices_ = outputs;

    fillDriverCombo (ltcInDriverCombo_, inputChoices_, prevInDriver);
    fillDriverCombo (ltcOutDriverCombo_, outputChoices_, prevOutDriver);

    refreshLtcDeviceListsByDriver();
    refreshLtcChannelCombos();
    refreshNetworkMidiLists();

    onInputSettingsChanged();
    onOutputSettingsChanged();
}

void MainContentComponent::refreshLtcDeviceListsByDriver()
{
    juce::String prevInType, prevInDev;
    juce::String prevOutType, prevOutDev;

    const int prevInId = ltcInDeviceCombo_.getSelectedId();
    const int prevInIdx = prevInId > 0 ? prevInId - 1 : -1;
    if (juce::isPositiveAndBelow (prevInIdx, filteredInputIndices_.size()))
    {
        const int realIdx = filteredInputIndices_[prevInIdx];
        if (juce::isPositiveAndBelow (realIdx, inputChoices_.size()))
        {
            prevInType = inputChoices_[realIdx].typeName;
            prevInDev = inputChoices_[realIdx].deviceName;
        }
    }

    const int prevOutId = ltcOutDeviceCombo_.getSelectedId();
    const int prevOutIdx = prevOutId > 0 ? prevOutId - 1 : -1;
    if (juce::isPositiveAndBelow (prevOutIdx, filteredOutputIndices_.size()))
    {
        const int realIdx = filteredOutputIndices_[prevOutIdx];
        if (juce::isPositiveAndBelow (realIdx, outputChoices_.size()))
        {
            prevOutType = outputChoices_[realIdx].typeName;
            prevOutDev = outputChoices_[realIdx].deviceName;
        }
    }

    filteredInputChoices_.clear();
    filteredOutputChoices_.clear();
    filteredInputIndices_.clear();
    filteredOutputIndices_.clear();

    for (int i = 0; i < inputChoices_.size(); ++i)
    {
        const auto& c = inputChoices_[i];
        if (matchesDriverFilter (ltcInDriverCombo_.getText(), c.typeName))
        {
            filteredInputChoices_.add (c);
            filteredInputIndices_.add (i);
        }
    }

    for (int i = 0; i < outputChoices_.size(); ++i)
    {
        const auto& c = outputChoices_[i];
        if (matchesDriverFilter (ltcOutDriverCombo_.getText(), c.typeName))
        {
            filteredOutputChoices_.add (c);
            filteredOutputIndices_.add (i);
        }
    }

    fillAudioCombo (ltcInDeviceCombo_, filteredInputChoices_);
    fillAudioCombo (ltcOutDeviceCombo_, filteredOutputChoices_);

    if (prevInDev.isNotEmpty())
    {
        const int idx = findFilteredIndex (filteredInputIndices_, inputChoices_, prevInType, prevInDev);
        if (idx >= 0)
            ltcInDeviceCombo_.setSelectedId (idx + 1, juce::dontSendNotification);
    }

    if (prevOutDev.isNotEmpty())
    {
        const int idx = findFilteredIndex (filteredOutputIndices_, outputChoices_, prevOutType, prevOutDev);
        if (idx >= 0)
            ltcOutDeviceCombo_.setSelectedId (idx + 1, juce::dontSendNotification);
    }
}

void MainContentComponent::refreshLtcChannelCombos()
{
    const auto refill = [] (juce::ComboBox& combo, int channelCount)
    {
        const auto previousText = combo.getText();
        fillChannelCombo (combo, channelCount);
        if (previousText.isNotEmpty())
            selectComboItemByText (combo, previousText);
    };

    int inputChannelCount = 0;
    const int inSelectedId = ltcInDeviceCombo_.getSelectedId();
    const int inIdx = inSelectedId > 0 ? inSelectedId - 1 : -1;
    if (juce::isPositiveAndBelow (inIdx, filteredInputIndices_.size()))
    {
        const int realIdx = filteredInputIndices_[inIdx];
        if (juce::isPositiveAndBelow (realIdx, inputChoices_.size()))
            inputChannelCount = inputChoices_[realIdx].channelCount;
    }
    refill (ltcInChannelCombo_, inputChannelCount);

    int outputChannelCount = 0;
    const int outSelectedId = ltcOutDeviceCombo_.getSelectedId();
    const int outIdx = outSelectedId > 0 ? outSelectedId - 1 : -1;
    if (juce::isPositiveAndBelow (outIdx, filteredOutputIndices_.size()))
    {
        const int realIdx = filteredOutputIndices_[outIdx];
        if (juce::isPositiveAndBelow (realIdx, outputChoices_.size()))
            outputChannelCount = outputChoices_[realIdx].channelCount;
    }
    refill (ltcOutChannelCombo_, outputChannelCount);
}

void MainContentComponent::refreshNetworkMidiLists()
{
    mtcInCombo_.clear();
    auto ins = bridgeEngine_.midiInputs();
    for (int i = 0; i < ins.size(); ++i)
        mtcInCombo_.addItem (ins[i], i + 1);
    if (mtcInCombo_.getNumItems() > 0)
        mtcInCombo_.setSelectedItemIndex (0, juce::dontSendNotification);

    mtcOutCombo_.clear();
    auto outs = bridgeEngine_.midiOutputs();
    for (int i = 0; i < outs.size(); ++i)
        mtcOutCombo_.addItem (outs[i], i + 1);
    if (mtcOutCombo_.getNumItems() > 0)
        mtcOutCombo_.setSelectedItemIndex (0, juce::dontSendNotification);

    auto ifaces = bridgeEngine_.artnetInterfaces();
    artnetInCombo_.clear();
    oscAdapterCombo_.clear();
    oscAdapterCombo_.addItem ("ALL INTERFACES (0.0.0.0)", 1);
    oscAdapterCombo_.addItem ("Loopback (127.0.0.1)", 2);
    auto refillArtAdapterCombo = [] (juce::ComboBox& combo, const juce::String& previousText, const juce::StringArray& interfaceNames)
    {
        combo.clear (juce::dontSendNotification);
        combo.addItem ("ALL INTERFACES (0.0.0.0)", 1);
        int itemId = 2;
        for (int i = 0; i < interfaceNames.size(); ++i)
        {
            if (interfaceNames[i].startsWithIgnoreCase ("ALL INTERFACES"))
                continue;
            combo.addItem (interfaceNames[i], itemId++);
        }

        bool restored = false;
        if (previousText.isNotEmpty())
        {
            for (int i = 0; i < combo.getNumItems(); ++i)
            {
                if (combo.getItemText (i) == previousText)
                {
                    combo.setSelectedItemIndex (i, juce::dontSendNotification);
                    restored = true;
                    break;
                }
            }
        }
        if (! restored && combo.getNumItems() > 0)
            combo.setSelectedItemIndex (0, juce::dontSendNotification);
    };
    for (int i = 0; i < ifaces.size(); ++i)
    {
        artnetInCombo_.addItem (ifaces[i], i + 1);
        if (! ifaces[i].startsWithIgnoreCase ("ALL INTERFACES"))
            oscAdapterCombo_.addItem (ifaces[i], i + 3);
    }
    if (artnetInCombo_.getNumItems() > 0)
        artnetInCombo_.setSelectedItemIndex (0, juce::dontSendNotification);
    if (oscAdapterCombo_.getNumItems() > 0)
        oscAdapterCombo_.setSelectedItemIndex (0, juce::dontSendNotification);
    for (auto& combo : artnetTargetAdapterCombos_)
    {
        const auto previous = combo.getText();
        refillArtAdapterCombo (combo, previous, ifaces);
    }
    syncOscIpWithAdapter();
}

void MainContentComponent::fillAudioCombo (juce::ComboBox& combo, const juce::Array<engine::AudioChoice>& choices)
{
    combo.clear();
    if (choices.isEmpty())
    {
        combo.addItem ("(No audio devices)", kPlaceholderItemId);
        combo.setSelectedId (kPlaceholderItemId, juce::dontSendNotification);
        return;
    }

    for (int i = 0; i < choices.size(); ++i)
        combo.addItem (choices[i].displayName, i + 1);
    combo.setSelectedId (1, juce::dontSendNotification);
}

double MainContentComponent::comboSampleRate (const juce::ComboBox& combo)
{
    const auto text = combo.getText().trim();
    if (text.startsWithIgnoreCase ("default"))
        return 0.0;
    return juce::jmax (0.0, text.getDoubleValue());
}

int MainContentComponent::comboBufferSize (const juce::ComboBox& combo)
{
    auto v = combo.getText().getIntValue();
    return v <= 0 ? 0 : v;
}

int MainContentComponent::comboChannelIndex (const juce::ComboBox& combo)
{
    const int selectedId = combo.getSelectedId();
    if (selectedId >= 1000)
        return -2 - (selectedId - 1000);
    if (selectedId == kPlaceholderItemId)
        return 0;
    return juce::jmax (0, combo.getSelectedItemIndex());
}

int MainContentComponent::offsetFromEditor (const juce::TextEditor& editor)
{
    return juce::jlimit (-30, 30, editor.getText().getIntValue());
}

// styleCombo / styleEditor / styleSlider are now free functions in
// ui/style/StyleHelpers.h (included via MainWindow.h) — no member impls needed.

void MainContentComponent::syncOscIpWithAdapter()
{
    const auto ip = parseBindIpFromAdapterLabel (oscAdapterCombo_.getText());
    const auto lockIp = (ip != "0.0.0.0");
    if (lockIp)
        oscIpEditor_.setText (ip, juce::dontSendNotification);
    else if (oscIpEditor_.getText().trim().isEmpty() || oscIpEditor_.getText().trim() == "127.0.0.1")
        oscIpEditor_.setText ("0.0.0.0", juce::dontSendNotification);
    oscIpEditor_.setReadOnly (lockIp);
}

void MainContentComponent::updateArtnetIpControls()
{
    artnetDestVisibleCount_ = juce::jlimit (1, (int) artnetDestIpEditors_.size(), artnetDestVisibleCount_);
    artnetAddIpButton_.setEnabled (artnetDestVisibleCount_ < (int) artnetDestIpEditors_.size());
    for (int i = 0; i < artnetDestVisibleCount_; ++i)
        artnetTargetExpandButtons_[(size_t) i].setExpanded (artnetTargetAdapterExpanded_[(size_t) i]);
}

juce::Array<engine::ArtnetTarget> MainContentComponent::collectArtnetTargets() const
{
    juce::Array<engine::ArtnetTarget> out;
    juce::StringArray seen;
    for (int i = 0; i < artnetDestVisibleCount_ && i < (int) artnetDestIpEditors_.size(); ++i)
    {
        const auto ip = artnetDestIpEditors_[(size_t) i].getText().trim();
        if (ip.isEmpty())
            continue;

        const int interfaceIndex = artnetTargetAdapterCombos_[(size_t) i].getSelectedItemIndex() - 1;
        const auto key = juce::String (interfaceIndex) + "|" + ip;
        if (seen.contains (key))
            continue;

        seen.add (key);
        out.add ({ interfaceIndex, ip });
    }
    if (out.isEmpty())
        out.add ({ -1, "255.255.255.255" });
    return out;
}

// findBridgeBaseDir() instance method removed — use platform::findBridgeBaseDir() everywhere.

void MainContentComponent::setStatusText (const juce::String& text, juce::Colour colour)
{
    statusButton_.setText (text, colour);
}

void MainContentComponent::setStatusSegments (juce::Array<StatusSegment> segments)
{
    statusButton_.setSegments (std::move (segments));
}

void MainContentComponent::openStatusMonitorWindow()
{
    if (statusMonitor_ != nullptr)
    {
        statusMonitor_->toFront (true);
        return;
    }

    auto getter = [this] (juce::Array<juce::String>& keys, juce::Array<juce::String>& vals)
    {
        keys.clearQuick();
        vals.clearQuick();

        keys.add ("Source:");        vals.add (sourceCombo_.getText());
        keys.add ("Input TC:");      vals.add (tcLabel_.getText()
                                               + "  (" + formatDisplayedInputFps (hasLatchedTc_, latchedFps_, fpsIndicatorStrip_.get()) + ")");
        keys.add ("Status:");        vals.add (statusButton_.getText());
        keys.add ("LTC Out:");       vals.add ((ltcOutSwitch_.getState() ? "ON" : "OFF")
                                               + juce::String ("  |  ") + ltcOutDeviceCombo_.getText());
        keys.add ("LTC Ch / Rate:"); vals.add (ltcOutChannelCombo_.getText()
                                               + "  |  " + ltcOutSampleRateCombo_.getText());
        keys.add ("MTC In:");        vals.add (mtcInCombo_.getText());
        keys.add ("MTC Out:");       vals.add ((mtcOutSwitch_.getState() ? "ON" : "OFF")
                                               + juce::String ("  |  ") + mtcOutCombo_.getText());
        keys.add ("ArtNet In:");     vals.add (artnetInCombo_.getText()
                                               + "  |  " + artnetListenIpEditor_.getText());
        keys.add ("ArtNet Out:");    vals.add ((artnetOutSwitch_.getState() ? "ON" : "OFF")
                                               + juce::String ("  |  ") + juce::String (collectArtnetTargets().size()) + " target(s)");
        keys.add ("OSC Listen:");    vals.add (oscIpEditor_.getText() + ":" + oscPortEditor_.getText());
        keys.add ("System Time:");   vals.add (systemTimeFpsCombo_.getText());
    };

    auto* win = new StatusMonitorWindow (std::move (getter), getParentComponent());
    statusMonitor_ = win;
}

juce::var MainContentComponent::buildConfigState() const
{
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty ("source", sourceCombo_.getSelectedId());
    obj->setProperty ("source_expanded", sourceExpanded_);
    obj->setProperty ("out_ltc_expanded", outLtcExpanded_);
    obj->setProperty ("out_mtc_expanded", outMtcExpanded_);
    obj->setProperty ("out_artnet_expanded", outArtExpanded_);
    obj->setProperty ("close_to_tray", closeToTray_);
    obj->setProperty ("ltc_in_driver", ltcInDriverCombo_.getText());
    obj->setProperty ("ltc_out_driver", ltcOutDriverCombo_.getText());
    obj->setProperty ("ltc_in_device", ltcInDeviceCombo_.getText());
    obj->setProperty ("ltc_in_channel", ltcInChannelCombo_.getText());
    obj->setProperty ("ltc_in_rate", ltcInSampleRateCombo_.getText());
    obj->setProperty ("ltc_in_gain_db", ltcInGainSlider_.getValue());
    obj->setProperty ("ltc_out_device", ltcOutDeviceCombo_.getText());
    obj->setProperty ("ltc_out_channel", ltcOutChannelCombo_.getText());
    obj->setProperty ("ltc_out_rate", ltcOutSampleRateCombo_.getText());
    obj->setProperty ("ltc_convert_fps", ltcConvertStrip_.getSelectedFps().has_value() ? frameRateToString (*ltcConvertStrip_.getSelectedFps()) : juce::String());
    obj->setProperty ("ltc_out_gain_db", ltcOutLevelSlider_.getValue());
    obj->setProperty ("ltc_out_enabled", ltcOutSwitch_.getState());
    obj->setProperty ("ltc_thru_enabled", ltcThruDot_.getState());
    obj->setProperty ("mtc_in", mtcInCombo_.getText());
    obj->setProperty ("mtc_convert_fps", mtcConvertStrip_.getSelectedFps().has_value() ? frameRateToString (*mtcConvertStrip_.getSelectedFps()) : juce::String());
    obj->setProperty ("mtc_out", mtcOutCombo_.getText());
    obj->setProperty ("mtc_out_enabled", mtcOutSwitch_.getState());
    obj->setProperty ("artnet_in", artnetInCombo_.getText());
    obj->setProperty ("artnet_convert_fps", artnetConvertStrip_.getSelectedFps().has_value() ? frameRateToString (*artnetConvertStrip_.getSelectedFps()) : juce::String());
    obj->setProperty ("artnet_out_enabled", artnetOutSwitch_.getState());
    obj->setProperty ("artnet_listen_ip", artnetListenIpEditor_.getText());
    obj->setProperty ("osc_adapter", oscAdapterCombo_.getText());
    obj->setProperty ("osc_ip", oscIpEditor_.getText());
    obj->setProperty ("osc_port", oscPortEditor_.getText());
    obj->setProperty ("osc_fps", oscFpsCombo_.getText());
    obj->setProperty ("system_time_fps", systemTimeFpsCombo_.getText());
    obj->setProperty ("osc_str", oscAddrStrEditor_.getText());
    obj->setProperty ("osc_float", oscAddrFloatEditor_.getText());
    obj->setProperty ("osc_float_type", oscFloatTypeCombo_.getSelectedId());
    obj->setProperty ("osc_float_max", oscFloatMaxEditor_.getText());

    juce::Array<juce::var> artnetTargetsVar;
    const auto artnetTargets = collectArtnetTargets();
    for (int i = 0; i < artnetTargets.size(); ++i)
    {
        juce::DynamicObject::Ptr targetObj = new juce::DynamicObject();
        targetObj->setProperty ("ip", artnetTargets.getReference (i).ip);
        targetObj->setProperty ("adapter", artnetTargetAdapterCombos_[(size_t) i].getText());
        targetObj->setProperty ("adapter_expanded", artnetTargetAdapterExpanded_[(size_t) i]);
        artnetTargetsVar.add (juce::var (targetObj.get()));
    }
    obj->setProperty ("artnet_targets", juce::var (artnetTargetsVar));
    obj->setProperty ("artnet_dest", artnetTargets.isEmpty() ? juce::String ("255.255.255.255") : artnetTargets[0].ip);
    obj->setProperty ("ltc_offset", ltcOffsetEditor_.getText());
    obj->setProperty ("mtc_offset", mtcOffsetEditor_.getText());
    obj->setProperty ("artnet_offset", artnetOffsetEditor_.getText());
    return juce::var (obj.get());
}

void MainContentComponent::refreshConfigDirtyState()
{
    if (suppressConfigDirtyTracking_)
        return;
    configDirty_ = true;
}

bool MainContentComponent::isCurrentConfigEqualToSavedFile() const
{
    if (! lastConfigFile_.existsAsFile())
        return false;

    const auto parsed = juce::JSON::parse (lastConfigFile_);
    auto* savedObj = parsed.getDynamicObject();
    if (savedObj == nullptr)
        return false;

    const auto current = buildConfigState();
    auto* currentObj = current.getDynamicObject();
    if (currentObj == nullptr)
        return false;

    static constexpr const char* kConfigKeys[] = {
        "source",
        "source_expanded",
        "out_ltc_expanded",
        "out_mtc_expanded",
        "out_artnet_expanded",
        "close_to_tray",
        "ltc_in_driver",
        "ltc_out_driver",
        "ltc_in_device",
        "ltc_in_channel",
        "ltc_in_rate",
        "ltc_in_gain_db",
        "ltc_out_device",
        "ltc_out_channel",
        "ltc_out_rate",
        "ltc_convert_fps",
        "ltc_out_gain_db",
        "ltc_out_enabled",
        "ltc_thru_enabled",
        "mtc_in",
        "mtc_convert_fps",
        "mtc_out",
        "mtc_out_enabled",
        "artnet_in",
        "artnet_convert_fps",
        "artnet_out_enabled",
        "artnet_listen_ip",
        "osc_adapter",
        "osc_ip",
        "osc_port",
        "osc_fps",
        "system_time_fps",
        "osc_str",
        "osc_float",
        "osc_float_type",
        "osc_float_max",
        "artnet_targets",
        "artnet_dest",
        "ltc_offset",
        "mtc_offset",
        "artnet_offset"
    };

    for (const auto* key : kConfigKeys)
    {
        const juce::Identifier id (key);
        if (! savedObj->hasProperty (id))
            continue;

        if (! varsEqualConfig (savedObj->getProperty (id), currentObj->getProperty (id)))
            return false;
    }

    return true;
}

bool MainContentComponent::shouldPromptSaveOnClose() const
{
    if (! lastConfigFile_.existsAsFile())
        return true;

    return ! isCurrentConfigEqualToSavedFile();
}

void MainContentComponent::prepareStartupStateBeforeShow()
{
    loadRuntimePrefs();
    maybeAutoLoadConfig();
    updateWindowHeight();

    if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
        window->setSize (window->getWidth(), window->getHeight() + kStartupHeightExtra);

    // After startup state is restored, scan devices async.
    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<MainContentComponent> (this)]
    {
        if (safe != nullptr)
            safe->startAudioDeviceScan();
    });
}

void MainContentComponent::saveConfigForExit (std::function<void(bool)> onDone)
{
    if (lastConfigFile_.existsAsFile())
    {
        const bool ok = saveConfigToFile (lastConfigFile_);
        if (onDone != nullptr)
            juce::MessageManager::callAsync ([cb = std::move (onDone), ok] { cb (ok); });
        return;
    }

    pendingSaveCallback_ = std::move (onDone);
    saveConfigAs();
}

void MainContentComponent::saveConfig()
{
    if (lastConfigFile_.existsAsFile())
        saveConfigToFile (lastConfigFile_);
    else
        saveConfigAs();
}

void MainContentComponent::saveConfigAs()
{
    saveChooser_ = std::make_unique<juce::FileChooser> (
        "Save Easy Bridge config",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("easy_bridge.ebrp"),
        "*.ebrp");

    saveChooser_->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                               [safe = juce::Component::SafePointer<MainContentComponent> (this)] (const juce::FileChooser& chooser)
                               {
                                   if (safe == nullptr)
                                       return;

                                   bool saved = false;
                                   auto file = chooser.getResult();
                                   if (file != juce::File{})
                                   {
                                       if (! file.hasFileExtension (".ebrp"))
                                           file = file.withFileExtension (".ebrp");
                                       saved = safe->saveConfigToFile (file);
                                   }

                                   if (safe->pendingSaveCallback_ != nullptr)
                                   {
                                       auto cb = std::move (safe->pendingSaveCallback_);
                                       safe->pendingSaveCallback_ = nullptr;
                                       cb (saved);
                                   }

                                   safe->saveChooser_.reset();
                               });
}

void MainContentComponent::loadConfigFrom()
{
    loadChooser_ = std::make_unique<juce::FileChooser> (
        "Load Easy Bridge config",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*.ebrp");

    loadChooser_->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                               [safe = juce::Component::SafePointer<MainContentComponent> (this)] (const juce::FileChooser& chooser)
                               {
                                   if (safe == nullptr)
                                       return;
                                   auto file = chooser.getResult();
                                   if (file != juce::File{})
                                       safe->loadConfigFromFile (file);
                                   safe->loadChooser_.reset();
                               });
}

bool MainContentComponent::saveConfigToFile (const juce::File& cfgFile)
{
    const auto state = buildConfigState();
    if (cfgFile.replaceWithText (juce::JSON::toString (state, true)))
    {
        lastConfigFile_ = cfgFile;
        autoLoadOnStartup_ = true;
        lastSavedConfigState_ = juce::JSON::toString (state, true);
        configDirty_ = false;
        saveRuntimePrefs();
        setStatusText ("STOPPED - config saved: " + cfgFile.getFileName(), juce::Colour::fromRGB (0xec, 0x48, 0x3c));
        return true;
    }

    setStatusText ("STOPPED - failed to save config", juce::Colour::fromRGB (0xde, 0x9b, 0x3c));
    return false;
}

void MainContentComponent::loadConfigFromFile (const juce::File& cfgFile)
{
    if (! cfgFile.existsAsFile())
    {
        setStatusText ("STOPPED - config not found", juce::Colour::fromRGB (0xde, 0x9b, 0x3c));
        return;
    }

    const auto parsed = juce::JSON::parse (cfgFile);
    if (! parsed.isObject())
    {
        setStatusText ("STOPPED - invalid config", juce::Colour::fromRGB (0xde, 0x9b, 0x3c));
        return;
    }

    auto* obj = parsed.getDynamicObject();
    if (obj == nullptr)
        return;

    auto propOr = [obj] (juce::Identifier key, juce::var fallback) -> juce::var
    {
        return obj->hasProperty (key) ? obj->getProperty (key) : fallback;
    };

    auto setComboText = [] (juce::ComboBox& combo, const juce::String& text)
    {
        for (int i = 0; i < combo.getNumItems(); ++i)
        {
            if (combo.getItemText (i) == text)
            {
                combo.setSelectedItemIndex (i, juce::dontSendNotification);
                return;
            }
        }
    };

    juce::ScopedValueSetter<bool> holdDirtyTracking (suppressConfigDirtyTracking_, true);

    sourceCombo_.setSelectedId ((int) propOr ("source", sourceCombo_.getSelectedId()), juce::dontSendNotification);
    sourceExpanded_ = (bool) propOr ("source_expanded", sourceExpanded_);
    outLtcExpanded_ = (bool) propOr ("out_ltc_expanded", outLtcExpanded_);
    outMtcExpanded_ = (bool) propOr ("out_mtc_expanded", outMtcExpanded_);
    outArtExpanded_ = (bool) propOr ("out_artnet_expanded", outArtExpanded_);
    closeToTray_ = (bool) propOr ("close_to_tray", closeToTray_);

    setComboText (ltcInDriverCombo_, propOr ("ltc_in_driver", ltcInDriverCombo_.getText()).toString());
    setComboText (ltcOutDriverCombo_, propOr ("ltc_out_driver", ltcOutDriverCombo_.getText()).toString());
    refreshLtcDeviceListsByDriver();
    refreshLtcChannelCombos();
    setComboText (ltcInDeviceCombo_, propOr ("ltc_in_device", ltcInDeviceCombo_.getText()).toString());
    refreshLtcChannelCombos();
    setComboText (ltcInChannelCombo_, propOr ("ltc_in_channel", ltcInChannelCombo_.getText()).toString());
    setComboText (ltcInSampleRateCombo_, propOr ("ltc_in_rate", ltcInSampleRateCombo_.getText()).toString());
    ltcInGainSlider_.setValue ((double) propOr ("ltc_in_gain_db", ltcInGainSlider_.getValue()), juce::dontSendNotification);
    setComboText (ltcOutDeviceCombo_, propOr ("ltc_out_device", ltcOutDeviceCombo_.getText()).toString());
    refreshLtcChannelCombos();
    setComboText (ltcOutChannelCombo_, propOr ("ltc_out_channel", ltcOutChannelCombo_.getText()).toString());
    setComboText (ltcOutSampleRateCombo_, propOr ("ltc_out_rate", ltcOutSampleRateCombo_.getText()).toString());
    ltcConvertStrip_.setSelectedFps (fpsFromString (propOr ("ltc_convert_fps", juce::String()).toString()));
    ltcOutLevelSlider_.setValue ((double) propOr ("ltc_out_gain_db", ltcOutLevelSlider_.getValue()), juce::dontSendNotification);
    ltcOutSwitch_.setState ((bool) propOr ("ltc_out_enabled", ltcOutSwitch_.getState()));
    ltcThruDot_.setState ((bool) propOr ("ltc_thru_enabled", ltcThruDot_.getState()));
    setComboText (mtcInCombo_, propOr ("mtc_in", mtcInCombo_.getText()).toString());
    mtcConvertStrip_.setSelectedFps (fpsFromString (propOr ("mtc_convert_fps", juce::String()).toString()));
    setComboText (mtcOutCombo_, propOr ("mtc_out", mtcOutCombo_.getText()).toString());
    mtcOutSwitch_.setState ((bool) propOr ("mtc_out_enabled", mtcOutSwitch_.getState()));
    setComboText (artnetInCombo_, propOr ("artnet_in", artnetInCombo_.getText()).toString());
    artnetConvertStrip_.setSelectedFps (fpsFromString (propOr ("artnet_convert_fps", juce::String()).toString()));
    artnetOutSwitch_.setState ((bool) propOr ("artnet_out_enabled", artnetOutSwitch_.getState()));
    setComboText (oscAdapterCombo_, propOr ("osc_adapter", oscAdapterCombo_.getText()).toString());
    setComboText (oscFpsCombo_, propOr ("osc_fps", oscFpsCombo_.getText()).toString());
    setComboText (systemTimeFpsCombo_, propOr ("system_time_fps", systemTimeFpsCombo_.getText()).toString());

    artnetListenIpEditor_.setText (propOr ("artnet_listen_ip", artnetListenIpEditor_.getText()).toString(), juce::dontSendNotification);
    oscIpEditor_.setText (propOr ("osc_ip", oscIpEditor_.getText()).toString(), juce::dontSendNotification);
    oscPortEditor_.setText (propOr ("osc_port", oscPortEditor_.getText()).toString(), juce::dontSendNotification);
    oscAddrStrEditor_.setText (propOr ("osc_str", oscAddrStrEditor_.getText()).toString(), juce::dontSendNotification);
    oscAddrFloatEditor_.setText (propOr ("osc_float", oscAddrFloatEditor_.getText()).toString(), juce::dontSendNotification);
    oscFloatTypeCombo_.setSelectedId ((int) propOr ("osc_float_type", oscFloatTypeCombo_.getSelectedId()), juce::dontSendNotification);
    oscFloatMaxEditor_.setText (propOr ("osc_float_max", oscFloatMaxEditor_.getText()).toString(), juce::dontSendNotification);
    for (auto& e : artnetDestIpEditors_)
        e.setText ("", juce::dontSendNotification);
    for (auto& combo : artnetTargetAdapterCombos_)
        combo.setSelectedItemIndex (0, juce::dontSendNotification);
    artnetTargetAdapterExpanded_ = { true, false, false, false, false };
    artnetDestVisibleCount_ = 1;
    auto artnetTargets = propOr ("artnet_targets", juce::var());
    if (auto* arr = artnetTargets.getArray())
    {
        int idx = 0;
        for (const auto& v : *arr)
        {
            if (idx >= (int) artnetDestIpEditors_.size())
                break;

            if (auto* targetObj = v.getDynamicObject())
            {
                const auto ip = targetObj->getProperty ("ip").toString().trim();
                if (ip.isEmpty())
                    continue;
                artnetDestIpEditors_[(size_t) idx].setText (ip, juce::dontSendNotification);
                setComboText (artnetTargetAdapterCombos_[(size_t) idx], targetObj->getProperty ("adapter").toString());
                artnetTargetAdapterExpanded_[(size_t) idx] = (bool) targetObj->getProperty ("adapter_expanded");
                ++idx;
            }
            else
            {
                const auto ip = v.toString().trim();
                if (ip.isNotEmpty())
                    artnetDestIpEditors_[(size_t) idx++].setText (ip, juce::dontSendNotification);
            }
        }
        artnetDestVisibleCount_ = juce::jmax (1, juce::jmin ((int) artnetDestIpEditors_.size(), idx));
    }
    if (artnetDestIpEditors_[0].getText().trim().isEmpty())
        artnetDestIpEditors_[0].setText (propOr ("artnet_dest", juce::String ("255.255.255.255")).toString(), juce::dontSendNotification);
    updateArtnetIpControls();
    ltcOffsetEditor_.setText (propOr ("ltc_offset", ltcOffsetEditor_.getText()).toString(), juce::dontSendNotification);
    mtcOffsetEditor_.setText (propOr ("mtc_offset", mtcOffsetEditor_.getText()).toString(), juce::dontSendNotification);
    artnetOffsetEditor_.setText (propOr ("artnet_offset", artnetOffsetEditor_.getText()).toString(), juce::dontSendNotification);

    sourceExpandBtn_.setExpanded (sourceExpanded_);
    outLtcExpandBtn_.setExpanded (outLtcExpanded_);
    outMtcExpandBtn_.setExpanded (outMtcExpanded_);
    outArtExpandBtn_.setExpanded (outArtExpanded_);

    // Update layout and paint immediately so the UI reflects the loaded state
    // before audio device init blocks the message thread.
    updateWindowHeight();
    repaint();
    juce::Timer::callAfterDelay (50, [safe = juce::Component::SafePointer<MainContentComponent> (this)]
    {
        if (auto* self = safe.getComponent())
        {
            self->onInputSettingsChanged();
            self->onOutputSettingsChanged();
        }
    });
    lastConfigFile_ = cfgFile;
    lastSavedConfigState_ = juce::JSON::toString (buildConfigState(), true);
    configDirty_ = false;
    saveRuntimePrefs();
    setStatusText ("STOPPED - config loaded: " + cfgFile.getFileName(), juce::Colour::fromRGB (0xec, 0x48, 0x3c));
}

juce::File MainContentComponent::prefsFilePath() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("EasyBridge")
        .getChildFile ("runtime_prefs.json");
}

void MainContentComponent::loadRuntimePrefs()
{
    const auto prefs = prefsFilePath();
    if (! prefs.existsAsFile())
        return;

    const auto parsed = juce::JSON::parse (prefs);
    if (! parsed.isObject())
        return;

    if (auto* obj = parsed.getDynamicObject())
    {
        if (obj->hasProperty ("auto_load_on_startup"))
            autoLoadOnStartup_ = (bool) obj->getProperty ("auto_load_on_startup");
        if (obj->hasProperty ("close_to_tray"))
            closeToTray_ = (bool) obj->getProperty ("close_to_tray");
        if (obj->hasProperty ("last_config_path"))
        {
            const auto path = obj->getProperty ("last_config_path").toString();
            if (path.isNotEmpty())
                lastConfigFile_ = juce::File (path);
        }
    }
}

void MainContentComponent::saveRuntimePrefs() const
{
    auto prefs = prefsFilePath();
    prefs.getParentDirectory().createDirectory();

    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty ("auto_load_on_startup", autoLoadOnStartup_);
    obj->setProperty ("close_to_tray", closeToTray_);
    obj->setProperty ("last_config_path", lastConfigFile_.getFullPathName());
    prefs.replaceWithText (juce::JSON::toString (juce::var (obj.get()), true));
}

void MainContentComponent::maybeAutoLoadConfig()
{
    if (autoLoadOnStartup_ && lastConfigFile_.existsAsFile())
        loadConfigFromFile (lastConfigFile_);
}

void MainContentComponent::resetToDefaults()
{
    sourceCombo_.setSelectedId (1, juce::dontSendNotification);
    sourceExpanded_ = true;
    outLtcExpanded_ = false;
    outMtcExpanded_ = false;
    outArtExpanded_ = false;
    sourceExpandBtn_.setExpanded (sourceExpanded_);
    outLtcExpandBtn_.setExpanded (outLtcExpanded_);
    outMtcExpandBtn_.setExpanded (outMtcExpanded_);
    outArtExpandBtn_.setExpanded (outArtExpanded_);

    ltcOutSwitch_.setState (false);
    mtcOutSwitch_.setState (false);
    artnetOutSwitch_.setState (false);
    ltcThruDot_.setState (false);
    ltcConvertStrip_.setSelectedFps (std::nullopt);
    mtcConvertStrip_.setSelectedFps (std::nullopt);
    artnetConvertStrip_.setSelectedFps (std::nullopt);
    ltcOffsetEditor_.setText ("0", juce::dontSendNotification);
    mtcOffsetEditor_.setText ("0", juce::dontSendNotification);
    artnetOffsetEditor_.setText ("0", juce::dontSendNotification);
    oscPortEditor_.setText ("9000", juce::dontSendNotification);
    oscIpEditor_.setText ("0.0.0.0", juce::dontSendNotification);
    artnetListenIpEditor_.setText ("0.0.0.0", juce::dontSendNotification);
    // OSC addresses and mode: restore the same defaults as at startup.
    oscAddrStrEditor_.setText ("/frames/str", juce::dontSendNotification);
    oscAddrFloatEditor_.setText ("/time", juce::dontSendNotification);
    oscFloatTypeCombo_.setSelectedId (1, juce::dontSendNotification);
    oscFloatMaxEditor_.setText ("3600", juce::dontSendNotification);
    for (auto& e : artnetDestIpEditors_)
        e.setText ("", juce::dontSendNotification);
    for (auto& combo : artnetTargetAdapterCombos_)
        combo.setSelectedItemIndex (0, juce::dontSendNotification);
    artnetDestIpEditors_[0].setText ("255.255.255.255", juce::dontSendNotification);
    artnetDestVisibleCount_ = 1;
    artnetTargetAdapterExpanded_ = { true, false, false, false, false };
    updateArtnetIpControls();

    onInputSettingsChanged();
    onOutputSettingsChanged();
    updateWindowHeight();
    resized();
    refreshConfigDirtyState();
    setStatusText ("Config reset", juce::Colour::fromRGB (0xec, 0x48, 0x3c));
}

void MainContentComponent::openSettingsMenu()
{
    juce::PopupMenu m;
    m.addItem (1, "Save");
    m.addItem (2, "Save As...");
    m.addSeparator();
    m.addItem (3, "Load...");
    m.addItem (4, "Load on startup", true, autoLoadOnStartup_);
    m.addSeparator();
    m.addItem (5, "Reset config");
    m.addSeparator();
    m.addItem (6, "Rescan audio");
    m.addSeparator();
    m.addItem (7, "Close to tray", true, closeToTray_);

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&settingsButton_),
                     [safe = juce::Component::SafePointer<MainContentComponent> (this)] (int result)
                     {
                         if (safe == nullptr)
                             return;

                         switch (result)
                         {
                             case 1: safe->saveConfig(); break;
                             case 2: safe->saveConfigAs(); break;
                             case 3: safe->loadConfigFrom(); break;
                             case 4:
                                 safe->autoLoadOnStartup_ = ! safe->autoLoadOnStartup_;
                                 safe->saveRuntimePrefs();
                                 safe->setStatusText (safe->autoLoadOnStartup_ ? "STOPPED - load on startup ON"
                                                                                : "STOPPED - load on startup OFF",
                                                      juce::Colour::fromRGB (0xec, 0x48, 0x3c));
                                 break;
                             case 5: safe->resetToDefaults(); break;
                             case 6:
                                 safe->refreshDeviceLists();
                                 safe->setStatusText ("STOPPED - audio devices rescanned",
                                                      juce::Colour::fromRGB (0xec, 0x48, 0x3c));
                                 break;
                             case 7:
                                 safe->closeToTray_ = ! safe->closeToTray_;
                                 safe->saveRuntimePrefs();
                                 safe->setStatusText (safe->closeToTray_ ? "STOPPED - close to tray ON" : "STOPPED - close to tray OFF",
                                                      juce::Colour::fromRGB (0xec, 0x48, 0x3c));
                                 break;
                             default: break;
                         }
                     });
}

void MainContentComponent::openHelpPage()
{
    // BUG-8 fix: use platform::findBridgeBaseDir() and open the file directly
    // via startAsProcess() so the OS picks the default browser without needing
    // a "file:///" prefix workaround.
    const auto base = platform::findBridgeBaseDir();
    if (! base.exists())
        return;

    const auto help = base.getChildFile ("Help/easy_bridge_v2_help.html");
    if (help.existsAsFile())
        help.startAsProcess();
}

MainWindow::MainWindow()
    : juce::DocumentWindow ("Easy Bridge",
                            juce::Colours::black,
                            juce::DocumentWindow::minimiseButton | juce::DocumentWindow::closeButton)
{
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour::fromRGB (0x11, 0x12, 0x16));
    setUsingNativeTitleBar (true);
    // Allow vertical resize only; width is fixed at 430 by the equal min/max below.
    setResizable (true, false);
    // Initial limits — updated immediately by MainContentComponent::updateWindowHeight().
    // Min = all-collapsed height; no upper cap so the user can drag to screen edge.
    setResizeLimits (430, 420, 430, 8192);
    setContentOwned (new MainContentComponent(), true);
    if (auto* content = dynamic_cast<MainContentComponent*> (getContentComponent()))
        content->prepareStartupStateBeforeShow();
    // After setContentOwned the window is already sized to the content's preferred
    // height (set via setSize() in MainContentComponent::MainContentComponent()).
    // Just centre on screen without forcing the height back to a hardcoded value.
    centreWithSize (430, getHeight());
    const auto icon = platform::loadBridgeAppIcon();
    setIcon (icon);
    createTrayIcon();
    if (trayIcon_ != nullptr && icon.isValid())
        trayIcon_->setIconImage (icon, icon);
    setVisible (true);

#if JUCE_WINDOWS
    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<MainWindow> (this)]
    {
        if (safe == nullptr)
            return;
        platform::applyNativeDarkTitleBar (*safe);
        // Note: applyNativeFixedWindow removed — vertical resize is now enabled.
    });
#endif
}

MainWindow::~MainWindow() = default;

void MainWindow::closeButtonPressed()
{
    bool closeToTray = true;
    if (auto* content = dynamic_cast<MainContentComponent*> (getContentComponent()))
        closeToTray = content->closeToTrayEnabled();

    if (closeToTray && ! quittingFromMenu_)
    {
        setVisible (false);
        if (trayIcon_ != nullptr)
            trayIcon_->showInfoBubble ("Easy Bridge", "Running in system tray");
        return;
    }

    requestQuitWithSavePrompt();
}

void MainWindow::requestQuitWithSavePrompt()
{
    auto* content = dynamic_cast<MainContentComponent*> (getContentComponent());
    if (content == nullptr)
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
        return;
    }

    if (! content->shouldPromptSaveOnClose())
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
        return;
    }

    if (closePromptOpen_)
        return;

    closePromptOpen_ = true;
    ExitSavePromptDialog::show (this, [safe = juce::Component::SafePointer<MainWindow> (this)] (int action)
    {
        if (auto* self = safe.getComponent())
            self->handleClosePromptResult (action);
    });
}

void MainWindow::handleClosePromptResult (int action)
{
    closePromptOpen_ = false;

    if (action == 1)
    {
        if (auto* content = dynamic_cast<MainContentComponent*> (getContentComponent()))
        {
            content->saveConfigForExit ([safe = juce::Component::SafePointer<MainWindow> (this)] (bool saved)
            {
                if (saved)
                {
                    if (safe != nullptr)
                        juce::JUCEApplication::getInstance()->systemRequestedQuit();
                }
                else if (auto* self = safe.getComponent())
                {
                    self->quittingFromMenu_ = false;
                }
            });
            return;
        }
    }
    else if (action == 2)
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
        return;
    }

    quittingFromMenu_ = false;
}

void MainWindow::createTrayIcon()
{
    trayIcon_ = std::make_unique<BridgeTrayIcon> (*this);
    trayIcon_->setIconTooltip ("Easy Bridge");
}

void MainWindow::showFromTray()
{
    setVisible (true);
    setMinimised (false);
    toFront (true);
}

void MainWindow::quitFromTray()
{
    quittingFromMenu_ = true;
    closeButtonPressed();
}
} // namespace bridge
