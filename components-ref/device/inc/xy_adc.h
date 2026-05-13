/**
 * @file xy_adc.h
 * @brief XinYi ADC Device Driver
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_ADC_H
#define XY_ADC_H

#include "xy_device.h"
#include "xy_hal_adc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ADC 事件类型
 */
typedef enum {
    XY_ADC_EVT_CONVERSION_COMPLETE = 0,  /**< 转换完成 */
    XY_ADC_EVT_SCAN_COMPLETE,            /**< 扫描完成 */
    XY_ADC_EVT_ERROR,                    /**< 错误事件 */
    XY_ADC_EVT_OVERRUN,                  /**< 溢出事件 */
} xy_adc_evt_t;

/**
 * @brief ADC 回调类型
 */
typedef void (*xy_adc_callback_t)(void *dev, xy_adc_evt_t evt, void *arg);

/**
 * @brief ADC 通道配置
 */
typedef struct {
    uint8_t channel;                    /**< 通道号 */
    uint8_t rank;                       /**< 转换顺序 */
    uint32_t sampling_time;             /**< 采样时间 */
    uint8_t enabled;                    /**< 是否使能 */
} xy_adc_channel_config_t;

/**
 * @brief ADC 设备控制命令
 */
typedef enum {
    XY_ADC_CMD_SET_CONFIG = 0,          /**< 设置配置 */
    XY_ADC_CMD_GET_CONFIG,              /**< 获取配置 */
    XY_ADC_CMD_SET_CHANNEL_CONFIG,      /**< 设置通道配置 */
    XY_ADC_CMD_GET_CHANNEL_CONFIG,      /**< 获取通道配置 */
    XY_ADC_CMD_START_CONVERSION,        /**< 开始转换 */
    XY_ADC_CMD_STOP_CONVERSION,         /**< 停止转换 */
    XY_ADC_CMD_SET_SAMPLING_TIME,       /**< 设置采样时间 */
    XY_ADC_CMD_GET_SAMPLING_TIME,       /**< 获取采样时间 */
    XY_ADC_CMD_SET_RESOLUTION,          /**< 设置分辨率 */
    XY_ADC_CMD_GET_RESOLUTION,          /**< 获取分辨率 */
    XY_ADC_CMD_SET_ALIGN,               /**< 设置对齐 */
    XY_ADC_CMD_GET_ALIGN,               /**< 获取对齐 */
    XY_ADC_CMD_SET_SCAN_MODE,           /**< 设置扫描模式 */
    XY_ADC_CMD_GET_SCAN_MODE,           /**< 获取扫描模式 */
    XY_ADC_CMD_SET_CONTINUOUS,          /**< 设置连续模式 */
    XY_ADC_CMD_GET_CONTINUOUS,          /**< 获取连续模式 */
    XY_ADC_CMD_SET_TRIGGER,             /**< 设置触发源 */
    XY_ADC_CMD_GET_TRIGGER,             /**< 获取触发源 */
    XY_ADC_CMD_SET_CALIBRATION,         /**< 设置校准 */
    XY_ADC_CMD_GET_CALIBRATION,         /**< 获取校准 */
} xy_adc_cmd_t;

/**
 * @brief ADC 数据结构
 */
typedef struct {
    uint32_t channel;                   /**< 通道号 */
    uint32_t value;                     /**< 转换值 */
    uint64_t timestamp;                 /**< 时间戳 */
} xy_adc_data_t;

/**
 * @brief ADC 多通道数据结构
 */
typedef struct {
    xy_adc_data_t *data;                /**< 数据数组 */
    size_t count;                       /**< 数据数量 */
} xy_adc_multi_data_t;

/**
 * @brief 初始化 ADC 设备
 * @param dev 设备指针
 * @param config 配置结构
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_adc_dev_init(void *dev, const xy_adc_config_t *config);

/**
 * @brief 配置 ADC 通道
 * @param dev 设备指针
 * @param channels 通道配置数组
 * @param count 通道数量
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_adc_dev_config_channels(void *dev, 
                                     const xy_adc_channel_config_t *channels,
                                     size_t count);

/**
 * @brief 启动 ADC 转换
 * @param dev 设备指针
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_adc_dev_start(void *dev);

/**
 * @brief 停止 ADC 转换
 * @param dev 设备指针
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_adc_dev_stop(void *dev);

/**
 * @brief 单通道 ADC 读取
 * @param dev 设备指针
 * @param channel 通道号
 * @param timeout 超时时间 (ms)
 * @return 转换值，负值表示错误
 */
int32_t xy_adc_dev_read(void *dev, uint8_t channel, uint32_t timeout);

/**
 * @brief 多通道 ADC 读取
 * @param dev 设备指针
 * @param channels 通道数组
 * @param values 值输出数组
 * @param count 通道数量
 * @param timeout 超时时间 (ms)
 * @return 实际读取通道数，负值表示错误
 */
int32_t xy_adc_dev_read_multi(void *dev, const uint8_t *channels, 
                              uint32_t *values, size_t count, uint32_t timeout);

/**
 * @brief 获取 ADC 转换值 (非阻塞)
 * @param dev 设备指针
 * @param channel 通道号
 * @return 转换值，负值表示错误或未就绪
 */
int32_t xy_adc_dev_read_nb(void *dev, uint8_t channel);

/**
 * @brief 设置采样时间
 * @param dev 设备指针
 * @param channel 通道号
 * @param sampling_time 采样时间
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_adc_dev_set_sampling_time(void *dev, uint8_t channel, 
                                       uint32_t sampling_time);

/**
 * @brief 获取采样时间
 * @param dev 设备指针
 * @param channel 通道号
 * @return 采样时间，负值表示错误
 */
int32_t xy_adc_dev_get_sampling_time(void *dev, uint8_t channel);

/**
 * @brief 设置分辨率
 * @param dev 设备指针
 * @param resolution 分辨率
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_adc_dev_set_resolution(void *dev, xy_adc_resolution_t resolution);

/**
 * @brief 获取分辨率
 * @param dev 设备指针
 * @return 分辨率，负值表示错误
 */
int32_t xy_adc_dev_get_resolution(void *dev);

/**
 * @brief 设置数据对齐
 * @param dev 设备指针
 * @param align 数据对齐方式
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_adc_dev_set_align(void *dev, xy_adc_align_t align);

/**
 * @brief 获取数据对齐
 * @param dev 设备指针
 * @return 数据对齐方式，负值表示错误
 */
int32_t xy_adc_dev_get_align(void *dev);

/**
 * @brief 设置扫描模式
 * @param dev 设备指针
 * @param scan_mode 扫描模式
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_adc_dev_set_scan_mode(void *dev, xy_adc_scan_t scan_mode);

/**
 * @brief 获取扫描模式
 * @param dev 设备指针
 * @return 扫描模式，负值表示错误
 */
int32_t xy_adc_dev_get_scan_mode(void *dev);

/**
 * @brief 设置连续转换模式
 * @param dev 设备指针
 * @param continuous 连续模式
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_adc_dev_set_continuous(void *dev, xy_adc_continuous_t continuous);

/**
 * @brief 获取连续转换模式
 * @param dev 设备指针
 * @return 连续转换模式，负值表示错误
 */
int32_t xy_adc_dev_get_continuous(void *dev);

/**
 * @brief 设置触发源
 * @param dev 设备指针
 * @param trigger 触发源
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_adc_dev_set_trigger(void *dev, xy_adc_trigger_src_t trigger);

/**
 * @brief 获取触发源
 * @param dev 设备指针
 * @return 触发源，负值表示错误
 */
int32_t xy_adc_dev_get_trigger(void *dev);

/**
 * @brief 启用 DMA 模式
 * @param dev 设备指针
 * @param buffer 数据缓冲区
 * @param size 缓冲区大小
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_adc_dev_enable_dma(void *dev, uint32_t *buffer, size_t size);

/**
 * @brief 禁用 DMA 模式
 * @param dev 设备指针
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_adc_dev_disable_dma(void *dev);

/**
 * @brief ADC 设备控制
 * @param dev 设备指针
 * @param cmd 控制命令
 * @param args 命令参数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_adc_dev_control(void *dev, uint32_t cmd, void *args);

/**
 * @brief 注册 ADC 回调
 * @param dev 设备指针
 * @param callback 回调函数
 * @param arg 用户参数
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_adc_dev_register_callback(void *dev, 
                                       xy_adc_callback_t callback, 
                                       void *arg);

/**
 * @brief 获取 ADC 最大值
 * @param dev 设备指针
 * @return 最大转换值
 */
uint32_t xy_adc_dev_get_max_value(void *dev);

/**
 * @brief ADC 值转电压
 * @param dev 设备指针
 * @param value ADC 值
 * @param vref 参考电压 (mV)
 * @return 电压值 (mV)
 */
uint32_t xy_adc_dev_value_to_mv(void *dev, uint32_t value, uint32_t vref);

/**
 * @brief ADC 设备校准
 * @param dev 设备指针
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_adc_dev_calibrate(void *dev);

/**
 * @brief 获取校准值
 * @param dev 设备指针
 * @param cal_data 校准数据输出
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_adc_dev_get_calibration(void *dev, void *cal_data);

/**
 * @brief 设置校准值
 * @param dev 设备指针
 * @param cal_data 校准数据
 * @return XY_OK 成功，其他值失败
 */
xy_error_t xy_adc_dev_set_calibration(void *dev, const void *cal_data);

#ifdef __cplusplus
}
#endif

#endif /* XY_ADC_H */
