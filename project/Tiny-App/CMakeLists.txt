cmake_minimum_required(VERSION 3.16)
project(bareos_pc C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ── Compiler flags ────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W3)
else()
    add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-sign-compare)
endif()

# ── Root paths ────────────────────────────────────────────────────────
set(COMP "${CMAKE_CURRENT_SOURCE_DIR}/../../components")

# ── Component sources ─────────────────────────────────────────────────
set(COMP_SRCS
    # clib
    ${COMP}/clib/xy_stdio.c
    ${COMP}/clib/xy_string.c
    ${COMP}/clib/xy_math.c
    ${COMP}/clib/xy_ctype.c
    ${COMP}/clib/xy_rb.c
    ${COMP}/clib/xy_common.c
    ${COMP}/clib/xy_assert.c

    # log
    ${COMP}/log/xy_log.c

    # crypto
    ${COMP}/crypto/src/xy_crc.c
    ${COMP}/crypto/src/xy_aes.c
    ${COMP}/crypto/src/xy_sha256_hmac.c
    ${COMP}/crypto/src/xy_md5.c
    ${COMP}/crypto/src/xy_hmac.c
    ${COMP}/crypto/src/xy_csprng.c
    ${COMP}/crypto/src/xy_chacha20poly1305.c
    ${COMP}/crypto/src/xy_blake2.c
    ${COMP}/crypto/src/xy_ecdsa.c
    ${COMP}/crypto/src/xy_base64.c
    ${COMP}/crypto/src/xy_hex.c
    ${COMP}/crypto/src/xy_random.c

    # protothreads / timers
    ${COMP}/pt/process.c
    ${COMP}/pt/etimer.c
    ${COMP}/pt/ctimer.c
    ${COMP}/pt/stimer.c
    ${COMP}/pt/rtimer.c

    # sys / io
    ${COMP}/sys/xy_sys.c
    ${COMP}/io/xy_io.c

    # TLV
    ${COMP}/tlv/xy_tlv.c

    # button
    ${COMP}/btn/xy_btn.c

    # GNSS
    ${COMP}/gnss/xy_nmea.c
    ${COMP}/gnss/xy_ubx.c

    # maritime protocols
    ${COMP}/ais/xy_ais.c
    ${COMP}/dsc/xy_dsc.c
    ${COMP}/epirb/xy_epirb.c
    ${COMP}/uwb/xy_uwb.c

    # storage
    ${COMP}/storage/vlog/xy_vlog.c
    ${COMP}/storage/flog/xy_flog.c

    # memory allocator
    ${COMP}/mem/xy_mem.c

    # command shell
    ${COMP}/cmd/tiny_cmd.c

    # actuator
    ${COMP}/actuator/xy_actuator.c

    # AT client
    ${COMP}/at/src/at_chat.c
    ${COMP}/at/src/at_port.c

    # cellular modem abstraction
    ${COMP}/cell/xy_cell.c
    ${COMP}/cell/drv/xy_cell_drv_sim76.c
    ${COMP}/cell/drv/xy_cell_drv_ec2x.c
    ${COMP}/cell/drv/xy_cell_drv_ml302.c
    ${COMP}/cell/drv/xy_cell_drv_fibocom.c

    # WiFi (ESP32 via ESP-AT)
    ${COMP}/wifi/xy_wifi.c
)

# ── PC application sources ────────────────────────────────────────────
set(APP_SRCS
    src/pc_bsp.c
    src/pc_uart.c
    src/app_tasks.c
    src/app_timer.c
    src/app_sys.c
    src/app_io.c
    src/main.c
)

# ── Include directories ───────────────────────────────────────────────
set(INCDIRS
    src
    ${COMP}/clib
    ${COMP}/log
    ${COMP}/crypto/inc
    ${COMP}/pt
    ${COMP}/tlv
    ${COMP}/btn
    ${COMP}/gnss
    ${COMP}/ais
    ${COMP}/dsc
    ${COMP}/epirb
    ${COMP}/uwb
    ${COMP}/storage/flog
    ${COMP}/storage/vlog
    ${COMP}/cmd
    ${COMP}/actuator
    ${COMP}/mem
    ${COMP}/sys
    ${COMP}/io
    ${COMP}/at/include
    ${COMP}/cell
    ${COMP}/wifi
)

# ── Compile definitions ───────────────────────────────────────────────
set(COMP_DEFS
    PC_BUILD=1
    XY_LOG_LEVEL=3          # DEBUG
    XY_FLOG_MAX_SECTORS=8
    TINY_CMD_TABLE_MAX=16
    XY_SW_VERSION_STR="1.0.0"
    XY_HW_VERSION_STR="PC-rev1"
)

# ── Target ───────────────────────────────────────────────────────────
add_executable(bareos_pc ${COMP_SRCS} ${APP_SRCS})
target_include_directories(bareos_pc PRIVATE ${INCDIRS})
target_compile_definitions(bareos_pc PRIVATE ${COMP_DEFS})

if(WIN32)
    target_link_libraries(bareos_pc PRIVATE crypt32)
else()
    target_compile_options(bareos_pc PRIVATE -Wno-unused-function)
endif()

# ── Regression test target ─────────────────────────────────────────────
# Pins down audit-fix behavior and the at_urc_recv_split helper contract.
# Run with: cmake --build build --target bareos_tests && ./build/bareos_tests
set(TEST_SRCS
    tests/test_audit_fixes.c
    tests/test_bsp_stub.c
)
add_executable(bareos_tests ${COMP_SRCS} ${TEST_SRCS})
target_include_directories(bareos_tests PRIVATE
    ${INCDIRS}
    tests
)
target_compile_definitions(bareos_tests PRIVATE
    ${COMP_DEFS}
    $<$<BOOL:${WIN32}>:__USE_MINGW_ANSI_STDIO=1>   # enable %lld on MinGW
)
if(WIN32)
    target_link_libraries(bareos_tests PRIVATE crypt32)
else()
    target_compile_options(bareos_tests PRIVATE -Wno-unused-function)
endif()
