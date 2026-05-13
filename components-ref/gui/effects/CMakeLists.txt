# GUI Effects Subdirectory CMakeLists.txt

cmake_minimum_required(VERSION 3.10)

# Effects sources
set(EFFECTS_SOURCES
    xy_gui_effect_fade.c
    xy_gui_effect_blink.c
    xy_gui_effect_breath.c
    xy_gui_effect_slide.c
    xy_gui_effect_rotate.c
)

# Create effects library
add_library(xy_gui_effects STATIC ${EFFECTS_SOURCES})

target_include_directories(xy_gui_effects PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(xy_gui_effects PUBLIC xy_trace)

target_compile_options(xy_gui_effects PRIVATE -Wall -Wextra -O2)

message(STATUS "GUI Effects: Built successfully")
