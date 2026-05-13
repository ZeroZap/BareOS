/**
 * @file test_spi.c
 * @brief SPI HAL Test Cases
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 测试 SPI 基本功能：配置/收发/全双工测试
 */

#include "xy_hal_test.h"
#include "../inc/xy_hal_spi_dev.h"

/* ==================== Test Cases ==================== */

/**
 * @brief 测试 SPI 设备绑定
 */
static xy_test_result_t test_spi_bind(void)
{
    xy_hal_spi_t spi = xy_hal_spi_bind("SPI1");
    XY_TEST_ASSERT_NOT_NULL(spi);
    
    xy_hal_error_t err = xy_hal_spi_unbind(spi);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    return XY_TEST_PASS;
}

/**
 * @brief 测试 SPI 配置
 */
static xy_test_result_t test_spi_configure(void)
{
    xy_hal_spi_t spi = xy_hal_spi_bind("SPI1");
    XY_TEST_ASSERT_NOT_NULL(spi);
    
    xy_hal_spi_config_t cfg = {
        .mode = XY_HAL_SPI_MODE_0,
        .direction = XY_HAL_SPI_DIR_2LINES,
        .datasize = XY_HAL_SPI_DATASIZE_8BIT,
        .firstbit = XY_HAL_SPI_FIRSTBIT_MSB,
        .nss = XY_HAL_SPI_NSS_SOFT,
        .baudrate_prescaler = 64,
        .is_master = 1,
        .transfer_mode = XY_HAL_SPI_TRANSFER_POLLING,
    };
    
    xy_hal_error_t err = xy_hal_spi_configure(spi, &cfg);
    XY_TEST_ASSERT_EQ(0, (int)err);
    
    /* 验证配置 */
    xy_hal_spi_config_t read_cfg;
    err = xy_hal_spi_get_config(spi, &read_cfg);
    XY_TEST_ASSERT_EQ(0, (int)err);
    XY_TEST_ASSERT_EQ(XY_HAL_SPI_MODE_0, read_cfg.mode);
    
    xy_hal_spi_unbind(spi);
    return XY_TEST_PASS;
}

/**
 * @brief 测试 SPI 发送
 */
static xy_test_result_t test_spi_send(void)
{
    xy_hal_spi_t spi = xy_hal_spi_bind("SPI1");
    XY_TEST_ASSERT_NOT_NULL(spi);
    
    xy_hal_spi_config_t cfg = {
        .mode = XY_HAL_SPI_MODE_0,
        .is_master = 1,
    };
    xy_hal_spi_configure(spi, &cfg);
    
    /* 测试发送数据 */
    uint8_t tx_data[10] = {0x01, 0x02, 0x03, 0x04, 0x05};
    int32_t sent = xy_hal_spi_send(spi, tx_data, 10, 100);
    XY_TEST_ASSERT(sent >= 0); /* 可能因硬件未连接返回 0 */
    
    xy_hal_spi_unbind(spi);
    return XY_TEST_PASS;
}

/**
 * @brief 测试 SPI 全双工收发
 */
static xy_test_result_t test_spi_transfer(void)
{
    xy_hal_spi_t spi = xy_hal_spi_bind("SPI1");
    XY_TEST_ASSERT_NOT_NULL(spi);
    
    xy_hal_spi_config_t cfg = {
        .mode = XY_HAL_SPI_MODE_0,
        .direction = XY_HAL_SPI_DIR_2LINES,
        .is_master = 1,
    };
    xy_hal_spi_configure(spi, &cfg);
    
    /* 测试全双工收发 */
    uint8_t tx_data[10] = {0x01, 0x02, 0x03};
    uint8_t rx_data[10] = {0};
    
    int32_t transferred = xy_hal_spi_transfer(spi, tx_data, rx_data, 10, 100);
    XY_TEST_ASSERT(transferred >= 0);
    
    xy_hal_spi_unbind(spi);
    return XY_TEST_PASS;
}

/**
 * @brief 测试 SPI 发送单字节
 */
static xy_test_result_t test_spi_send_byte(void)
{
    xy_hal_spi_t spi = xy_hal_spi_bind("SPI1");
    XY_TEST_ASSERT_NOT_NULL(spi);
    
    xy_hal_spi_config_t cfg = {
        .mode = XY_HAL_SPI_MODE_0,
        .is_master = 1,
    };
    xy_hal_spi_configure(spi, &cfg);
    
    /* 发送单字节并接收响应 */
    int32_t response = xy_hal_spi_send_byte(spi, 0x55, 100);
    XY_TEST_ASSERT(response >= 0); /* 返回接收到的字节或错误码 */
    
    xy_hal_spi_unbind(spi);
    return XY_TEST_PASS;
}

/**
 * @brief 测试 SPI 错误处理
 */
static xy_test_result_t test_spi_error_handling(void)
{
    /* 测试 NULL 句柄 */
    xy_hal_error_t err = xy_hal_spi_configure(NULL, NULL);
    XY_TEST_ASSERT(err != XY_HAL_OK);
    
    /* 测试无效参数 */
    xy_hal_spi_t spi = xy_hal_spi_bind("SPI1");
    XY_TEST_ASSERT_NOT_NULL(spi);
    
    err = xy_hal_spi_send(spi, NULL, 10, 100);
    XY_TEST_ASSERT(err != XY_HAL_OK);
    
    xy_hal_spi_unbind(spi);
    return XY_TEST_PASS;
}

/* ==================== Test Suite ==================== */

static xy_test_case_t spi_test_cases[6];

xy_test_suite_t *spi_get_test_suite(void)
{
    static xy_test_suite_t suite;
    
    suite.name = "SPI HAL Tests";
    suite.cases = spi_test_cases;
    
    xy_test_add_case(&suite, "SPI Bind", test_spi_bind);
    xy_test_add_case(&suite, "SPI Configure", test_spi_configure);
    xy_test_add_case(&suite, "SPI Send", test_spi_send);
    xy_test_add_case(&suite, "SPI Transfer", test_spi_transfer);
    xy_test_add_case(&suite, "SPI Send Byte", test_spi_send_byte);
    xy_test_add_case(&suite, "SPI Error Handling", test_spi_error_handling);
    
    return &suite;
}

void run_spi_tests(void)
{
    xy_test_suite_t *suite = spi_get_test_suite();
    xy_test_run(suite);
    xy_test_print_report(suite);
}
