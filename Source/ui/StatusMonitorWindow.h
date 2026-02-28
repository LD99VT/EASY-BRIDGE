#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "../platform/WindowUtils.h"

namespace bridge
{
// ─── Live Bridge Status Monitor popup window ──────────────────────────────────
class StatusMonitorWindow final : public juce::DocumentWindow,
                                  private juce::Timer
{
public:
    using Getter = std::function<void (juce::Array<juce::String>&, juce::Array<juce::String>&)>;

    StatusMonitorWindow (Getter getter, juce::Component* relativeTo)
        : juce::DocumentWindow ("Bridge Status Monitor",
                                juce::Colour::fromRGB (0x1e, 0x1e, 0x1e),
                                juce::DocumentWindow::closeButton),
          getter_ (std::move (getter))
    {
        setUsingNativeTitleBar (true);
        setResizable (false, false);
        setContentOwned (new Content (*this), true);
        centreWithSize (420, 390);
        if (relativeTo != nullptr)
        {
            const auto rc = relativeTo->getScreenBounds();
            setBounds (rc.getCentreX() - 210, rc.getCentreY() - 195, 420, 390);
        }
        setVisible (true);
#if JUCE_WINDOWS
        platform::applyNativeDarkTitleBar (*this);
        platform::applyNativeFixedWindow (*this);
        platform::removeNativeTitleBarIcon (*this);
#endif
        toFront (true);
        startTimerHz (5);
    }

    // BUG-10 fix: use callAsync to avoid deleting from within an event handler.
    void closeButtonPressed() override
    {
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<StatusMonitorWindow> (this)]
        {
            if (auto* w = safe.getComponent())
                delete w;
        });
    }

    void timerCallback() override
    {
        if (auto* c = dynamic_cast<Content*> (getContentComponent()))
            c->refresh();
    }

    void getValues (juce::Array<juce::String>& keys, juce::Array<juce::String>& vals)
    {
        getter_ (keys, vals);
    }

private:
    Getter getter_;

    static constexpr int kRows = 10;

    struct Content final : juce::Component
    {
        StatusMonitorWindow& win_;

        juce::Label keyLbls_[kRows];
        juce::Label valLbls_[kRows];
        juce::TextButton ok_ { "OK" };

        explicit Content (StatusMonitorWindow& w) : win_ (w)
        {
            const juce::Colour keyCol = juce::Colour::fromRGB (0x7a, 0x7a, 0x86);
            const juce::Colour valCol = juce::Colour::fromRGB (0xe0, 0xe0, 0xe0);

            for (int i = 0; i < kRows; ++i)
            {
                keyLbls_[i].setFont (juce::FontOptions (12.5f).withStyle ("Bold"));
                keyLbls_[i].setColour (juce::Label::textColourId, keyCol);
                keyLbls_[i].setJustificationType (juce::Justification::centredRight);
                addAndMakeVisible (keyLbls_[i]);

                valLbls_[i].setFont (juce::FontOptions (12.5f));
                valLbls_[i].setColour (juce::Label::textColourId, valCol);
                valLbls_[i].setJustificationType (juce::Justification::centredLeft);
                addAndMakeVisible (valLbls_[i]);
            }

            ok_.setColour (juce::TextButton::buttonColourId,   juce::Colour::fromRGB (0x4a, 0x4a, 0x4a));
            ok_.setColour (juce::TextButton::buttonOnColourId, juce::Colour::fromRGB (0x4a, 0x4a, 0x4a));
            ok_.setColour (juce::TextButton::textColourOffId,  juce::Colour::fromRGB (0xe1, 0xe6, 0xef));
            ok_.setColour (juce::TextButton::textColourOnId,   juce::Colour::fromRGB (0xe1, 0xe6, 0xef));
            ok_.onClick = [this]
            {
                juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<StatusMonitorWindow> (&win_)]
                {
                    if (auto* w = safe.getComponent())
                        delete w;
                });
            };
            addAndMakeVisible (ok_);

            refresh();
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (juce::Colour::fromRGB (0x17, 0x17, 0x17));
        }

        void resized() override
        {
            constexpr int kRowH = 30;
            constexpr int kPad  = 14;
            constexpr int kKeyW = 120;
            constexpr int kGap  = 10;
            const int valW = getWidth() - kPad * 2 - kKeyW - kGap;

            for (int i = 0; i < kRows; ++i)
            {
                const int y = kPad + i * kRowH;
                keyLbls_[i].setBounds (kPad, y, kKeyW, kRowH);
                valLbls_[i].setBounds (kPad + kKeyW + kGap, y, valW, kRowH);
            }

            const int btnY = kPad + kRows * kRowH + kPad;
            ok_.setBounds ((getWidth() - 100) / 2, btnY, 100, 32);
        }

        void refresh()
        {
            juce::Array<juce::String> keys, vals;
            win_.getValues (keys, vals);
            for (int i = 0; i < juce::jmin (kRows, keys.size()); ++i)
            {
                keyLbls_[i].setText (keys[i], juce::dontSendNotification);
                valLbls_[i].setText (vals[i], juce::dontSendNotification);
            }
        }
    };
};
} // namespace bridge
