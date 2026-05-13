/**
 * @file test_gpio.c
 * @brief GPIO HAL Test Cases
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 测试 GPIO 基本功能：配置/读写/翻转
 */

#include "xy_hal_test.h"
#include "../inc/xy_hal_gpio_dev.h"

/* ==================== Test Cases ==================== */

/**
 * @brief 测试 GPIO 设备绑定
 */
static xy_test_result_t test_gpio_bind(void)
{
    /* 测试绑定 GPIOA */
    xy_hal_gpio_t gpio = xy_hal_gpio_bind("GPIOA");
    XY_TEST_ASSERT_NOT_NULL(gpio);
    
    /* 测试解绑 */
    xy_hal_error_t err = xy_hal_gpio_unbind(gpio);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    return XY_TEST_PASS;
}

/**
 * @brief 测试 GPIO 配置
 */
static xy_test_result_t test_gpio_configure(void)
{
    xy_hal_gpio_t gpio = xy_hal_gpio_bind("GPIOA");
    XY_TEST_ASSERT_NOT_NULL(gpio);
    
    /* 配置 PA5 为输出模式 */
    xy_hal_gpio_config_t cfg = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
        .pull = XY_HAL_GPIO_PULL_NONE,
        .otype = XY_HAL_GPIO_OTYPE_PP,
        .speed = XY_HAL_GPIO_SPEED_HIGH,
    };
    
    xy_hal_error_t err = xy_hal_gpio_configure(gpio, 5, &cfg);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    /* 验证配置 */
    xy_hal_gpio_config_t read_cfg;
    err = xy_hal_gpio_get_config(gpio, 5, &read_cfg);
    XY_TEST_ASSERT_EQ(0, (int)err);
    XY_TEST_ASSERT_EQ(XY_HAL_GPIO_MODE_OUTPUT, read_cfg.mode);
    
    xy_hal_gpio_unbind(gpio);
    return XY_TEST_PASS;
}

/**
 * @brief 测试 GPIO 读写
 */
static xy_test_result_t test_gpio_read_write(void)
{
    xy_hal_gpio_t gpio = xy_hal_gpio_bind("GPIOA");
    XY_TEST_ASSERT_NOT_NULL(gpio);
    
    /* 配置 PA5 为输出 */
    xy_hal_gpio_config_t cfg = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
    };
    xy_hal_gpio_configure(gpio, 5, &cfg);
    
    /* 测试写操作 */
    xy_hal_error_t err = xy_hal_gpio_write(gpio, 5, 1);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    err = xy_hal_gpio_write(gpio, 5, 0);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    /* 测试翻转 */
    err = xy_hal_gpio_toggle(gpio, 5);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    /* 测试读操作 (配置为输入) */
    cfg.mode = XY_HAL_GPIO_MODE_INPUT;
    xy_hal_gpio_configure(gpio, 5, &cfg);
    
    int32_t value = xy_hal_gpio_read(gpio, 5);
    XY_TEST_ASSERT(value == 0 || value == 1);
    
    xy_hal_gpio_unbind(gpio);
    return XY_TEST_PASS;
}

/**
 * @brief 测试 GPIO 端口操作
 */
static xy_test_result_t test_gpio_port_operation(void)
{
    xy_hal_gpio_t gpio = xy_hal_gpio_bind("GPIOA");
    XY_TEST_ASSERT_NOT_NULL(gpio);
    
    /* 配置 PA0-PA7 为输出 */
    for (int pin = 0; pin < 8; pin++) {
        xy_hal_gpio_config_t cfg = {
            .mode = XY_HAL_GPIO_MODE_OUTPUT,
        };
        xy_hal_gpio_configure(gpio, pin, &cfg);
    }
    
    /* 测试端口写 */
    xy_hal_gpio_mask_t mask = 0x00FF;
    xy_hal_gpio_value_t value = 0x0055;
    
    xy_hal_error_t err = xy_hal_gpio_port_write(gpio, mask, value);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    /* 测试端口读 */
    xy_hal_gpio_value_t read_value = xy_hal_gpio_port_read(gpio, mask);
    XY_TEST_ASSERT(read_value == value || read_value == 0); /* 取决于硬件 */
    
    xy_hal_gpio_unbind(gpio);
    return XY_TEST_PASS;
}

/**
 * @brief 测试无效参数处理
 */
static xy_test_result_t test_gpio_invalid_params(void)
{
    /* 测试 NULL 句柄 */
    xy_hal_error_t err = xy_hal_gpio_configure(NULL, 5, NULL);
    XY_TEST_ASSERT(err != XY_HAL_OK);
    
    /* 测试无效引脚号 */
    xy_hal_gpio_t gpio = xy_hal_gpio_bind("GPIOA");
    XY_TEST_ASSERT_NOT_NULL(gpio);
    
    err = xy_hal_gpio_configure(gpio, 16, NULL); /* 引脚号超出范围 */
    XY_TEST_ASSERT(err != XY_HAL_OK);
    
    xy_hal_gpio_unbind(gpio);
    return XY_TEST_PASS;
}

/**
 * @brief 测试 GPIO 边界条件
 */
static xy_test_result_t test_gpio_edge_cases(void)
{
    xy_hal_gpio_t gpio = xy_hal_gpio_bind("GPIOA");
    XY_TEST_ASSERT_NOT_NULL(gpio);
    
    /* 测试引脚 0 (最低引脚) */
    xy_hal_gpio_config_t cfg = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
    };
    xy_hal_error_t err = xy_hal_gpio_configure(gpio, 0, &cfg);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    err = xy_hal_gpio_write(gpio, 0, 1);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    /* 测试引脚 15 (最高引脚) */
    err = xy_hal_gpio_configure(gpio, 15, &cfg);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    err = xy_hal_gpio_write(gpio, 15, 0);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    /* 测试端口全 1 掩码 */
    err = xy_hal_gpio_port_write(gpio, 0xFFFF, 0xFFFF);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    /* 测试端口全 0 掩码 */
    err = xy_hal_gpio_port_write(gpio, 0x0000, 0x0000);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    xy_hal_gpio_unbind(gpio);
    return XY_TEST_PASS;
}

/* ==================== Test Suite ==================== */

/* 静态分配测试用例数组 */
static xy_test_case_t gpio_test_cases[6];

xy_test_suite_t *gpio_get_test_suite(void)
{
    static xy_test_suite_t suite;
    
    suite.name = "GPIO HAL Tests";
    suite.cases = gpio_test_cases;
    suite.case_count = 0;
    suite.pass_count = 0;
    suite.fail_count = 0;
    suite.skip_count = 0;
    suite.total_duration_ms = 0;
    
    xy_test_add_case(&suite, "GPIO Bind", test_gpio_bind);
    xy_test_add_case(&suite, "GPIO Configure", test_gpio_configure);
    xy_test_add_case(&suite, "GPIO Read/Write", test_gpio_read_write);
    xy_test_add_case(&suite, "GPIO Port Operation", test_gpio_port_operation);
    xy_test_add_case(&suite, "GPIO Invalid Params", test_gpio_invalid_params);
    xy_test_add_case(&suite, "GPIO Edge Cases", test_gpio_edge_cases);
    
    return &suite;
}

void run_gpio_tests(void)
{
    xy_test_suite_t *suite = gpio_get_test_suite();
    xy_test_run(suite);
    xy_test_print_report(suite);
}
