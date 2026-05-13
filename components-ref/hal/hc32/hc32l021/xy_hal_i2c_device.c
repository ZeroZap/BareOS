/**
 * @file xy_hal_i2c_device.c
 * @brief HC32L021 I2C Device Implementation - Unified HAL API
 * @version 1.0.0
 * @date 2026-03-16
 * 
 * @note 实现统一的 I2C 设备 API，基于 HC32L021 寄存器
 */

#include "../inc/xy_hal_i2c_dev.h"
#include "../inc/xy_hal_i2c_types.h"
#include "../inc/xy_hal_error.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* HC32L021 最小化头文件 */
#include "hc32l021_minimal.h"

/* ==================== Private Definitions ==================== */

#define HC32L021_I2C_COUNT  (2)  /* I2C0/I2C1 */

/* ==================== Private Types ==================== */

/**
 * @brief HC32L021 I2C 设备私有数据
 */
typedef struct {
    uint8_t instance;          /* I2C 实例号 (0/1) */
    bool initialized;          /* 初始化标志 */
    xy_hal_i2c_config_t config; /* 当前配置 */
    bool busy;                 /* 忙标志 */
} hc32_i2c_dev_t;

/* ==================== Private Variables ==================== */

/* I2C 设备实例池 */
static hc32_i2c_dev_t i2c_devices[HC32L021_I2C_COUNT];
static bool i2c_devices_initialized = false;

/* ==================== Private Functions ==================== */

/**
 * @brief 初始化 I2C 设备池
 */
static void i2c_init_devices(void)
{
    if (!i2c_devices_initialized) {
        memset(i2c_devices, 0, sizeof(i2c_devices));
        i2c_devices_initialized = true;
    }
}

/**
 * @brief 查找或分配 I2C 设备
 * @param instance I2C 实例号
 * @return hc32_i2c_dev_t* I2C 设备指针
 */
static hc32_i2c_dev_t *i2c_find_or_alloc(uint8_t instance)
{
    i2c_init_devices();
    
    if (instance >= HC32L021_I2C_COUNT) {
        return NULL;
    }
    
    hc32_i2c_dev_t *dev = &i2c_devices[instance];
    
    if (!dev->initialized) {
        dev->instance = instance;
        dev->initialized = true;
        dev->busy = false;
    }
    
    return dev;
}

/**
 * @brief 解析 I2C 名称
 * @param name I2C 名称 (如 "I2C0", "I2C1")
 * @param instance 输出：实例号
 * @return 0 成功，-1 失败
 */
static int parse_i2c_name(const char *name, uint8_t *instance)
{
    if (!name || !instance) {
        return -1;
    }
    
    if (strncmp(name, "I2C", 3) == 0) {
        if (name[3] >= '0' && name[3] <= '1') {
            *instance = name[3] - '0';
            return 0;
        }
    }
    
    return -1;
}

/* ==================== API Implementation ==================== */

xy_hal_i2c_t xy_hal_i2c_bind(const char *name)
{
    uint8_t instance;
    
    if (parse_i2c_name(name, &instance) != 0) {
        return NULL;
    }
    
    hc32_i2c_dev_t *dev = i2c_find_or_alloc(instance);
    if (!dev) {
        return NULL;
    }
    
    return (xy_hal_i2c_t)dev;
}

xy_hal_error_t xy_hal_i2c_unbind(xy_hal_i2c_t i2c)
{
    if (!i2c) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_i2c_dev_t *dev = (hc32_i2c_dev_t *)i2c;
    dev->initialized = false;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_configure(xy_hal_i2c_t i2c,
                                    const xy_hal_i2c_config_t *config)
{
    if (!i2c || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_i2c_dev_t *dev = (hc32_i2c_dev_t *)i2c;
    
    /* 保存配置 */
    memcpy(&dev->config, config, sizeof(dev->config));
    
    /* 配置 HC32L021 I2C 硬件寄存器 */
    M0P_I2C_TypeDef *i2c_reg;
    
    switch (dev->instance) {
        case 0:
            i2c_reg = M0P_I2C0;
            Sysctrl_SetPeripheralGate(SysctrlPeripheralI2c0, TRUE);
            break;
        case 1:
            i2c_reg = M0P_I2C1;
            Sysctrl_SetPeripheralGate(SysctrlPeripheralI2c1, TRUE);
            break;
        default:
            return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 复位 I2C */
    i2c_reg->CR1_f.PE = 0;
    i2c_reg->CR1_f.PE = 1;
    
    /* 配置时钟频率 */
    uint32_t pclk_freq = SystemCoreClock; // 假设系统时钟为 PCLK
    uint16_t ccr_value = 0;
    uint8_t trise_value = 0;
    
    if (config->baud_rate <= 100000) {
        /* 标准模式 (100kHz) */
        ccr_value = pclk_freq / (2 * config->baud_rate);
        trise_value = pclk_freq / 1000000 + 1; // 1us rise time
    } else if (config->baud_rate <= 400000) {
        /* 快速模式 (400kHz) */
        ccr_value = pclk_freq / (3 * config->baud_rate);
        trise_value = pclk_freq / 1000000 + 1; // 0.3us rise time for fast mode
    } else {
        /* 快速模式+ (1MHz) */
        ccr_value = pclk_freq / (3 * config->baud_rate);
        trise_value = pclk_freq / 1000000 + 1; // 0.12us rise time
    }
    
    /* 设置 CCR 和 TRISE */
    i2c_reg->CCR = ccr_value & 0xFFF;
    i2c_reg->TRISE = trise_value & 0x3F;
    
    /* 使能 I2C */
    i2c_reg->CR1_f.PE = 1;
    
    return XY_HAL_OK;
}

int32_t xy_hal_i2c_master_transmit(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                   const uint8_t *data, size_t length,
                                   uint32_t timeout)
{
    if (!i2c || !data || length == 0) {
        return -1;
    }
    
    hc32_i2c_dev_t *dev = (hc32_i2c_dev_t *)i2c;
    
    if (dev->busy) {
        return -XY_HAL_ERROR_BUSY;
    }
    
    dev->busy = true;
    
    /* 实现 I2C 主模式发送 */
    M0P_I2C_TypeDef *i2c_reg;
    switch (dev->instance) {
        case 0: i2c_reg = M0P_I2C0; break;
        case 1: i2c_reg = M0P_I2C1; break;
        default: 
            dev->busy = false;
            return -XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 发送 START 条件 */
    i2c_reg->CR1_f.START = 1;
    
    /* 等待 SB 标志 */
    while (!i2c_reg->SR1_f.SB);
    
    /* 发送设备地址 (写) */
    i2c_reg->DR = (dev_addr << 1) & 0xFE;
    
    /* 等待 ADDR 标志 */
    while (!i2c_reg->SR1_f.ADDR);
    
    /* 清除 ADDR 标志 */
    volatile uint8_t dummy = i2c_reg->SR2;
    (void)dummy;
    
    /* 发送数据 */
    for (size_t i = 0; i < length; i++) {
        /* 等待 TXE 标志 */
        while (!i2c_reg->SR1_f.TXE);
        
        /* 发送数据字节 */
        i2c_reg->DR = data[i];
    }
    
    /* 等待 BTF 标志 (字节传输完成) */
    while (!i2c_reg->SR1_f.BTF);
    
    /* 发送 STOP 条件 */
    i2c_reg->CR1_f.STOP = 1;
    
    dev->busy = false;
    
    return (int32_t)length;
}

int32_t xy_hal_i2c_master_receive(xy_hal_i2c_t i2c, uint16_t dev_addr,
                                  uint8_t *data, size_t length,
                                  uint32_t timeout)
{
    if (!i2c || !data || length == 0) {
        return -1;
    }
    
    hc32_i2c_dev_t *dev = (hc32_i2c_dev_t *)i2c;
    
    if (dev->busy) {
        return -XY_HAL_ERROR_BUSY;
    }
    
    dev->busy = true;
    
    /* 实现 I2C 主模式接收 */
    M0P_I2C_TypeDef *i2c_reg;
    switch (dev->instance) {
        case 0: i2c_reg = M0P_I2C0; break;
        case 1: i2c_reg = M0P_I2C1; break;
        default: 
            dev->busy = false;
            return -XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 发送 START 条件 */
    i2c_reg->CR1_f.START = 1;
    
    /* 等待 SB 标志 */
    while (!i2c_reg->SR1_f.SB);
    
    /* 发送设备地址 (读) */
    i2c_reg->DR = ((dev_addr << 1) & 0xFE) | 0x01;
    
    /* 等待 ADDR 标志 */
    while (!i2c_reg->SR1_f.ADDR);
    
    if (length == 1) {
        /* 单字节接收：发送 NACK */
        i2c_reg->CR1_f.ACK = 0;
        
        /* 清除 ADDR 标志 */
        volatile uint8_t dummy = i2c_reg->SR2;
        (void)dummy;
        
        /* 发送 STOP 条件 */
        i2c_reg->CR1_f.STOP = 1;
        
        /* 等待 RXNE 标志 */
        while (!i2c_reg->SR1_f.RXNE);
        
        /* 读取数据 */
        data[0] = i2c_reg->DR;
    } else {
        /* 多字节接收 */
        /* 清除 ADDR 标志 */
        volatile uint8_t dummy = i2c_reg->SR2;
        (void)dummy;
        
        for (size_t i = 0; i < length; i++) {
            if (i == length - 2) {
                /* 倒数第二个字节：准备发送 NACK */
                i2c_reg->CR1_f.ACK = 0;
            } else if (i == length - 1) {
                /* 最后一个字节：发送 STOP */
                i2c_reg->CR1_f.STOP = 1;
            }
            
            /* 等待 RXNE 标志 */
            while (!i2c_reg->SR1_f.RXNE);
            
            /* 读取数据 */
            data[i] = i2c_reg->DR;
        }
    }
    
    dev->busy = false;
    
    return (int32_t)length;
}

/* ==================== End of File ==================== */
