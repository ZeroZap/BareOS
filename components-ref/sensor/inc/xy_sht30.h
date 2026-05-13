/**
 * @file xy_sht30.h
 * @brief SHT30 Temperature & Humidity Sensor Driver
 * @version 1.0.0
 * @date 2026-03-01
 */

#ifndef XY_SHT30_H
#define XY_SHT30_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_dev_i2c.h"
#include <stdint.h>

/**
 * @brief SHT30 默认 I2C 地址
 */
#define SHT30_ADDR_DEFAULT      0x44
#define SHT30_ADDR_ALT          0x45

/**
 * @brief SHT30 命令
 */
#define SHT30_CMD_MEASURE_H     0x2400  /* 高重复性 */
#define SHT30_CMD_MEASURE_M     0x240B  /* 中重复性 */
#define SHT30_CMD_MEASURE_L     0x2416  /* 低重复性 */
#define SHT30_CMD_READ_STATUS   0xF32D
#define SHT30_CMD_CLEAR_STATUS  0x3041
#define SHT30_CMD_SOFT_RESET    0x30A2
#define SHT30_CMD_HEATER_ON     0x306D
#define SHT30_CMD_HEATER_OFF    0x3066

/**
 * @brief 错误码
 */
#define XY_SHT30_OK             0
#define XY_SHT30_ERROR          (-1)
#define XY_SHT30_INVALID_PARAM  (-2)
#define XY_SHT30_NOT_FOUND      (-3)
#define XY_SHT30_CRC_ERROR      (-4)

/**
 * @brief SHT30 设备结构
 */
typedef struct {
    xy_i2c_device_t i2c_dev;    /**< I2C 设备 */
    uint8_t addr;               /**< I2C 地址 */
    int16_t temperature;        /**< 温度 (0.01°C) */
    uint16_t humidity;          /**< 湿度 (0.01%RH) */
    uint8_t initialized;        /**< 初始化标志 */
} xy_sht30_t;

/**
 * @brief 初始化 SHT30
 * @param dev SHT30 设备句柄
 * @param i2c_handle I2C 句柄
 * @param addr I2C 地址 (0x44 或 0x45)
 * @return XY_SHT30_OK 成功，其他值失败
 */
int xy_sht30_init(xy_sht30_t *dev, void *i2c_handle, uint8_t addr);

/**
 * @brief 反初始化 SHT30
 * @param dev SHT30 设备句柄
 * @return XY_SHT30_OK 成功，其他值失败
 */
int xy_sht30_deinit(xy_sht30_t *dev);

/**
 * @brief 读取温湿度
 * @param dev SHT30 设备句柄
 * @return XY_SHT30_OK 成功，其他值失败
 */
int xy_sht30_read(xy_sht30_t *dev);

/**
 * @brief 读取温度
 * @param dev SHT30 设备句柄
 * @param temperature 温度指针 (0.01°C)
 * @return XY_SHT30_OK 成功，其他值失败
 */
int xy_sht30_read_temperature(xy_sht30_t *dev, int16_t *temperature);

/**
 * @brief 读取湿度
 * @param dev SHT30 设备句柄
 * @param humidity 湿度指针 (0.01%RH)
 * @return XY_SHT30_OK 成功，其他值失败
 */
int xy_sht30_read_humidity(xy_sht30_t *dev, uint16_t *humidity);

/**
 * @brief 软件复位
 * @param dev SHT30 设备句柄
 * @return XY_SHT30_OK 成功，其他值失败
 */
int xy_sht30_soft_reset(xy_sht30_t *dev);

/**
 * @brief 开启加热器
 * @param dev SHT30 设备句柄
 * @return XY_SHT30_OK 成功，其他值失败
 */
int xy_sht30_heater_on(xy_sht30_t *dev);

/**
 * @brief 关闭加热器
 * @param dev SHT30 设备句柄
 * @return XY_SHT30_OK 成功，其他值失败
 */
int xy_sht30_heater_off(xy_sht30_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* XY_SHT30_H */
