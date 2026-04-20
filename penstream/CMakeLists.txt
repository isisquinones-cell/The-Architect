cmake_minimum_required(VERSION 3.25)
project(PenStream VERSION 1.0.0 LANGUAGES CXX)

# C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(BUILD_SERVER "Build Windows server" ON)
option(BUILD_ANDROID "Build Android client (via Gradle)" OFF)
option(ENABLE_TESTS "Build unit tests" OFF)
option(ENABLE_NVENC "Enable NVIDIA NVENC support" ON)
option(ENABLE_AMF "Enable AMD AMF support" OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Server build (Windows only)
if(BUILD_SERVER AND WIN32)
    message(STATUS "Building PenStream Server for Windows...")
    add_subdirectory(server)
endif()

# Android client is built via Gradle, not CMake
if(BUILD_ANDROID)
    message(STATUS "Android client should be built via Gradle:")
    message(STATUS "  cd android && gradlew.bat assembleDebug")
endif()

# Tests (optional)
if(ENABLE_TESTS)
    enable_testing()
    message(STATUS "Tests enabled - run with: ctest --output-on-failure")
endif()

# Installation
install(DIRECTORY scripts/ DESTINATION scripts)
install(FILES README.md LICENSE config.json DESTINATION .)
