/**
 * @file xy_hal_gpio_device.c
 * @brief HC32L021 GPIO HAL Implementation - Minimal Compile Test
 * @version 1.0.0
 * @date 2026-03-16
 * 
 * @note 最小化编译测试版本 - 直接使用寄存器定义
 */

/* Include XinYi HAL headers */
#include "../../inc/xy_hal_gpio.h"
#include "../../inc/xy_hal_gpio_dev.h"  /* Defines xy_hal_gpio_t */

/* Include HC32 minimal header (no board dependencies) */
#include "hc32l021_minimal.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ==================== Use hc32l021_minimal.h Definitions ==================== */
/* GPIO types and macros are defined in hc32l021_minimal.h */
/* M0P_GPIO_TypeDef, GPIO_Init, GPIO_WriteBit, etc. */

/* ==================== Private Types ==================== */

/**
 * @brief GPIO 私有数据
 */
typedef struct {
    M0P_GPIO_TypeDef *port;    /* GPIO 端口基地址 */
    uint8_t pin;               /* GPIO 引脚 (0-7) */
    bool initialized;          /* 初始化标志 */
} hc32_gpio_data_t;

/* ==================== Private Variables ==================== */

#define MAX_GPIO_INSTANCES 32
static hc32_gpio_data_t gpio_instances[MAX_GPIO_INSTANCES];
static bool gpio_pool_initialized = false;

/* ==================== Private Functions ==================== */

static void gpio_init_pool(void)
{
    if (!gpio_pool_initialized) {
        memset(gpio_instances, 0, sizeof(gpio_instances));
        gpio_pool_initialized = true;
    }
}

static hc32_gpio_data_t *gpio_find_or_alloc(M0P_GPIO_TypeDef *port, uint8_t pin)
{
    gpio_init_pool();
    
    /* 查找已存在的实例 */
    for (int i = 0; i < MAX_GPIO_INSTANCES; i++) {
        if (gpio_instances[i].port == port && 
            gpio_instances[i].pin == pin &&
            gpio_instances[i].initialized) {
            return &gpio_instances[i];
        }
    }
    
    /* 分配新实例 */
    for (int i = 0; i < MAX_GPIO_INSTANCES; i++) {
        if (!gpio_instances[i].initialized) {
            hc32_gpio_data_t *data = &gpio_instances[i];
            data->port = port;
            data->pin = pin;
            data->initialized = true;
            return data;
        }
    }
    
    return NULL;
}

/* ==================== Public Implementation ==================== */

xy_hal_gpio_t xy_hal_gpio_bind(const char *name)
{
    if (!name) {
        return NULL;
    }
    
    M0P_GPIO_TypeDef *port = NULL;
    uint8_t pin_num = 0;
    
    /* 解析引脚名称 (如 "PA0", "GPIOA.0") */
    if (strncmp(name, "PA", 2) == 0 || strncmp(name, "GPIOA.", 6) == 0) {
        port = M0P_GPIO_PA;
        pin_num = (strncmp(name, "PA", 2) == 0) ? (name[2] - '0') : (name[6] - '0');
    } else if (strncmp(name, "PB", 2) == 0 || strncmp(name, "GPIOB.", 6) == 0) {
        port = M0P_GPIO_PB;
        pin_num = (strncmp(name, "PB", 2) == 0) ? (name[2] - '0') : (name[6] - '0');
    } else if (strncmp(name, "PC", 2) == 0 || strncmp(name, "GPIOC.", 6) == 0) {
        port = M0P_GPIO_PC;
        pin_num = (strncmp(name, "PC", 2) == 0) ? (name[2] - '0') : (name[6] - '0');
    } else if (strncmp(name, "PH", 2) == 0 || strncmp(name, "GPIOH.", 6) == 0) {
        port = M0P_GPIO_PH;
        pin_num = (strncmp(name, "PH", 2) == 0) ? (name[2] - '0') : (name[6] - '0');
    }
    
    if (!port || pin_num > 7) {
        return NULL;
    }
    
    hc32_gpio_data_t *data = gpio_find_or_alloc(port, (1 << pin_num));
    if (!data) {
        return NULL;
    }
    
    return (xy_hal_gpio_t)data;
}

xy_hal_error_t xy_hal_gpio_configure(xy_hal_gpio_t gpio, 
                                     xy_hal_gpio_pin_t pin,
                                     const xy_hal_gpio_config_t *config)
{
    if (!gpio || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_gpio_data_t *data = (hc32_gpio_data_t *)gpio;
    
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
    GPIO_Init(data->port, data->pin, &gpio_cfg);
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_t gpio, 
                                 xy_hal_gpio_pin_t pin, 
                                 uint8_t value)
{
    if (!gpio) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_gpio_data_t *data = (hc32_gpio_data_t *)gpio;
    
    GPIO_WriteBit(data->port, data->pin, value ? 1 : 0);
    
    return XY_HAL_OK;
}

int32_t xy_hal_gpio_read(xy_hal_gpio_t gpio, xy_hal_gpio_pin_t pin)
{
    if (!gpio) {
        return -1;
    }
    
    hc32_gpio_data_t *data = (hc32_gpio_data_t *)gpio;
    
    return GPIO_ReadInputDataBit(data->port, data->pin);
}

xy_hal_error_t xy_hal_gpio_toggle(xy_hal_gpio_t gpio, xy_hal_gpio_pin_t pin)
{
    if (!gpio) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    hc32_gpio_data_t *data = (hc32_gpio_data_t *)gpio;
    
    GPIO_ToggleBits(data->port, data->pin);
    
    return XY_HAL_OK;
}
