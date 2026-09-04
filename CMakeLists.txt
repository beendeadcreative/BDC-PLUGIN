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
    COMPANY_NAME "BDC"
    PLUGIN_MANUFACTURER_CODE Bdcm
    PLUGIN_CODE Bdc1
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "BDC Plugin"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

juce_generate_juce_header(BDCPlugin)

target_sources(BDCPlugin
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/CircularBuffer.cpp
        Source/DSP/SeedGenerator.cpp
        Source/DSP/GenerativeEngine.cpp
        Source/DSP/Granulator.cpp
        Source/DSP/ChorusModule.cpp
        Source/DSP/RotaryModule.cpp
        Source/DSP/DelayModule.cpp
)

target_compile_definitions(BDCPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(BDCPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
