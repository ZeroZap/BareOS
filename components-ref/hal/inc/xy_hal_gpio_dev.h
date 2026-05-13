/**
 * @file xy_hal_gpio_dev.h
 * @brief XinYi HAL GPIO Unified Device API
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 统一 GPIO 设备 API，支持设备模型和传统 API 两种使用方式
 * 
 * @section usage 使用示例
 * 
 * ### 方式 1: 设备模型 API (推荐)
 * 
 * ```c
 * #include "xy_hal_gpio_dev.h"
 * 
 * // 1. 获取 GPIO 设备
 * xy_hal_gpio_t gpio = xy_hal_gpio_bind("GPIOA");
 * 
 * // 2. 配置 GPIO
 * xy_hal_gpio_config_t cfg = {
 *     .mode = XY_HAL_GPIO_MODE_OUTPUT,
 *     .pull = XY_HAL_GPIO_PULL_NONE,
 *     .otype = XY_HAL_GPIO_OTYPE_PP,
 *     .speed = XY_HAL_GPIO_SPEED_HIGH,
 * };
 * xy_hal_gpio_configure(gpio, 5, &cfg);  // PA5 推挽输出
 * 
 * // 3. 控制 GPIO
 * xy_hal_gpio_write(gpio, 5, 1);  // 高电平
 * xy_hal_gpio_write(gpio, 5, 0);  // 低电平
 * 
 * // 4. 读取 GPIO
 * int32_t value = xy_hal_gpio_read(gpio, 3);  // 读取 PA3
 * 
 * // 5. 释放设备
 * xy_hal_gpio_unbind(gpio);
 * ```
 * 
 * ### 方式 2: 传统 API (向后兼容)
 * 
 * ```c
 * #include "xy_hal_gpio.h"
 * 
 * // 1. 配置 GPIO
 * xy_hal_gpio_config_t cfg = { ... };
 * xy_hal_gpio_init(GPIOA, 5, &cfg);
 * 
 * // 2. 控制 GPIO
 * xy_hal_gpio_write(GPIOA, 5, 1);
 * 
 * // 3. 读取 GPIO
 * int32_t value = xy_hal_gpio_read(GPIOA, 3);
 * ```
 */

#ifndef XY_HAL_GPIO_DEV_H
#define XY_HAL_GPIO_DEV_H
#include "xy_hal_error.h"

#include "xy_hal_gpio_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== GPIO Device Handle ==================== */

/**
 * @brief GPIO 设备句柄 (不透明指针)
 */
typedef struct xy_hal_gpio_dev *xy_hal_gpio_t;

/**
 * @brief GPIO 设备常量句柄
 */
typedef const struct xy_hal_gpio_dev *xy_hal_gpio_const_t;

/* ==================== GPIO Device API ==================== */

/**
 * @brief GPIO 设备操作集 (虚表)
 * 
 * 每个平台实现自己的操作集
 */
typedef struct xy_hal_gpio_api {
    /**
     * @brief 配置 GPIO 引脚
     * @param dev GPIO 设备
     * @param pin 引脚号 (0-15)
     * @param config 配置结构
     * @return XY_HAL_OK 成功，其他值失败
     */
    xy_hal_error_t (*configure)(xy_hal_gpio_const_t dev, uint32_t pin,
                                const xy_hal_gpio_config_t *config);
    
    /**
     * @brief 反初始化 GPIO 引脚
     * @param dev GPIO 设备
     * @param pin 引脚号
     * @return XY_HAL_OK 成功，其他值失败
     */
    xy_hal_error_t (*deconfigure)(xy_hal_gpio_const_t dev, uint32_t pin);
    
    /**
     * @brief 设置 GPIO 输出值
     * @param dev GPIO 设备
     * @param pin 引脚号
     * @param value 输出值 (0=低电平，1=高电平)
     * @return XY_HAL_OK 成功，其他值失败
     */
    xy_hal_error_t (*write)(xy_hal_gpio_const_t dev, uint32_t pin, uint8_t value);
    
    /**
     * @brief 读取 GPIO 输入值
     * @param dev GPIO 设备
     * @param pin 引脚号
     * @return GPIO 值 (0=低电平，1=高电平)，负值表示错误
     */
    int32_t (*read)(xy_hal_gpio_const_t dev, uint32_t pin);
    
    /**
     * @brief 翻转 GPIO 输出值
     * @param dev GPIO 设备
     * @param pin 引脚号
     * @return XY_HAL_OK 成功，其他值失败
     */
    xy_hal_error_t (*toggle)(xy_hal_gpio_const_t dev, uint32_t pin);
    
    /**
     * @brief 批量设置 GPIO 端口值
     * @param dev GPIO 设备
     * @param mask 引脚掩码 (16 位)
     * @param value 值 (16 位)
     * @return XY_HAL_OK 成功，其他值失败
     */
    xy_hal_error_t (*port_write)(xy_hal_gpio_const_t dev, 
                                 xy_hal_gpio_mask_t mask,
                                 xy_hal_gpio_value_t value);
    
    /**
     * @brief 批量读取 GPIO 端口值
     * @param dev GPIO 设备
     * @return 端口值 (16 位)，每位对应一个引脚
     */
    xy_hal_gpio_value_t (*port_read)(xy_hal_gpio_const_t dev);
    
    /**
     * @brief 配置 GPIO 中断
     * @param dev GPIO 设备
     * @param pin 引脚号
     * @param mode 中断模式
     * @param cb 回调函数
     * @return XY_HAL_OK 成功，其他值失败
     */
    xy_hal_error_t (*irq_configure)(xy_hal_gpio_const_t dev, uint32_t pin,
                                    xy_hal_gpio_irq_mode_t mode,
                                    const xy_hal_gpio_irq_cb_t *cb);
    
    /**
     * @brief 使能 GPIO 中断
     * @param dev GPIO 设备
     * @param pin 引脚号
     * @return XY_HAL_OK 成功，其他值失败
     */
    xy_hal_error_t (*irq_enable)(xy_hal_gpio_const_t dev, uint32_t pin);
    
    /**
     * @brief 禁用 GPIO 中断
     * @param dev GPIO 设备
     * @param pin 引脚号
     * @return XY_HAL_OK 成功，其他值失败
     */
    xy_hal_error_t (*irq_disable)(xy_hal_gpio_const_t dev, uint32_t pin);
    
    /**
     * @brief 清除 GPIO 中断标志
     * @param dev GPIO 设备
     * @param pin 引脚号
     * @return XY_HAL_OK 成功，其他值失败
     */
    xy_hal_error_t (*irq_clear)(xy_hal_gpio_const_t dev, uint32_t pin);
    
    /**
     * @brief 获取 GPIO 端口信息
     * @param dev GPIO 设备
     * @return 端口信息结构指针
     */
    const xy_hal_gpio_port_info_t *(*get_info)(xy_hal_gpio_const_t dev);
    
} xy_hal_gpio_api_t;

/* ==================== GPIO Device Structure ==================== */

/**
 * @brief GPIO 设备结构
 */
struct xy_hal_gpio_dev {
    const char *name;                /**< 设备名称 (如 "GPIOA") */
    const xy_hal_gpio_api_t *api;    /**< API 虚表 */
    void *data;                      /**< 平台特定数据 */
    const void *config;              /**< 配置数据 */
    xy_hal_gpio_flag_t flags;        /**< 设备标志 */
    uint8_t port_num;                /**< 端口号 (0=A, 1=B, ...) */
    uint8_t pin_count;               /**< 引脚数量 */
};

/* ==================== Device Management ==================== */

/**
 * @brief 绑定 GPIO 设备
 * @param name 设备名称 (如 "GPIOA", "GPIOB")
 * @return GPIO 设备句柄，NULL 表示失败
 * 
 * @note 设备名称不区分大小写
 * @note 返回的句柄需要调用 xy_hal_gpio_unbind() 释放
 */
xy_hal_gpio_t xy_hal_gpio_bind(const char *name);

/**
 * @brief 释放 GPIO 设备
 * @param dev GPIO 设备句柄
 * 
 * @note 释放后不应再使用该句柄
 */
void xy_hal_gpio_unbind(xy_hal_gpio_t dev);

/**
 * @brief 获取 GPIO 设备数量
 * @return GPIO 设备数量
 */
uint32_t xy_hal_gpio_count(void);

/**
 * @brief 枚举 GPIO 设备
 * @param index 设备索引 (0 ~ count-1)
 * @return 设备名称，NULL 表示索引越界
 */
const char *xy_hal_gpio_name(uint32_t index);

/* ==================== GPIO Configuration ==================== */

/**
 * @brief 配置 GPIO 引脚
 * @param dev GPIO 设备句柄
 * @param pin 引脚号 (0-15)
 * @param config 配置结构
 * @return XY_HAL_OK 成功，XY_HAL_ERROR_INVALID_PARAM 参数错误，
 *         XY_HAL_ERROR_NOT_SUPPORTED 不支持该配置
 * 
 * @note 配置前必须先调用 xy_hal_gpio_bind() 获取设备句柄
 */
xy_hal_error_t xy_hal_gpio_configure(xy_hal_gpio_t dev, uint32_t pin,
                                     const xy_hal_gpio_config_t *config);

/**
 * @brief 反初始化 GPIO 引脚 (恢复默认状态)
 * @param dev GPIO 设备句柄
 * @param pin 引脚号
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_deconfigure(xy_hal_gpio_t dev, uint32_t pin);

/**
 * @brief 设置 GPIO 模式
 * @param dev GPIO 设备句柄
 * @param pin 引脚号
 * @param mode GPIO 模式
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_set_mode(xy_hal_gpio_t dev, uint32_t pin,
                                    xy_hal_gpio_mode_t mode);

/**
 * @brief 获取 GPIO 模式
 * @param dev GPIO 设备句柄
 * @param pin 引脚号
 * @return GPIO 模式，负值表示错误
 */
int32_t xy_hal_gpio_get_mode(xy_hal_gpio_t dev, uint32_t pin);

/* ==================== GPIO Operations ==================== */

/**
 * @brief 设置 GPIO 输出值
 * @param dev GPIO 设备句柄
 * @param pin 引脚号
 * @param value 输出值 (0=低电平，1=高电平)
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_t dev, uint32_t pin, uint8_t value);

/**
 * @brief 读取 GPIO 输入值
 * @param dev GPIO 设备句柄
 * @param pin 引脚号
 * @return GPIO 值 (0=低电平，1=高电平)，负值表示错误
 */
int32_t xy_hal_gpio_read(xy_hal_gpio_t dev, uint32_t pin);

/**
 * @brief 翻转 GPIO 输出值
 * @param dev GPIO 设备句柄
 * @param pin 引脚号
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_toggle(xy_hal_gpio_t dev, uint32_t pin);

/**
 * @brief 批量设置 GPIO 端口值
 * @param dev GPIO 设备句柄
 * @param mask 引脚掩码 (16 位，1 表示需要设置的引脚)
 * @param value 值 (16 位，对应掩码中的引脚)
 * @return XY_HAL_OK 成功，其他值失败
 * 
 * @example
 * // 设置 PA0 和 PA2 为高电平，其他引脚不变
 * xy_hal_gpio_port_write(gpio, 0x05, 0x05);
 */
xy_hal_error_t xy_hal_gpio_port_write(xy_hal_gpio_t dev, 
                                      xy_hal_gpio_mask_t mask,
                                      xy_hal_gpio_value_t value);

/**
 * @brief 批量读取 GPIO 端口值
 * @param dev GPIO 设备句柄
 * @return 端口值 (16 位，每位对应一个引脚)
 */
xy_hal_gpio_value_t xy_hal_gpio_port_read(xy_hal_gpio_t dev);

/* ==================== GPIO Interrupt ==================== */

/**
 * @brief 配置 GPIO 中断
 * @param dev GPIO 设备句柄
 * @param pin 引脚号
 * @param mode 中断模式
 * @param handler 回调函数
 * @param arg 回调参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_irq_configure(xy_hal_gpio_t dev, uint32_t pin,
                                         xy_hal_gpio_irq_mode_t mode,
                                         xy_hal_gpio_irq_handler_t handler,
                                         void *arg);

/**
 * @brief 使能 GPIO 中断
 * @param dev GPIO 设备句柄
 * @param pin 引脚号
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_irq_enable(xy_hal_gpio_t dev, uint32_t pin);

/**
 * @brief 禁用 GPIO 中断
 * @param dev GPIO 设备句柄
 * @param pin 引脚号
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_irq_disable(xy_hal_gpio_t dev, uint32_t pin);

/**
 * @brief 清除 GPIO 中断标志
 * @param dev GPIO 设备句柄
 * @param pin 引脚号
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_gpio_irq_clear(xy_hal_gpio_t dev, uint32_t pin);

/* ==================== Utility Functions ==================== */

/**
 * @brief 获取 GPIO 端口信息
 * @param dev GPIO 设备句柄
 * @return 端口信息结构指针
 */
const xy_hal_gpio_port_info_t *xy_hal_gpio_get_info(xy_hal_gpio_t dev);

/**
 * @brief 检查 GPIO 引脚是否有效
 * @param dev GPIO 设备句柄
 * @param pin 引脚号
 * @return 1=有效，0=无效
 */
int xy_hal_gpio_pin_valid(xy_hal_gpio_t dev, uint32_t pin);

/**
 * @brief 检查 GPIO 引脚是否配置为输出模式
 * @param dev GPIO 设备句柄
 * @param pin 引脚号
 * @return 1=输出模式，0=非输出模式
 */
int xy_hal_gpio_is_output(xy_hal_gpio_t dev, uint32_t pin);

/**
 * @brief 检查 GPIO 引脚是否配置为输入模式
 * @param dev GPIO 设备句柄
 * @param pin 引脚号
 * @return 1=输入模式，0=非输入模式
 */
int xy_hal_gpio_is_input(xy_hal_gpio_t dev, uint32_t pin);

/* ==================== Backward Compatibility ==================== */

/**
 * @brief 传统 GPIO 初始化函数 (向后兼容)
 * @param port GPIO 端口 (如 GPIOA, GPIOB)
 * @param pin 引脚号
 * @param config 配置结构
 * @return XY_HAL_OK 成功，其他值失败
 * 
 * @deprecated 请使用 xy_hal_gpio_bind() + xy_hal_gpio_configure()
 */
xy_hal_error_t xy_hal_gpio_init(void *port, uint8_t pin,
                                const xy_hal_gpio_config_t *config);

/* Legacy GPIO functions temporarily disabled due to type conflicts */
/*
xy_hal_error_t xy_hal_gpio_write(void *port, uint8_t pin, uint8_t value);
int32_t xy_hal_gpio_read(void *port, uint8_t pin);
*/

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_GPIO_DEV_H */
