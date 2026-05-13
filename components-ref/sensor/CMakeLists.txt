# Sensor Component CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

# Sensor component - now properly organized
# All driver implementations are in .c files, .h files contain only declarations

if(NOT XY_XY_SENSOR_ENABLE AND NOT XY_COMPONENT_SENSOR)
    return()
endif()

# Add sources (only include files that exist)
set(SENSOR_SOURCES
    sensor_core.c
    sensor_calibration.c
    sensor_dma.c
    sensor_fifo.c
    sensor_filter.c
    sensor_fusion.c
    sensor_interrupt.c
    sensor_moton.c
    sensor_power.c
    sensor_self_test.c
    sensor_threshold.c
    sensor_mem_pool.c
    sensor_adt7420.c
    sensor_apds9960.c
)

# Add driver sources from sensors/ directory
file(GLOB SENSOR_DRIVERS "${CMAKE_CURRENT_LIST_DIR}/sensors/sensor_*.c")
list(APPEND SENSOR_SOURCES ${SENSOR_DRIVERS})

add_library(sensor_component STATIC ${SENSOR_SOURCES})
target_include_directories(sensor_component PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc)
target_include_directories(sensor_component PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(sensor_component PUBLIC hardware_abstraction)
