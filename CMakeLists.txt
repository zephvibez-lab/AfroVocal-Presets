cmake_minimum_required(VERSION 3.22)
project(AfroVocalPresets VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Some minimal Linux sandbox images omit this internal rule after JUCE enables
# its helper C targets. Define the standard CMake rule explicitly so configure
# remains portable without changing the plug-in's source or DSP code.
if(NOT DEFINED CMAKE_C_COMPILE_OBJECT)
    set(CMAKE_C_COMPILE_OBJECT "<CMAKE_C_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>" CACHE STRING "C compiler rule" FORCE)
endif()

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.10
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(AfroVocalPresets
    COMPANY_NAME "Personal Audio Tools"
    BUNDLE_ID "com.personalaudiotools.afrovocalpresets"
    PLUGIN_MANUFACTURER_CODE Pato
    PLUGIN_CODE Avp1
    FORMATS VST3 Standalone
    PRODUCT_NAME "AfroVocal Presets"
    VERSION "0.1.0"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

juce_generate_juce_header(AfroVocalPresets)

target_sources(AfroVocalPresets PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
)

target_compile_definitions(AfroVocalPresets PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(AfroVocalPresets PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

if(MSVC)
    target_compile_options(AfroVocalPresets PRIVATE /W4)
else()
    target_compile_options(AfroVocalPresets PRIVATE -Wall -Wextra -Wpedantic)
    if(WIN32)
        target_compile_options(AfroVocalPresets PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-include> $<$<COMPILE_LANGUAGE:CXX>:cstring>)
    endif()
endif()
