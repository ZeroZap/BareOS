# XinYi Third-Party USB/BLE Components CMake

#====================================================================
# TinyUSB Integration (temporarily disabled - needs port files)
#====================================================================

# add_library(xy_tinyusb_port STATIC)
# target_sources(xy_tinyusb_port
#     PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/port/xy_usb/device/xy_usb_device.c
# )
# target_include_directories(xy_tinyusb_port PUBLIC
#     ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/port/xy_usb
#     ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/src
#     ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/src/common
#     ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/src/device
#     ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb/src/class/cdc
# )
# target_link_libraries(xy_tinyusb_port PUBLIC
#     tinyusb
# )

#====================================================================
# NimBLE Integration (temporarily disabled - needs port files)
#====================================================================

# add_library(xy_nimble_port STATIC)
# target_sources(xy_nimble_port
#     PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}/nimble/port/xy_ble/xy_ble_host.c
# )
# target_include_directories(xy_nimble_port PUBLIC
#     ${CMAKE_CURRENT_SOURCE_DIR}/nimble/port/xy_ble
#     ${CMAKE_CURRENT_SOURCE_DIR}/nimble/nimble/include
#     ${CMAKE_CURRENT_SOURCE_DIR}/nimble/nimble/host/include
#     ${CMAKE_CURRENT_SOURCE_DIR}/nimble/nimble/host/store/include
# )
# target_link_libraries(xy_nimble_port PUBLIC
#     nimble_host
# )

#====================================================================
# FatFS Integration (placeholder)
#====================================================================

# add_library(xy_fatfs_port STATIC)
# target_sources(xy_fatfs_port
#     PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/port/xy_fatfs_disk.c
#         ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/port/xy_fatfs_io.c
# )
# target_include_directories(xy_fatfs_port PUBLIC
#     ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/fatfs/source
#     ${CMAKE_CURRENT_SOURCE_DIR}/filesystem/port
# )
# target_link_libraries(xy_fatfs_port PUBLIC fatfs)

#====================================================================
# FlashDB Integration
#====================================================================

add_library(xy_flashdb STATIC)
target_sources(xy_flashdb
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/flashdb/src/fdb.c
)

target_include_directories(xy_flashdb PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/flashdb/inc
)

# Optional: FAL (Flash Abstraction Layer) support
# target_sources(xy_flashdb
#     PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}/flashdb/port/fal/fal.c
# )
# target_include_directories(xy_flashdb PUBLIC
#     ${CMAKE_CURRENT_SOURCE_DIR}/flashdb/port/fal
# )
