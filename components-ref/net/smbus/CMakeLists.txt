# SMBus Component CMakeLists.txt
# SMBus / PMBus 协议栈

cmake_minimum_required(VERSION 3.10)

if(NOT XY_COMPONENT_SMBUS AND NOT XY_CONFIG_SMBUS_ENABLED)
    return()
endif()

# SMBus sources
set(SMBUS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/xy_smbus.c
    ${CMAKE_CURRENT_SOURCE_DIR}/xy_pmbus.c
)

# Add library
add_library(xy_smbus STATIC ${SMBUS_SOURCES})

target_include_directories(xy_smbus PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(xy_smbus PRIVATE xy_hal xy_log)

target_compile_definitions(xy_smbus PRIVATE XY_COMPONENT_SMBUS=1)
target_compile_options(xy_smbus PRIVATE -Wall -Wextra)