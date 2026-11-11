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

project(Md101 VERSION 1.1.1 LANGUAGES C CXX)

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

juce_add_binary_data(Md101BinaryData
    SOURCES
        assets/translations/en-US.json
        assets/translations/pt-BR.json
)

juce_add_plugin(Md101
    COMPANY_NAME "LU5"
    COMPANY_WEBSITE "https://lu5.art.br"
    VERSION "1.1.1"
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
        Source/LocalizationManager.h
        Source/LocalizationManager.cpp
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
        Md101BinaryData
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

# ==============================================================================
# CMake Install & CPack Packaging Rules (Windows & Linux)
# ==============================================================================
include(GNUInstallDirs)

if (WIN32)
    set(MD101_VST3_INSTALL_DIR "Common Files/VST3" CACHE STRING "VST3 installation subpath")
    set(MD101_STANDALONE_INSTALL_DIR "LU5/Md-101" CACHE STRING "Standalone installation subpath")
else()
    set(MD101_VST3_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/vst3" CACHE STRING "VST3 installation subpath")
    set(MD101_STANDALONE_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}" CACHE STRING "Standalone installation subpath")
endif()

install(
    DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Md101_artefacts/$<CONFIG>/VST3/Md-101.vst3"
    DESTINATION "${MD101_VST3_INSTALL_DIR}"
    COMPONENT VST3
    OPTIONAL
)

install(
    TARGETS Md101_Standalone
    RUNTIME DESTINATION "${MD101_STANDALONE_INSTALL_DIR}"
    BUNDLE  DESTINATION "${MD101_STANDALONE_INSTALL_DIR}"
    COMPONENT Standalone
)

if (UNIX AND NOT APPLE)
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/installer/linux/md-101.desktop"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
        COMPONENT Standalone
    )
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/md-101.svg"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps"
        COMPONENT Standalone
    )
endif()

install(
    FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/README-en.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
    COMPONENT Documentation
)

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/docs/"
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
    COMPONENT Documentation
)

set(CPACK_PACKAGE_NAME "md-101")
set(CPACK_PACKAGE_VENDOR "LU5")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://lu5.art.br")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Md-101 — Periodic Table Polyphonic VST3 & Standalone Synthesizer")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "LU5 <contato@lu5.art.br>")
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.31), libasound2 | libasound2t64, libfreetype6, libx11-6, libxinerama1, libxrandr2, libxcursor1, libxcomposite1")
include(CPack)

