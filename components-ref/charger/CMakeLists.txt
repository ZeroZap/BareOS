# XinYi Charger Component CMake Configuration

cmake_minimum_required(VERSION 3.10)

# Charger Sources
set(CHARGER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_bq25620.c
    # Add more charger drivers here
    # ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_bq2561x.c
    # ${CMAKE_CURRENT_SOURCE_DIR}/src/xy_mp2615.c
)

# Charger Headers
set(CHARGER_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# Create Charger Library
add_library(charger STATIC ${CHARGER_SOURCES})

target_include_directories(charger PUBLIC
    ${CHARGER_INCLUDE_DIRS}
)

# Link dependencies
target_link_libraries(charger PUBLIC
    xy_device
    xy_hal
)

# Compile Definitions
target_compile_definitions(charger PUBLIC
    XY_CHARGER_ENABLED
)
