# DM (Device Management) Component CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

if(NOT XY_COMPONENT_DM)
    return()
endif()

message(STATUS "=== DM Component: Building ===")

# Collect DM source files
set(DM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_json.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_fs.c
)

# ==================== NOR Flash Support ====================
if(CONFIG_NOR_FLASH_ENABLED)
    message(STATUS "   DM: Including NOR Flash support")
    list(APPEND DM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/xy_norflash/xy_nor.c
        ${CMAKE_CURRENT_SOURCE_DIR}/xy_norflash/xy_nor_port.c
    )
endif()

# ==================== FlashDB Support ====================
if(CONFIG_FS_FLASHDB)
    message(STATUS "   DM: Including FlashDB support")
    list(APPEND DM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/xy_flash/xy_flashdb.c
    )
endif()

# Create DM library
add_library(xy_dm STATIC ${DM_SOURCES})

# Include directories
target_include_directories(xy_dm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/coreJSON
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/components/trace/xy_log/inc
)

# NOR Flash config
if(CONFIG_NOR_FLASH_ENABLED)
    target_compile_definitions(xy_dm PUBLIC NOR_FLASH_ENABLED=1)
    target_compile_definitions(xy_dm PUBLIC 
        NOR_FLASH_BASE_ADDR=${CONFIG_NOR_FLASH_BASE_ADDR}
        NOR_FLASH_SIZE=${CONFIG_NOR_FLASH_SIZE}
    )
    message(STATUS "   DM: NOR Flash base addr: ${CONFIG_NOR_FLASH_BASE_ADDR}")
endif()

# FlashDB includes
if(CONFIG_FS_FLASHDB)
    target_compile_definitions(xy_dm PUBLIC FLASHDB_ENABLED=1)
    target_include_directories(xy_dm PUBLIC
        ${CMAKE_SOURCE_DIR}/components/third_party/flashdb/inc
    )
    target_link_libraries(xy_dm PUBLIC xy_flashdb)
    message(STATUS "   DM: FlashDB enabled")
endif()

target_compile_definitions(xy_dm PRIVATE COMPONENT_DM=1)

# Link dependencies
target_link_libraries(xy_dm PUBLIC
    xy_trace
    xy_clib
)

# Add HAL dependency for STM32 NOR Flash
if(XY_PLATFORM_STM32 AND CONFIG_NOR_FLASH_ENABLED)
    target_link_libraries(xy_dm PUBLIC xy_hal)
endif()

# Compiler options
target_compile_options(xy_dm PRIVATE -Wall -Wextra -O2)

message(STATUS "=== DM Component: Done ===")