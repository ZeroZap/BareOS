# XinYi 设备模型测试套件 CMakeLists.txt

cmake_minimum_required(VERSION 3.10)
project(xy_device_tests C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译优化：利用多核并行编译
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wall -Wextra")

# 测试框架源文件
set(TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../hal/tests/xy_hal_test.c
    ${CMAKE_CURRENT_LIST_DIR}/test_device_pm.c
    ${CMAKE_CURRENT_LIST_DIR}/test_device_async.c
    ${CMAKE_CURRENT_LIST_DIR}/main.c
)

# 设备模型源文件
set(DEVICE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../src/xy_device_pm.c
    ${CMAKE_CURRENT_LIST_DIR}/../src/xy_device_async.c
)

# 创建测试可执行文件 (PC 模拟)
add_executable(xy_device_tests_pc
    ${TEST_SOURCES}
    ${DEVICE_SOURCES}
)

target_include_directories(xy_device_tests_pc PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../inc
    ${CMAKE_CURRENT_LIST_DIR}/../hal/tests
)

target_compile_definitions(xy_device_tests_pc PRIVATE
    XY_HAL_PLATFORM_PC=1
    XY_HAL_TEST_MODE=1
)

# 编译优化提示
message(STATUS "========================================")
message(STATUS "XinYi Device Tests Configuration:")
message(STATUS "  - Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  - Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  - Optimization: -O2")
message(STATUS "  - Parallel Jobs: Use -j$(nproc) for fast build")
message(STATUS "========================================")
