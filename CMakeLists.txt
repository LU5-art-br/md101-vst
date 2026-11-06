cmake_minimum_required(VERSION 3.22)

if (POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
endif()

# Link MSVC C/C++ runtime statically (/MT in Release, /MTd in Debug) so the
# VST3/Standalone binaries have zero dependency on MSVCP140/VCRUNTIME140 DLLs
# (prevents LoadLibraryW ordinal mismatch error 182 on CI and end-user DAWs).
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(Md101 VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MD101_COPY_AFTER_BUILD "Copy built VST3 plugin to system VST3 folder" OFF)
option(MD101_BUILD_TESTS "Build automated DSP & UI verification runner" ON)

# Locate JUCE locally in external/JUCE (offline-friendly) or fetch automatically on fresh machines
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    add_subdirectory(external/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(Md101
    COMPANY_NAME "LU5"
    COMPANY_WEBSITE "https://lu5.art.br"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD ${MD101_COPY_AFTER_BUILD}
    PLUGIN_MANUFACTURER_CODE Lu5A
    PLUGIN_CODE Md01
    FORMATS VST3 Standalone
    PRODUCT_NAME "Md-101"
)

target_sources(Md101
    PRIVATE
        Source/ElementData.h
        Source/ElementData.cpp
        Source/SynthEngine.h
        Source/SynthEngine.cpp
        Source/PianoKeyboard.h
        Source/PianoKeyboard.cpp
        Source/PeriodicTableGrid.h
        Source/PeriodicTableGrid.cpp
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginEditor.cpp
)

target_compile_features(Md101 PUBLIC cxx_std_17)

target_compile_options(Md101
    PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

target_compile_definitions(Md101
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Md101
    PUBLIC
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if (MD101_BUILD_TESTS)
    add_executable(Md101Verify
        Tests/VerifyPlugin.cpp
    )

    target_compile_features(Md101Verify PRIVATE cxx_std_17)
    target_compile_options(Md101Verify PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

    target_link_libraries(Md101Verify
        PRIVATE
            Md101
    )
endif()
