/**
 * @file xy_gpio.h
 * @brief XinYi GPIO Device Driver
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_GPIO_H
#define XY_GPIO_H

#include "xy_device.h"
#include "xy_hal_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GPIO 事件类型
 */
typedef enum {
    XY_GPIO_EVT_RISING = 0,          /**< 上升沿事件 */
    XY_GPIO_EVT_FALLING,             /**< 下降沿事件 */
    XY_GPIO_EVT_BOTH,                /**< 双边沿事件 */
    XY_GPIO_EVT_ERROR,               /**< 错误事件 */
} xy_gpio_evt_t;

/**
 * @brief GPIO 回调类型
 */
typedef void (*xy_gpio_callback_t)(void *dev, uint8_t pin, xy_gpio_evt_t evt, void *arg);

/**
 * @brief GPIO 中断模式
 */
typedef enum {
    XY_GPIO_IRQ_MODE_RISING = 0,     /**< 上升沿中断 */
    XY_GPIO_IRQ_MODE_FALLING,        /**< 下降沿中断 */
    XY_GPIO_IRQ_MODE_BOTH,           /**< 双边沿中断 */
} xy_gpio_irq_mode_t;

/**
 * @brief GPIO 中断回调
 */
typedef void (*xy_gpio_irq_handler_t)(void *arg);

/**
 * @brief GPIO 设备控制命令
 */
typedef enum {
    XY_GPIO_CMD_SET_CONFIG = 0,      /**< 设置配置 */
    XY_GPIO_CMD_GET_CONFIG,          /**< 获取配置 */
    XY_GPIO_CMD_SET_MODE,            /**< 设置模式 */
    XY_GPIO_CMD_GET_MODE,            /**< 获取模式 */
    XY_GPIO_CMD_SET_PULL,            /**< 设置上下拉 */
    XY_GPIO_CMD_GET_PULL,            /**< 获取上下拉 */
    XY_GPIO_CMD_SET_SPEED,           /**< 设置速度 */
    XY_GPIO_CMD_GET_SPEED,           /**< 获取速度 */
    XY_GPIO_CMD_ATTACH_IRQ,          /**< 附加中断 */
    XY_GPIO_CMD_DETACH_IRQ,          /**< 分离中断 */
    XY_GPIO_CMD_SET_OUTPUT_TYPE,     /**< 设置输出类型 */
    XY_GPIO_CMD_GET_OUTPUT_TYPE,     /**< 获取输出类型 */
    XY_GPIO_CMD_SET_DRIVE_STRENGTH,  /**< 设置驱动强度 */
    XY_GPIO_CMD_GET_DRIVE_STRENGTH,  /**< 获取驱动强度 */
} xy_gpio_cmd_t;

/**
 * @brief 初始化 GPIO 设备
 * @param dev 设备指针
 * @param config 配置结构
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_gpio_dev_init(void *dev, const xy_gpio_config_t *config);

/**
 * @brief 设置引脚输出值
 * @param dev 设备指针
 * @param pin 引脚号
 * @param value 输出值 (0=低电平, 1=高电平)
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_gpio_dev_write(void *dev, uint8_t pin, uint8_t value);

/**
 * @brief 读取引脚输入值
 * @param dev 设备指针
 * @param pin 引脚号
 * @return 引脚值 (0=低电平, 1=高电平)，负值表示错误
 */
int32_t xy_gpio_dev_read(void *dev, uint8_t pin);

/**
 * @brief 翻转引脚输出值
 * @param dev 设备指针
 * @param pin 引脚号
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_gpio_dev_toggle(void *dev, uint8_t pin);

/**
 * @brief 批量设置引脚输出值
 * @param dev 设备指针
 * @param pin_mask 引脚掩码
 * @param value_mask 值掩码
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_gpio_dev_write_batch(void *dev, uint16_t pin_mask, uint16_t value_mask);

/**
 * @brief 批量读取引脚输入值
 * @param dev 设备指针
 * @param pin_mask 引脚掩码
 * @return 引脚值掩码，负值表示错误
 */
int32_t xy_gpio_dev_read_batch(void *dev, uint16_t pin_mask);

/**
 * @brief 附加 GPIO 中断
 * @param dev 设备指针
 * @param pin 引脚号
 * @param mode 中断模式
 * @param handler 中断处理函数
 * @param arg 用户参数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_gpio_dev_attach_irq(void *dev, uint8_t pin, 
                                 xy_gpio_irq_mode_t mode,
                                 xy_gpio_irq_handler_t handler, 
                                 void *arg);

/**
 * @brief 分离 GPIO 中断
 * @param dev 设备指针
 * @param pin 引脚号
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_gpio_dev_detach_irq(void *dev, uint8_t pin);

/**
 * @brief 使能 GPIO 中断
 * @param dev 设备指针
 * @param pin 引脚号
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_gpio_dev_irq_enable(void *dev, uint8_t pin);

/**
 * @brief 禁用 GPIO 中断
 * @param dev 设备指针
 * @param pin 引脚号
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_gpio_dev_irq_disable(void *dev, uint8_t pin);

/**
 * @brief 设置 GPIO 模式
 * @param dev 设备指针
 * @param pin 引脚号
 * @param mode GPIO 模式
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_gpio_dev_set_mode(void *dev, uint8_t pin, xy_gpio_mode_t mode);

/**
 * @brief 获取 GPIO 模式
 * @param dev 设备指针
 * @param pin 引脚号
 * @return GPIO 模式，负值表示错误
 */
int32_t xy_gpio_dev_get_mode(void *dev, uint8_t pin);

/**
 * @brief 设置 GPIO 上下拉
 * @param dev 设备指针
 * @param pin 引脚号
 * @param pull 上下拉模式
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_gpio_dev_set_pull(void *dev, uint8_t pin, xy_gpio_pull_t pull);

/**
 * @brief 获取 GPIO 上下拉
 * @param dev 设备指针
 * @param pin 引脚号
 * @return GPIO 上下拉模式，负值表示错误
 */
int32_t xy_gpio_dev_get_pull(void *dev, uint8_t pin);

/**
 * @brief 设置 GPIO 速度
 * @param dev 设备指针
 * @param pin 引脚号
 * @param speed 速度等级
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_gpio_dev_set_speed(void *dev, uint8_t pin, xy_gpio_speed_t speed);

/**
 * @brief 获取 GPIO 速度
 * @param dev 设备指针
 * @param pin 引脚号
 * @return GPIO 速度等级，负值表示错误
 */
int32_t xy_gpio_dev_get_speed(void *dev, uint8_t pin);

/**
 * @brief 设置 GPIO 输出类型
 * @param dev 设备指针
 * @param pin 引脚号
 * @param otype 输出类型
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_gpio_dev_set_otype(void *dev, uint8_t pin, xy_gpio_otype_t otype);

/**
 * @brief 获取 GPIO 输出类型
 * @param dev 设备指针
 * @param pin 引脚号
 * @return GPIO 输出类型，负值表示错误
 */
int32_t xy_gpio_dev_get_otype(void *dev, uint8_t pin);

/**
 * @brief GPIO 设备控制
 * @param dev 设备指针
 * @param cmd 控制命令
 * @param args 命令参数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_gpio_dev_control(void *dev, uint32_t cmd, void *args);

/**
 * @brief 注册 GPIO 回调
 * @param dev 设备指针
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_gpio_dev_register_callback(void *dev, 
                                        xy_gpio_callback_t callback, 
                                        void *arg);

#ifdef __cplusplus
}
#endif

#endif /* XY_GPIO_H */
