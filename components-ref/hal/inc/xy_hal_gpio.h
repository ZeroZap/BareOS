/**
 * @file xy_hal_gpio.h
 * @brief XinYi GPIO Hardware Abstraction Layer
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_HAL_GPIO_H
#define XY_HAL_GPIO_H
#include "xy_hal_error.h"

#include "xy_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GPIO 模式
 */
typedef enum {
    XY_HAL_GPIO_MODE_INPUT = 0,      /**< 输入模式 */
    XY_HAL_GPIO_MODE_OUTPUT,         /**< 输出模式 */
    XY_HAL_GPIO_MODE_AF,             /**< 复用功能 */
    XY_HAL_GPIO_MODE_ANALOG,         /**< 模拟模式 */
    XY_HAL_GPIO_MODE_IT_RISING,      /**< 上升沿中断 */
    XY_HAL_GPIO_MODE_IT_FALLING,     /**< 下降沿中断 */
    XY_HAL_GPIO_MODE_IT_BOTH,        /**< 双边沿中断 */
    XY_HAL_GPIO_MODE_EVT_RISING,     /**< 上升沿事件 */
    XY_HAL_GPIO_MODE_EVT_FALLING,    /**< 下降沿事件 */
    XY_HAL_GPIO_MODE_EVT_BOTH,       /**< 双边沿事件 */
} xy_hal_gpio_mode_t;

/**
 * @brief GPIO 上下拉配置
 */
typedef enum {
    XY_HAL_GPIO_PULL_NONE = 0,       /**< 无上下拉 */
    XY_HAL_GPIO_PULL_UP,             /**< 上拉 */
    XY_HAL_GPIO_PULL_DOWN,           /**< 下拉 */
} xy_hal_gpio_pull_t;

/**
 * @brief GPIO 输出类型
 */
typedef enum {
    XY_HAL_GPIO_OTYPE_PP = 0,        /**< 推挽输出 */
    XY_HAL_GPIO_OTYPE_OD,            /**< 开漏输出 */
} xy_hal_gpio_otype_t;

/**
 * @brief GPIO 速度配置
 */
typedef enum {
    XY_HAL_GPIO_SPEED_LOW = 0,       /**< 低速 */
    XY_HAL_GPIO_SPEED_MEDIUM,        /**< 中速 */
    XY_HAL_GPIO_SPEED_HIGH,          /**< 高速 */
    XY_HAL_GPIO_SPEED_VERY_HIGH,     /**< 超高速 */
} xy_hal_gpio_speed_t;

/**
 * @brief GPIO 中断触发模式
 */
typedef enum {
    XY_HAL_GPIO_IRQ_RISING = 0,      /**< 上升沿触发 */
    XY_HAL_GPIO_IRQ_FALLING,         /**< 下降沿触发 */
    XY_HAL_GPIO_IRQ_BOTH,            /**< 双边沿触发 */
} xy_hal_gpio_irq_mode_t;

/**
 * @brief GPIO 配置结构
 */
typedef struct {
    xy_hal_gpio_mode_t mode;         /**< 模式 */
    xy_hal_gpio_pull_t pull;         /**< 上下拉 */
    xy_hal_gpio_otype_t otype;       /**< 输出类型 */
    xy_hal_gpio_speed_t speed;       /**< 速度 */
    uint8_t alternate;               /**< 复用功能编号 */
} xy_hal_gpio_config_t;

/**
 * @brief GPIO 中断回调函数
 */
typedef void (*xy_hal_gpio_irq_handler_t)(void *arg);

/**
 * @brief GPIO 端口类型
 */
#ifdef HAL_PLATFORM_PC
/* PC platform: forward declare, actual struct in xy_hal_pc.h */
struct xy_hal_gpio_port;
typedef struct xy_hal_gpio_port *xy_hal_gpio_port_t;
#else
/* Embedded platforms: opaque pointer */
typedef void *xy_hal_gpio_port_t;
#endif

/**
 * @brief 初始化 GPIO
 * @param port GPIO 端口 (如 GPIOA, GPIOB)
 * @param pin 引脚号 (0-15)
 * @param config 配置结构
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin,
                                const xy_hal_gpio_config_t *config);

/**
 * @brief 反初始化 GPIO
 * @param port GPIO 端口
 * @param pin 引脚号
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_deinit(xy_hal_gpio_port_t port, uint8_t pin);

/**
 * @brief 设置 GPIO 输出值
 * @param port GPIO 端口
 * @param pin 引脚号
 * @param value 输出值 (0=低电平, 1=高电平)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_port_t port, uint8_t pin, uint8_t value);

/**
 * @brief 读取 GPIO 输入值
 * @param port GPIO 端口
 * @param pin 引脚号
 * @return GPIO 值 (0=低电平, 1=高电平)，负值表示错误
 */
int32_t xy_hal_gpio_read(xy_hal_gpio_port_t port, uint8_t pin);

/**
 * @brief 翻转 GPIO 输出值
 * @param port GPIO 端口
 * @param pin 引脚号
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_toggle(xy_hal_gpio_port_t port, uint8_t pin);

/**
 * @brief 设置 GPIO 模式
 * @param port GPIO 端口
 * @param pin 引脚号
 * @param mode GPIO 模式
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_set_mode(xy_hal_gpio_port_t port, uint8_t pin,
                                   xy_hal_gpio_mode_t mode);

/**
 * @brief 获取 GPIO 模式
 * @param port GPIO 端口
 * @param pin 引脚号
 * @return GPIO 模式，负值表示错误
 */
int32_t xy_hal_gpio_get_mode(xy_hal_gpio_port_t port, uint8_t pin);

/**
 * @brief 设置 GPIO 上下拉
 * @param port GPIO 端口
 * @param pin 引脚号
 * @param pull 上下拉模式
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_set_pull(xy_hal_gpio_port_t port, uint8_t pin,
                                   xy_hal_gpio_pull_t pull);

/**
 * @brief 获取 GPIO 上下拉
 * @param port GPIO 端口
 * @param pin 引脚号
 * @return GPIO 上下拉模式，负值表示错误
 */
int32_t xy_hal_gpio_get_pull(xy_hal_gpio_port_t port, uint8_t pin);

/**
 * @brief 设置 GPIO 输出类型
 * @param port GPIO 端口
 * @param pin 引脚号
 * @param otype 输出类型
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_set_otype(xy_hal_gpio_port_t port, uint8_t pin,
                                   xy_hal_gpio_otype_t otype);

/**
 * @brief 获取 GPIO 输出类型
 * @param port GPIO 端口
 * @param pin 引脚号
 * @return GPIO 输出类型，负值表示错误
 */
int32_t xy_hal_gpio_get_otype(xy_hal_gpio_port_t port, uint8_t pin);

/**
 * @brief 设置 GPIO 速度
 * @param port GPIO 端口
 * @param pin 引脚号
 * @param speed 速度
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_set_speed(xy_hal_gpio_port_t port, uint8_t pin,
                                    xy_hal_gpio_speed_t speed);

/**
 * @brief 获取 GPIO 速度
 * @param port GPIO 端口
 * @param pin 引脚号
 * @return GPIO 速度，负值表示错误
 */
int32_t xy_hal_gpio_get_speed(xy_hal_gpio_port_t port, uint8_t pin);

/**
 * @brief 设置复用功能
 * @param port GPIO 端口
 * @param pin 引脚号
 * @param alternate 复用功能编号
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_set_alternate(xy_hal_gpio_port_t port, uint8_t pin,
                                        uint8_t alternate);

/**
 * @brief 获取复用功能
 * @param port GPIO 端口
 * @param pin 引脚号
 * @return 复用功能编号，负值表示错误
 */
int32_t xy_hal_gpio_get_alternate(xy_hal_gpio_port_t port, uint8_t pin);

/**
 * @brief 批量设置 GPIO
 * @param port GPIO 端口
 * @param pin_mask 引脚掩码
 * @param value_mask 值掩码
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_write_batch(xy_hal_gpio_port_t port, uint16_t pin_mask,
                                      uint16_t value_mask);

/**
 * @brief 批量读取 GPIO
 * @param port GPIO 端口
 * @param pin_mask 引脚掩码
 * @return GPIO 值掩码，负值表示错误
 */
int32_t xy_hal_gpio_read_batch(xy_hal_gpio_port_t port, uint16_t pin_mask);

/**
 * @brief 附加 GPIO 中断
 * @param port GPIO 端口
 * @param pin 引脚号
 * @param mode 中断模式
 * @param handler 中断处理函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_attach_irq(xy_hal_gpio_port_t port, uint8_t pin,
                                     xy_hal_gpio_irq_mode_t mode,
                                     xy_hal_gpio_irq_handler_t handler,
                                     void *arg);

/**
 * @brief 分离 GPIO 中断
 * @param port GPIO 端口
 * @param pin 引脚号
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_detach_irq(xy_hal_gpio_port_t port, uint8_t pin);

/**
 * @brief 使能 GPIO 中断
 * @param port GPIO 端口
 * @param pin 引脚号
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_irq_enable(xy_hal_gpio_port_t port, uint8_t pin);

/**
 * @brief 禁用 GPIO 中断
 * @param port GPIO 端口
 * @param pin 引脚号
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_irq_disable(xy_hal_gpio_port_t port, uint8_t pin);

/**
 * @brief 获取中断状态
 * @param port GPIO 端口
 * @param pin 引脚号
 * @return 1=中断发生，0=无中断，负值表示错误
 */
int32_t xy_hal_gpio_get_irq_status(xy_hal_gpio_port_t port, uint8_t pin);

/**
 * @brief 清除中断状态
 * @param port GPIO 端口
 * @param pin 引脚号
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_clear_irq_status(xy_hal_gpio_port_t port, uint8_t pin);

/**
 * @brief GPIO 控制
 * @param port GPIO 端口
 * @param pin 引脚号
 * @param cmd 控制命令
 * @param args 命令参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_control(xy_hal_gpio_port_t port, uint8_t pin,
                                  uint32_t cmd, void *args);

/**
 * @brief GPIO 休眠模式
 * @param port GPIO 端口
 * @param pin 引脚号
 * @param sleep_state 休眠状态
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_set_sleep_state(xy_hal_gpio_port_t port, uint8_t pin,
                                          uint8_t sleep_state);

/**
 * @brief 获取 GPIO 休眠模式
 * @param port GPIO 端口
 * @param pin 引脚号
 * @return 休眠状态，负值表示错误
 */
int32_t xy_hal_gpio_get_sleep_state(xy_hal_gpio_port_t port, uint8_t pin);

/**
 * @brief GPIO 驱动信息
 * @param port GPIO 端口
 * @param info 驱动信息输出
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_get_driver_info(xy_hal_gpio_port_t port, void *info);

/**
 * @brief GPIO 引脚复用配置
 * @param port GPIO 端口
 * @param pin 引脚号
 * @param config 引脚复用配置
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_pinmux_config(xy_hal_gpio_port_t port, uint8_t pin,
                                        const void *config);

/**
 * @brief GPIO 引脚驱动强度配置
 * @param port GPIO 端口
 * @param pin 引脚号
 * @param strength 驱动强度
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_set_drive_strength(xy_hal_gpio_port_t port, uint8_t pin,
                                             uint8_t strength);

/**
 * @brief 获取 GPIO 引脚驱动强度
 * @param port GPIO 端口
 * @param pin 引脚号
 * @return 驱动强度，负值表示错误
 */
int32_t xy_hal_gpio_get_drive_strength(xy_hal_gpio_port_t port, uint8_t pin);

/**
 * @brief GPIO 引脚 slew rate 配置
 * @param port GPIO 端口
 * @param pin 引脚号
 * @param slew_rate slew rate
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_set_slew_rate(xy_hal_gpio_port_t port, uint8_t pin,
                                       uint8_t slew_rate);

/**
 * @brief 获取 GPIO 引脚 slew rate
 * @param port GPIO 端口
 * @param pin 引脚号
 * @return slew rate，负值表示错误
 */
int32_t xy_hal_gpio_get_slew_rate(xy_hal_gpio_port_t port, uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_GPIO_H */
