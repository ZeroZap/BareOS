/**
 * @file xy_bmp280.h
 * @brief BMP280 Barometric Pressure Sensor Driver
 * @version 1.0.0
 * @date 2026-02-28
 */

#ifndef XY_BMP280_H
#define XY_BMP280_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_device.h"
#include <stdint.h>

/**
 * @brief BMP280 设备结构
 */
typedef struct {
    xy_i2c_device_t i2c_dev;
    int32_t temperature;    /* 温度 (0.01°C) */
    uint32_t pressure;      /* 压力 (Pa) */
    uint32_t calibration[8];
} xy_bmp280_t;

/**
 * @brief 初始化 BMP280
 * @param bmp BMP280 设备
 * @param i2c_handle I2C 句柄
 * @return XY_DEVICE_OK 成功
 */
int xy_bmp280_init(xy_bmp280_t *bmp, void *i2c_handle);

/**
 * @brief 读取温度和压力
 * @param bmp BMP280 设备
 * @return XY_DEVICE_OK 成功
 */
int xy_bmp280_read(xy_bmp280_t *bmp);

/**
 * @brief 读取温度
 * @param bmp BMP280 设备
 * @return 温度 (0.01°C)
 */
int32_t xy_bmp280_read_temperature(xy_bmp280_t *bmp);

/**
 * @brief 读取压力
 * @param bmp BMP280 设备
 * @return 压力 (Pa)
 */
uint32_t xy_bmp280_read_pressure(xy_bmp280_t *bmp);

#ifdef __cplusplus
}
#endif

#endif /* XY_BMP280_H */
