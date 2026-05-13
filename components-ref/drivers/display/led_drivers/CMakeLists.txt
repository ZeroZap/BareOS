# LED Drivers CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

if(NOT DRIVER_DISPLAY_LED)
    return()
endif()

message(STATUS "Display: Building LED driver backends")

# Charlieplex driver
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/charlieplex)
    add_subdirectory(charlieplex)
endif()

# Matrix scan driver
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/matrix_scan)
    add_subdirectory(matrix_scan)
endif()

# Serial RGB driver
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/serial_rgb)
    add_subdirectory(serial_rgb)
endif()
