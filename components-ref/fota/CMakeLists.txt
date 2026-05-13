# FOTA Component CMakeLists.txt
# XinYi Framework - FOTA Component

cmake_minimum_required(VERSION 3.10)

# Check if FOTA is enabled (Kconfig generates CONFIG_FOTA_ENABLED)
if(NOT CONFIG_FOTA_ENABLED)
    return()
endif()

message(STATUS "=== FOTA Component: Building ===")

# Include directories
target_include_directories(xy_fota PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../hal/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../kernel/osal/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../trace/xy_log/inc
)

# Source files - main + src/
file(GLOB FOTA_ROOT_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)
list(FILTER FOTA_ROOT_SOURCES EXCLUDE REGEX "test/.*")

file(GLOB FOTA_SRC_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

set(FOTA_SOURCES
    ${FOTA_ROOT_SOURCES}
    ${FOTA_SRC_SOURCES}
)

# Add NOR Flash support if enabled (check CONFIG_* from Kconfig)
if(CONFIG_FOTA_EXTERNAL_FLASH AND CONFIG_NOR_FLASH_ENABLED)
    list(APPEND FOTA_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_fota_nor.c
    )
    message(STATUS "   FOTA: Using external NOR Flash backup")
else()
    message(STATUS "   FOTA: Using internal Flash")
endif()

# Create static library
add_library(xy_fota STATIC ${FOTA_SOURCES})

# Compile definitions
target_compile_definitions(xy_fota PRIVATE 
    FOTA_ENABLED=1
)

if(CONFIG_FOTA_DUAL_BANK)
    target_compile_definitions(xy_fota PRIVATE FOTA_DUAL_BANK=1)
endif()

if(CONFIG_FOTA_ROLLBACK)
    target_compile_definitions(xy_fota PRIVATE FOTA_ROLLBACK=1)
endif()

if(CONFIG_FOTA_EXTERNAL_FLASH)
    target_compile_definitions(xy_fota PRIVATE FOTA_EXTERNAL_FLASH=1)
endif()

# Link dependencies
target_link_libraries(xy_fota PUBLIC
    xy_trace
    xy_clib
)

# Add HAL dependency for STM32
if(XY_PLATFORM_STM32)
    target_link_libraries(xy_fota PUBLIC xy_hal)
endif()

# Compiler options
target_compile_options(xy_fota PRIVATE -Wall -Wextra -O2)

message(STATUS "=== FOTA Component: Done ===")