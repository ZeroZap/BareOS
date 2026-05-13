/**
 * @file xy_hal_timer_device.c
 * @brief HC32L021 Timer Device Implementation - Unified HAL API
 * @version 1.0.0
 * @date 2026-03-16
 * 
 * @note 实现统一的 Timer 设备 API，基于 HC32L021 寄存器
 */

#include "../inc/xy_hal_timer_dev.h"
#include "../inc/xy_hal_error.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* HC32L021 最小化头文件 */
#include "hc32l021_minimal.h"

/* ==================== Private Definitions ==================== */

#define HC32L021_TIMER_COUNT  (4)  /* Timer0-Timer3 */

/* ==================== Private Types ==================== */

/**
 * @brief HC32L021 Timer 设备私有数据
 */
typedef struct {
    uint8_t instance;          /* Timer 实例号 (0-3) */
    bool initialized;          /* 初始化标志 */
    xy_hal_timer_config_t config; /* 当前配置 */
    bool running;              /* 运行标志 */
    uint32_t counter;          /* 当前计数值 */
} hc32_timer_dev_t;

/* ==================== Private Variables ==================== */

/* Timer 设备实例池 */
static hc32_timer_dev_t timer_devices[HC32L021_TIMER_COUNT];
static bool timer_devices_initialized = false;

/* ==================== Private Functions ==================== */

/**
 * @brief 初始化 Timer 设备池
 */
static void timer_init_devices(void)
{
    if (!timer_devices_initialized) {
        memset(timer_devices, 0, sizeof(timer_devices));
        timer_devices_initialized = true;
    }
}

/**
 * @brief 查找或分配 Timer 设备
 * @param instance Timer 实例号
 * @return hc32_timer_dev_t* Timer 设备指针
 */
static hc32_timer_dev_t *timer_find_or_alloc(uint8_t instance)
{
    timer_init_devices();
    
    if (instance >= HC32L021_TIMER_COUNT) {
        return NULL;
    }
    
    hc32_timer_dev_t *dev = &timer_devices[instance];
    
    if (!dev->initialized) {
        dev->instance = instance;
        dev->initialized = true;
        dev->running = false;
        dev->counter = 0;
    }
    
    return dev;
}

/**
 * @brief 解析 Timer 名称
 * @param name Timer 名称 (如 "TIM0", "TIM1")
 * @param instance 输出：实例号
 * @return 0 成功，-1 失败
 */
static int parse_timer_name(const char *name, uint8_t *instance)
{
    if (!name || !instance) {
        return -1;
    }
    
    if (strncmp(name, "TIM", 3) == 0 || strncmp(name, "TIMER", 5) == 0) {
        char num_char = (strncmp(name, "TIM", 3) == 0) ? name[3] : name[5];
        if (num_char >= '0' && num_char <= '3') {
            *instance = num_char - '0';
            return 0;
        }
    }
    
    return -1;
}

/* ==================== API Implementation ==================== */

xy_hal_timer_t xy_hal_timer_bind(const char *name)
{
    uint8_t instance;
    
    if (parse_timer_name(name, &instance) != 0) {
        return NULL;
    }
    
    hc32_timer_dev_t *dev = timer_find_or_alloc(instance);
    if (!dev) {
        return NULL;
    }
    
    return (xy_hal_timer_t)dev;
}

xy_hal_error_t xy_hal_timer_unbind(xy_hal_timer_t timer)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_timer_dev_t *dev = (hc32_timer_dev_t *)timer;
    dev->initialized = false;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_configure(xy_hal_timer_t timer,
                                      const xy_hal_timer_config_t *config)
{
    if (!timer || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_timer_dev_t *dev = (hc32_timer_dev_t *)timer;
    
    /* 保存配置 */
    memcpy(&dev->config, config, sizeof(dev->config));
    
    // 配置 HC32L021 Timer 硬件寄存器
    // 使用 Timer0 作为通用定时器
    M0P_TMR0->TMRx_f.TMR = device->config.period;
    
    // 设置工作模式（定时器模式）
    M0P_TMR0->TMRx_f.MODE = 0x00; // Timer mode
    
    // 设置预分频器
    M0P_TMR0->TMRx_f.PRS = device->config.prescaler;
    
    // 使能中断（如果需要）
    if (device->callback) {
        M0P_TMR0->TMRx_f.IE = 1;
    }
    /* 1. 配置预分频器 */
    /* 2. 配置自动重载值 */
    /* 3. 配置计数模式 (向上/向下/中央对齐) */
    /* 4. 配置时钟源 */
    /* 5. 使能 Timer */
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_start(xy_hal_timer_t timer)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_timer_dev_t *dev = (hc32_timer_dev_t *)timer;
    
    if (!dev->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    
    // 启动 Timer
    M0P_TMR0->TMRx_f.EN = 1;
    dev->running = true;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_stop(xy_hal_timer_t timer)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_timer_dev_t *dev = (hc32_timer_dev_t *)timer;
    
    if (!dev->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    
    // 停止 Timer
    M0P_TMR0->TMRx_f.EN = 0;
    dev->running = false;
    
    return XY_HAL_OK;
}

uint32_t xy_hal_timer_get_counter(xy_hal_timer_t timer)
{
    if (!timer) {
        return 0;
    }
    
    hc32_timer_dev_t *dev = (hc32_timer_dev_t *)timer;
    
    if (!dev->initialized) {
        return 0;
    }
    
    // 读取硬件计数器
    *counter = M0P_TMR0->TMRx_f.TMR;
    return dev->counter;
}

xy_hal_error_t xy_hal_timer_set_counter(xy_hal_timer_t timer, uint32_t value)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_timer_dev_t *dev = (hc32_timer_dev_t *)timer;
    
    if (!dev->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    
    // 设置硬件计数器
    M0P_TMR0->TMRx_f.TMR = counter;
    dev->counter = value;
    
    return XY_HAL_OK;
}

/* ==================== End of File ==================== */
