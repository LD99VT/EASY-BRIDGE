#pragma once

#include <array>
#include <juce_gui_extra/juce_gui_extra.h>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "engine/BridgeEngine.h"

// ─── UI style & widget components (extracted from monolith) ──────────────────
#include "ui/style/BridgeColours.h"
#include "ui/style/BridgeLookAndFeel.h"
#include "ui/style/StyleHelpers.h"
#include "ui/widgets/CircleButtons.h"
#include "ui/widgets/DotToggle.h"
#include "ui/widgets/MacSwitch.h"
#include "ui/widgets/LevelMeter.h"

namespace bridge
{
class BridgeAudioScanThread;

class MainContentComponent final : public juce::Component, private juce::Timer
{
public:
    MainContentComponent();
    ~MainContentComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool closeToTrayEnabled() const { return closeToTray_; }

private:
    friend class BridgeAudioScanThread;
    void timerCallback() override;

    void loadFontsAndIcon();
    void applyLookAndFeel();
    void restartSelectedSource();
    void queueLtcOutputApply();
    void ltcOutputApplyLoop();
    void onOutputToggleChanged();
    void onOutputSettingsChanged();
    void onInputSettingsChanged();
    int calcPreferredHeight() const;
    int calcHeightForState (bool sourceExpanded, int sourceId, bool outLtcExpanded, bool outMtcExpanded, bool outArtExpanded) const;
    void updateWindowHeight();

    void refreshDeviceLists();
    void startAudioDeviceScan();
    void onAudioScanComplete (const juce::Array<engine::AudioChoice>& inputs,
                              const juce::Array<engine::AudioChoice>& outputs);
    void refreshNetworkMidiLists();
    void refreshLtcDeviceListsByDriver();
    void fillAudioCombo (juce::ComboBox& combo, const juce::Array<engine::AudioChoice>& choices);
    static double comboSampleRate (const juce::ComboBox& combo);
    static int comboBufferSize (const juce::ComboBox& combo);
    static int comboChannelIndex (const juce::ComboBox& combo);
    static int offsetFromEditor (const juce::TextEditor& editor);
    void syncOscIpWithAdapter();
    void updateArtnetIpControls();
    juce::StringArray collectArtnetTargets() const;
    void setStatusText (const juce::String& text, juce::Colour colour);
    void openStatusMonitorWindow();
    void openSettingsMenu();
    void saveConfigAs();
    void loadConfigFrom();
    void saveConfigToFile (const juce::File& cfgFile);
    void loadConfigFromFile (const juce::File& cfgFile);
    juce::File prefsFilePath() const;
    void loadRuntimePrefs();
    void saveRuntimePrefs() const;
    void maybeAutoLoadConfig();
    void resetToDefaults();
    void openHelpPage();

    engine::BridgeEngine bridgeEngine_;
    juce::Array<engine::AudioChoice> inputChoices_;
    juce::Array<engine::AudioChoice> outputChoices_;
    juce::Array<engine::AudioChoice> filteredInputChoices_;
    juce::Array<engine::AudioChoice> filteredOutputChoices_;
    juce::Array<int> filteredInputIndices_;
    juce::Array<int> filteredOutputIndices_;
    std::unique_ptr<BridgeAudioScanThread> scanThread_;
    std::unique_ptr<BridgeLookAndFeel> lookAndFeel_;
    juce::Image appIcon_;
    juce::Font titleEasyFont_;
    juce::Font titleBridgeFont_;
    juce::Font monoFont_;
    bool hasLatchedTc_ { false };
    Timecode latchedTc_ {};
    FrameRate latchedFps_ { FrameRate::FPS_25 };

    juce::Label titleEasyLabel_;
    juce::Label titleBridgeLabel_;
    juce::Label titleVersionLabel_;
    HelpCircleButton helpButton_;
    juce::Label tcLabel_;
    juce::Label tcFpsLabel_;
    juce::TextButton statusButton_;
    juce::Component::SafePointer<juce::Component> statusMonitor_;
    juce::TextButton settingsButton_ { "Settings" };
    juce::TextButton quitButton_ { "Quit" };
    bool closeToTray_ { false };
    bool autoLoadOnStartup_ { false };
    juce::File lastConfigFile_;
    std::unique_ptr<juce::FileChooser> saveChooser_;
    std::unique_ptr<juce::FileChooser> loadChooser_;
    std::thread ltcOutputApplyThread_;
    std::mutex ltcOutputApplyMutex_;
    std::condition_variable ltcOutputApplyCv_;
    bool ltcOutputApplyExit_ { false };
    bool ltcOutputApplyPending_ { false };
    engine::AudioChoice pendingLtcOutputChoice_;
    int pendingLtcOutputChannel_ { 0 };
    double pendingLtcOutputSampleRate_ { 0.0 };
    int pendingLtcOutputBufferSize_ { 0 };
    bool pendingLtcOutputEnabled_ { false };

    juce::ComboBox sourceCombo_;
    ExpandCircleButton sourceExpandBtn_;
    juce::ComboBox ltcInDriverCombo_;
    juce::ComboBox ltcOutDriverCombo_;
    juce::Label sourceHeaderLabel_ { {}, "Source" };
    juce::Label outLtcHeaderLabel_ { {}, "Out LTC" };
    juce::Label outMtcHeaderLabel_ { {}, "Out MTC" };
    juce::Label outArtHeaderLabel_ { {}, "Out ArtNet" };
    ExpandCircleButton outLtcExpandBtn_;
    ExpandCircleButton outMtcExpandBtn_;
    ExpandCircleButton outArtExpandBtn_;
    juce::Label inDriverLbl_       { {}, "Driver:" };
    juce::Label inDeviceLbl_       { {}, "Device (input):" };
    juce::Label inChannelLbl_      { {}, "Channel:" };
    juce::Label inRateLbl_         { {}, "Sample rate:" };
    juce::Label inLevelLbl_        { {}, "Level:" };
    juce::Label inGainLbl_         { {}, "Input gain:" };
    juce::Label mtcInLbl_          { {}, "MTC Input:" };
    juce::Label artInLbl_          { {}, "ArtNet adapter:" };
    juce::Label artInListenIpLbl_  { {}, "Listen IP:" };
    juce::Label oscIpLbl_          { {}, "OSC Listen IP:" };
    juce::Label oscPortLbl_        { {}, "OSC Port:" };
    juce::Label oscAdapterLbl_     { {}, "OSC adapter:" };
    juce::Label oscFpsLbl_         { {}, "OSC FPS:" };
    juce::Label oscStrLbl_         { {}, "OSC str cmd:" };
    juce::Label oscFloatLbl_       { {}, "OSC float cmd:" };

    juce::Label outDriverLbl_  { {}, "Driver:" };
    juce::Label outDeviceLbl_  { {}, "Device (out):" };
    juce::Label outChannelLbl_ { {}, "Channel:" };
    juce::Label outRateLbl_    { {}, "Sample rate:" };
    juce::Label outOffsetLbl_  { {}, "Offset (frames):" };
    juce::Label outLevelLbl_   { {}, "Output level:" };
    juce::Label mtcOutLbl_     { {}, "MIDI Output:" };
    juce::Label mtcOffsetLbl_  { {}, "Offset (frames):" };
    juce::Label artOutLbl_     { {}, "Interface:" };
    juce::Label artIpLbl_      { {}, "Destination IP:" };
    std::array<juce::Label, 4> artIpExtraLbls_;
    juce::Label artOffsetLbl_  { {}, "Offset (frames):" };

    juce::ComboBox ltcInDeviceCombo_;
    juce::ComboBox ltcInChannelCombo_;
    juce::ComboBox ltcInSampleRateCombo_;
    LevelMeter ltcInLevelBar_;
    float ltcInLevelSmoothed_ { 0.0f };
    juce::Slider ltcInGainSlider_;

    juce::ComboBox mtcInCombo_;
    juce::ComboBox artnetInCombo_;
    juce::TextEditor artnetListenIpEditor_;
    juce::ComboBox oscAdapterCombo_;
    juce::TextEditor oscIpEditor_;
    juce::TextEditor oscPortEditor_;
    juce::ComboBox oscFpsCombo_;
    juce::TextEditor oscAddrStrEditor_;
    juce::TextEditor oscAddrFloatEditor_;

    juce::ComboBox ltcOutDeviceCombo_;
    juce::ComboBox ltcOutChannelCombo_;
    juce::ComboBox ltcOutSampleRateCombo_;
    juce::TextEditor ltcOffsetEditor_;
    juce::Slider ltcOutLevelSlider_;
    MacSwitch ltcOutSwitch_;
    DotToggle ltcThruDot_;
    juce::Label ltcThruLbl_ { {}, "Thru" };

    juce::ComboBox mtcOutCombo_;
    juce::TextEditor mtcOffsetEditor_;
    MacSwitch mtcOutSwitch_;
    DotToggle mtcThruDot_;
    juce::Label mtcThruLbl_ { {}, "Thru" };

    juce::ComboBox artnetOutCombo_;
    std::array<juce::TextEditor, 5> artnetDestIpEditors_;
    std::array<juce::TextButton, 4> artnetDestRemoveButtons_;
    juce::TextButton artnetAddIpButton_ { "+ Add IP" };
    int artnetDestVisibleCount_ { 1 };
    juce::TextEditor artnetOffsetEditor_;
    MacSwitch artnetOutSwitch_;

    bool sourceExpanded_ { true };
    bool outLtcExpanded_ { false };
    bool outMtcExpanded_ { false };
    bool outArtExpanded_ { false };

    juce::Array<juce::Rectangle<int>> paramRowRects_;
    juce::Array<juce::Rectangle<int>> sectionRowRects_;
    juce::Rectangle<int> headerRect_;
    juce::Rectangle<int> statusRect_;
    juce::Rectangle<int> buttonRowRect_;
};

class MainWindow final : public juce::DocumentWindow
{
public:
    MainWindow();
    ~MainWindow() override;
    void closeButtonPressed() override;
    void createTrayIcon();
    void showFromTray();
    void quitFromTray();

private:
    std::unique_ptr<juce::SystemTrayIconComponent> trayIcon_;
    bool quittingFromMenu_ { false };
};
} // namespace bridge
