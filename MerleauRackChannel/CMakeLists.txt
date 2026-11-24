cmake_minimum_required(VERSION 3.22)

project(MerleauRackChannel VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Formats construits : par defaut AU + VST3 + application autonome (macOS)
if(APPLE)
    set(MR_FORMATS AU VST3 Standalone CACHE STRING "Plugin formats")
else()
    set(MR_FORMATS VST3 CACHE STRING "Plugin formats")
endif()

# JUCE (telecharge automatiquement)
include(FetchContent)
set(MR_JUCE_TAG "8.0.4" CACHE STRING "JUCE git tag")
if(DEFINED ENV{JUCE_DIR} AND EXISTS "$ENV{JUCE_DIR}/CMakeLists.txt")
    add_subdirectory("$ENV{JUCE_DIR}" JUCE)
else()
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        ${MR_JUCE_TAG}
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(MerleauRackChannel
    COMPANY_NAME                "Merleau Audio"
    COMPANY_WEBSITE             "https://merleau.audio"
    BUNDLE_ID                   com.merleauaudio.merleaurackchannel
    PLUGIN_MANUFACTURER_CODE    Merl
    PLUGIN_CODE                 Mrck
    PRODUCT_NAME                "MerleauRack Channel"
    VERSION                     ${PROJECT_VERSION}
    FORMATS                     ${MR_FORMATS}
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Fx Dynamics EQ
    AU_MAIN_TYPE                kAudioUnitType_Effect
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT  "MerleauRack Channel a besoin de l'entree audio pour traiter le son en direct.")

target_sources(MerleauRackChannel PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(MerleauRackChannel PRIVATE Source)

target_compile_definitions(MerleauRackChannel PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MerleauRackChannel
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
