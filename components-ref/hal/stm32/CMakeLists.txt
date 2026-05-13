##
## @file CMakeLists.txt
## @brief CMake configuration for XY HAL STM32
## @version 2.0
## @date 2025-10-26
##

cmake_minimum_required(VERSION 3.12)

# Project name
project(xy_hal_stm32 C)

# STM32 Sub-series - modify according to your MCU
set(STM32_SERIES "stm32f4" CACHE STRING "STM32 Series (e.g., stm32f4, stm32u0, stm32l4, stm32h7)")
set(STM32_FAMILY "STM32F4" CACHE STRING "STM32 Family (e.g., STM32F1, STM32F4, STM32L4)")
set(STM32_DEVICE "STM32F407xx" CACHE STRING "STM32 Device (e.g., STM32F407xx)")

# Compiler settings
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Source files from the selected series directory
file(GLOB HAL_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/${STM32_SERIES}/*.c"
)

# Header files
set(HAL_HEADERS
    ../inc/xy_hal.h
    ../inc/xy_hal_pin.h
    ../inc/xy_hal_uart.h
    ../inc/xy_hal_spi.h
    ../inc/xy_hal_i2c.h
    ../inc/xy_hal_timer.h
    ../inc/xy_hal_pwm.h
    ../inc/xy_hal_rtc.h
    ../inc/xy_hal_dma.h
    ../inc/xy_hal_lp_timer.h
    stm32_hal.h
)

# Create library
add_library(xy_hal_stm32 STATIC ${HAL_SOURCES})

# Include directories
target_include_directories(xy_hal_stm32 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Compile definitions
target_compile_definitions(xy_hal_stm32 PUBLIC
    ${STM32_FAMILY}
    ${STM32_DEVICE}
    STM32_HAL_ENABLED
    USE_HAL_DRIVER
)

# Link with STM32 HAL (if available as a library)
if(TARGET stm32_hal)
    target_link_libraries(xy_hal_stm32 PUBLIC stm32_hal)
endif()

# Installation
install(TARGETS xy_hal_stm32
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${HAL_HEADERS}
    DESTINATION include/xy_hal
)

# Example target (optional)
option(BUILD_HAL_EXAMPLES "Build XY HAL examples" OFF)
if(BUILD_HAL_EXAMPLES)
    add_executable(xy_hal_example example_usage.c)
    target_link_libraries(xy_hal_example xy_hal_stm32)
endif()

# Print configuration
message(STATUS "XY HAL Configuration:")
message(STATUS "  STM32 Series: ${STM32_SERIES}")
message(STATUS "  STM32 Family: ${STM32_FAMILY}")
message(STATUS "  STM32 Device: ${STM32_DEVICE}")
message(STATUS "  Build Examples: ${BUILD_HAL_EXAMPLES}")

##
## Usage Example:
##
## 1. In your main CMakeLists.txt:
##
##    # Set STM32 configuration
##    set(STM32_SERIES "stm32f4")  # or stm32u0, stm32l4, stm32h7
##    set(STM32_FAMILY "STM32F4")
##    set(STM32_DEVICE "STM32F407xx")
##
##    # Add XY HAL
##    add_subdirectory(bsp/xy_hal/stm32)
##
##    # Link with your application
##    target_link_libraries(your_app xy_hal_stm32)
##
## 2. Build:
##    mkdir build && cd build
##    cmake .. -DSTM32_SERIES=stm32f4 -DSTM32_FAMILY=STM32F4 -DSTM32_DEVICE=STM32F407xx
##    make
##
