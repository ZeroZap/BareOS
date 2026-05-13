/**
 * @file xy_hal_pwm_device.c
 * @brief HC32L021 PWM Device Implementation - Unified HAL API
 * @version 1.0.0
 * @date 2026-03-16
 * 
 * @note 实现统一的 PWM 设备 API，基于 HC32L021 寄存器
 */

#include "../inc/xy_hal_pwm_dev.h"
#include "../inc/xy_hal_error.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* HC32L021 最小化头文件 */
#include "hc32l021_minimal.h"

/* ==================== Private Definitions ==================== */

#define HC32L021_PWM_COUNT  (4)  /* PWM0-PWM3 */

/* ==================== Private Types ==================== */

/**
 * @brief HC32L021 PWM 设备私有数据
 */
typedef struct {
    uint8_t instance;          /* PWM 实例号 (0-3) */
    bool initialized;          /* 初始化标志 */
    xy_hal_pwm_config_t config; /* 当前配置 */
    bool running;              /* 运行标志 */
    uint32_t period;           /* 周期值 */
    uint32_t duty_cycle;       /* 占空比值 */
} hc32_pwm_dev_t;

/* ==================== Private Variables ==================== */

/* PWM 设备实例池 */
static hc32_pwm_dev_t pwm_devices[HC32L021_PWM_COUNT];
static bool pwm_devices_initialized = false;

/* ==================== Private Functions ==================== */

/**
 * @brief 初始化 PWM 设备池
 */
static void pwm_init_devices(void)
{
    if (!pwm_devices_initialized) {
        memset(pwm_devices, 0, sizeof(pwm_devices));
        pwm_devices_initialized = true;
    }
}

/**
 * @brief 查找或分配 PWM 设备
 * @param instance PWM 实例号
 * @return hc32_pwm_dev_t* PWM 设备指针
 */
static hc32_pwm_dev_t *pwm_find_or_alloc(uint8_t instance)
{
    pwm_init_devices();
    
    if (instance >= HC32L021_PWM_COUNT) {
        return NULL;
    }
    
    hc32_pwm_dev_t *dev = &pwm_devices[instance];
    
    if (!dev->initialized) {
        dev->instance = instance;
        dev->initialized = true;
        dev->running = false;
        dev->period = 0;
        dev->duty_cycle = 0;
    }
    
    return dev;
}

/**
 * @brief 解析 PWM 名称
 * @param name PWM 名称 (如 "PWM0", "PWM1")
 * @param instance 输出：实例号
 * @return 0 成功，-1 失败
 */
static int parse_pwm_name(const char *name, uint8_t *instance)
{
    if (!name || !instance) {
        return -1;
    }
    
    if (strncmp(name, "PWM", 3) == 0) {
        if (name[3] >= '0' && name[3] <= '3') {
            *instance = name[3] - '0';
            return 0;
        }
    }
    
    return -1;
}

/* ==================== API Implementation ==================== */

xy_hal_pwm_t xy_hal_pwm_bind(const char *name)
{
    uint8_t instance;
    
    if (parse_pwm_name(name, &instance) != 0) {
        return NULL;
    }
    
    hc32_pwm_dev_t *dev = pwm_find_or_alloc(instance);
    if (!dev) {
        return NULL;
    }
    
    return (xy_hal_pwm_t)dev;
}

xy_hal_error_t xy_hal_pwm_unbind(xy_hal_pwm_t pwm)
{
    if (!pwm) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_pwm_dev_t *dev = (hc32_pwm_dev_t *)pwm;
    dev->initialized = false;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pwm_configure(xy_hal_pwm_t pwm,
                                    const xy_hal_pwm_config_t *config)
{
    if (!pwm || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_pwm_dev_t *dev = (hc32_pwm_dev_t *)pwm;
    
    /* 保存配置 */
    memcpy(&dev->config, config, sizeof(dev->config));
    
    // 配置 HC32L021 PWM 硬件寄存器
    // 假设使用 Timer6 作为 PWM 源
    M0P_TMR6->TMRx_f.TMR = device->config.period;
    
    // 设置 PWM 模式
    M0P_TMR6->TMRx_f.MODE = 0x02; // PWM mode
    
    // 设置预分频器（根据时钟频率计算）
    M0P_TMR6->TMRx_f.PRS = device->config.prescaler;
    
    // 使能 Timer
    M0P_TMR6->TMRx_f.EN = 1;
    /* 1. 配置周期 */
    /* 2. 配置占空比 */
    /* 3. 配置极性 */
    /* 4. 配置对齐模式 */
    /* 5. 使能 PWM 输出 */
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pwm_start(xy_hal_pwm_t pwm)
{
    if (!pwm) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_pwm_dev_t *dev = (hc32_pwm_dev_t *)pwm;
    
    if (!dev->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    
    // 启动 PWM
    M0P_TMR6->TMRx_f.EN = 1;
    dev->running = true;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pwm_stop(xy_hal_pwm_t pwm)
{
    if (!pwm) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_pwm_dev_t *dev = (hc32_pwm_dev_t *)pwm;
    
    if (!dev->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    
    // 停止 PWM
    M0P_TMR6->TMRx_f.EN = 0;
    dev->running = false;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pwm_set_duty_cycle(xy_hal_pwm_t pwm, uint32_t duty_cycle)
{
    if (!pwm) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_pwm_dev_t *dev = (hc32_pwm_dev_t *)pwm;
    
    if (!dev->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    
    // 设置占空比
    // 占空比 = (period - duty_cycle) / period
    uint32_t compare_value = device->config.period - duty_cycle;
    M0P_TMR6->TMRx_f.CMP = compare_value;
    dev->duty_cycle = duty_cycle;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pwm_set_period(xy_hal_pwm_t pwm, uint32_t period)
{
    if (!pwm) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_pwm_dev_t *dev = (hc32_pwm_dev_t *)pwm;
    
    if (!dev->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }
    
    // 设置周期
    M0P_TMR6->TMRx_f.TMR = period;
    device->config.period = period;
    dev->period = period;
    
    return XY_HAL_OK;
}

/* ==================== End of File ==================== */
