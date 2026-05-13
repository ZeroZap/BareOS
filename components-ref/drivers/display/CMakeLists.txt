# Display Drivers CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

if(NOT DRIVER_DISPLAY_ENABLE)
    return()
endif()

message(STATUS "Display: Building display drivers")

# OLED Display Support
if(DRIVER_DISPLAY_OLED)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/oled)
        add_subdirectory(oled)
    endif()
endif()

# LCD Display Support
if(DRIVER_DISPLAY_LCD)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lcd)
        add_subdirectory(lcd)
    endif()
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lcd_drivers)
        add_subdirectory(lcd_drivers)
    endif()
endif()

# LED Display Support
if(DRIVER_DISPLAY_LED)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/led)
        add_subdirectory(led)
    endif()
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/led_drivers)
        add_subdirectory(led_drivers)
    endif()
endif()

# RGB Display Support
if(DRIVER_DISPLAY_RGB)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/xy_ls)
        add_subdirectory(xy_ls)
    endif()
endif()

# Common display sources (if any files exist in display root)
file(GLOB DISPLAY_COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
if(DISPLAY_COMMON_SOURCES)
    xy_add_srcs(${DISPLAY_COMMON_SOURCES})
endif()
