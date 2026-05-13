cmake_minimum_required(VERSION 3.10)
project(xy_xy_clib VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler options
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(inc)

# Collect source files
file(GLOB_RECURSE COMPONENT_SOURCES "*.c")
list(FILTER COMPONENT_SOURCES EXCLUDE REGEX "test/.*")
list(FILTER COMPONENT_SOURCES EXCLUDE REGEX "examples/.*")
list(FILTER COMPONENT_SOURCES EXCLUDE REGEX "_example\\.c$")

file(GLOB_RECURSE COMPONENT_HEADERS "inc/*.h")
file(GLOB TEST_SOURCES "test/*.c")

# Create static library
add_library(xy_xy_clib STATIC ${COMPONENT_SOURCES})

# Set library properties
set_target_properties(xy_xy_clib PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${COMPONENT_HEADERS}"
)

# Windows system needs additional libraries
if(WIN32)
    target_link_libraries(xy_xy_clib PRIVATE advapi32)
endif()

# Create test executable if test sources exist and testing is enabled
if(TEST_SOURCES AND BUILD_TESTING)
    add_executable(test_xy_clib ${TEST_SOURCES})
    target_link_libraries(test_xy_clib xy_xy_clib)

    if(WIN32)
        target_link_libraries(test_xy_clib advapi32)
    endif()

    # Enable testing
    enable_testing()
    add_test(NAME xy_clib_test COMMAND test_xy_clib)
endif()

# Installation rules
install(TARGETS xy_xy_clib
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

# Packaging
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)