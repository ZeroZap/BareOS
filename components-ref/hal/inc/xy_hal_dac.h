/**
 * @file xy_hal_dac.h
 * @brief DAC (Digital-to-Analog Converter) Hardware Abstraction Layer
 * @version 2.0
 * @date 2026-02-27
 */

#ifndef XY_HAL_DAC_H
#define XY_HAL_DAC_H
#include "xy_hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_hal.h"
#include <stdint.h>

/**
 * @brief DAC 通道
 */
typedef enum {
    XY_HAL_DAC_CHANNEL_1 = 0,   /**< 通道 1 */
    XY_HAL_DAC_CHANNEL_2,       /**< 通道 2 */
} xy_hal_dac_channel_t;

/**
 * @brief DAC 分辨率
 */
typedef enum {
    XY_HAL_DAC_RESOLUTION_8B = 0,   /**< 8 位分辨率 */
    XY_HAL_DAC_RESOLUTION_12B,      /**< 12 位分辨率 */
} xy_hal_dac_resolution_t;

/**
 * @brief DAC 数据对齐方式
 */
typedef enum {
    XY_HAL_DAC_DATAALIGN_RIGHT = 0, /**< 右对齐 */
    XY_HAL_DAC_DATAALIGN_LEFT,      /**< 左对齐 */
} xy_hal_dac_align_t;

/**
 * @brief DAC 触发源
 */
typedef enum {
    XY_HAL_DAC_TRIGGER_SOFTWARE = 0,    /**< 软件触发 */
    XY_HAL_DAC_TRIGGER_TIMER,           /**< 定时器触发 */
    XY_HAL_DAC_TRIGGER_EXTI,            /**< 外部中断触发 */
} xy_hal_dac_trigger_src_t;

/**
 * @brief DAC 波形生成
 */
typedef enum {
    XY_HAL_DAC_WAVE_NONE = 0,       /**< 无波形生成 */
    XY_HAL_DAC_WAVE_NOISE,          /**< 噪声波形 */
    XY_HAL_DAC_WAVE_TRIANGLE,       /**< 三角波 */
} xy_hal_dac_wave_t;

/**
 * @brief DAC 配置结构
 */
typedef struct {
    xy_hal_dac_resolution_t resolution;     /**< 分辨率 */
    xy_hal_dac_align_t align;               /**< 数据对齐 */
    xy_hal_dac_trigger_src_t trigger_src;   /**< 触发源 */
    xy_hal_dac_wave_t wave;                 /**< 波形生成 */
    uint8_t buffer_enable;                  /**< 输出缓冲使能 */
} xy_hal_dac_config_t;

/**
 * @brief DAC 事件类型
 */
typedef enum {
    XY_HAL_DAC_EVENT_DMAUNDERRUN = 0,   /**< DMA 欠载 */
    XY_HAL_DAC_EVENT_READY,             /**< 准备就绪 */
} xy_hal_dac_event_t;

/**
 * @brief DAC 回调类型
 */
typedef void (*xy_hal_dac_callback_t)(void *dac, xy_hal_dac_channel_t channel,
                                      xy_hal_dac_event_t event, void *arg);

/**
 * @brief 初始化 DAC
 * @param dac DAC 实例
 * @param config DAC 配置
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_dac_init(void *dac, const xy_hal_dac_config_t *config);

/**
 * @brief 反初始化 DAC
 * @param dac DAC 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_dac_deinit(void *dac);

/**
 * @brief 使能 DAC 通道
 * @param dac DAC 实例
 * @param channel DAC 通道
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_dac_enable_channel(void *dac, xy_hal_dac_channel_t channel);

/**
 * @brief 禁用 DAC 通道
 * @param dac DAC 实例
 * @param channel DAC 通道
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_dac_disable_channel(void *dac, xy_hal_dac_channel_t channel);

/**
 * @brief 设置 DAC 输出值
 * @param dac DAC 实例
 * @param channel DAC 通道
 * @param value 输出值 (根据分辨率 0-255 或 0-4095)
 * @param align 数据对齐方式
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_dac_set_value(void *dac, xy_hal_dac_channel_t channel,
                                    uint32_t value, xy_hal_dac_align_t align);

/**
 * @brief 获取 DAC 输出值
 * @param dac DAC 实例
 * @param channel DAC 通道
 * @return 输出值，负值表示错误
 */
int xy_hal_dac_get_value(void *dac, xy_hal_dac_channel_t channel);

/**
 * @brief 启动 DAC 输出
 * @param dac DAC 实例
 * @param channel DAC 通道
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_dac_start_output(void *dac, xy_hal_dac_channel_t channel);

/**
 * @brief 停止 DAC 输出
 * @param dac DAC 实例
 * @param channel DAC 通道
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_dac_stop_output(void *dac, xy_hal_dac_channel_t channel);

/**
 * @brief 软件触发 DAC 转换
 * @param dac DAC 实例
 * @param channel DAC 通道
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_dac_software_trigger(void *dac, xy_hal_dac_channel_t channel);

/**
 * @brief DAC DMA 输出
 * @param dac DAC 实例
 * @param channel DAC 通道
 * @param data 数据缓冲区
 * @param count 数据数量
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_dac_output_dma(void *dac, xy_hal_dac_channel_t channel,
                                     const uint32_t *data, size_t count);

/**
 * @brief 生成锯齿波
 * @param dac DAC 实例
 * @param channel DAC 通道
 * @param amplitude 幅度
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_dac_generate_triangle(void *dac, xy_hal_dac_channel_t channel,
                                            uint32_t amplitude);

/**
 * @brief 生成噪声
 * @param dac DAC 实例
 * @param channel DAC 通道
 * @param mask 掩码
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_dac_generate_noise(void *dac, xy_hal_dac_channel_t channel,
                                         uint32_t mask);

/**
 * @brief 注册 DAC 回调
 * @param dac DAC 实例
 * @param channel DAC 通道
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_dac_register_callback(void *dac, xy_hal_dac_channel_t channel,
                                            xy_hal_dac_callback_t callback, void *arg);

/**
 * @brief DAC 值转电压
 * @param dac DAC 实例
 * @param value DAC 值
 * @param vref 参考电压 (mV)
 * @return 电压值 (mV)
 */
uint32_t xy_hal_dac_value_to_mv(void *dac, uint32_t value, uint32_t vref);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_DAC_H */
