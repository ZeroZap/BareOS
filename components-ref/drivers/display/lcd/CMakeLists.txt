# LCD Display Drivers CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

if(NOT DRIVER_DISPLAY_LCD)
    return()
endif()

message(STATUS "Display: Building LCD drivers")

# Core LCD framework
file(GLOB LCD_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/xy_lcd.c
)

if(LCD_CORE_SOURCES)
    xy_add_srcs(${LCD_CORE_SOURCES})
endif()

# SPI interface
if(DRIVER_DISPLAY_LCD_SPI)
    file(GLOB LCD_SPI_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/xy_lcd_spi.c
    )
    if(LCD_SPI_SOURCES)
        xy_add_srcs(${LCD_SPI_SOURCES})
    endif()
endif()

# I8080 interface
if(DRIVER_DISPLAY_LCD_I8080)
    file(GLOB LCD_I8080_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/xy_lcd_i8080.c
    )
    if(LCD_I8080_SOURCES)
        xy_add_srcs(${LCD_I8080_SOURCES})
    endif()
endif()

# ST7789 driver
if(DRIVER_DISPLAY_LCD_ST7789)
    file(GLOB LCD_ST7789_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/xy_lcd_st7789.c
    )
    if(LCD_ST7789_SOURCES)
        xy_add_srcs(${LCD_ST7789_SOURCES})
    endif()
endif()
