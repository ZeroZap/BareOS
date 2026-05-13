# Serial RGB Driver CMakeLists.txt
# WS2812/SK6812 and similar addressable RGB LED drivers

cmake_minimum_required(VERSION 3.10)

if(NOT DRIVER_DISPLAY_LED_SERIAL_RGB)
    return()
endif()

message(STATUS "Display: Building Serial RGB driver (WS2812/SK6812)")

# Serial RGB sources
set(SERIAL_RGB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/xy_ws2812.c
    ${CMAKE_CURRENT_SOURCE_DIR}/xy_rgb_matrix.c
)

# Add library
add_library(xy_serial_rgb STATIC ${SERIAL_RGB_SOURCES})

target_include_directories(xy_serial_rgb PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(xy_serial_rgb PRIVATE xy_hal)

target_compile_definitions(xy_serial_rgb PRIVATE XY_SERIAL_RGB=1)
target_compile_options(xy_serial_rgb PRIVATE -Wall -Wextra)
