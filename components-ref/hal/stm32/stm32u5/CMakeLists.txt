##
## @file CMakeLists.txt
## @brief CMake configuration for XY HAL STM32U5
## @version 2.0
## @date 2026-02-28
##

cmake_minimum_required(VERSION 3.12)

# Project name
project(xy_hal_stm32u5 C)

# Compiler settings
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# STM32U5 Device Configuration
set(STM32_FAMILY "STM32U5" CACHE STRING "STM32 Family")
set(STM32_DEVICE "STM32U5xx" CACHE STRING "STM32 Device")

# Source files
set(HAL_SOURCES
    xy_hal_pin.c
    xy_hal_uart.c
    xy_hal_spi.c
    xy_hal_i2c.c
    xy_hal_timer.c
    xy_hal_pwm.c
    xy_hal_rtc.c
    xy_hal_dma.c
    xy_hal_adc.c
    xy_hal_dac.c
    xy_hal_flash.c
    xy_hal_wdg.c
    xy_hal_rng.c
    xy_hal_exti.c
    xy_hal_lp_timer.c
    xy_hal_tgpio.c
    xy_hal_ir.c
    xy_hal_i2s.c
    xy_hal_can.c
)

# Header files
set(HAL_HEADERS
    stm32u5_platform.h
    ../inc/xy_hal.h
    ../inc/xy_hal_pin.h
    ../inc/xy_hal_uart.h
    ../inc/xy_hal_spi.h
    ../inc/xy_hal_i2c.h
    ../inc/xy_hal_timer.h
    ../inc/xy_hal_pwm.h
    ../inc/xy_hal_rtc.h
    ../inc/xy_hal_dma.h
    ../inc/xy_hal_adc.h
    ../inc/xy_hal_dac.h
    ../inc/xy_hal_flash.h
    ../inc/xy_hal_wdg.h
    ../inc/xy_hal_rng.h
    ../inc/xy_hal_exti.h
    ../inc/xy_hal_lp_timer.h
    ../inc/xy_hal_tgpio.h
    ../inc/xy_hal_ir.h
    ../inc/xy_hal_i2s.h
    ../inc/xy_hal_can.h
    ../inc/xy_hal_sys.h
)

# Create library
add_library(xy_hal_stm32u5 STATIC ${HAL_SOURCES})

# Include directories
target_include_directories(xy_hal_stm32u5 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../osal/inc
)

# Compile definitions
target_compile_definitions(xy_hal_stm32u5 PUBLIC
    STM32U5
    STM32U5xx
    STM32_HAL_ENABLED
    USE_HAL_DRIVER
)

# Compile options
target_compile_options(xy_hal_stm32u5 PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    -fdata-sections
    -ffunction-sections
)

# Link with STM32U5 HAL (if available)
if(TARGET stm32u5_hal)
    target_link_libraries(xy_hal_stm32u5 PUBLIC stm32u5_hal)
endif()

# Installation
install(TARGETS xy_hal_stm32u5
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${HAL_HEADERS}
    DESTINATION include/xy_hal
)

# Example target
option(BUILD_HAL_EXAMPLES "Build XY HAL examples" OFF)
if(BUILD_HAL_EXAMPLES)
    add_executable(xy_hal_example example_usage.c)
    target_link_libraries(xy_hal_example xy_hal_stm32u5)
endif()

# Print configuration
message(STATUS "XY HAL STM32U5 Configuration:")
message(STATUS "  STM32 Family: ${STM32_FAMILY}")
message(STATUS "  STM32 Device: ${STM32_DEVICE}")
message(STATUS "  Build Examples: ${BUILD_HAL_EXAMPLES}")
