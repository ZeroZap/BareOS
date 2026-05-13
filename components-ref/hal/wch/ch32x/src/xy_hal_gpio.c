/**
 * @file xy_hal_gpio.c
 * @brief WCH CH32V30x GPIO HAL Implementation
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#include "xy_hal_gpio.h"
#include "xy_log.h"

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

#ifdef MCU_CH32

#include "ch32v30x.h"

/**
 * @brief GPIO 时钟使能
 */
static void xy_hal_gpio_enable_clock(void *port)
{
    if (port == GPIOA) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    } else if (port == GPIOB) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    } else if (port == GPIOC) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    } else if (port == GPIOD) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    } else if (port == GPIOE) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
    }
}

xy_hal_error_t xy_hal_gpio_init(void *port, uint16_t pin, const xy_hal_gpio_config_t *config)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    if (!port || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 使能时钟 */
    xy_hal_gpio_enable_clock(port);
    
    /* 配置 GPIO 参数 */
    GPIO_InitStructure.GPIO_Pin = pin;
    
    switch (config->mode) {
        case XY_HAL_GPIO_MODE_INPUT:
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
            GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
            break;
            
        case XY_HAL_GPIO_MODE_OUTPUT:
            if (config->pull == XY_HAL_GPIO_PULL_UP) {
                GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
            } else {
                GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
            }
            GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
            break;
            
        case XY_HAL_GPIO_MODE_AF:
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
            GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
            break;
            
        case XY_HAL_GPIO_MODE_ANALOG:
            GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
            break;
            
        default:
            return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 初始化 GPIO */
    GPIO_Init(port, &GPIO_InitStructure);
    
    /* 设置上下拉 */
    if (config->mode == XY_HAL_GPIO_MODE_INPUT) {
        if (config->pull == XY_HAL_GPIO_PULL_UP) {
            GPIO_SetBits(port, pin);
        } else if (config->pull == XY_HAL_GPIO_PULL_DOWN) {
            GPIO_ResetBits(port, pin);
        }
    }
    
    xy_log_d("WCH GPIO init: port=%p, pin=%d, mode=%d\n", port, pin, config->mode);
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_deinit(void *port, uint16_t pin)
{
    if (!port) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 复位 GPIO */
    GPIO_DeInit(port);
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_read(void *port, uint16_t pin, xy_hal_gpio_state_t *state)
{
    if (!port || !state) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    *state = GPIO_ReadInputDataBit(port, pin) ? XY_HAL_GPIO_HIGH : XY_HAL_GPIO_LOW;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_write(void *port, uint16_t pin, xy_hal_gpio_state_t state)
{
    if (!port) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    if (state == XY_HAL_GPIO_HIGH) {
        GPIO_SetBits(port, pin);
    } else {
        GPIO_ResetBits(port, pin);
    }
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_toggle(void *port, uint16_t pin)
{
    if (!port) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    GPIO_WriteBit(port, pin, (BitAction)(1 - GPIO_ReadOutputDataBit(port, pin)));
    
    return XY_HAL_OK;
}

#else

xy_hal_error_t xy_hal_gpio_init(void *port, uint16_t pin, const xy_hal_gpio_config_t *config)
{
    (void)port;
    (void)pin;
    (void)config;
    return XY_HAL_ERROR_NOT_SUPPORT;
}

#endif
