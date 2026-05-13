/**
 * @file test_uart.c
 * @brief UART HAL Test Cases
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 测试 UART 基本功能：配置/收发/回环测试
 */

#include "xy_hal_test.h"
#include "../inc/xy_hal_uart_dev.h"

/* ==================== Test Cases ==================== */

/**
 * @brief 测试 UART 设备绑定
 */
static xy_test_result_t test_uart_bind(void)
{
    xy_hal_uart_t uart = xy_hal_uart_bind("USART2");
    XY_TEST_ASSERT_NOT_NULL(uart);
    
    xy_hal_error_t err = xy_hal_uart_unbind(uart);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    return XY_TEST_PASS;
}

/**
 * @brief 测试 UART 配置
 */
static xy_test_result_t test_uart_configure(void)
{
    xy_hal_uart_t uart = xy_hal_uart_bind("USART2");
    XY_TEST_ASSERT_NOT_NULL(uart);
    
    xy_hal_uart_config_t cfg = {
        .baudrate = 115200,
        .wordlen = XY_HAL_UART_WORDLEN_8B,
        .stopbits = XY_HAL_UART_STOPBITS_1,
        .parity = XY_HAL_UART_PARITY_NONE,
        .flowctrl = XY_HAL_UART_FLOWCTRL_NONE,
        .mode = XY_HAL_UART_MODE_TX_RX,
        .transfer_mode = XY_HAL_UART_TRANSFER_POLLING,
    };
    
    xy_hal_error_t err = xy_hal_uart_configure(uart, &cfg);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    /* 验证配置 */
    xy_hal_uart_config_t read_cfg;
    err = xy_hal_uart_get_config(uart, &read_cfg);
    XY_TEST_ASSERT_EQ(0, (int)err);
    XY_TEST_ASSERT_EQ(115200, read_cfg.baudrate);
    
    xy_hal_uart_unbind(uart);
    return XY_TEST_PASS;
}

/**
 * @brief 测试 UART 波特率设置
 */
static xy_test_result_t test_uart_baudrate(void)
{
    xy_hal_uart_t uart = xy_hal_uart_bind("USART2");
    XY_TEST_ASSERT_NOT_NULL(uart);
    
    /* 设置不同波特率 */
    xy_hal_error_t err = xy_hal_uart_set_baudrate(uart, 9600);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    uint32_t baud = xy_hal_uart_get_baudrate(uart);
    XY_TEST_ASSERT_EQ(9600, baud);
    
    err = xy_hal_uart_set_baudrate(uart, 115200);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    xy_hal_uart_unbind(uart);
    return XY_TEST_PASS;
}

/**
 * @brief 测试 UART 阻塞收发
 */
static xy_test_result_t test_uart_blocking_transfer(void)
{
    xy_hal_uart_t uart = xy_hal_uart_bind("USART2");
    XY_TEST_ASSERT_NOT_NULL(uart);
    
    /* 配置 UART */
    xy_hal_uart_config_t cfg = {
        .baudrate = 115200,
        .wordlen = XY_HAL_UART_WORDLEN_8B,
        .stopbits = XY_HAL_UART_STOPBITS_1,
        .parity = XY_HAL_UART_PARITY_NONE,
        .mode = XY_HAL_UART_MODE_TX_RX,
    };
    xy_hal_uart_configure(uart, &cfg);
    
    /* 测试发送字符串 */
    const char *test_str = "Hello UART Test!\r\n";
    int32_t sent = xy_hal_uart_puts(uart, test_str, 100);
    XY_TEST_ASSERT(sent > 0);
    
    /* 测试发送单个字符 */
    xy_hal_error_t err = xy_hal_uart_putchar(uart, 'X', 100);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    xy_hal_uart_unbind(uart);
    return XY_TEST_PASS;
}

/**
 * @brief 测试 UART 非阻塞收发
 */
static xy_test_result_t test_uart_nonblocking_transfer(void)
{
    xy_hal_uart_t uart = xy_hal_uart_bind("USART2");
    XY_TEST_ASSERT_NOT_NULL(uart);
    
    /* 检查发送缓冲区状态 */
    int32_t tx_empty = xy_hal_uart_tx_empty(uart);
    XY_TEST_ASSERT(tx_empty == 0 || tx_empty == 1);
    
    /* 检查接收缓冲区状态 */
    int32_t available = xy_hal_uart_data_available(uart);
    XY_TEST_ASSERT(available == 0 || available == 1);
    
    xy_hal_uart_unbind(uart);
    return XY_TEST_PASS;
}

/**
 * @brief 测试 UART 错误处理
 */
static xy_test_result_t test_uart_error_handling(void)
{
    /* 测试 NULL 句柄 */
    xy_hal_error_t err = xy_hal_uart_configure(NULL, NULL);
    XY_TEST_ASSERT(err != XY_HAL_OK);
    
    /* 测试无效参数 */
    xy_hal_uart_t uart = xy_hal_uart_bind("USART2");
    XY_TEST_ASSERT_NOT_NULL(uart);
    
    err = xy_hal_uart_write(uart, NULL, 10, 100);
    XY_TEST_ASSERT(err != XY_HAL_OK);
    
    xy_hal_uart_unbind(uart);
    return XY_TEST_PASS;
}

/**
 * @brief 测试 UART 边界条件
 */
static xy_test_result_t test_uart_edge_cases(void)
{
    xy_hal_uart_t uart = xy_hal_uart_bind("USART2");
    XY_TEST_ASSERT_NOT_NULL(uart);
    
    /* 配置 UART */
    xy_hal_uart_config_t cfg = {
        .baudrate = 115200,
        .wordlen = XY_HAL_UART_WORDLEN_8B,
        .stopbits = XY_HAL_UART_STOPBITS_1,
        .parity = XY_HAL_UART_PARITY_NONE,
        .mode = XY_HAL_UART_MODE_TX_RX,
    };
    xy_hal_uart_configure(uart, &cfg);
    
    /* 测试最低波特率 */
    xy_hal_error_t err = xy_hal_uart_set_baudrate(uart, 1200);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    /* 测试最高波特率 */
    err = xy_hal_uart_set_baudrate(uart, 921600);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    /* 测试零长度数据 */
    err = xy_hal_uart_write(uart, (const uint8_t *)"", 0, 100);
    XY_TEST_ASSERT(err != XY_HAL_OK); /* 应返回错误 */
    
    /* 测试超时时间为 0 */
    int32_t ret = xy_hal_uart_write(uart, (const uint8_t *)"test", 4, 0);
    XY_TEST_ASSERT(ret >= 0 || ret == XY_HAL_ERROR_TIMEOUT);
    
    xy_hal_uart_unbind(uart);
    return XY_TEST_PASS;
}

/* ==================== Test Suite ==================== */

static xy_test_case_t uart_test_cases[7];

xy_test_suite_t *uart_get_test_suite(void)
{
    static xy_test_suite_t suite;
    
    suite.name = "UART HAL Tests";
    suite.cases = uart_test_cases;
    
    xy_test_add_case(&suite, "UART Bind", test_uart_bind);
    xy_test_add_case(&suite, "UART Configure", test_uart_configure);
    xy_test_add_case(&suite, "UART Baudrate", test_uart_baudrate);
    xy_test_add_case(&suite, "UART Blocking Transfer", test_uart_blocking_transfer);
    xy_test_add_case(&suite, "UART Non-blocking Transfer", test_uart_nonblocking_transfer);
    xy_test_add_case(&suite, "UART Error Handling", test_uart_error_handling);
    xy_test_add_case(&suite, "UART Edge Cases", test_uart_edge_cases);
    
    return &suite;
}

void run_uart_tests(void)
{
    xy_test_suite_t *suite = uart_get_test_suite();
    xy_test_run(suite);
    xy_test_print_report(suite);
}
