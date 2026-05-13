cmake_minimum_required(VERSION 3.10)
project(xy_device C)

set(CMAKE_C_STANDARD 99)

# Include directories
include_directories(
    inc
    ../hal/inc
    ../kernel/osal
    ../trace/xy_log/inc
)

# Collect source files
set(DEVICE_SOURCES
    xy_device.c
    # xy_device_core.c  # Temporarily disabled - signature mismatches
    xy_eeprom_24xx.c
    xy_oled_ssd1306.c
    xy_mpu6050.c
    xy_bmp280.c
    xy_sht30.c
    xy_ads1115.c
)

set(DEVICE_HEADERS
    xy_device.h
    xy_device_core.h
    xy_eeprom_24xx.h
    xy_oled_ssd1306.h
    xy_mpu6050.h
    xy_bmp280.h
    xy_sht30.h
    xy_ads1115.h
)

# Create library
add_library(xy_device STATIC ${DEVICE_SOURCES})

# HAL Platform Selection (same as xy_hal component)
set(HAL_PLATFORM "PC" CACHE STRING "HAL Platform for device component")

# Include HAL sources based on platform
if(HAL_PLATFORM STREQUAL "PC")
    set(HAL_SOURCES
        ../hal/PC/xy_hal_pc.c
        ../hal/PC/xy_hal_pin_pc.c
        ../hal/PC/xy_hal_i2c_pc.c
        ../hal/PC/xy_hal_spi_pc.c
        ../hal/PC/xy_hal_uart_pc.c
    )
    set(HAL_INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/../hal/inc
        ${CMAKE_CURRENT_SOURCE_DIR}/../hal
    )
    set(HAL_DEFINES
        HAL_PLATFORM_PC
        XY_HAL_PC_SIM
    )
else()
    # For embedded platforms, link with external xy_hal
    message(STATUS "Device: Using external xy_hal for ${HAL_PLATFORM}")
    set(HAL_SOURCES "")
    set(HAL_INCLUDE_DIRS "")
    set(HAL_DEFINES "")
endif()

target_include_directories(xy_device PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${HAL_INCLUDE_DIRS}
)

# Add HAL sources if building for PC
if(HAL_SOURCES)
    target_sources(xy_device PRIVATE ${HAL_SOURCES})
endif()

# Compile definitions
target_compile_definitions(xy_device PUBLIC ${HAL_DEFINES})

# For non-PC platforms, link with xy_hal library
if(NOT HAL_PLATFORM STREQUAL "PC")
    target_link_libraries(xy_device PUBLIC xy_hal)
endif()

# Installation
install(FILES ${DEVICE_HEADERS} DESTINATION include/xy_device)
install(TARGETS xy_device ARCHIVE DESTINATION lib)
