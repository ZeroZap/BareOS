# xy_sm3 CMakeLists.txt - SM3 Hash Algorithm

add_library(xy_sm3 STATIC
    xy_sm3.c
)

target_include_directories(xy_sm3 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_options(xy_sm3 PRIVATE
    ${XY_CRYPTO_CFLAGS}
)

set_target_properties(xy_sm3 PROPERTIES
    OUTPUT_NAME "xy_sm3"
    POSITION_INDEPENDENT_CODE 0
)
