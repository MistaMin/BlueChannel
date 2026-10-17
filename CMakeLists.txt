cmake_minimum_required(VERSION 3.22)

# Single source of truth for the version: VERSION (MAJOR.MINOR.PATCH).
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" BLUECHANNEL_VERSION_STR)
string(STRIP "${BLUECHANNEL_VERSION_STR}" BLUECHANNEL_VERSION_STR)

# macOS release builds: -DBLUECHANNEL_UNIVERSAL=ON builds arm64 + x86_64.
option(BLUECHANNEL_UNIVERSAL "Build arm64 + x86_64 on macOS" OFF)
if(APPLE AND BLUECHANNEL_UNIVERSAL)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(BlueChannel VERSION ${BLUECHANNEL_VERSION_STR} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
    add_compile_definitions(_USE_MATH_DEFINES NOMINMAX)
endif()

enable_testing()

# --- Engine tools and tests: no JUCE, every platform ------------------------------------------------
add_executable(BlueChannelStageTest tests/stage_test.cpp)
target_include_directories(BlueChannelStageTest PRIVATE Source)
add_test(NAME stage_netlists COMMAND BlueChannelStageTest ${CMAKE_CURRENT_SOURCE_DIR}/tests/reference)

add_executable(BlueChannelModelTest tests/model_test.cpp)
target_include_directories(BlueChannelModelTest PRIVATE Source)
add_test(NAME model COMMAND BlueChannelModelTest)

add_executable(BlueChannelCalibrate tests/calibrate.cpp)
target_include_directories(BlueChannelCalibrate PRIVATE Source)

add_executable(BlueChannelBench tests/bench.cpp)
target_include_directories(BlueChannelBench PRIVATE Source)

add_executable(BlueChannelMeasure tests/measure.cpp)
target_include_directories(BlueChannelMeasure PRIVATE Source)

# --- JUCE -------------------------------------------------------------------------------------------
option(BLUECHANNEL_BUILD_PLUGIN "Build the JUCE plugin targets" ON)
if(NOT BLUECHANNEL_BUILD_PLUGIN)
    return()
endif()

include(FetchContent)
set(BLUECHANNEL_JUCE_SOURCE "" CACHE PATH "Use an existing JUCE checkout instead of downloading")
if(BLUECHANNEL_JUCE_SOURCE)
    set(FETCHCONTENT_SOURCE_DIR_JUCE "${BLUECHANNEL_JUCE_SOURCE}")
endif()
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        9.0.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# GoodLookinUI: plugin-free UI toolkit (git submodule).
set(GOODLOOKINUI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/GoodLookinUI" CACHE PATH "GoodLookinUI toolkit location")
if(NOT EXISTS "${GOODLOOKINUI_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR
        "GoodLookinUI not found at ${GOODLOOKINUI_ROOT}.\n"
        "Run: git submodule update --init --recursive\n"
        "or pass -DGOODLOOKINUI_ROOT=<path to a checkout>.")
endif()
add_subdirectory("${GOODLOOKINUI_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/goodlookinui")

# --- Formats ----------------------------------------------------------------------------------------
# VST3, LV2 and Standalone everywhere, AU on macOS, CLAP through clap-juce-extensions. AAX is built when an
# AAX SDK is present (not distributed; see docs/RELEASE.md).
set(BLUECHANNEL_FORMATS VST3 LV2 Standalone)
if(APPLE)
    list(APPEND BLUECHANNEL_FORMATS AU)
endif()
set(BLUECHANNEL_AAX_SDK "${CMAKE_CURRENT_SOURCE_DIR}/../aax-sdk-2-8-1" CACHE PATH "Optional AAX SDK location")
option(BLUECHANNEL_BUILD_AAX "Build the AAX format when the SDK is available" ON)
if(BLUECHANNEL_BUILD_AAX AND (APPLE OR WIN32) AND EXISTS "${BLUECHANNEL_AAX_SDK}/Interfaces/AAX.h")
    juce_set_aax_sdk_path("${BLUECHANNEL_AAX_SDK}")
    list(APPEND BLUECHANNEL_FORMATS AAX)
else()
    message(STATUS "AAX SDK not used")
endif()
message(STATUS "Blue Channel formats: ${BLUECHANNEL_FORMATS}")

juce_add_plugin(BlueChannel
    COMPANY_NAME             "OpenGrid"
    COMPANY_WEBSITE          ""
    BUNDLE_ID                com.opengrid.bluechannel
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    NEEDS_MIDI_OUTPUT        FALSE
    PLUGIN_MANUFACTURER_CODE OpGr
    PLUGIN_CODE              BlCh
    FORMATS                  ${BLUECHANNEL_FORMATS}
    PRODUCT_NAME             "Blue Channel"
    DESCRIPTION              "Analogue-modelled channel strip"
    VST3_CATEGORIES          Fx Dynamics EQ
    AU_MAIN_TYPE             kAudioUnitType_Effect
    AAX_CATEGORY             AAX_ePlugInCategory_Dynamics
    LV2URI                   "urn:opengrid:bluechannel"
    COPY_PLUGIN_AFTER_BUILD  FALSE
)

option(BLUECHANNEL_BUILD_CLAP "Build the CLAP plugin" ON)
if(BLUECHANNEL_BUILD_CLAP)
    set(BLUECHANNEL_CLAP_EXT_SOURCE "" CACHE PATH "Use an existing clap-juce-extensions checkout")
    if(BLUECHANNEL_CLAP_EXT_SOURCE)
        set(FETCHCONTENT_SOURCE_DIR_CLAP-JUCE-EXTENSIONS "${BLUECHANNEL_CLAP_EXT_SOURCE}")
    endif()
    FetchContent_Declare(
        clap-juce-extensions
        GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
        # Pinned commit on main (the last tag predates JUCE 8).
        GIT_TAG        55525c9858d4b25687be7759a5e0f70eccef218e
        GIT_SUBMODULES_RECURSE TRUE
    )
    FetchContent_MakeAvailable(clap-juce-extensions)
endif()

target_sources(BlueChannel PRIVATE
    Source/Plugin/Params.cpp
    Source/Plugin/PluginProcessor.cpp
    Source/Plugin/PluginEditor.cpp
)
target_include_directories(BlueChannel PRIVATE Source Source/Plugin Source/UI "${GOODLOOKINUI_ROOT}/adapters/juce")
target_compile_definitions(BlueChannel PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    BLUECHANNEL_VERSION="${BLUECHANNEL_VERSION_STR}"
)
target_link_libraries(BlueChannel
    PRIVATE
        goodlookinui_juce
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Refresh the local ad-hoc bundle seals after JUCE writes moduleinfo.json (Developer ID signing happens in
# the installer flow).
if(APPLE)
    foreach(local_bundle BlueChannel_VST3 BlueChannel_AU BlueChannel_Standalone)
        if(TARGET ${local_bundle})
            add_custom_command(TARGET ${local_bundle} POST_BUILD
                COMMAND ${CMAKE_COMMAND} "-Dsrc=$<TARGET_BUNDLE_DIR:${local_bundle}>"
                    -P "${JUCE_SOURCE_DIR}/extras/Build/CMake/checkBundleSigning.cmake"
                VERBATIM)
        endif()
    endforeach()
endif()

if(BLUECHANNEL_BUILD_CLAP)
    clap_juce_extensions_plugin(TARGET BlueChannel
        CLAP_ID "com.opengrid.bluechannel"
        CLAP_FEATURES audio-effect equalizer compressor limiter mono stereo
        CLAP_MANUAL_URL ""
        CLAP_SUPPORT_URL "")
endif()

# --- Host-level tests of the real processor and editor ---------------------------------------------
juce_add_console_app(BlueChannelProcessorTest PRODUCT_NAME "Blue Channel Processor Test")
target_sources(BlueChannelProcessorTest PRIVATE tests/processor_test.cpp)
target_include_directories(BlueChannelProcessorTest PRIVATE Source Source/Plugin Source/UI "${GOODLOOKINUI_ROOT}/adapters/juce")
target_compile_definitions(BlueChannelProcessorTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(BlueChannelProcessorTest PRIVATE BlueChannel goodlookinui_juce juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
add_test(NAME processor COMMAND BlueChannelProcessorTest)

juce_add_console_app(BlueChannelSnapshot PRODUCT_NAME "Blue Channel Snapshot")
target_sources(BlueChannelSnapshot PRIVATE tests/ui_snapshot.cpp)
target_include_directories(BlueChannelSnapshot PRIVATE Source Source/Plugin Source/UI "${GOODLOOKINUI_ROOT}/adapters/juce")
target_compile_definitions(BlueChannelSnapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_link_libraries(BlueChannelSnapshot PRIVATE BlueChannel goodlookinui_juce juce::juce_audio_utils juce::juce_recommended_config_flags)
