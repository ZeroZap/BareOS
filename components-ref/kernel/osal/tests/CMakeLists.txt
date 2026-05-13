##
## @file CMakeLists.txt
## @brief OSAL Unit Tests CMake Configuration
## @version 1.0
## @date 2026-02-28
##

cmake_minimum_required(VERSION 3.12)

project(osal_tests C)

# Enable testing
enable_testing()

# Test configuration
set(OSAL_TEST_BACKEND "baremetal" CACHE STRING "OSAL backend for tests")
set_property(CACHE OSAL_TEST_BACKEND PROPERTY STRINGS baremetal freertos)

# Unity test framework
set(UNITY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/unity.c
    ${CMAKE_CURRENT_SOURCE_DIR}/unity.h
)

# OSAL sources for testing
set(OSAL_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/xy_os_tick.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/xy_os_timer_sw.c
)

# Backend-specific sources
if(OSAL_TEST_BACKEND STREQUAL "baremetal")
    set(OSAL_BACKEND_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../backend/baremetal/xy_os_baremetal.c)
    set(OSAL_BACKEND_DEF XY_OS_BACKEND_BAREMETAL)
elseif(OSAL_TEST_BACKEND STREQUAL "freertos")
    set(OSAL_BACKEND_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../backend/freertos/osal_freertos.c)
    set(OSAL_BACKEND_DEF XY_OS_BACKEND_FREERTOS)
    # Add FreeRTOS include path
    include_directories(${CMAKE_SOURCE_DIR}/third_party/freertos/include)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Test executable
add_executable(test_osal
    ${UNITY_SOURCES}
    ${OSAL_TEST_SOURCES}
    ${OSAL_BACKEND_SRC}
    ${CMAKE_CURRENT_SOURCE_DIR}/test_osal.c
)

# Compile definitions
target_compile_definitions(test_osal PUBLIC
    ${OSAL_BACKEND_DEF}
    UNITY_SUPPORT_TEST_CASES
)

# Compile options
target_compile_options(test_osal PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -O0
    -g
)

# Register test with CTest
add_test(
    NAME OSAL_Unit_Tests
    COMMAND test_osal
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Coverage option (optional)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
if(ENABLE_COVERAGE)
    target_compile_options(test_osal PRIVATE --coverage)
    target_link_options(test_osal PRIVATE --coverage)
endif()

# Print configuration
message(STATUS "OSAL Test Configuration:")
message(STATUS "  Backend: ${OSAL_TEST_BACKEND}")
message(STATUS "  Coverage: ${ENABLE_COVERAGE}")
