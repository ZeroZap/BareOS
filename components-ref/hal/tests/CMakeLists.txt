# HAL Test Suite CMakeLists.txt

cmake_minimum_required(VERSION 3.10)
project(xy_hal_tests C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译选项
add_compile_options(-Wall -Wextra -Wpedantic)

# 测试框架源文件
set(TEST_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/xy_hal_test.c
    ${CMAKE_CURRENT_LIST_DIR}/test_gpio.c
    ${CMAKE_CURRENT_LIST_DIR}/test_uart.c
    ${CMAKE_CURRENT_LIST_DIR}/test_spi.c
    ${CMAKE_CURRENT_LIST_DIR}/test_i2c.c
    ${CMAKE_CURRENT_LIST_DIR}/main.c
)

# 测试框架头文件
set(TEST_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../inc
)

# 创建测试可执行文件 (PC 模拟)
add_executable(xy_hal_tests_pc
    ${TEST_SOURCES}
)

target_include_directories(xy_hal_tests_pc PRIVATE
    ${TEST_INCLUDES}
)

# PC 测试定义
target_compile_definitions(xy_hal_tests_pc PRIVATE
    XY_HAL_PLATFORM_PC=1
    XY_HAL_TEST_MODE=1
)

# 如果需要 STM32U5 硬件测试，取消以下注释
# add_executable(xy_hal_tests_stm32
#     ${TEST_SOURCES}
#     # 添加 STM32U5 HAL 源文件
# )
#
# target_include_directories(xy_hal_tests_stm32 PRIVATE
#     ${TEST_INCLUDES}
#     ${STM32U5_HAL_PATH}/Drivers/STM32U5xx_HAL_Driver/Inc
#     ${STM32U5_HAL_PATH}/Drivers/CMSIS/Device/ST/STM32U5xx/Include
# )
#
# target_compile_definitions(xy_hal_tests_stm32 PRIVATE
#     USE_HAL_DRIVER
#     STM32U575xx
#     XY_HAL_PLATFORM_STM32U5=1
#     XY_HAL_TEST_MODE=1
# )
