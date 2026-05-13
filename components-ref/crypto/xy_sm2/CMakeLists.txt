# xy_sm2 CMakeLists.txt - SM2 Elliptic Curve

add_library(xy_sm2 STATIC
    xy_sm2.c
)

target_include_directories(xy_sm2 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/components/crypto
    ${CMAKE_SOURCE_DIR}/components/crypto/xy_sm3
    ${CMAKE_SOURCE_DIR}/components/crypto/xy_sm4
    ${CMAKE_SOURCE_DIR}/components/crypto/xy_rng
)

target_compile_options(xy_sm2 PRIVATE
    ${XY_CRYPTO_CFLAGS}
)

set_target_properties(xy_sm2 PROPERTIES
    OUTPUT_NAME "xy_sm2"
    POSITION_INDEPENDENT_CODE 0
)
