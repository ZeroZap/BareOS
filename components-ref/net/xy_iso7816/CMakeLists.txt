# XY ISO7816 CMake Configuration
cmake_minimum_required(VERSION 3.10)

project(xy_iso7816 VERSION 1.0.0 LANGUAGES C)

# Project options
option(XY_ISO7816_BUILD_EXAMPLES "Build examples" OFF)
option(XY_ISO7816_BUILD_STATIC "Build static library" ON)
option(XY_ISO7816_BUILD_SHARED "Build shared library" OFF)
option(XY_ISO7816_ENABLE_DEBUG "Enable debug logging" OFF)

# C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
set(INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../xy_clib
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../bsp/xy_hal/inc
)

# Source files
set(SOURCES
    xy_iso7816.c
)

# Header files
set(HEADERS
    xy_iso7816.h
    xy_iso7816_cfg.h
)

# Compiler flags
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Debug configuration
if(XY_ISO7816_ENABLE_DEBUG)
    add_definitions(-DXY_ISO7816_CFG_DEBUG_LOG=1)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g)
    endif()
endif()

# ============================================================================
# Library targets
# ============================================================================

# Static library
if(XY_ISO7816_BUILD_STATIC)
    add_library(xy_iso7816_static STATIC ${SOURCES})
    target_include_directories(xy_iso7816_static PUBLIC ${INCLUDE_DIRS})
    set_target_properties(xy_iso7816_static PROPERTIES
        OUTPUT_NAME xy_iso7816
        PUBLIC_HEADER "${HEADERS}"
    )
endif()

# Shared library
if(XY_ISO7816_BUILD_SHARED)
    add_library(xy_iso7816_shared SHARED ${SOURCES})
    target_include_directories(xy_iso7816_shared PUBLIC ${INCLUDE_DIRS})
    set_target_properties(xy_iso7816_shared PROPERTIES
        OUTPUT_NAME xy_iso7816
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        PUBLIC_HEADER "${HEADERS}"
    )
endif()

# ============================================================================
# Examples
# ============================================================================

if(XY_ISO7816_BUILD_EXAMPLES)
    add_executable(xy_iso7816_example xy_iso7816_example.c ${SOURCES})
    target_include_directories(xy_iso7816_example PRIVATE ${INCLUDE_DIRS})
    target_compile_definitions(xy_iso7816_example PRIVATE ISO7816_EXAMPLE_MAIN)
endif()

# ============================================================================
# Installation
# ============================================================================

# Install library
if(XY_ISO7816_BUILD_STATIC)
    install(TARGETS xy_iso7816_static
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include/xy_iso7816
    )
endif()

if(XY_ISO7816_BUILD_SHARED)
    install(TARGETS xy_iso7816_shared
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include/xy_iso7816
    )
endif()

# Install documentation
install(FILES README.md DESTINATION share/doc/xy_iso7816)

# ============================================================================
# Package configuration
# ============================================================================

# Generate package config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/xy_iso7816ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# ============================================================================
# Testing (optional)
# ============================================================================

# Enable testing
option(XY_ISO7816_BUILD_TESTS "Build tests" OFF)

if(XY_ISO7816_BUILD_TESTS)
    enable_testing()
    # Add test cases here
endif()

# ============================================================================
# Status
# ============================================================================

message(STATUS "=== XY ISO7816 Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build static library: ${XY_ISO7816_BUILD_STATIC}")
message(STATUS "Build shared library: ${XY_ISO7816_BUILD_SHARED}")
message(STATUS "Build examples: ${XY_ISO7816_BUILD_EXAMPLES}")
message(STATUS "Enable debug: ${XY_ISO7816_ENABLE_DEBUG}")
message(STATUS "C standard: C${CMAKE_C_STANDARD}")
message(STATUS "================================")
