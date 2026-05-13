/**
 * @file test_stress.c
 * @brief HAL Stress Test Cases
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 压力测试和性能测试
 */

#include "xy_hal_test.h"
#include "../inc/xy_hal_gpio_dev.h"
#include "../inc/xy_hal_uart_dev.h"

/* ==================== Stress Test Cases ==================== */

/**
 * @brief GPIO 高频切换压力测试
 */
static xy_test_result_t test_gpio_stress_toggle(void)
{
    xy_hal_gpio_t gpio = xy_hal_gpio_bind("GPIOA");
    XY_TEST_ASSERT_NOT_NULL(gpio);
    
    /* 配置 PA5 为输出 */
    xy_hal_gpio_config_t cfg = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
        .speed = XY_HAL_GPIO_SPEED_VERY_HIGH,
    };
    xy_hal_gpio_configure(gpio, 5, &cfg);
    
    /* 高频切换 1000 次 */
    const int iterations = 1000;
    for (int i = 0; i < iterations; i++) {
        xy_hal_gpio_toggle(gpio, 5);
    }
    
    /* 验证最终状态 */
    int32_t value = xy_hal_gpio_read(gpio, 5);
    XY_TEST_ASSERT(value == 0 || value == 1);
    
    xy_hal_gpio_unbind(gpio);
    
    printf("  Stress: %d toggles completed\n", iterations);
    return XY_TEST_PASS;
}

/**
 * @brief GPIO 批量配置压力测试
 */
static xy_test_result_t test_gpio_stress_bulk_config(void)
{
    xy_hal_gpio_t gpio = xy_hal_gpio_bind("GPIOB");
    XY_TEST_ASSERT_NOT_NULL(gpio);
    
    /* 批量配置 16 个引脚 */
    const int pin_count = 16;
    for (int pin = 0; pin < pin_count; pin++) {
        xy_hal_gpio_config_t cfg = {
            .mode = XY_HAL_GPIO_MODE_OUTPUT,
            .pull = XY_HAL_GPIO_PULL_NONE,
            .speed = XY_HAL_GPIO_SPEED_HIGH,
        };
        xy_hal_error_t err = xy_hal_gpio_configure(gpio, pin, &cfg);
        XY_TEST_ASSERT_EQ(0, (int)err);
    }
    
    /* 批量写入测试 */
    xy_hal_gpio_port_write(gpio, 0xFFFF, 0xAAAA);
    xy_hal_gpio_port_write(gpio, 0xFFFF, 0x5555);
    
    xy_hal_gpio_unbind(gpio);
    
    printf("  Stress: %d pins configured\n", pin_count);
    return XY_TEST_PASS;
}

/**
 * @brief UART 连续发送压力测试
 */
static xy_test_result_t test_uart_stress_continuous_send(void)
{
    xy_hal_uart_t uart = xy_hal_uart_bind("USART2");
    XY_TEST_ASSERT_NOT_NULL(uart);
    
    /* 配置 UART */
    xy_hal_uart_config_t cfg = {
        .baudrate = 115200,
        .wordlen = XY_HAL_UART_WORDLEN_8B,
        .stopbits = XY_HAL_UART_STOPBITS_1,
        .parity = XY_HAL_UART_PARITY_NONE,
        .mode = XY_HAL_UART_MODE_TX,
    };
    xy_hal_uart_configure(uart, &cfg);
    
    /* 连续发送 100 次 */
    const char *test_data = "Stress test data\r\n";
    const int iterations = 100;
    
    for (int i = 0; i < iterations; i++) {
        int32_t sent = xy_hal_uart_write(uart, (const uint8_t *)test_data, 
                                         strlen(test_data), 100);
        XY_TEST_ASSERT(sent > 0);
    }
    
    /* 等待发送完成 */
    xy_hal_uart_wait_tx_complete(uart, 1000);
    
    xy_hal_uart_unbind(uart);
    
    printf("  Stress: %d transmissions completed\n", iterations);
    return XY_TEST_PASS;
}

/**
 * @brief UART 大数据量压力测试
 */
static xy_test_result_t test_uart_stress_large_data(void)
{
    xy_hal_uart_t uart = xy_hal_uart_bind("USART2");
    XY_TEST_ASSERT_NOT_NULL(uart);
    
    /* 配置 UART */
    xy_hal_uart_config_t cfg = {
        .baudrate = 115200,
        .mode = XY_HAL_UART_MODE_TX,
    };
    xy_hal_uart_configure(uart, &cfg);
    
    /* 发送大数据块 (1KB) */
    const int data_size = 1024;
    uint8_t *test_data = (uint8_t *)malloc(data_size);
    XY_TEST_ASSERT_NOT_NULL(test_data);
    
    /* 填充测试数据 */
    for (int i = 0; i < data_size; i++) {
        test_data[i] = (uint8_t)(i & 0xFF);
    }
    
    /* 发送数据 */
    int32_t sent = xy_hal_uart_write(uart, test_data, data_size, 5000);
    XY_TEST_ASSERT_EQ(data_size, sent);
    
    free(test_data);
    xy_hal_uart_unbind(uart);
    
    printf("  Stress: %d bytes transmitted\n", data_size);
    return XY_TEST_PASS;
}

/* ==================== Test Suite ==================== */

static xy_test_case_t stress_test_cases[4];

xy_test_suite_t *stress_get_test_suite(void)
{
    static xy_test_suite_t suite;
    
    suite.name = "HAL Stress Tests";
    suite.cases = stress_test_cases;
    
    xy_test_add_case(&suite, "GPIO Toggle Stress", test_gpio_stress_toggle);
    xy_test_add_case(&suite, "GPIO Bulk Config Stress", test_gpio_stress_bulk_config);
    xy_test_add_case(&suite, "UART Continuous Send Stress", test_uart_stress_continuous_send);
    xy_test_add_case(&suite, "UART Large Data Stress", test_uart_stress_large_data);
    
    return &suite;
}

void run_stress_tests(void)
{
    xy_test_suite_t *suite = stress_get_test_suite();
    xy_test_run(suite);
    xy_test_print_report(suite);
}
