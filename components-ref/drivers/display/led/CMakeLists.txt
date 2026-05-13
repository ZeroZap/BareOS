# LED Display CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

if(NOT DRIVER_DISPLAY_LED)
    return()
endif()

message(STATUS "Display: Building LED drivers")
