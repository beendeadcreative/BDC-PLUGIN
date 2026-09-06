// Custom Standalone app shell. JUCE's default Standalone window puts
// minimise/close on the right and its "Options" button on the left; the
// user wants that flipped (minimise/close on the left, Options on the
// right), which isn't something JUCE exposes a parameter for - so this
// file opts in to providing our own JUCEApplication (via
// JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP, set in CMakeLists.txt) and
// subclasses StandaloneFilterWindow to reposition those buttons after
// JUCE lays them out. Everything else is a close copy of JUCE's own
// juce_audio_plugin_client_Standalone.cpp.

#include <juce_core/system/juce_TargetPlatform.h>

#if JucePlugin_Build_Standalone

#include <juce_audio_plugin_client/detail/juce_IncludeModuleHeaders.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

namespace
{
    class BDCStandaloneWindow : public juce::StandaloneFilterWindow
    {
    public:
        BDCStandaloneWindow (const juce::String& title, juce::Colour backgroundColour,
                             std::unique_ptr<juce::StandalonePluginHolder> holder)
            : juce::StandaloneFilterWindow (title, backgroundColour, std::move (holder))
        {
           #if ! (JUCE_IOS || JUCE_ANDROID)
            setTitleBarButtonsRequired (juce::DocumentWindow::minimiseButton | juce::DocumentWindow::closeButton, true);
           #endif
        }

        void resized() override
        {
            StandaloneFilterWindow::resized(); // normal layout first, including the Options button on the left

            // The Options button is a private member of StandaloneFilterWindow
            // with no accessor, but it's still a plain child component - find
            // it by its button text and move it to the right instead.
            for (int i = 0; i < getNumChildComponents(); ++i)
            {
                if (auto* button = dynamic_cast<juce::TextButton*> (getChildComponent (i)))
                {
                    if (button->getButtonText() == "Options")
                    {
                        button->setBounds (getWidth() - 68, 6, 60, getTitleBarHeight() - 8);
                        break;
                    }
                }
            }
        }
    };

    class BDCStandaloneApp final : public juce::JUCEApplication
    {
    public:
        BDCStandaloneApp()
        {
            juce::PropertiesFile::Options options;
            options.applicationName     = juce::CharPointer_UTF8 (JucePlugin_Name);
            options.filenameSuffix      = ".settings";
            options.osxLibrarySubFolder = "Application Support";
           #if JUCE_LINUX || JUCE_BSD
            options.folderName          = "~/.config";
           #else
            options.folderName          = "";
           #endif

            appProperties.setStorageParameters (options);
        }

        const juce::String getApplicationName() override    { return juce::CharPointer_UTF8 (JucePlugin_Name); }
        const juce::String getApplicationVersion() override  { return JucePlugin_VersionString; }
        bool moreThanOneInstanceAllowed() override           { return true; }
        void anotherInstanceStarted (const juce::String&) override {}

        std::unique_ptr<juce::StandalonePluginHolder> createPluginHolder()
        {
            constexpr auto autoOpenMidiDevices = false;

           #ifdef JucePlugin_PreferredChannelConfigurations
            constexpr juce::StandalonePluginHolder::PluginInOuts channels[] { JucePlugin_PreferredChannelConfigurations };
            const juce::Array<juce::StandalonePluginHolder::PluginInOuts> channelConfig (channels, juce::numElementsInArray (channels));
           #else
            const juce::Array<juce::StandalonePluginHolder::PluginInOuts> channelConfig;
           #endif

            return std::make_unique<juce::StandalonePluginHolder> (appProperties.getUserSettings(),
                                                                     false,
                                                                     juce::String{},
                                                                     nullptr,
                                                                     channelConfig,
                                                                     autoOpenMidiDevices);
        }

        void initialise (const juce::String&) override
        {
            if (juce::Desktop::getInstance().getDisplays().displays.isEmpty())
            {
                jassertfalse;
                return;
            }

            mainWindow = std::make_unique<BDCStandaloneWindow> (
                getApplicationName(),
                juce::LookAndFeel::getDefaultLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId),
                createPluginHolder());

            mainWindow->setVisible (true);
        }

        void shutdown() override
        {
            mainWindow = nullptr;
            appProperties.saveIfNeeded();
        }

        void systemRequestedQuit() override
        {
            if (mainWindow != nullptr)
                mainWindow->pluginHolder->savePluginState();

            if (juce::ModalComponentManager::getInstance()->cancelAllModalComponents())
            {
                juce::Timer::callAfterDelay (100, []()
                {
                    if (auto app = juce::JUCEApplicationBase::getInstance())
                        app->systemRequestedQuit();
                });
            }
            else
            {
                quit();
            }
        }

    private:
        juce::ApplicationProperties appProperties;
        std::unique_ptr<BDCStandaloneWindow> mainWindow;
    };
}

// main() itself is still provided by JUCE's own
// juce_audio_plugin_client_Standalone.cpp (also compiled into this target)
// - it just needs juce_CreateApplication() to exist, which this supplies.
// Defining JUCE_MAIN_FUNCTION_DEFINITION here too would double-define main().
juce::JUCEApplicationBase* juce_CreateApplication() { return new BDCStandaloneApp(); }

#endif // JucePlugin_Build_Standalone
