cmake_minimum_required(VERSION 3.22)

project(BDCPlugin VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(BDCPlugin
    COMPANY_NAME "Been Dead Creative"
    BUNDLE_ID "com.beendeadcreative.bdcplugin"
    PLUGIN_MANUFACTURER_CODE Bdcm
    PLUGIN_CODE Bdc1
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "BDC"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "BDC needs audio input access to capture what you play."
)

juce_generate_juce_header(BDCPlugin)

juce_add_binary_data(BDCPluginAssets
    SOURCES
        Resources/TAYLennon-Regular.otf
        Resources/BDCLogo.png
)

target_sources(BDCPlugin
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Presets.cpp
        Source/UI/BDCLookAndFeel.cpp
        Source/DSP/CircularBuffer.cpp
        Source/DSP/InputActivityDetector.cpp
        Source/DSP/TransientDetector.cpp
        Source/DSP/PitchDetector.cpp
        Source/DSP/KeyTracker.cpp
        Source/DSP/GenerativeEngine.cpp
        Source/DSP/Granulator.cpp
        Source/DSP/ChorusModule.cpp
        Source/DSP/RotaryModule.cpp
        Source/DSP/DelayModule.cpp
        Source/DSP/TapeModule.cpp
        Source/DSP/AnalogGlueModule.cpp
)

target_compile_definitions(BDCPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# Standalone-only: our own app shell (Source/StandaloneApp.cpp) swaps which
# side the title bar's minimise/close and Options buttons sit on. Only the
# Standalone target needs this - AU/VST3 have no such window, since the
# host provides its own chrome around the plugin editor.
if (TARGET BDCPlugin_Standalone)
    target_sources(BDCPlugin_Standalone
        PRIVATE
            Source/StandaloneApp.cpp
    )
    target_compile_definitions(BDCPlugin_Standalone
        PRIVATE
            JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
    )
endif()

target_link_libraries(BDCPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        BDCPluginAssets
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
