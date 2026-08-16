cmake_minimum_required(VERSION 3.22)
project(ParamedicEQ VERSION 2.0.5)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.13
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_binary_data(ParamedicAssets
    SOURCES Assets/xcore_logo_gold.png
)

juce_add_plugin(ParamedicEQ
    COMPANY_NAME "Dvinesoul"
    PRODUCT_NAME "Paramedic EQ"
    PLUGIN_MANUFACTURER_CODE Dvsl
    PLUGIN_CODE PmEQ
    FORMATS VST3 Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES "Fx" "EQ"
)

juce_generate_juce_header(ParamedicEQ)

target_sources(ParamedicEQ PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(ParamedicEQ PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(ParamedicEQ PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    ParamedicAssets
)
