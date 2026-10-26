cmake_minimum_required(VERSION 3.24)

project(Slrye VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

include(cmake/SlryePlugin.cmake)

option(SLRYE_BUILD_EXAMPLES "Build examples" OFF)
option(SLRYE_BUILD_TESTS    "Build tests"    OFF)
option(SLRYE_BUILD_TOOLS    "Build tools"    OFF)
option(SLRYE_BUILD_PLUGINS  "Build bundled plugins" ON)
option(SLRYE_ENABLE_CONSOLE "Keep a console window attached to the editor executable" ON)

add_subdirectory(slrye)

if(SLRYE_BUILD_PLUGINS)
    add_subdirectory(plugins)
endif()

if(SLRYE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

if(SLRYE_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(SLRYE_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()
