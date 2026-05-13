/**
 * @file xy_adc.h
 * @brief ADC/DAC Helper Library
 * @version 1.0.0
 * @date 2026-02-28
 */

#ifndef XY_ADC_H
#define XY_ADC_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Configuration ==================== */

#ifndef ADC_MAX_CHANNELS
#define ADC_MAX_CHANNELS 16
#endif

#ifndef DAC_MAX_CHANNELS
#define DAC_MAX_CHANNELS 2
#endif

/* ==================== Error Codes ==================== */

#define XY_ADC_OK               0
#define XY_ADC_ERROR            (-1)
#define XY_ADC_INVALID_PARAM    (-2)
#define XY_ADC_NOT_READY        (-3)

/* ==================== ADC Data Structures ==================== */

/**
 * @brief ADC 分辨率
 */
typedef enum {
    ADC_RESOLUTION_8BIT = 8,
    ADC_RESOLUTION_10BIT = 10,
    ADC_RESOLUTION_12BIT = 12,
    ADC_RESOLUTION_14BIT = 14,
    ADC_RESOLUTION_16BIT = 16,
} xy_adc_resolution_t;

/**
 * @brief ADC 参考电压
 */
typedef enum {
    ADC_REF_INTERNAL = 0,    /* 内部参考 */
    ADC_REF_EXTERNAL,        /* 外部参考 */
    ADC_REF_VDDA,            /* VDDA */
} xy_adc_ref_t;

/**
 * @brief ADC 通道配置
 */
typedef struct {
    uint8_t channel;             /* 通道号 */
    xy_adc_resolution_t res;     /* 分辨率 */
    bool enabled;                /* 使能 */
} xy_adc_channel_t;

/**
 * @brief ADC 设备结构
 */
typedef struct {
    uint8_t id;                  /* 设备 ID */
    xy_adc_ref_t ref;            /* 参考电压 */
    uint32_t ref_voltage_mv;     /* 参考电压 (mV) */
    xy_adc_resolution_t res;     /* 分辨率 */
    xy_adc_channel_t channels[ADC_MAX_CHANNELS];
    uint8_t channel_count;
    bool initialized;
} xy_adc_t;

/**
 * @brief ADC 采样结果
 */
typedef struct {
    uint8_t channel;
    uint32_t raw_value;
    uint32_t voltage_mv;
    bool valid;
} xy_adc_sample_t;

/* ==================== DAC Data Structures ==================== */

/**
 * @brief DAC 分辨率
 */
typedef enum {
    DAC_RESOLUTION_8BIT = 8,
    DAC_RESOLUTION_10BIT = 10,
    DAC_RESOLUTION_12BIT = 12,
} xy_dac_resolution_t;

/**
 * @brief DAC 通道配置
 */
typedef struct {
    uint8_t channel;
    xy_dac_resolution_t res;
    bool enabled;
} xy_dac_channel_t;

/**
 * @brief DAC 设备结构
 */
typedef struct {
    uint8_t id;
    xy_dac_resolution_t res;
    uint32_t ref_voltage_mv;
    xy_dac_channel_t channels[DAC_MAX_CHANNELS];
    uint8_t channel_count;
    bool initialized;
} xy_dac_t;

/* ==================== ADC Operations ==================== */

/**
 * @brief 初始化 ADC
 * @param adc ADC 句柄
 * @param ref_voltage_mv 参考电压 (mV)
 * @param resolution 分辨率
 * @return XY_ADC_OK 成功
 */
int xy_adc_init(xy_adc_t *adc, uint32_t ref_voltage_mv, xy_adc_resolution_t resolution);

/**
 * @brief 反初始化 ADC
 * @param adc ADC 句柄
 * @return XY_ADC_OK 成功
 */
int xy_adc_deinit(xy_adc_t *adc);

/**
 * @brief 配置 ADC 通道
 * @param adc ADC 句柄
 * @param channel 通道号
 * @param enabled 使能
 * @return XY_ADC_OK 成功
 */
int xy_adc_config_channel(xy_adc_t *adc, uint8_t channel, bool enabled);

/**
 * @brief 单次采样
 * @param adc ADC 句柄
 * @param channel 通道号
 * @param sample 采样结果
 * @return XY_ADC_OK 成功
 */
int xy_adc_sample(xy_adc_t *adc, uint8_t channel, xy_adc_sample_t *sample);

/**
 * @brief 多通道采样
 * @param adc ADC 句柄
 * @param samples 采样结果数组
 * @param count 通道数量
 * @return 采样成功数量
 */
int xy_adc_sample_multi(xy_adc_t *adc, xy_adc_sample_t *samples, uint8_t count);

/**
 * @brief 原始值转电压
 * @param adc ADC 句柄
 * @param raw_value 原始值
 * @return 电压值 (mV)
 */
uint32_t xy_adc_raw_to_voltage(xy_adc_t *adc, uint32_t raw_value);

/**
 * @brief 电压转原始值
 * @param adc ADC 句柄
 * @param voltage_mv 电压 (mV)
 * @return 原始值
 */
uint32_t xy_adc_voltage_to_raw(xy_adc_t *adc, uint32_t voltage_mv);

/* ==================== DAC Operations ==================== */

/**
 * @brief 初始化 DAC
 * @param dac DAC 句柄
 * @param ref_voltage_mv 参考电压 (mV)
 * @param resolution 分辨率
 * @return XY_ADC_OK 成功
 */
int xy_dac_init(xy_dac_t *dac, uint32_t ref_voltage_mv, xy_dac_resolution_t resolution);

/**
 * @brief 反初始化 DAC
 * @param dac DAC 句柄
 * @return XY_ADC_OK 成功
 */
int xy_dac_deinit(xy_dac_t *dac);

/**
 * @brief 配置 DAC 通道
 * @param dac DAC 句柄
 * @param channel 通道号
 * @param enabled 使能
 * @return XY_ADC_OK 成功
 */
int xy_dac_config_channel(xy_dac_t *dac, uint8_t channel, bool enabled);

/**
 * @brief 设置 DAC 输出电压
 * @param dac DAC 句柄
 * @param channel 通道号
 * @param voltage_mv 电压 (mV)
 * @return XY_ADC_OK 成功
 */
int xy_dac_set_voltage(xy_dac_t *dac, uint8_t channel, uint32_t voltage_mv);

/**
 * @brief 设置 DAC 原始值
 * @param dac DAC 句柄
 * @param channel 通道号
 * @param raw_value 原始值
 * @return XY_ADC_OK 成功
 */
int xy_dac_set_raw(xy_dac_t *dac, uint8_t channel, uint32_t raw_value);

/**
 * @brief 获取 DAC 输出电压
 * @param dac DAC 句柄
 * @param channel 通道号
 * @return 电压 (mV)
 */
uint32_t xy_dac_get_voltage(xy_dac_t *dac, uint8_t channel);

/* ==================== Helper Macros ==================== */

#define ADC_MAX_VALUE(res) ((1UL << (res)) - 1)
#define DAC_MAX_VALUE(res) ((1UL << (res)) - 1)

#ifdef __cplusplus
}
#endif

#endif /* XY_ADC_H */
