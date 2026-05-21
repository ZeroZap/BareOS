cmake_minimum_required(VERSION 3.16)
project(bareos_plb C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
enable_testing()

if(MSVC)
    add_compile_options(/W3)
else()
    add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-sign-compare)
endif()

set(COMP "${CMAKE_CURRENT_SOURCE_DIR}/../../components")

set(COMP_SRCS
    ${COMP}/clib/xy_stdio.c
    ${COMP}/clib/xy_string.c
    ${COMP}/clib/xy_math.c
    ${COMP}/clib/xy_ctype.c
    ${COMP}/clib/xy_rb.c
    ${COMP}/clib/xy_common.c
    ${COMP}/clib/xy_assert.c
    ${COMP}/log/xy_log.c
    ${COMP}/mem/xy_mem.c
    ${COMP}/at/src/at_chat.c
    ${COMP}/at/src/at_port.c
    ${COMP}/cell/xy_cell.c
    ${COMP}/cell/drv/xy_cell_drv_sim76.c
    ${COMP}/cell/drv/xy_cell_drv_ec2x.c
    ${COMP}/cell/drv/xy_cell_drv_ml302.c
    ${COMP}/cell/drv/xy_cell_drv_fibocom.c
    ${COMP}/gnss/xy_gnss.c
    ${COMP}/storage/evtlog/xy_evtlog.c
)

set(APP_SRCS
    src/plb_pc_bsp.c
    src/plb_comm_mock_backend.c
    src/plb_comm_cell_backend.c
    src/plb_comm.c
    src/plb_app.c
    src/main.c
)

add_executable(bareos_plb ${COMP_SRCS} ${APP_SRCS})

target_include_directories(bareos_plb PRIVATE
    src
    ${COMP}/clib
    ${COMP}/log
    ${COMP}/mem
    ${COMP}/at/include
    ${COMP}/cell
    ${COMP}/gnss
    ${COMP}/storage/evtlog
)

target_compile_definitions(bareos_plb PRIVATE
    PC_BUILD=1
    XY_LOG_LEVEL=3
)

set(PLB_COMMON_SRCS
    ${COMP_SRCS}
    src/plb_pc_bsp.c
    src/plb_comm_mock_backend.c
    src/plb_comm_cell_backend.c
    src/plb_comm.c
    src/plb_app.c
)

add_executable(bareos_plb_tests
    ${PLB_COMMON_SRCS}
    tests/test_plb_flow.c
)

target_include_directories(bareos_plb_tests PRIVATE
    src
    ${COMP}/clib
    ${COMP}/log
    ${COMP}/mem
    ${COMP}/at/include
    ${COMP}/cell
    ${COMP}/gnss
    ${COMP}/storage/evtlog
)

target_compile_definitions(bareos_plb_tests PRIVATE
    PC_BUILD=1
    XY_LOG_LEVEL=3
)

add_test(NAME bareos_plb_flow COMMAND bareos_plb_tests)
