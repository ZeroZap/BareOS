# RGB LED (xy_ls) Display CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

if(NOT DRIVER_DISPLAY_RGB)
    return()
endif()

message(STATUS "Display: Building RGB LED drivers")

file(GLOB RGB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

if(RGB_SOURCES)
    xy_add_srcs(${RGB_SOURCES})
endif()
