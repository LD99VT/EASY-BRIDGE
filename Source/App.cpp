#include "App.h"
#include "MainWindow.h"
#include "core/BridgeVersion.h"

namespace bridge
{
const juce::String BridgeApplication::getApplicationName()
{
    return "Easy Bridge";
}

const juce::String BridgeApplication::getApplicationVersion()
{
    return bridge::version::kAppVersion;
}

void BridgeApplication::initialise (const juce::String&)
{
    mainWindow_ = std::make_unique<MainWindow>();
}

void BridgeApplication::shutdown()
{
    if (mainWindow_ != nullptr)
        mainWindow_->setVisible (false); // prevent white flash during destruction
    mainWindow_.reset();
}

void BridgeApplication::systemRequestedQuit()
{
    quit();
}

void BridgeApplication::anotherInstanceStarted (const juce::String&)
{
    // BUG-9: bring existing window to front instead of launching a second instance.
    if (mainWindow_ != nullptr)
        mainWindow_->showFromTray();
}
} // namespace bridge
