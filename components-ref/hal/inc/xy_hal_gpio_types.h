/**
 * @file xy_hal_gpio_types.h
 * @brief XinYi HAL GPIO Unified Type Definitions
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 统一 GPIO 类型定义，适用于所有平台 (STM32/WCH/HC32)
 */

#ifndef XY_HAL_GPIO_TYPES_H
#define XY_HAL_GPIO_TYPES_H
#include "xy_hal_error.h"

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== GPIO Pin Definitions ==================== */

/**
 * @brief GPIO 引脚号 (0-15)
 */
typedef uint8_t xy_hal_gpio_pin_t;

/**
 * @brief GPIO 端口掩码 (16 位，对应 16 个引脚)
 */
typedef uint16_t xy_hal_gpio_mask_t;

/**
 * @brief GPIO 端口值 (16 位，对应 16 个引脚)
 */
typedef uint16_t xy_hal_gpio_value_t;

/**
 * @brief GPIO 引脚编号宏
 * @param port 端口号 (0=A, 1=B, 2=C, ...)
 * @param pin 引脚号 (0-15)
 * @return 统一引脚编号
 */
#define XY_HAL_GPIO_PIN(port, pin)  (((port) << 4) | ((pin) & 0x0F))

/**
 * @brief 从统一引脚编号提取端口号
 * @param pin_def 统一引脚编号
 * @return 端口号
 */
#define XY_HAL_GPIO_PORT(pin_def)   (((pin_def) >> 4) & 0x0F)

/**
 * @brief 从统一引脚编号提取引脚号
 * @param pin_def 统一引脚编号
 * @return 引脚号 (0-15)
 */
#define XY_HAL_GPIO_PIN_NUM(pin_def)  ((pin_def) & 0x0F)

/**
 * @brief 创建引脚掩码
 * @param pin 引脚号 (0-15)
 * @return 引脚掩码
 */
#define XY_HAL_GPIO_PIN_MASK(pin)  (1U << ((pin) & 0x0F))

/* ==================== GPIO Mode ==================== */

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
    XY_HAL_GPIO_MODE_COUNT           /**< 模式数量 (用于验证) */
} xy_hal_gpio_mode_t;

/* ==================== GPIO Pull Resistors ==================== */

/**
 * @brief GPIO 上下拉配置
 */
typedef enum {
    XY_HAL_GPIO_PULL_NONE = 0,       /**< 无上下拉 */
    XY_HAL_GPIO_PULL_UP,             /**< 上拉 */
    XY_HAL_GPIO_PULL_DOWN,           /**< 下拉 */
    XY_HAL_GPIO_PULL_COUNT           /**< 配置数量 (用于验证) */
} xy_hal_gpio_pull_t;

/* ==================== GPIO Output Type ==================== */

/**
 * @brief GPIO 输出类型
 */
typedef enum {
    XY_HAL_GPIO_OTYPE_PP = 0,        /**< 推挽输出 */
    XY_HAL_GPIO_OTYPE_OD,            /**< 开漏输出 */
    XY_HAL_GPIO_OTYPE_COUNT          /**< 类型数量 (用于验证) */
} xy_hal_gpio_otype_t;

/* ==================== GPIO Speed ==================== */

/**
 * @brief GPIO 速度配置
 */
typedef enum {
    XY_HAL_GPIO_SPEED_LOW = 0,       /**< 低速 */
    XY_HAL_GPIO_SPEED_MEDIUM,        /**< 中速 */
    XY_HAL_GPIO_SPEED_HIGH,          /**< 高速 */
    XY_HAL_GPIO_SPEED_VERY_HIGH,     /**< 超高速 */
    XY_HAL_GPIO_SPEED_COUNT          /**< 速度数量 (用于验证) */
} xy_hal_gpio_speed_t;

/* ==================== GPIO Interrupt Mode ==================== */

/**
 * @brief GPIO 中断触发模式
 */
typedef enum {
    XY_HAL_GPIO_IRQ_RISING = 0,      /**< 上升沿触发 */
    XY_HAL_GPIO_IRQ_FALLING,         /**< 下降沿触发 */
    XY_HAL_GPIO_IRQ_BOTH,            /**< 双边沿触发 */
    XY_HAL_GPIO_IRQ_LEVEL_HIGH,      /**< 高电平触发 */
    XY_HAL_GPIO_IRQ_LEVEL_LOW,       /**< 低电平触发 */
    XY_HAL_GPIO_IRQ_COUNT            /**< 中断模式数量 */
} xy_hal_gpio_irq_mode_t;

/* ==================== GPIO Configuration ==================== */

/**
 * @brief GPIO 配置结构
 * 
 * 用于配置 GPIO 引脚的所有参数
 */
typedef struct {
    xy_hal_gpio_mode_t mode;         /**< 模式 */
    xy_hal_gpio_pull_t pull;         /**< 上下拉 */
    xy_hal_gpio_otype_t otype;       /**< 输出类型 (仅输出模式有效) */
    xy_hal_gpio_speed_t speed;       /**< 速度 (仅输出模式有效) */
    uint8_t alternate;               /**< 复用功能编号 (仅 AF 模式有效) */
} xy_hal_gpio_config_t;

/**
 * @brief GPIO 默认配置 (输入模式)
 */
#define XY_HAL_GPIO_DEFAULT_CONFIG  \
    {                               \
        .mode = XY_HAL_GPIO_MODE_INPUT, \
        .pull = XY_HAL_GPIO_PULL_NONE,  \
        .otype = XY_HAL_GPIO_OTYPE_PP,  \
        .speed = XY_HAL_GPIO_SPEED_MEDIUM, \
        .alternate = 0,               \
    }

/* ==================== GPIO Flags ==================== */

/**
 * @brief GPIO 设备标志
 */
typedef enum {
    XY_HAL_GPIO_FLAG_NONE     = 0x00, /**< 无标志 */
    XY_HAL_GPIO_FLAG_OUTPUT   = 0x01, /**< 输出能力 */
    XY_HAL_GPIO_FLAG_INPUT    = 0x02, /**< 输入能力 */
    XY_HAL_GPIO_FLAG_IT       = 0x04, /**< 支持中断 */
    XY_HAL_GPIO_FLAG_EVT      = 0x08, /**< 支持事件 */
    XY_HAL_GPIO_FLAG_AF       = 0x10, /**< 支持复用功能 */
    XY_HAL_GPIO_FLAG_ANALOG   = 0x20, /**< 支持模拟模式 */
    XY_HAL_GPIO_FLAG_PULL     = 0x40, /**< 支持上下拉 */
} xy_hal_gpio_flag_t;

/* ==================== GPIO Interrupt Callback ==================== */

/**
 * @brief GPIO 中断回调函数类型
 * @param pin 引脚号 (统一编号)
 * @param arg 用户参数
 */
typedef void (*xy_hal_gpio_irq_handler_t)(uint32_t pin, void *arg);

/**
 * @brief GPIO 中断回调结构
 */
typedef struct {
    xy_hal_gpio_irq_handler_t handler; /**< 回调函数 */
    void *arg;                          /**< 用户参数 */
} xy_hal_gpio_irq_cb_t;

/* ==================== GPIO Port Information ==================== */

/**
 * @brief GPIO 端口信息
 */
typedef struct {
    const char *name;                /**< 端口名称 (如 "GPIOA") */
    uint32_t base_addr;              /**< 基地址 */
    uint8_t port_num;                /**< 端口号 (0=A, 1=B, ...) */
    uint8_t pin_count;               /**< 引脚数量 (通常 16) */
    xy_hal_gpio_flag_t flags;        /**< 支持的功能标志 */
    uint32_t clock_mask;             /**< 时钟使能掩码 */
} xy_hal_gpio_port_info_t;

/* ==================== Helper Macros ==================== */

/**
 * @brief 检查 GPIO 模式是否有效
 * @param mode GPIO 模式
 * @return 1=有效，0=无效
 */
#define XY_HAL_GPIO_MODE_VALID(mode)  ((mode) < XY_HAL_GPIO_MODE_COUNT)
#include "xy_hal_error.h"

/**
 * @brief 检查 GPIO 上下拉配置是否有效
 * @param pull 上下拉配置
 * @return 1=有效，0=无效
 */
#define XY_HAL_GPIO_PULL_VALID(pull)  ((pull) < XY_HAL_GPIO_PULL_COUNT)
#include "xy_hal_error.h"

/**
 * @brief 检查 GPIO 输出类型是否有效
 * @param otype 输出类型
 * @return 1=有效，0=无效
 */
#define XY_HAL_GPIO_OTYPE_VALID(otype)  ((otype) < XY_HAL_GPIO_OTYPE_COUNT)
#include "xy_hal_error.h"

/**
 * @brief 检查 GPIO 速度配置是否有效
 * @param speed 速度配置
 * @return 1=有效，0=无效
 */
#define XY_HAL_GPIO_SPEED_VALID(speed)  ((speed) < XY_HAL_GPIO_SPEED_COUNT)
#include "xy_hal_error.h"

/**
 * @brief 检查 GPIO 引脚号是否有效
 * @param pin 引脚号
 * @return 1=有效，0=无效
 */
#define XY_HAL_GPIO_PIN_VALID(pin)  ((pin) <= 15)
#include "xy_hal_error.h"

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_GPIO_TYPES_H */
