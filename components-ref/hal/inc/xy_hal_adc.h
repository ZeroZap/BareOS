/**
 * @file xy_hal_adc.h
 * @brief XinYi ADC Hardware Abstraction Layer
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_HAL_ADC_H
#define XY_HAL_ADC_H
#include "xy_hal_error.h"

#include "xy_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ADC 分辨率
 */
typedef enum {
    XY_HAL_ADC_RESOLUTION_6B = 0,    /**< 6 位分辨率 */
    XY_HAL_ADC_RESOLUTION_8B,        /**< 8 位分辨率 */
    XY_HAL_ADC_RESOLUTION_10B,       /**< 10 位分辨率 */
    XY_HAL_ADC_RESOLUTION_12B,       /**< 12 位分辨率 */
    XY_HAL_ADC_RESOLUTION_14B,       /**< 14 位分辨率 */
    XY_HAL_ADC_RESOLUTION_16B,       /**< 16 位分辨率 */
} xy_hal_adc_resolution_t;

/**
 * @brief ADC 数据对齐方式
 */
typedef enum {
    XY_HAL_ADC_DATAALIGN_RIGHT = 0,  /**< 右对齐 */
    XY_HAL_ADC_DATAALIGN_LEFT,       /**< 左对齐 */
} xy_hal_adc_align_t;

/**
 * @brief ADC 扫描模式
 */
typedef enum {
    XY_HAL_ADC_SCAN_DISABLE = 0,     /**< 禁用扫描模式 */
    XY_HAL_ADC_SCAN_ENABLE,          /**< 启用扫描模式 */
} xy_hal_adc_scan_t;

/**
 * @brief ADC 连续转换模式
 */
typedef enum {
    XY_HAL_ADC_CONTINUOUS_DISABLE = 0, /**< 单次转换 */
    XY_HAL_ADC_CONTINUOUS_ENABLE,      /**< 连续转换 */
} xy_hal_adc_continuous_t;

/**
 * @brief ADC 触发源
 */
typedef enum {
    XY_HAL_ADC_TRIGGER_SOFTWARE = 0,   /**< 软件触发 */
    XY_HAL_ADC_TRIGGER_TIMER,          /**< 定时器触发 */
    XY_HAL_ADC_TRIGGER_EXTI,           /**< 外部中断触发 */
    XY_HAL_ADC_TRIGGER_TIM1_CC1,       /**< TIM1 比较通道 1 */
    XY_HAL_ADC_TRIGGER_TIM1_CC2,       /**< TIM1 比较通道 2 */
    XY_HAL_ADC_TRIGGER_TIM1_CC3,       /**< TIM1 比较通道 3 */
    XY_HAL_ADC_TRIGGER_TIM2_CC2,       /**< TIM2 比较通道 2 */
    XY_HAL_ADC_TRIGGER_TIM3_TRGO,      /**< TIM3 触发输出 */
    XY_HAL_ADC_TRIGGER_EXTI11,         /**< EXTI 线 11 */
} xy_hal_adc_trigger_src_t;

/**
 * @brief ADC 通道配置
 */
typedef struct {
    uint8_t channel;                 /**< 通道号 */
    uint8_t rank;                    /**< 转换顺序 */
    uint32_t sampling_time;          /**< 采样时间 */
    uint8_t enabled;                 /**< 是否启用 */
} xy_hal_adc_channel_config_t;

/**
 * @brief ADC 配置结构
 */
typedef struct {
    xy_hal_adc_resolution_t resolution;   /**< 分辨率 */
    xy_hal_adc_align_t align;              /**< 数据对齐 */
    xy_hal_adc_scan_t scan_mode;           /**< 扫描模式 */
    xy_hal_adc_continuous_t continuous;    /**< 连续转换模式 */
    xy_hal_adc_trigger_src_t trigger_src;  /**< 触发源 */
    uint32_t clock_div;                   /**< 时钟分频 */
    uint32_t sampling_time;               /**< 采样时间 */
    uint8_t enable_vrefint;               /**< 使能内部参考电压 */
    uint8_t enable_temp_sensor;           /**< 使能温度传感器 */
    uint8_t enable_vbat;                  /**< 使能电池电压监测 */
} xy_hal_adc_config_t;

/**
 * @brief ADC 事件类型
 */
typedef enum {
    XY_HAL_ADC_EVENT_EOC = 0,        /**< 转换完成 */
    XY_HAL_ADC_EVENT_OVR,            /**< 溢出 */
    XY_HAL_ADC_EVENT_AWD,            /**< 窗口监视 */
    XY_HAL_ADC_EVENT_JEOC,           /**< 注入转换完成 */
    XY_HAL_ADC_EVENT_JQOVF,          /**< 注入队列溢出 */
    XY_HAL_ADC_EVENT_ERROR,          /**< 错误事件 */
} xy_hal_adc_evt_t;

/**
 * @brief ADC 回调类型
 */
typedef void (*xy_hal_adc_callback_t)(void *adc, xy_hal_adc_evt_t event, void *arg);

/**
 * @brief 初始化 ADC
 * @param adc ADC 实例
 * @param config 配置结构
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_init(void *adc, const xy_hal_adc_config_t *config);

/**
 * @brief 反初始化 ADC
 * @param adc ADC 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_deinit(void *adc);

/**
 * @brief 配置 ADC 通道
 * @param adc ADC 实例
 * @param channels 通道配置数组
 * @param count 通道数量
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_config_channels(void *adc,
                                         const xy_hal_adc_channel_config_t *channels,
                                         size_t count);

/**
 * @brief 启动 ADC 转换
 * @param adc ADC 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_start(void *adc);

/**
 * @brief 停止 ADC 转换
 * @param adc ADC 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_stop(void *adc);

/**
 * @brief 读取 ADC 值 (阻塞)
 * @param adc ADC 实例
 * @param channel 通道号
 * @param timeout 超时时间 (ms)
 * @return ADC 值，负值表示错误
 */
int32_t xy_hal_adc_read(void *adc, uint8_t channel, uint32_t timeout);

/**
 * @brief 读取 ADC 值 (非阻塞)
 * @param adc ADC 实例
 * @param channel 通道号
 * @return ADC 值，负值表示错误
 */
int32_t xy_hal_adc_read_nb(void *adc, uint8_t channel);

/**
 * @brief 多通道 ADC 读取
 * @param adc ADC 实例
 * @param channels 通道数组
 * @param values 值输出数组
 * @param count 通道数量
 * @param timeout 超时时间 (ms)
 * @return 实际读取数量，负值表示错误
 */
int32_t xy_hal_adc_read_multi(void *adc, const uint8_t *channels,
                              uint32_t *values, size_t count, uint32_t timeout);

/**
 * @brief 读取 ADC 值 (DMA 方式)
 * @param adc ADC 实例
 * @param buffer 数据缓冲区
 * @param count 转换数量
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_read_dma(void *adc, uint32_t *buffer, size_t count);

/**
 * @brief 获取 ADC 分辨率
 * @param adc ADC 实例
 * @return 分辨率 (位数)，负值表示错误
 */
int32_t xy_hal_adc_get_resolution(void *adc);

/**
 * @brief 获取 ADC 最大值
 * @param adc ADC 实例
 * @return 最大转换值
 */
uint32_t xy_hal_adc_get_max_value(void *adc);

/**
 * @brief ADC 值转电压 (mV)
 * @param adc ADC 实例
 * @param value ADC 值
 * @param vref 参考电压 (mV)
 * @return 电压值 (mV)
 */
uint32_t xy_hal_adc_value_to_mv(void *adc, uint32_t value, uint32_t vref);

/**
 * @brief 设置采样时间
 * @param adc ADC 实例
 * @param channel 通道号
 * @param sampling_time 采样时间
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_set_sampling_time(void *adc, uint8_t channel,
                                           uint32_t sampling_time);

/**
 * @brief 获取采样时间
 * @param adc ADC 实例
 * @param channel 通道号
 * @return 采样时间，负值表示错误
 */
int32_t xy_hal_adc_get_sampling_time(void *adc, uint8_t channel);

/**
 * @brief 设置触发源
 * @param adc ADC 实例
 * @param trigger_src 触发源
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_set_trigger(void *adc, xy_hal_adc_trigger_src_t trigger_src);

/**
 * @brief 获取触发源
 * @param adc ADC 实例
 * @return 触发源，负值表示错误
 */
int32_t xy_hal_adc_get_trigger(void *adc);

/**
 * @brief 设置连续转换模式
 * @param adc ADC 实例
 * @param continuous 连续模式
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_set_continuous(void *adc, xy_hal_adc_continuous_t continuous);

/**
 * @brief 获取连续转换模式
 * @param adc ADC 实例
 * @return 连续转换模式，负值表示错误
 */
int32_t xy_hal_adc_get_continuous(void *adc);

/**
 * @brief 使能温度传感器
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_enable_temp_sensor(void);

/**
 * @brief 禁用温度传感器
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_disable_temp_sensor(void);

/**
 * @brief 使能内部参考电压
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_enable_vrefint(void);

/**
 * @brief 禁用内部参考电压
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_disable_vrefint(void);

/**
 * @brief 获取内部参考电压值
 * @return 参考电压 ADC 值，负值表示错误
 */
int32_t xy_hal_adc_get_vrefint_value(void);

/**
 * @brief 使能电池电压监测
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_enable_vbat(void);

/**
 * @brief 禁用电池电压监测
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_disable_vbat(void);

/**
 * @brief 获取电池电压 ADC 值
 * @return 电池电压 ADC 值，负值表示错误
 */
int32_t xy_hal_adc_get_vbat_value(void);

/**
 * @brief 获取温度值 (摄氏度)
 * @return 温度值 (°C * 100)，负值表示错误
 */
int32_t xy_hal_adc_get_temp_value(void);

/**
 * @brief 设置窗口监视阈值
 * @param adc ADC 实例
 * @param low_threshold 低阈值
 * @param high_threshold 高阈值
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_set_window_threshold(void *adc, uint32_t low_threshold,
                                              uint32_t high_threshold);

/**
 * @brief 获取窗口监视阈值
 * @param adc ADC 实例
 * @param low_threshold 低阈值输出
 * @param high_threshold 高阈值输出
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_get_window_threshold(void *adc, uint32_t *low_threshold,
                                              uint32_t *high_threshold);

/**
 * @brief 使能窗口监视
 * @param adc ADC 实例
 * @param channel 通道号
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_enable_window_watchdog(void *adc, uint8_t channel);

/**
 * @brief 禁用窗口监视
 * @param adc ADC 实例
 * @param channel 通道号
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_disable_window_watchdog(void *adc, uint8_t channel);

/**
 * @brief 注册 ADC 回调
 * @param adc ADC 实例
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_register_callback(void *adc,
                                           xy_hal_adc_callback_t callback,
                                           void *arg);

/**
 * @brief ADC 控制
 * @param adc ADC 实例
 * @param cmd 控制命令
 * @param args 命令参数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_control(void *adc, uint32_t cmd, void *args);

/**
 * @brief 获取 ADC 状态
 * @param adc ADC 实例
 * @return 状态，负值表示错误
 */
int32_t xy_hal_adc_get_state(void *adc);

/**
 * @brief 校准 ADC
 * @param adc ADC 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_calibrate(void *adc);

/**
 * @brief 设置 ADC 预分频
 * @param adc ADC 实例
 * @param prescaler 预分频值
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_set_prescaler(void *adc, uint8_t prescaler);

/**
 * @brief 获取 ADC 预分频
 * @param adc ADC 实例
 * @return 预分频值，负值表示错误
 */
int32_t xy_hal_adc_get_prescaler(void *adc);

/**
 * @brief 设置过采样
 * @param adc ADC 实例
 * @param oversampling 过采样率
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_adc_set_oversampling(void *adc, uint8_t oversampling);

/**
 * @brief 获取过采样
 * @param adc ADC 实例
 * @return 过采样率，负值表示错误
 */
int32_t xy_hal_adc_get_oversampling(void *adc);

/**
 * @brief ADC 事件回调处理
 * @param adc ADC 实例
 * @param event 事件类型
 * @param arg 用户参数
 */
void xy_hal_adc_event_handler(void *adc, xy_hal_adc_evt_t event, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_ADC_H */
