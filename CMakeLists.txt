cmake_minimum_required(VERSION 3.22)
project(Imperial VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Point to local JUCE
add_subdirectory($ENV{HOME}/JUCE juce_build)

juce_add_plugin(Imperial
    COMPANY_NAME "niklaskroe"
    PLUGIN_MANUFACTURER_CODE Nklk
    PLUGIN_CODE TkIm
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Imperial"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES "Fx" "Distortion"
    AU_MAIN_TYPE kAudioUnitType_Effect
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Imperial needs microphone access to process your guitar signal."
)

juce_generate_juce_header(Imperial)

target_sources(Imperial PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/PreampStage.cpp
    Source/PowerAmpStage.cpp
    Source/CabinetStage.cpp
)

target_compile_definitions(Imperial PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Imperial PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
