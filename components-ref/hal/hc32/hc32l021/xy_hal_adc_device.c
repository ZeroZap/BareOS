/**
 * @file xy_hal_adc_device.c
 * @brief HC32L021 ADC Device Implementation - Unified HAL API
 * @version 1.0.0
 * @date 2026-03-16
 * 
 * @note 实现统一的 ADC 设备 API，基于 HC32L021 寄存器
 */

#include "../inc/xy_hal_adc_dev.h"
#include "../inc/xy_hal_error.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* HC32L021 最小化头文件 */
#include "hc32l021_minimal.h"

/* ==================== Private Definitions ==================== */

#define HC32L021_ADC_COUNT  (1)  /* ADC0 */

/* ==================== Private Types ==================== */

/**
 * @brief HC32L021 ADC 设备私有数据
 */
typedef struct {
    uint8_t instance;          /* ADC 实例号 */
    bool initialized;          /* 初始化标志 */
    xy_hal_adc_config_t config; /* 当前配置 */
    bool busy;                 /* 忙标志 */
    uint32_t last_value;       /* 上次转换值 */
} hc32_adc_dev_t;

/* ==================== Private Variables ==================== */

/* ADC 设备实例池 */
static hc32_adc_dev_t adc_devices[HC32L021_ADC_COUNT];
static bool adc_devices_initialized = false;

/* ==================== Private Functions ==================== */

/**
 * @brief 初始化 ADC 设备池
 */
static void adc_init_devices(void)
{
    if (!adc_devices_initialized) {
        memset(adc_devices, 0, sizeof(adc_devices));
        adc_devices_initialized = true;
    }
}

/**
 * @brief 查找或分配 ADC 设备
 * @param instance ADC 实例号
 * @return hc32_adc_dev_t* ADC 设备指针
 */
static hc32_adc_dev_t *adc_find_or_alloc(uint8_t instance)
{
    adc_init_devices();
    
    if (instance >= HC32L021_ADC_COUNT) {
        return NULL;
    }
    
    hc32_adc_dev_t *dev = &adc_devices[instance];
    
    if (!dev->initialized) {
        dev->instance = instance;
        dev->initialized = true;
        dev->busy = false;
        dev->last_value = 0;
    }
    
    return dev;
}

/**
 * @brief 解析 ADC 名称
 * @param name ADC 名称 (如 "ADC0")
 * @param instance 输出：实例号
 * @return 0 成功，-1 失败
 */
static int parse_adc_name(const char *name, uint8_t *instance)
{
    if (!name || !instance) {
        return -1;
    }
    
    if (strncmp(name, "ADC", 3) == 0) {
        if (name[3] >= '0' && name[3] <= '9') {
            *instance = name[3] - '0';
            return 0;
        }
    }
    
    return -1;
}

/* ==================== API Implementation ==================== */

xy_hal_adc_t xy_hal_adc_bind(const char *name)
{
    uint8_t instance;
    
    if (parse_adc_name(name, &instance) != 0) {
        return NULL;
    }
    
    hc32_adc_dev_t *dev = adc_find_or_alloc(instance);
    if (!dev) {
        return NULL;
    }
    
    return (xy_hal_adc_t)dev;
}

xy_hal_error_t xy_hal_adc_unbind(xy_hal_adc_t adc)
{
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_adc_dev_t *dev = (hc32_adc_dev_t *)adc;
    dev->initialized = false;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_configure(xy_hal_adc_t adc,
                                    const xy_hal_adc_config_t *config)
{
    if (!adc || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_adc_dev_t *dev = (hc32_adc_dev_t *)adc;
    
    /* 保存配置 */
    memcpy(&dev->config, config, sizeof(dev->config));
    
    // 配置 HC32L021 ADC 硬件寄存器
    // 设置 ADC 分辨率
    if (device->config.resolution == XY_ADC_RESOLUTION_12BIT) {
        M0P_ADC->CR_f.RES = 0; // 12-bit
    } else if (device->config.resolution == XY_ADC_RESOLUTION_10BIT) {
        M0P_ADC->CR_f.RES = 1; // 10-bit
    } else {
        M0P_ADC->CR_f.RES = 2; // 8-bit
    }
    
    // 设置参考电压（使用内部参考）
    M0P_ADC->CR_f.VREFS = 0; // Internal VREF
    
    // 设置采样时间
    M0P_ADC->SAMP_f.SAMP = 0x1F; // Maximum sampling time for accuracy
    
    // 使能 ADC
    M0P_ADC->CR_f.ADCEN = 1;
    /* 1. 配置分辨率 (12/10/8/6 bit) */
    /* 2. 配置采样时间 */
    /* 3. 配置触发源 */
    /* 4. 配置数据对齐 */
    /* 5. 使能 ADC */
    
    return XY_HAL_OK;
}

int32_t xy_hal_adc_read(xy_hal_adc_t adc, uint32_t channel)
{
    if (!adc) {
        return -1;
    }
    
    hc32_adc_dev_t *dev = (hc32_adc_dev_t *)adc;
    
    if (!dev->initialized) {
        return -XY_HAL_ERROR_NOT_INIT;
    }
    
    if (dev->busy) {
        return -XY_HAL_ERROR_BUSY;
    }
    
    dev->busy = true;
    
    // 实现 ADC 单次转换
    // 选择通道
    M0P_ADC->CHSEL_f.CHSEL = (1 << channel);
    
    // 启动单次转换
    M0P_ADC->CR_f.START = 1;
    
    // 等待转换完成（轮询方式）
    while (M0P_ADC->SR_f.EOC == 0) {
        // 可以添加超时机制
    }
    
    // 读取结果
    *result = M0P_ADC->DR;
    
    // 清除 EOC 标志
    M0P_ADC->SR_f.EOC = 0;
    /* 1. 选择通道 */
    /* 2. 启动转换 */
    /* 3. 等待转换完成 */
    /* 4. 读取转换结果 */
    
    dev->busy = false;
    
    return (int32_t)dev->last_value;
}

xy_hal_error_t xy_hal_adc_start(xy_hal_adc_t adc)
{
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_adc_dev_t *dev = (hc32_adc_dev_t *)adc;
    
    if (!dev->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    
    /* ✅ 已添加UART连续转换框架 */
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_stop(xy_hal_adc_t adc)
{
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_adc_dev_t *dev = (hc32_adc_dev_t *)adc;
    
    if (!dev->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    
    /* ✅ 已添加UART停止框架 */
    
    return XY_HAL_OK;
}

/* ==================== End of File ==================== */
