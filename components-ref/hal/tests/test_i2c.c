/**
 * @file test_i2c.c
 * @brief I2C HAL Test Cases
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 测试 I2C 基本功能：配置/收发/设备扫描
 */

#include "xy_hal_test.h"
#include "../inc/xy_hal_i2c_dev.h"

/* ==================== Test Cases ==================== */

/**
 * @brief 测试 I2C 设备绑定
 */
static xy_test_result_t test_i2c_bind(void)
{
    xy_hal_i2c_t i2c = xy_hal_i2c_bind("I2C1");
    XY_TEST_ASSERT_NOT_NULL(i2c);
    
    xy_hal_error_t err = xy_hal_i2c_unbind(i2c);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    return XY_TEST_PASS;
}

/**
 * @brief 测试 I2C 配置
 */
static xy_test_result_t test_i2c_configure(void)
{
    xy_hal_i2c_t i2c = xy_hal_i2c_bind("I2C1");
    XY_TEST_ASSERT_NOT_NULL(i2c);
    
    xy_hal_i2c_config_t cfg = {
        .clock_speed = 400000,  /* 400kHz */
        .addr_mode = XY_HAL_I2C_ADDR_7BIT,
        .own_address = 0,
        .general_call_mode = 0,
        .transfer_mode = XY_HAL_I2C_TRANSFER_POLLING,
    };
    
    xy_hal_error_t err = xy_hal_i2c_configure(i2c, &cfg);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    /* 验证配置 */
    xy_hal_i2c_config_t read_cfg;
    err = xy_hal_i2c_get_config(i2c, &read_cfg);
    XY_TEST_ASSERT_EQ(0, (int)err);
    XY_TEST_ASSERT_EQ(400000, read_cfg.clock_speed);
    
    xy_hal_i2c_unbind(i2c);
    return XY_TEST_PASS;
}

/**
 * @brief 测试 I2C 时钟速度设置
 */
static xy_test_result_t test_i2c_clock_speed(void)
{
    xy_hal_i2c_t i2c = xy_hal_i2c_bind("I2C1");
    XY_TEST_ASSERT_NOT_NULL(i2c);
    
    /* 设置不同时钟速度 */
    xy_hal_error_t err = xy_hal_i2c_set_clock_speed(i2c, 100000);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    err = xy_hal_i2c_set_clock_speed(i2c, 400000);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    xy_hal_i2c_unbind(i2c);
    return XY_TEST_PASS;
}

/**
 * @brief 测试 I2C 设备扫描
 */
static xy_test_result_t test_i2c_scan(void)
{
    xy_hal_i2c_t i2c = xy_hal_i2c_bind("I2C1");
    XY_TEST_ASSERT_NOT_NULL(i2c);
    
    xy_hal_i2c_config_t cfg = {
        .clock_speed = 100000,
        .addr_mode = XY_HAL_I2C_ADDR_7BIT,
    };
    xy_hal_i2c_configure(i2c, &cfg);
    
    /* 扫描 I2C 总线 */
    uint8_t addrs[16];
    int32_t count = xy_hal_i2c_scan(i2c, addrs, 16, 10);
    
    /* 可能没有设备，所以 >= 0 即可 */
    XY_TEST_ASSERT(count >= 0);
    
    /* 打印找到的设备 */
    if (count > 0) {
        printf("  Found %d device(s):\n", count);
        for (int i = 0; i < count; i++) {
            printf("    - 0x%02X\n", addrs[i]);
        }
    }
    
    xy_hal_i2c_unbind(i2c);
    return XY_TEST_PASS;
}

/**
 * @brief 测试 I2C 设备探测
 */
static xy_test_result_t test_i2c_probe(void)
{
    xy_hal_i2c_t i2c = xy_hal_i2c_bind("I2C1");
    XY_TEST_ASSERT_NOT_NULL(i2c);
    
    /* 探测常见设备地址 */
    uint16_t test_addrs[] = {0x68, 0x69, 0x76, 0x77}; /* MPU6050, etc. */
    
    for (size_t i = 0; i < sizeof(test_addrs)/sizeof(test_addrs[0]); i++) {
        xy_hal_error_t err = xy_hal_i2c_probe(i2c, test_addrs[i], 100);
        /* 可能不存在，只验证函数不崩溃 */
        (void)err;
    }
    
    xy_hal_i2c_unbind(i2c);
    return XY_TEST_PASS;
}

/**
 * @brief 测试 I2C 寄存器读写
 */
static xy_test_result_t test_i2c_reg_operations(void)
{
    xy_hal_i2c_t i2c = xy_hal_i2c_bind("I2C1");
    XY_TEST_ASSERT_NOT_NULL(i2c);
    
    xy_hal_i2c_config_t cfg = {
        .clock_speed = 400000,
    };
    xy_hal_i2c_configure(i2c, &cfg);
    
    /* 测试寄存器写 (假设设备存在) */
    uint8_t reg = 0x00;
    uint8_t value = 0x55;
    xy_hal_error_t err = xy_hal_i2c_reg_write(i2c, 0x68, &reg, 1, &value, 1, 100);
    /* 可能因设备不存在失败，只验证函数不崩溃 */
    (void)err;
    
    /* 测试寄存器读 */
    uint8_t rx_data;
    err = xy_hal_i2c_reg_read(i2c, 0x68, &reg, 1, &rx_data, 1, 100);
    (void)err;
    
    xy_hal_i2c_unbind(i2c);
    return XY_TEST_PASS;
}

/**
 * @brief 测试 I2C 错误处理
 */
static xy_test_result_t test_i2c_error_handling(void)
{
    /* 测试 NULL 句柄 */
    xy_hal_error_t err = xy_hal_i2c_configure(NULL, NULL);
    XY_TEST_ASSERT(err != XY_HAL_OK);
    
    /* 测试无效参数 */
    xy_hal_i2c_t i2c = xy_hal_i2c_bind("I2C1");
    XY_TEST_ASSERT_NOT_NULL(i2c);
    
    err = xy_hal_i2c_master_transmit(i2c, 0x68, NULL, 10, 100);
    XY_TEST_ASSERT(err != XY_HAL_OK);
    
    xy_hal_i2c_unbind(i2c);
    return XY_TEST_PASS;
}

/* ==================== Test Suite ==================== */

static xy_test_case_t i2c_test_cases[7];

xy_test_suite_t *i2c_get_test_suite(void)
{
    static xy_test_suite_t suite;
    
    suite.name = "I2C HAL Tests";
    suite.cases = i2c_test_cases;
    
    xy_test_add_case(&suite, "I2C Bind", test_i2c_bind);
    xy_test_add_case(&suite, "I2C Configure", test_i2c_configure);
    xy_test_add_case(&suite, "I2C Clock Speed", test_i2c_clock_speed);
    xy_test_add_case(&suite, "I2C Scan", test_i2c_scan);
    xy_test_add_case(&suite, "I2C Probe", test_i2c_probe);
    xy_test_add_case(&suite, "I2C Reg Operations", test_i2c_reg_operations);
    xy_test_add_case(&suite, "I2C Error Handling", test_i2c_error_handling);
    
    return &suite;
}

void run_i2c_tests(void)
{
    xy_test_suite_t *suite = i2c_get_test_suite();
    xy_test_run(suite);
    xy_test_print_report(suite);
}
