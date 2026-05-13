# LCD Drivers CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

if(NOT DRIVER_DISPLAY_LCD)
    return()
endif()

file(GLOB LCD_DRIVER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

if(LCD_DRIVER_SOURCES)
    xy_add_srcs(${LCD_DRIVER_SOURCES})
endif()
