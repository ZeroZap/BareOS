/**
 * @file xy_sht30.h
 * @brief SHT30 Temperature/Humidity Sensor Driver
 * @version 1.0.0
 * @date 2026-02-28
 */

#ifndef XY_SHT30_H
#define XY_SHT30_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_device.h"
#include <stdint.h>

/**
 * @brief SHT30 设备结构
 */
typedef struct {
    xy_i2c_device_t i2c_dev;
    int16_t temperature;    /* 温度 (0.01°C) */
    uint16_t humidity;      /* 湿度 (0.01%RH) */
} xy_sht30_t;

/**
 * @brief 初始化 SHT30
 * @param sht SHT30 设备
 * @param i2c_handle I2C 句柄
 * @return XY_DEVICE_OK 成功
 */
int xy_sht30_init(xy_sht30_t *sht, void *i2c_handle);

/**
 * @brief 读取温度和湿度
 * @param sht SHT30 设备
 * @return XY_DEVICE_OK 成功
 */
int xy_sht30_read(xy_sht30_t *sht);

/**
 * @brief 读取温度
 * @param sht SHT30 设备
 * @return 温度 (0.01°C)
 */
int16_t xy_sht30_read_temperature(xy_sht30_t *sht);

/**
 * @brief 读取湿度
 * @param sht SHT30 设备
 * @return 湿度 (0.01%RH)
 */
uint16_t xy_sht30_read_humidity(xy_sht30_t *sht);

#ifdef __cplusplus
}
#endif

#endif /* XY_SHT30_H */
