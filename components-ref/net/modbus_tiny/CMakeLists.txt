# Modbus Tiny Component
# Nano Modbus RTU Only - Minimal Footprint

add_library(xy_modbus_tiny INTERFACE)

target_include_directories(xy_modbus_tiny INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_sources(xy_modbus_tiny INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/mb_tiny.h
    ${CMAKE_CURRENT_LIST_DIR}/mb_tiny.c
)
