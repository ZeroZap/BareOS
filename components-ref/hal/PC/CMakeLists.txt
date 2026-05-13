cmake_minimum_required(VERSION 3.10)

# HAL PC Simulation Library
project(xy_hal_pc VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)

# PC Simulation Mode
add_definitions(-DXY_HAL_PC_SIM -DHAL_PLATFORM_PC)

message(STATUS "HAL PC: Building PC simulation layer")

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc
)

# Source files
set(SOURCES
    xy_hal_pc.c
    xy_hal_pc_impl.c
    xy_hal_i2c_pc.c
    xy_hal_spi_pc.c
    xy_hal_uart_pc.c
    xy_hal_pin_pc.c
)

# Create library
add_library(xy_hal_pc STATIC ${SOURCES})

# Set include directories for consumers
target_include_directories(xy_hal_pc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

message(STATUS "HAL PC: xy_hal_pc library configured")
