cmake_minimum_required(VERSION 3.18)

project(DroidFakeCam
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "Zygisk module for virtual camera feed injection"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Android NDK minimum API level
set(ANDROID_PLATFORM android-26)

# Module name
set(MODULE_NAME droidfakecam)

# Source files
set(SOURCES
    zygisk_main.cpp
    camera_hook.cpp
    frame_utils.cpp
    media_reader.cpp
)

# Header files
set(HEADERS
    zygisk.hpp
    config.hpp
    camera_hook.hpp
    frame_utils.hpp
    media_reader.hpp
)

# Create shared library
add_library(${MODULE_NAME} SHARED ${SOURCES} ${HEADERS})

# Set output properties for Zygisk loading
set_target_properties(${MODULE_NAME} PROPERTIES
    OUTPUT_NAME ${MODULE_NAME}
    SUFFIX ".so"
    # Strip symbols for release
    LINK_FLAGS_RELEASE "-s"
)

# Include directories
target_include_directories(${MODULE_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ANDROID_NDK}/sources/android/cpufeatures
)

# Link Android libraries
target_link_libraries(${MODULE_NAME}
    # Android logging
    log
    # Android native window/surface
    android
    # Media NDK libraries
    mediandk
    # Dynamic linker
    dl
)

# Compiler flags
target_compile_options(${MODULE_NAME} PRIVATE
    -Wall
    -Wextra
    -Werror=return-type
    -fno-exceptions
    -fno-rtti
    -fvisibility=hidden
    -ffunction-sections
    -fdata-sections
)

# Linker flags
target_link_options(${MODULE_NAME} PRIVATE
    -Wl,--gc-sections
    -Wl,--exclude-libs,ALL
)

# Output to architecture-specific zygisk directory
set(ZYGISK_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/../zygisk")

# Copy library to zygisk directory after build
add_custom_command(TARGET ${MODULE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ZYGISK_OUTPUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${MODULE_NAME}>" 
            "${ZYGISK_OUTPUT_DIR}/${CMAKE_ANDROID_ARCH_ABI}.so"
    COMMENT "Copying ${MODULE_NAME} to ${ZYGISK_OUTPUT_DIR}/${CMAKE_ANDROID_ARCH_ABI}.so"
)

# Debug info
message(STATUS "Building DroidFakeCam for ${CMAKE_ANDROID_ARCH_ABI}")
message(STATUS "Android platform: ${ANDROID_PLATFORM}")
message(STATUS "NDK path: ${ANDROID_NDK}")
