cmake_minimum_required(VERSION 3.10)
project(pid_examples C)
set(CMAKE_C_STANDARD 99)

set(PLATFORM "PC" CACHE STRING "Target platform")

if(PLATFORM STREQUAL "PC")
    add_definitions(-DHAL_PLATFORM_PC -DXY_HAL_PC_SIM)
    message(STATUS "Building PID examples for PC simulation")
endif()

# Include directories
set(INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/components/pid/inc
    ${CMAKE_SOURCE_DIR}/components/log/inc
)

include_directories(${INCLUDE_DIRS})

# Build all examples
set(EXAMPLES
    example_basic
    example_incremental
    example_temperature
    example_charging
    example_auto_tune
)

foreach(example ${EXAMPLES})
    add_executable(${example} ${example}.c)
    target_link_libraries(${example} m)
    target_include_directories(${example} PRIVATE ${INCLUDE_DIRS})

    # Add install target
    install(TARGETS ${example} DESTINATION bin/pid_examples)
endforeach()

message(STATUS "PID examples build configuration complete")
message(STATUS "Available examples: ${EXAMPLES}")
