# SSD1306 OLED Driver CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

if(NOT DRIVER_DISPLAY_SSD1306)
    return()
endif()

file(GLOB SSD1306_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

if(SSD1306_SOURCES)
    xy_add_srcs(${SSD1306_SOURCES})
endif()
