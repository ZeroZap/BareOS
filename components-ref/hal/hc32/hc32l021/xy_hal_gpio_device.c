/**
 * @file xy_hal_gpio_device.c
 * @brief HC32L021 GPIO Device Implementation - Unified HAL API
 * @version 1.0.0
 * @date 2026-03-16
 * 
 * @note 实现统一的 GPIO 设备 API，基于 HC32L021 寄存器
 */

#include "../inc/xy_hal_gpio_dev.h"
#include "../inc/xy_hal_gpio_types.h"
#include "../inc/xy_hal_error.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* HC32L021 最小化头文件 (无 board 依赖) */
#include "hc32l021_minimal.h"

/* ==================== Private Definitions ==================== */

#define HC32L021_GPIO_PORT_COUNT  (4)  /* GPIOA/B/C/H */

/* GPIO 端口基地址数组 */
static M0P_GPIO_TypeDef *const gpio_port_base[HC32L021_GPIO_PORT_COUNT] = {
    M0P_GPIO_PA, M0P_GPIO_PB, M0P_GPIO_PC, M0P_GPIO_PH
};

/* ==================== Private Types ==================== */

/**
 * @brief HC32L021 GPIO 设备私有数据
 */
typedef struct {
    M0P_GPIO_TypeDef *port;      /* GPIO 端口基地址 */
    uint8_t port_index;          /* 端口索引 (0-3) */
    uint8_t initialized_pins;    /* 已初始化引脚掩码 */
    bool initialized;            /* 设备初始化标志 */
} hc32_gpio_dev_t;

/* ==================== Private Variables ==================== */

/* GPIO 设备实例池 */
static hc32_gpio_dev_t gpio_devices[HC32L021_GPIO_PORT_COUNT];
static bool gpio_devices_initialized = false;

/* ==================== Private Functions ==================== */

/**
 * @brief 初始化 GPIO 设备池
 */
static void gpio_init_devices(void)
{
    if (!gpio_devices_initialized) {
        memset(gpio_devices, 0, sizeof(gpio_devices));
        gpio_devices_initialized = true;
    }
}

/**
 * @brief 查找或分配 GPIO 设备
 * @param port_index 端口索引
 * @return hc32_gpio_dev_t* GPIO 设备指针
 */
static hc32_gpio_dev_t *gpio_find_or_alloc(uint8_t port_index)
{
    gpio_init_devices();
    
    if (port_index >= HC32L021_GPIO_PORT_COUNT) {
        return NULL;
    }
    
    hc32_gpio_dev_t *dev = &gpio_devices[port_index];
    
    if (!dev->initialized) {
        dev->port = gpio_port_base[port_index];
        dev->port_index = port_index;
        dev->initialized_pins = 0;
        dev->initialized = true;
    }
    
    return dev;
}

/**
 * @brief 解析 GPIO 名称
 * @param name GPIO 名称 (如 "GPIOA", "PA", "GPIOA.5")
 * @param port_index 输出：端口索引
 * @param pin 输出：引脚号
 * @return 0 成功，-1 失败
 */
static int parse_gpio_name(const char *name, uint8_t *port_index, uint8_t *pin)
{
    if (!name || !port_index || !pin) {
        return -1;
    }
    
    /* 支持格式：GPIOA, PA, GPIOA.5, PA.5 */
    if (strncmp(name, "GPIO", 4) == 0) {
        char port_char = name[4];
        if (port_char >= 'A' && port_char <= 'H') {
            *port_index = port_char - 'A';
            if (name[5] == '.' && name[6] >= '0' && name[6] <= '7') {
                *pin = name[6] - '0';
            } else {
                *pin = 0;
            }
            return 0;
        }
    } else if (name[0] == 'P' && (name[1] >= 'A' && name[1] <= 'H')) {
        *port_index = name[1] - 'A';
        if (name[2] == '.' && name[3] >= '0' && name[3] <= '7') {
            *pin = name[3] - '0';
        } else {
            *pin = 0;
        }
        return 0;
    }
    
    return -1;
}

/* ==================== API Implementation ==================== */

xy_hal_gpio_t xy_hal_gpio_bind(const char *name)
{
    uint8_t port_index, pin;
    
    if (parse_gpio_name(name, &port_index, &pin) != 0) {
        return NULL;
    }
    
    if (port_index >= HC32L021_GPIO_PORT_COUNT) {
        return NULL;
    }
    
    hc32_gpio_dev_t *dev = gpio_find_or_alloc(port_index);
    if (!dev) {
        return NULL;
    }
    
    return (xy_hal_gpio_t)dev;
}

xy_hal_error_t xy_hal_gpio_unbind(xy_hal_gpio_t gpio)
{
    if (!gpio) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_gpio_dev_t *dev = (hc32_gpio_dev_t *)gpio;
    dev->initialized_pins = 0;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_configure(xy_hal_gpio_t gpio, xy_hal_gpio_pin_t pin,
                                     const xy_hal_gpio_config_t *config)
{
    if (!gpio || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_gpio_dev_t *dev = (hc32_gpio_dev_t *)gpio;
    
    /* 配置 GPIO 使用最小化 API */
    stc_gpio_cfg_t gpio_cfg;
    memset(&gpio_cfg, 0, sizeof(gpio_cfg));
    
    /* 设置功能 */
    gpio_cfg.enFunc = (config->mode == XY_HAL_GPIO_MODE_OUTPUT) ? GpioFuncPortOut : GpioFuncPortIn;
    
    /* 设置上下拉 */
    gpio_cfg.enPu = (config->pull == XY_HAL_GPIO_PULL_UP) ? GpioPuEnable : GpioPuDisable;
    gpio_cfg.enPd = (config->pull == XY_HAL_GPIO_PULL_DOWN) ? GpioPdEnable : GpioPdDisable;
    
    /* 设置驱动能力 */
    gpio_cfg.enDrv = (config->speed >= XY_HAL_GPIO_SPEED_HIGH) ? GpioDrvH : GpioDrvL;
    
    /* 应用配置 */
    GPIO_Init(dev->port, pin, &gpio_cfg);
    
    /* 标记引脚已初始化 */
    dev->initialized_pins |= pin;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_t gpio, xy_hal_gpio_pin_t pin, uint8_t value)
{
    if (!gpio) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_gpio_dev_t *dev = (hc32_gpio_dev_t *)gpio;
    GPIO_WriteBit(dev->port, pin, value ? 1 : 0);
    
    return XY_HAL_OK;
}

int32_t xy_hal_gpio_read(xy_hal_gpio_t gpio, xy_hal_gpio_pin_t pin)
{
    if (!gpio) {
        return -1;
    }
    
    hc32_gpio_dev_t *dev = (hc32_gpio_dev_t *)gpio;
    return GPIO_ReadInputDataBit(dev->port, pin);
}

xy_hal_error_t xy_hal_gpio_toggle(xy_hal_gpio_t gpio, xy_hal_gpio_pin_t pin)
{
    if (!gpio) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_gpio_dev_t *dev = (hc32_gpio_dev_t *)gpio;
    GPIO_ToggleBits(dev->port, pin);
    
    return XY_HAL_OK;
}

/* ==================== End of File ==================== */
