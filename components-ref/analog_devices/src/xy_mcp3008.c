/**
 * @file xy_mcp3008.c
 * @brief MCP3008 10-bit 8-channel ADC Driver
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_adc_ext.h"
#include "xy_hal_spi.h"
#include "xy_log.h"

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/**
 * @brief SPI 传输
 */
static uint16_t xy_mcp3008_spi_transfer(xy_mcp3008_t *mcp, uint16_t data)
{
    uint8_t tx_buf[2];
    uint8_t rx_buf[2];
    
    tx_buf[0] = (data >> 8) & 0xFF;
    tx_buf[1] = data & 0xFF;
    
    xy_hal_spi_transfer(mcp->spi_handle, tx_buf, rx_buf, 2, 100);
    
    return ((uint16_t)rx_buf[0] << 8) | rx_buf[1];
}

int xy_mcp3008_init(xy_mcp3008_t *mcp, void *spi_handle, uint8_t cs_pin)
{
    if (!mcp || !spi_handle) {
        return -1;
    }
    
    mcp->spi_handle = spi_handle;
    mcp->cs_pin = cs_pin;
    
    /* 初始化 CS 引脚 */
    xy_hal_gpio_config_t gpio_cfg = {
        .mode = XY_HAL_GPIO_MODE_OUTPUT,
        .pull = XY_HAL_GPIO_PULL_NONE,
    };
    xy_hal_gpio_init(mcp->cs_pin, &gpio_cfg);
    xy_hal_gpio_set(mcp->cs_pin, 1);
    
    xy_log_i("MCP3008 initialized (CS=%d)\n", cs_pin);
    return 0;
}

int xy_mcp3008_read(xy_mcp3008_t *mcp, uint8_t channel, uint16_t *value)
{
    uint16_t cmd;
    uint16_t result;
    
    if (!mcp || !value || channel > 7) {
        return -1;
    }
    
    /* 构建命令 */
    /* Start bit + Single/Diff + D2 D1 D0 + Null */
    cmd = 0x0180;  /* Start bit (1), Single-ended (1) */
    cmd |= ((channel & 0x07) << 4);  /* 通道选择 */
    
    /* 片选有效 */
    xy_hal_gpio_set(mcp->cs_pin, 0);
    
    /* SPI 传输 */
    result = xy_mcp3008_spi_transfer(mcp, cmd);
    
    /* 片选无效 */
    xy_hal_gpio_set(mcp->cs_pin, 1);
    
    /* 提取 10 位数据 */
    *value = (result >> 1) & 0x3FF;
    
    xy_log_d("MCP3008 CH%d: %d\n", channel, *value);
    return 0;
}
