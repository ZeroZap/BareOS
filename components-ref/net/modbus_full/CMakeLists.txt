# Modbus Full Component
# Nano Modbus RTU + TCP + ASCII - Full Feature Set

add_library(xy_modbus_full INTERFACE)

target_include_directories(xy_modbus_full INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_sources(xy_modbus_full INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/mb_full.h
    ${CMAKE_CURRENT_LIST_DIR}/mb_full.c
)
