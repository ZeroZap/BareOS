# OLED Display CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

if(NOT DRIVER_DISPLAY_OLED)
    return()
endif()

message(STATUS "Display: Building OLED drivers")

# SSD1306 OLED Driver
if(DRIVER_DISPLAY_SSD1306)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ssd1306)
        add_subdirectory(ssd1306)
    endif()
endif()
