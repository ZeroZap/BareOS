/**
 * @file xy_hal_spi.c
 * @brief WCH CH32V30x SPI HAL Implementation
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_hal_spi.h"
#include "xy_log.h"

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

#ifdef MCU_CH32

#include "ch32v30x.h"

/**
 * @brief SPI 时钟使能
 */
static void xy_hal_spi_enable_clock(void *instance)
{
    if (instance == SPI1) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    } else if (instance == SPI2) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
    } else if (instance == SPI3) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);
    }
}

xy_hal_error_t xy_hal_spi_init(void *instance, const xy_hal_spi_config_t *config)
{
    SPI_InitTypeDef SPI_InitStructure;
    
    if (!instance || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 使能时钟 */
    xy_hal_spi_enable_clock(instance);
    
    /* 配置 GPIO */
    if (instance == SPI1) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
        /* PA5-SCK, PA6-MISO, PA7-MOSI */
        GPIO_PinRemapConfig(GPIO_Remap_SPI1, ENABLE);
    }
    
    /* 配置 SPI 参数 */
    SPI_InitStructure.SPI_Direction = config->mode == XY_HAL_SPI_MASTER ? 
                                      SPI_Direction_2Lines_FullDuplex : 
                                      SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = config->mode == XY_HAL_SPI_MASTER ?
                                 SPI_Mode_Master : SPI_Mode_Slave;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    
    /* 配置时钟极性和相位 */
    switch (config->polarity) {
        case 0:
            SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
            break;
        case 1:
            SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
            break;
    }
    
    switch (config->phase) {
        case 0:
            SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
            break;
        case 1:
            SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
            break;
    }
    
    /* NSS 管理 */
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    
    /* 波特率 */
    SPI_InitStructure.SPI_BaudRatePrescaler = config->baudrate_prescaler;
    
    /* MSB/LSB */
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    
    /* CRC */
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    
    SPI_Init(instance, &SPI_InitStructure);
    SPI_Cmd(instance, ENABLE);
    
    xy_log_d("WCH SPI init: instance=%p, mode=%d, prescaler=%d\n",
             instance, config->mode, config->baudrate_prescaler);
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_deinit(void *instance)
{
    if (!instance) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    SPI_Cmd(instance, DISABLE);
    SPI_I2S_DeInit(instance);
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_transfer(void *instance, const uint8_t *tx_data, 
                                   uint8_t *rx_data, uint16_t len, uint32_t timeout)
{
    uint16_t i;
    uint32_t start;
    
    if (!instance) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    for (i = 0; i < len; i++) {
        /* 等待发送缓冲区空 */
        start = xy_os_tick_get();
        while (SPI_I2S_GetFlagStatus(instance, SPI_I2S_FLAG_TXE) == RESET) {
            if (timeout > 0 && (xy_os_tick_get() - start) > timeout) {
                return XY_HAL_ERROR_TIMEOUT;
            }
        }
        
        /* 发送数据 */
        if (tx_data) {
            SPI_I2S_SendData(instance, tx_data[i]);
        } else {
            SPI_I2S_SendData(instance, 0xFF);
        }
        
        /* 等待接收缓冲区满 */
        start = xy_os_tick_get();
        while (SPI_I2S_GetFlagStatus(instance, SPI_I2S_FLAG_RXNE) == RESET) {
            if (timeout > 0 && (xy_os_tick_get() - start) > timeout) {
                return XY_HAL_ERROR_TIMEOUT;
            }
        }
        
        /* 读取数据 */
        if (rx_data) {
            rx_data[i] = SPI_I2S_ReceiveData(instance);
        } else {
            SPI_I2S_ReceiveData(instance);
        }
    }
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_write(void *instance, const uint8_t *data, 
                                uint16_t len, uint32_t timeout)
{
    return xy_hal_spi_transfer(instance, data, NULL, len, timeout);
}

xy_hal_error_t xy_hal_spi_read(void *instance, uint8_t *data, 
                               uint16_t len, uint32_t timeout)
{
    return xy_hal_spi_transfer(instance, NULL, data, len, timeout);
}

#else

xy_hal_error_t xy_hal_spi_init(void *instance, const xy_hal_spi_config_t *config)
{
    (void)instance;
    (void)config;
    return XY_HAL_ERROR_NOT_SUPPORT;
}

#endif
