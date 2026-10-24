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

# GoodLookinUI: plugin-free UI toolkit (vendored runtime headers).
set(GOODLOOKINUI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/GoodLookinUI" CACHE PATH "GoodLookinUI toolkit location")
if(NOT EXISTS "${GOODLOOKINUI_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR
        "GoodLookinUI not found at ${GOODLOOKINUI_ROOT}.\n"
        "Restore the vendored third_party/GoodLookinUI directory\n"
        "or pass -DGOODLOOKINUI_ROOT=<path to a checkout>.")
endif()
add_subdirectory("${GOODLOOKINUI_ROOT}" "${CMAKE_CURRENT_BINARY_DIR}/goodlookinui")

# HQ uses the authored numerical circuit and an isolated shared simulator per instance.
# The authored numerical runtime model is public; private reference documents are never build inputs.
option(BLUECHANNEL_HQ "Enable the offline component solver" OFF)
if(BLUECHANNEL_HQ)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    find_path(BLUECHANNEL_NGSPICE_INCLUDE ngspice/sharedspice.h HINTS /opt/homebrew/include)
    find_library(BLUECHANNEL_NGSPICE_LIBRARY NAMES ngspice libngspice HINTS /opt/homebrew/lib)
    if(NOT BLUECHANNEL_NGSPICE_INCLUDE OR NOT BLUECHANNEL_NGSPICE_LIBRARY)
        message(FATAL_ERROR "HQ requires the shared ngspice library and development header")
    endif()
    if(APPLE AND BLUECHANNEL_UNIVERSAL)
        execute_process(COMMAND lipo "${BLUECHANNEL_NGSPICE_LIBRARY}" -verify_arch arm64 x86_64
            RESULT_VARIABLE hq_arch_check ERROR_VARIABLE hq_arch_error)
        if(NOT hq_arch_check EQUAL 0)
            message(FATAL_ERROR "Universal HQ requires an arm64+x86_64 simulator: ${hq_arch_error}")
        endif()
    endif()
    get_filename_component(BLUECHANNEL_HQ_LIBRARY_NAME "${BLUECHANNEL_NGSPICE_LIBRARY}" NAME)
    get_filename_component(BLUECHANNEL_HQ_LIBRARY_REAL "${BLUECHANNEL_NGSPICE_LIBRARY}" REALPATH)
    get_filename_component(hq_library_dir "${BLUECHANNEL_HQ_LIBRARY_REAL}" DIRECTORY)
    get_filename_component(hq_prefix "${hq_library_dir}" DIRECTORY)
    set(BLUECHANNEL_HQ_NOTICES "${hq_prefix}/COPYING" CACHE FILEPATH "Simulator distribution licence notices")
    if(NOT EXISTS "${BLUECHANNEL_HQ_NOTICES}")
        message(FATAL_ERROR "Set BLUECHANNEL_HQ_NOTICES to the simulator's complete COPYING notices")
    endif()
endif()

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
    ICON_BIG                 "${CMAKE_CURRENT_SOURCE_DIR}/assets/blue-channel-app-icon.png"
    ICON_SMALL               "${CMAKE_CURRENT_SOURCE_DIR}/assets/blue-channel-app-icon.png"
    FORMATS                  ${BLUECHANNEL_FORMATS}
    PRODUCT_NAME             "Blue Channel"
    DESCRIPTION              "Analogue-modelled channel strip"
    VST3_CATEGORIES          Fx Dynamics EQ
    AU_MAIN_TYPE             kAudioUnitType_Effect
    AAX_CATEGORY             AAX_ePlugInCategory_Dynamics
    LV2URI                   "urn:opengrid:bluechannel"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Blue Channel uses audio input to process live sound in the standalone application."
    HARDENED_RUNTIME_ENABLED TRUE
    HARDENED_RUNTIME_OPTIONS com.apple.security.device.audio-input
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

# Knob design: Designs/knobs.csv is baked into the editor. Developer builds also read and autosave the file in
# place (see Source/UI/DevLayout.h).
option(BLUECHANNEL_DEV_EDITOR "Developer mode: move and resize knobs in the editor and save Designs/knobs.csv" OFF)
set(BLUECHANNEL_DESIGN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Designs/knobs.csv")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${BLUECHANNEL_DESIGN_FILE}")
set(BLUECHANNEL_KNOB_DESIGN "")
if(EXISTS "${BLUECHANNEL_DESIGN_FILE}")
    file(READ "${BLUECHANNEL_DESIGN_FILE}" BLUECHANNEL_KNOB_DESIGN)
endif()
set(BLUECHANNEL_EDITOR_LAYOUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/Designs/editor-layout.json")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${BLUECHANNEL_EDITOR_LAYOUT_FILE}")
file(READ "${BLUECHANNEL_EDITOR_LAYOUT_FILE}" BLUECHANNEL_EDITOR_LAYOUT)
configure_file(Source/UI/EmbeddedDesign.h.in generated/EmbeddedDesign.h @ONLY)

target_sources(BlueChannel PRIVATE
    Source/Plugin/HQRenderer.cpp
    Source/Plugin/Params.cpp
    Source/Plugin/PluginProcessor.cpp
    Source/Plugin/PluginEditor.cpp
)
target_include_directories(BlueChannel PUBLIC Source Source/Plugin Source/UI "${GOODLOOKINUI_ROOT}/adapters/juce"
                           "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_compile_definitions(BlueChannel PUBLIC
    BLUECHANNEL_HQ=$<BOOL:${BLUECHANNEL_HQ}>
    BLUECHANNEL_DEV_EDITOR=$<BOOL:${BLUECHANNEL_DEV_EDITOR}>
    GOODLOOKINUI_ENABLE_EDITOR=$<BOOL:${BLUECHANNEL_DEV_EDITOR}>
    BLUECHANNEL_DESIGN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Designs"
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    BLUECHANNEL_VERSION="${BLUECHANNEL_VERSION_STR}"
)
if(BLUECHANNEL_HQ)
    target_include_directories(BlueChannel PRIVATE "${BLUECHANNEL_NGSPICE_INCLUDE}")
    target_compile_definitions(BlueChannel PRIVATE
        BLUECHANNEL_HQ_LIBRARY="${BLUECHANNEL_NGSPICE_LIBRARY}"
        BLUECHANNEL_HQ_LIBRARY_NAME="${BLUECHANNEL_HQ_LIBRARY_NAME}")
endif()
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

# Place the dynamically loaded simulator alongside the native plugin executable. Re-sign after copying.
if(BLUECHANNEL_HQ AND APPLE)
    foreach(hq_target BlueChannel_Standalone BlueChannel_AU BlueChannel_VST3 BlueChannel_CLAP)
        if(TARGET ${hq_target})
            add_custom_command(TARGET ${hq_target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${BLUECHANNEL_HQ_LIBRARY_REAL}"
                    "$<TARGET_BUNDLE_DIR:${hq_target}>/Contents/MacOS/${BLUECHANNEL_HQ_LIBRARY_NAME}"
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:${hq_target}>/Contents/Resources/ThirdParty"
                COMMAND ${CMAKE_COMMAND} -E copy "${BLUECHANNEL_HQ_NOTICES}"
                    "$<TARGET_BUNDLE_DIR:${hq_target}>/Contents/Resources/ThirdParty/ngspice-COPYING.txt"
                COMMAND codesign --force --sign - "$<TARGET_BUNDLE_DIR:${hq_target}>/Contents/MacOS/${BLUECHANNEL_HQ_LIBRARY_NAME}"
                COMMAND codesign --force --sign - "$<TARGET_BUNDLE_DIR:${hq_target}>"
                VERBATIM)
        endif()
    endforeach()
endif()

if(BLUECHANNEL_HQ)
    juce_add_console_app(BlueChannelHQTest PRODUCT_NAME "Blue Channel HQ Test")
    target_sources(BlueChannelHQTest PRIVATE tests/hq_test.cpp)
    target_link_libraries(BlueChannelHQTest PRIVATE BlueChannel juce::juce_audio_utils juce::juce_dsp)
    add_test(NAME hq_component_solver COMMAND BlueChannelHQTest)
    set_tests_properties(hq_component_solver PROPERTIES TIMEOUT 240)
endif()

if(BLUECHANNEL_HQ AND TARGET BlueChannel_LV2)
    add_custom_command(TARGET BlueChannel_LV2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${BLUECHANNEL_HQ_LIBRARY_REAL}"
            "$<TARGET_FILE_DIR:BlueChannel_LV2>/${BLUECHANNEL_HQ_LIBRARY_NAME}"
        COMMAND ${CMAKE_COMMAND} -E copy "${BLUECHANNEL_HQ_NOTICES}"
            "$<TARGET_FILE_DIR:BlueChannel_LV2>/ngspice-COPYING.txt"
        VERBATIM)
endif()
