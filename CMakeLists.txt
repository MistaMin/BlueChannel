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

# --- Engine tools and tests of the model in Source/. The plugin itself is built from plugin/. --------
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
