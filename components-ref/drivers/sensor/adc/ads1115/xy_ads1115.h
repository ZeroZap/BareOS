/**
 * @file xy_ads1115.h
 * @brief ADS1115 16-bit ADC Device Driver
 * @version 1.0.0
 * @date 2026-02-28
 */

#ifndef XY_ADS1115_H
#define XY_ADS1115_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_device.h"
#include <stdint.h>

/**
 * @brief ADS1115 设备结构
 */
typedef struct {
    xy_i2c_device_t i2c_dev;
    int16_t adc_value[4];
    float voltage[4];
    float vref;
} xy_ads1115_t;

/**
 * @brief 初始化 ADS1115
 * @param ads ADS1115 设备
 * @param i2c_handle I2C 句柄
 * @param vref 参考电压
 * @return XY_DEVICE_OK 成功
 */
int xy_ads1115_init(xy_ads1115_t *ads, void *i2c_handle, float vref);

/**
 * @brief 读取所有通道
 * @param ads ADS1115 设备
 * @return XY_DEVICE_OK 成功
 */
int xy_ads1115_read(xy_ads1115_t *ads);

/**
 * @brief 读取单通道
 * @param ads ADS1115 设备
 * @param channel 通道号 (0-3)
 * @return ADC 值
 */
int16_t xy_ads1115_read_channel(xy_ads1115_t *ads, uint8_t channel);

/**
 * @brief 获取电压值
 * @param ads ADS1115 设备
 * @param channel 通道号
 * @return 电压值 (V)
 */
float xy_ads1115_get_voltage(xy_ads1115_t *ads, uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif /* XY_ADS1115_H */
