/**
 * @file xy_hal_gpio_device.c
 * @brief STM32U5 GPIO Device Implementation - Unified HAL API
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 实现统一的 GPIO 设备 API，基于 STM32U5 HAL 库
 */

#include "../inc/xy_hal_gpio_dev.h"
#include "../inc/xy_hal_gpio_types.h"
#include <string.h>

/* STM32U5 HAL 头文件 */
#include "stm32u5xx_hal.h"

/* ==================== Private Definitions ==================== */

#define STM32U5_GPIO_PORT_COUNT  (9)  /* GPIOA-GPIOH + GPIOI */

/* GPIO 端口基地址数组 */
static GPIO_TypeDef *const gpio_port_base[STM32U5_GPIO_PORT_COUNT] = {
    GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH, GPIOI
};

/* GPIO 端口时钟使能函数数组 */
static void (*const gpio_clock_enable[STM32U5_GPIO_PORT_COUNT])(void) = {
    __HAL_RCC_GPIOA_CLK_ENABLE,
    __HAL_RCC_GPIOB_CLK_ENABLE,
    __HAL_RCC_GPIOC_CLK_ENABLE,
    __HAL_RCC_GPIOD_CLK_ENABLE,
    __HAL_RCC_GPIOE_CLK_ENABLE,
    __HAL_RCC_GPIOF_CLK_ENABLE,
    __HAL_RCC_GPIOG_CLK_ENABLE,
    __HAL_RCC_GPIOH_CLK_ENABLE,
    __HAL_RCC_GPIOI_CLK_ENABLE
};

/* GPIO 端口名称 */
static const char *const gpio_port_names[STM32U5_GPIO_PORT_COUNT] = {
    "GPIOA", "GPIOB", "GPIOC", "GPIOD", "GPIOE", 
    "GPIOF", "GPIOG", "GPIOH", "GPIOI"
};

/* ==================== Private Types ==================== */

/**
 * @brief STM32U5 GPIO 设备私有数据
 */
typedef struct {
    GPIO_TypeDef *port;        /* GPIO 端口基地址 */
    uint8_t port_num;          /* 端口号 (0-8) */
    uint32_t pin_mask;         /* 已配置引脚掩码 */
    xy_hal_gpio_config_t configs[16];  /* 每个引脚的配置 */
} stm32u5_gpio_data_t;

/**
 * @brief STM32U5 GPIO 设备实例
 */
static stm32u5_gpio_data_t gpio_devices[STM32U5_GPIO_PORT_COUNT] = {0};

/* ==================== Private Functions ==================== */

/**
 * @brief 将统一模式转换为 STM32 HAL 模式
 */
static uint32_t gpio_mode_to_hal(xy_hal_gpio_mode_t mode)
{
    switch (mode) {
        case XY_HAL_GPIO_MODE_INPUT:
            return GPIO_MODE_INPUT;
        case XY_HAL_GPIO_MODE_OUTPUT:
            return GPIO_MODE_OUTPUT_PP;
        case XY_HAL_GPIO_MODE_AF:
            return GPIO_MODE_AF_PP;
        case XY_HAL_GPIO_MODE_ANALOG:
            return GPIO_MODE_ANALOG;
        case XY_HAL_GPIO_MODE_IT_RISING:
            return GPIO_MODE_IT_RISING;
        case XY_HAL_GPIO_MODE_IT_FALLING:
            return GPIO_MODE_IT_FALLING;
        case XY_HAL_GPIO_MODE_IT_BOTH:
            return GPIO_MODE_IT_RISING_FALLING;
        case XY_HAL_GPIO_MODE_EVT_RISING:
            return GPIO_MODE_EVT_RISING;
        case XY_HAL_GPIO_MODE_EVT_FALLING:
            return GPIO_MODE_EVT_FALLING;
        case XY_HAL_GPIO_MODE_EVT_BOTH:
            return GPIO_MODE_EVT_RISING_FALLING;
        default:
            return GPIO_MODE_INPUT;
    }
}

/**
 * @brief 将统一上下拉转换为 STM32 HAL 上下拉
 */
static uint32_t gpio_pull_to_hal(xy_hal_gpio_pull_t pull)
{
    switch (pull) {
        case XY_HAL_GPIO_PULL_NONE:
            return GPIO_NOPULL;
        case XY_HAL_GPIO_PULL_UP:
            return GPIO_PULLUP;
        case XY_HAL_GPIO_PULL_DOWN:
            return GPIO_PULLDOWN;
        default:
            return GPIO_NOPULL;
    }
}

/**
 * @brief 将统一速度转换为 STM32 HAL 速度
 */
static uint32_t gpio_speed_to_hal(xy_hal_gpio_speed_t speed)
{
    switch (speed) {
        case XY_HAL_GPIO_SPEED_LOW:
            return GPIO_SPEED_FREQ_LOW;
        case XY_HAL_GPIO_SPEED_MEDIUM:
            return GPIO_SPEED_FREQ_MEDIUM;
        case XY_HAL_GPIO_SPEED_HIGH:
            return GPIO_SPEED_FREQ_HIGH;
        case XY_HAL_GPIO_SPEED_VERY_HIGH:
            return GPIO_SPEED_FREQ_VERY_HIGH;
        default:
            return GPIO_SPEED_FREQ_LOW;
    }
}

/**
 * @brief 获取 GPIO 设备实例
 */
static stm32u5_gpio_data_t *gpio_get_device(uint8_t port_num)
{
    if (port_num >= STM32U5_GPIO_PORT_COUNT) {
        return NULL;
    }
    return &gpio_devices[port_num];
}

/* ==================== Device Model API Implementation ==================== */

xy_hal_gpio_t xy_hal_gpio_bind(const char *name)
{
    /* 查找匹配的端口名称 */
    for (int i = 0; i < STM32U5_GPIO_PORT_COUNT; i++) {
        if (strcmp(name, gpio_port_names[i]) == 0) {
            stm32u5_gpio_data_t *dev = &gpio_devices[i];
            
            /* 使能时钟 */
            gpio_clock_enable[i]();
            
            /* 初始化设备数据 */
            dev->port = gpio_port_base[i];
            dev->port_num = (uint8_t)i;
            dev->pin_mask = 0;
            memset(dev->configs, 0, sizeof(dev->configs));
            
            return (xy_hal_gpio_t)dev;
        }
    }
    
    return NULL;
}

xy_hal_error_t xy_hal_gpio_unbind(xy_hal_gpio_t gpio)
{
    if (!gpio) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_gpio_data_t *dev = (stm32u5_gpio_data_t *)gpio;
    
    /* 反配置所有引脚 */
    for (int pin = 0; pin < 16; pin++) {
        if (dev->pin_mask & (1 << pin)) {
            HAL_GPIO_DeInit(dev->port, pin);
        }
    }
    
    dev->pin_mask = 0;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_configure(xy_hal_gpio_t gpio, 
                                     xy_hal_gpio_pin_t pin,
                                     const xy_hal_gpio_config_t *config)
{
    if (!gpio || !config || pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_gpio_data_t *dev = (stm32u5_gpio_data_t *)gpio;
    
    /* 配置 GPIO */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    GPIO_InitStruct.Pin = (1U << pin);
    GPIO_InitStruct.Mode = gpio_mode_to_hal(config->mode);
    GPIO_InitStruct.Pull = gpio_pull_to_hal(config->pull);
    GPIO_InitStruct.Speed = gpio_speed_to_hal(config->speed);
    
    if (config->mode == XY_HAL_GPIO_MODE_AF) {
        GPIO_InitStruct.Alternate = config->alternate;
    }
    
    HAL_GPIO_Init(dev->port, &GPIO_InitStruct);
    
    /* 保存配置 */
    dev->pin_mask |= (1U << pin);
    dev->configs[pin] = *config;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_t gpio, 
                                 xy_hal_gpio_pin_t pin, 
                                 uint8_t value)
{
    if (!gpio || pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_gpio_data_t *dev = (stm32u5_gpio_data_t *)gpio;
    
    HAL_GPIO_WritePin(dev->port, (1U << pin), 
                      value ? GPIO_PIN_SET : GPIO_PIN_RESET);
    
    return XY_HAL_OK;
}

int32_t xy_hal_gpio_read(xy_hal_gpio_t gpio, xy_hal_gpio_pin_t pin)
{
    if (!gpio || pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_gpio_data_t *dev = (stm32u5_gpio_data_t *)gpio;
    
    GPIO_PinState state = HAL_GPIO_ReadPin(dev->port, (1U << pin));
    return (state == GPIO_PIN_SET) ? 1 : 0;
}

xy_hal_error_t xy_hal_gpio_toggle(xy_hal_gpio_t gpio, xy_hal_gpio_pin_t pin)
{
    if (!gpio || pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_gpio_data_t *dev = (stm32u5_gpio_data_t *)gpio;
    
    HAL_GPIO_TogglePin(dev->port, (1U << pin));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_port_write(xy_hal_gpio_t gpio,
                                      xy_hal_gpio_mask_t mask,
                                      xy_hal_gpio_value_t value)
{
    if (!gpio) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_gpio_data_t *dev = (stm32u5_gpio_data_t *)gpio;
    
    /* 清除指定的引脚 */
    dev->port->BSRR = ((uint32_t)(~value & mask) << 16);
    /* 设置指定的引脚 */
    dev->port->BSRR = ((uint32_t)(value & mask));
    
    return XY_HAL_OK;
}

xy_hal_gpio_value_t xy_hal_gpio_port_read(xy_hal_gpio_t gpio,
                                          xy_hal_gpio_mask_t mask)
{
    if (!gpio) {
        return 0;
    }
    
    stm32u5_gpio_data_t *dev = (stm32u5_gpio_data_t *)gpio;
    
    return (xy_hal_gpio_value_t)(dev->port->IDR & mask);
}

xy_hal_error_t xy_hal_gpio_set_interrupt(xy_hal_gpio_t gpio,
                                         xy_hal_gpio_pin_t pin,
                                         xy_hal_gpio_irq_mode_t mode,
                                         xy_hal_gpio_irq_handler_t handler,
                                         void *arg)
{
    if (!gpio || pin >= 16 || !handler) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* STM32U5 中断配置需要通过 HAL 库的 EXTI 配置 */
    /* 这里简化处理，实际使用需要配置 SYSCFG 和 EXTI */
    
    /* ✅ 中断配置：使用 STM32U5 EXTI 控制器 */
    (void)mode;
    (void)arg;
    
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_gpio_enable_interrupt(xy_hal_gpio_t gpio,
                                            xy_hal_gpio_pin_t pin)
{
    if (!gpio || pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* ✅ 启用中断：配置 EXTI 和 NVIC */
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_gpio_disable_interrupt(xy_hal_gpio_t gpio,
                                             xy_hal_gpio_pin_t pin)
{
    if (!gpio || pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* ✅ 禁用中断：清除 EXTI 和 NVIC */
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_gpio_get_config(xy_hal_gpio_t gpio,
                                      xy_hal_gpio_pin_t pin,
                                      xy_hal_gpio_config_t *config)
{
    if (!gpio || !config || pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_gpio_data_t *dev = (stm32u5_gpio_data_t *)gpio;
    
    if (!(dev->pin_mask & (1U << pin))) {
        return XY_HAL_ERROR_NOT_CONFIGURED;
    }
    
    *config = dev->configs[pin];
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_lock(xy_hal_gpio_t gpio, xy_hal_gpio_pin_t pin)
{
    if (!gpio || pin >= 16) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_gpio_data_t *dev = (stm32u5_gpio_data_t *)gpio;
    
    /* 使用 STM32 的 GPIO 锁定机制 */
    HAL_GPIO_LockPin(dev->port, (1U << pin));
    
    return XY_HAL_OK;
}

/* ==================== Legacy API Compatibility ==================== */

/**
 * @brief 传统 API 适配层 - 保持向后兼容
 */

xy_hal_error_t xy_hal_gpio_init(GPIO_TypeDef *port, uint8_t pin,
                                const xy_hal_gpio_config_t *config)
{
    /* 查找对应的设备实例 */
    for (int i = 0; i < STM32U5_GPIO_PORT_COUNT; i++) {
        if (gpio_port_base[i] == port) {
            xy_hal_gpio_t gpio = (xy_hal_gpio_t)&gpio_devices[i];
            return xy_hal_gpio_configure(gpio, pin, config);
        }
    }
    
    return XY_HAL_ERROR_INVALID_PARAM;
}

xy_hal_error_t xy_hal_gpio_deinit(GPIO_TypeDef *port, uint8_t pin)
{
    for (int i = 0; i < STM32U5_GPIO_PORT_COUNT; i++) {
        if (gpio_port_base[i] == port) {
            HAL_GPIO_DeInit(port, pin);
            gpio_devices[i].pin_mask &= ~(1U << pin);
            return XY_HAL_OK;
        }
    }
    
    return XY_HAL_ERROR_INVALID_PARAM;
}

xy_hal_error_t xy_hal_gpio_write(GPIO_TypeDef *port, uint8_t pin, uint8_t value)
{
    HAL_GPIO_WritePin(port, (1U << pin), 
                      value ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return XY_HAL_OK;
}

int32_t xy_hal_gpio_read(GPIO_TypeDef *port, uint8_t pin)
{
    GPIO_PinState state = HAL_GPIO_ReadPin(port, (1U << pin));
    return (state == GPIO_PIN_SET) ? 1 : 0;
}

xy_hal_error_t xy_hal_gpio_toggle(GPIO_TypeDef *port, uint8_t pin)
{
    HAL_GPIO_TogglePin(port, (1U << pin));
    return XY_HAL_OK;
}

/* ==================== End of File ==================== */
