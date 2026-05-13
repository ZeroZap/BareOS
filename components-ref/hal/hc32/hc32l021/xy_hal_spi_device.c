/**
 * @file xy_hal_spi_device.c
 * @brief HC32L021 SPI Device Implementation - Unified HAL API
 * @version 1.0.0
 * @date 2026-03-16
 * 
 * @note 实现统一的 SPI 设备 API，基于 HC32L021 寄存器
 */

#include "../inc/xy_hal_spi_dev.h"
#include "../inc/xy_hal_spi_types.h"
#include "../inc/xy_hal_error.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* HC32L021 最小化头文件 (无 board 依赖) */
#include "hc32l021_minimal.h"

/* ==================== Private Definitions ==================== */

#define HC32L021_SPI_COUNT  (2)  /* SPI0/SPI1 */

/* ==================== Private Types ==================== */

/**
 * @brief HC32L021 SPI 设备私有数据
 */
typedef struct {
    uint8_t instance;          /* SPI 实例号 (0/1) */
    bool initialized;          /* 初始化标志 */
    xy_hal_spi_config_t config; /* 当前配置 */
    bool busy;                 /* 忙标志 */
} hc32_spi_dev_t;

/* ==================== Private Variables ==================== */

/* SPI 设备实例池 */
static hc32_spi_dev_t spi_devices[HC32L021_SPI_COUNT];
static bool spi_devices_initialized = false;

/* ==================== Private Functions ==================== */

/**
 * @brief 初始化 SPI 设备池
 */
static void spi_init_devices(void)
{
    if (!spi_devices_initialized) {
        memset(spi_devices, 0, sizeof(spi_devices));
        spi_devices_initialized = true;
    }
}

/**
 * @brief 查找或分配 SPI 设备
 * @param instance SPI 实例号
 * @return hc32_spi_dev_t* SPI 设备指针
 */
static hc32_spi_dev_t *spi_find_or_alloc(uint8_t instance)
{
    spi_init_devices();
    
    if (instance >= HC32L021_SPI_COUNT) {
        return NULL;
    }
    
    hc32_spi_dev_t *dev = &spi_devices[instance];
    
    if (!dev->initialized) {
        dev->instance = instance;
        dev->initialized = true;
        dev->busy = false;
    }
    
    return dev;
}

/**
 * @brief 解析 SPI 名称
 * @param name SPI 名称 (如 "SPI0", "SPI1")
 * @param instance 输出：实例号
 * @return 0 成功，-1 失败
 */
static int parse_spi_name(const char *name, uint8_t *instance)
{
    if (!name || !instance) {
        return -1;
    }
    
    if (strncmp(name, "SPI", 3) == 0) {
        if (name[3] >= '0' && name[3] <= '1') {
            *instance = name[3] - '0';
            return 0;
        }
    }
    
    return -1;
}

/* ==================== API Implementation ==================== */

xy_hal_spi_t xy_hal_spi_bind(const char *name)
{
    uint8_t instance;
    
    if (parse_spi_name(name, &instance) != 0) {
        return NULL;
    }
    
    hc32_spi_dev_t *dev = spi_find_or_alloc(instance);
    if (!dev) {
        return NULL;
    }
    
    return (xy_hal_spi_t)dev;
}

xy_hal_error_t xy_hal_spi_unbind(xy_hal_spi_t spi)
{
    if (!spi) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_spi_dev_t *dev = (hc32_spi_dev_t *)spi;
    dev->initialized = false;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_configure(xy_hal_spi_t spi,
                                    const xy_hal_spi_config_t *config)
{
    if (!spi || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_spi_dev_t *dev = (hc32_spi_dev_t *)spi;
    
    /* 保存配置 */
    memcpy(&dev->config, config, sizeof(dev->config));
    
    /* 配置 HC32L021 SPI 硬件寄存器 */
    M0P_SPI_TypeDef *spi_reg;
    
    switch (dev->instance) {
        case 0:
            spi_reg = M0P_SPI0;
            Sysctrl_SetPeripheralGate(SysctrlPeripheralSpi0, TRUE);
            break;
        case 1:
            spi_reg = M0P_SPI1;
            Sysctrl_SetPeripheralGate(SysctrlPeripheralSpi1, TRUE);
            break;
        default:
            return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 复位 SPI */
    spi_reg->CR1_f.SPIRST = 1;
    spi_reg->CR1_f.SPIRST = 0;
    
    /* 配置 SPI 控制寄存器 */
    spi_reg->CR1_f.MSTR = (config->mode & XY_HAL_SPI_MODE_MASTER) ? 1 : 0;
    spi_reg->CR1_f.CPOL = (config->mode & XY_HAL_SPI_MODE_CPOL_HIGH) ? 1 : 0;
    spi_reg->CR1_f.CPHA = (config->mode & XY_HAL_SPI_MODE_CPHA_SECOND) ? 1 : 0;
    spi_reg->CR1_f.DFF = (config->data_bits == 16) ? 1 : 0;
    
    /* 配置波特率 */
    uint8_t br_div = 0;
    if (config->baud_rate <= 250000) br_div = 7;    // fPCLK/256
    else if (config->baud_rate <= 500000) br_div = 6; // fPCLK/128
    else if (config->baud_rate <= 1000000) br_div = 5; // fPCLK/64
    else if (config->baud_rate <= 2000000) br_div = 4; // fPCLK/32
    else if (config->baud_rate <= 4000000) br_div = 3; // fPCLK/16
    else if (config->baud_rate <= 8000000) br_div = 2; // fPCLK/8
    else if (config->baud_rate <= 16000000) br_div = 1; // fPCLK/4
    else br_div = 0; // fPCLK/2
    
    spi_reg->CR1_f.BR = br_div;
    
    /* 使能 SPI */
    spi_reg->CR1_f.SPE = 1;
    
    return XY_HAL_OK;
}

int32_t xy_hal_spi_send(xy_hal_spi_t spi, const uint8_t *tx_data,
                        size_t length, uint32_t timeout)
{
    if (!spi || !tx_data || length == 0) {
        return -1;
    }
    
    hc32_spi_dev_t *dev = (hc32_spi_dev_t *)spi;
    
    if (dev->busy) {
        return -XY_HAL_ERROR_BUSY;
    }
    
    dev->busy = true;
    
    /* 实现 SPI 发送 */
    M0P_SPI_TypeDef *spi_reg;
    switch (dev->instance) {
        case 0: spi_reg = M0P_SPI0; break;
        case 1: spi_reg = M0P_SPI1; break;
        default: 
            dev->busy = false;
            return -XY_HAL_ERROR_INVALID_PARAM;
    }
    
    for (size_t i = 0; i < length; i++) {
        /* 等待发送缓冲区空 */
        while (!spi_reg->SR_f.TXE);
        
        /* 发送数据 */
        if (dev->config.data_bits == 16) {
            spi_reg->DR = ((uint16_t*)tx_data)[i];
        } else {
            spi_reg->DR = tx_data[i];
        }
        
        /* 等待传输完成 */
        while (!spi_reg->SR_f.BSY);
    }
    
    dev->busy = false;
    
    return (int32_t)length;
}

int32_t xy_hal_spi_receive(xy_hal_spi_t spi, uint8_t *rx_data,
                           size_t length, uint32_t timeout)
{
    if (!spi || !rx_data || length == 0) {
        return -1;
    }
    
    hc32_spi_dev_t *dev = (hc32_spi_dev_t *)spi;
    
    if (dev->busy) {
        return -XY_HAL_ERROR_BUSY;
    }
    
    dev->busy = true;
    
    /* 实现 SPI 接收 */
    M0P_SPI_TypeDef *spi_reg;
    switch (dev->instance) {
        case 0: spi_reg = M0P_SPI0; break;
        case 1: spi_reg = M0P_SPI1; break;
        default: 
            dev->busy = false;
            return -XY_HAL_ERROR_INVALID_PARAM;
    }
    
    for (size_t i = 0; i < length; i++) {
        /* 发送 dummy 数据产生时钟 */
        if (dev->config.data_bits == 16) {
            spi_reg->DR = 0xFFFF;
        } else {
            spi_reg->DR = 0xFF;
        }
        
        /* 等待接收完成 */
        while (!spi_reg->SR_f.RXNE);
        
        /* 读取数据 */
        if (dev->config.data_bits == 16) {
            ((uint16_t*)rx_data)[i] = spi_reg->DR;
        } else {
            rx_data[i] = spi_reg->DR;
        }
    }
    
    dev->busy = false;
    
    return (int32_t)length;
}

int32_t xy_hal_spi_transfer(xy_hal_spi_t spi, const uint8_t *tx_data,
                            uint8_t *rx_data, size_t length, uint32_t timeout)
{
    if (!spi || !tx_data || !rx_data || length == 0) {
        return -1;
    }
    
    hc32_spi_dev_t *dev = (hc32_spi_dev_t *)spi;
    
    if (dev->busy) {
        return -XY_HAL_ERROR_BUSY;
    }
    
    dev->busy = true;
    
    /* 实现 SPI 全双工传输 */
    M0P_SPI_TypeDef *spi_reg;
    switch (dev->instance) {
        case 0: spi_reg = M0P_SPI0; break;
        case 1: spi_reg = M0P_SPI1; break;
        default: 
            dev->busy = false;
            return -XY_HAL_ERROR_INVALID_PARAM;
    }
    
    for (size_t i = 0; i < length; i++) {
        /* 发送数据 */
        if (dev->config.data_bits == 16) {
            spi_reg->DR = ((uint16_t*)tx_data)[i];
        } else {
            spi_reg->DR = tx_data[i];
        }
        
        /* 等待接收完成 */
        while (!spi_reg->SR_f.RXNE);
        
        /* 读取接收数据 */
        if (dev->config.data_bits == 16) {
            ((uint16_t*)rx_data)[i] = spi_reg->DR;
        } else {
            rx_data[i] = spi_reg->DR;
        }
    }
    
    dev->busy = false;
    
    return (int32_t)length;
}

/* ==================== End of File ==================== */
