cmake_minimum_required(VERSION 3.22)
project(Fuzz14 VERSION 0.1.0)

# Build the C++ runtime into the exe / VST3, so they run on PCs without the Visual C++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_library(circuit STATIC Source/circuit/Circuit.cpp Source/circuit/Fuzz14.cpp)
target_compile_features(circuit PUBLIC cxx_std_17)

# checks the solver against ngspice (tools/compare.py)
add_executable(verify tools/verify.cpp)
target_link_libraries(verify PRIVATE circuit)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_binary_data(Fuzz14Skin SOURCES Source/skin.svg)

set(FUZZ14_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

set(FUZZ14_DEFINITIONS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_ASIO=1                      # ASIO in the standalone app
    JUCE_VST3_CAN_REPLACE_VST2=0
    FUZZ14_VERSION="${PROJECT_VERSION}")

juce_add_plugin(Fuzz14
    COMPANY_NAME "Cursed Entertainment"
    PLUGIN_MANUFACTURER_CODE Crsd
    PLUGIN_CODE Fz14
    FORMATS VST3 Standalone
    PRODUCT_NAME "Fuzz14"
    DESCRIPTION "A 2014 breadboard fuzz, simulated part by part"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx Distortion
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Fuzz14 PRIVATE ${FUZZ14_SOURCES})
target_compile_features(Fuzz14 PRIVATE cxx_std_17)
target_compile_definitions(Fuzz14 PUBLIC ${FUZZ14_DEFINITIONS})
target_link_libraries(Fuzz14
    PRIVATE circuit Fuzz14Skin juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# Offline test: the window to a PNG, a synthetic guitar through the pedal to WAVs, and timing.
juce_add_console_app(Fuzz14Render PRODUCT_NAME "Fuzz14Render")
target_sources(Fuzz14Render PRIVATE tools/render.cpp ${FUZZ14_SOURCES})
target_compile_features(Fuzz14Render PRIVATE cxx_std_17)
target_compile_definitions(Fuzz14Render PRIVATE ${FUZZ14_DEFINITIONS} JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(Fuzz14Render
    PRIVATE circuit Fuzz14Skin juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
