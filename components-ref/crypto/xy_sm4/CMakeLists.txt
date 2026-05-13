# xy_sm4 CMakeLists.txt - SM4 Block Cipher

add_library(xy_sm4 STATIC
    xy_sm4.c
)

target_include_directories(xy_sm4 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/components/crypto
    ${CMAKE_SOURCE_DIR}/components/crypto/xy_sm3
)

target_compile_options(xy_sm4 PRIVATE
    ${XY_CRYPTO_CFLAGS}
)

set_target_properties(xy_sm4 PROPERTIES
    OUTPUT_NAME "xy_sm4"
    POSITION_INDEPENDENT_CODE 0
)
