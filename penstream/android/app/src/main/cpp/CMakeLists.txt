cmake_minimum_required(VERSION 3.22)
project(penstream_android)

# C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Android log library
find_library(log-lib log)
find_library(android-lib android)

# Native library
add_library(penstream SHARED
    jni_interface.cpp
    video_decoder.cpp
    renderer.cpp
    input_capture.cpp
    network_client.cpp
)

# Include directories
target_include_directories(penstream PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# Link libraries
target_link_libraries(penstream
    android
    ${android-lib}
    ${log-lib}
    EGL
    GLESv3
    OpenSLES
    mediandk
)

# Compiler flags
target_compile_options(penstream PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
)

# Optimizations for release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(penstream PRIVATE
        -O3
        -DNDEBUG
    )
endif()

# Debug flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(penstream PRIVATE
        -g
        -O0
        -DDEBUG
    )
endif()
