/**
 * @file xy_sht40.h
 * @brief SHT40/SHT41/SHT45 Temperature & Humidity Sensor Driver
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#ifndef XY_SHT40_H
#define XY_SHT40_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_dev_i2c.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief SHT40 I2C 地址
 */
#define SHT40_ADDR              0x44

/**
 * @brief SHT40 命令
 */
#define SHT40_CMD_READ_SERIAL   0x89
#define SHT40_CMD_MEASURE_HPM   0xFD  /* 高精度模式 */
#define SHT40_CMD_MEASURE_HPL   0xF6  /* 高精度低功耗 */
#define SHT40_CMD_MEASURE_MPM   0x2F  /* 中精度模式 */
#define SHT40_CMD_MEASURE_MPL   0x24  /* 中精度低功耗 */
#define SHT40_CMD_MEASURE_LPM   0x1D  /* 低精度模式 */
#define SHT40_CMD_MEASURE_LPL   0x15  /* 低精度低功耗 */

/**
 * @brief 错误码
 */
#define XY_SHT40_OK             0
#define XY_SHT40_ERROR          (-1)
#define XY_SHT40_INVALID_PARAM  (-2)
#define XY_SHT40_NOT_FOUND      (-3)
#define XY_SHT40_CRC_ERROR      (-4)

/**
 * @brief 精度模式
 */
typedef enum {
    XY_SHT40_HIGH_PRECISION = 0,
    XY_SHT40_MEDIUM_PRECISION,
    XY_SHT40_LOW_PRECISION,
} xy_sht40_precision_t;

/**
 * @brief 传感器数据
 */
typedef struct {
    int16_t temperature;        /* 温度 (0.01°C) */
    uint16_t humidity;          /* 湿度 (0.01%RH) */
    uint32_t serial[2];         /* 序列号 */
    uint32_t timestamp;         /* 时间戳 (ms) */
} xy_sht40_data_t;

/**
 * @brief SHT40 句柄
 */
typedef struct {
    xy_i2c_device_t i2c_dev;    /* I2C 设备 */
    uint8_t addr;               /* I2C 地址 */
    xy_sht40_precision_t precision;  /* 精度模式 */
    xy_sht40_data_t data;       /* 最新数据 */
    bool initialized;           /* 初始化标志 */
} xy_sht40_t;

/**
 * @brief 初始化 SHT40
 * @param sht40 SHT40 句柄
 * @param i2c_handle I2C 句柄
 * @return XY_SHT40_OK 成功，其他值失败
 */
int xy_sht40_init(xy_sht40_t *sht40, void *i2c_handle);

/**
 * @brief 反初始化
 * @param sht40 SHT40 句柄
 * @return XY_SHT40_OK 成功，其他值失败
 */
int xy_sht40_deinit(xy_sht40_t *sht40);

/**
 * @brief 读取温湿度
 * @param sht40 SHT40 句柄
 * @return XY_SHT40_OK 成功，其他值失败
 */
int xy_sht40_read(xy_sht40_t *sht40);

/**
 * @brief 读取温度
 * @param sht40 SHT40 句柄
 * @param temperature 温度指针 (0.01°C)
 * @return XY_SHT40_OK 成功，其他值失败
 */
int xy_sht40_get_temperature(xy_sht40_t *sht40, int16_t *temperature);

/**
 * @brief 读取湿度
 * @param sht40 SHT40 句柄
 * @param humidity 湿度指针 (0.01%RH)
 * @return XY_SHT40_OK 成功，其他值失败
 */
int xy_sht40_get_humidity(xy_sht40_t *sht40, uint16_t *humidity);

/**
 * @brief 读取序列号
 * @param sht40 SHT40 句柄
 * @param serial 序列号数组
 * @return XY_SHT40_OK 成功，其他值失败
 */
int xy_sht40_get_serial(xy_sht40_t *sht40, uint32_t *serial);

/**
 * @brief 设置精度模式
 * @param sht40 SHT40 句柄
 * @param precision 精度模式
 * @return XY_SHT40_OK 成功，其他值失败
 */
int xy_sht40_set_precision(xy_sht40_t *sht40, xy_sht40_precision_t precision);

#ifdef __cplusplus
}
#endif

#endif
