/**
 * @file xy_aht20.h
 * @brief AHT10/AHT15/AHT20/AHT21 Temperature & Humidity Sensor Driver
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#ifndef XY_AHT20_H
#define XY_AHT20_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_dev_i2c.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief AHT20 I2C 地址
 */
#define AHT20_ADDR              0x38

/**
 * @brief AHT20 命令
 */
#define AHT20_CMD_INIT          0xBE
#define AHT20_CMD_TRIGGER       0xAC
#define AHT20_CMD_RESET         0xBA

/**
 * @brief 错误码
 */
#define XY_AHT20_OK             0
#define XY_AHT20_ERROR          (-1)
#define XY_AHT20_INVALID_PARAM  (-2)
#define XY_AHT20_NOT_FOUND      (-3)
#define XY_AHT20_BUSY           (-4)

/**
 * @brief 传感器数据
 */
typedef struct {
    int16_t temperature;        /* 温度 (0.01°C) */
    uint16_t humidity;          /* 湿度 (0.01%RH) */
    uint32_t timestamp;         /* 时间戳 (ms) */
} xy_aht20_data_t;

/**
 * @brief AHT20 句柄
 */
typedef struct {
    xy_i2c_device_t i2c_dev;    /* I2C 设备 */
    uint8_t addr;               /* I2C 地址 */
    xy_aht20_data_t data;       /* 最新数据 */
    bool calibrated;            /* 已校准 */
    bool initialized;           /* 初始化标志 */
} xy_aht20_t;

/**
 * @brief 初始化 AHT20
 * @param aht20 AHT20 句柄
 * @param i2c_handle I2C 句柄
 * @return XY_AHT20_OK 成功，其他值失败
 */
int xy_aht20_init(xy_aht20_t *aht20, void *i2c_handle);

/**
 * @brief 反初始化
 * @param aht20 AHT20 句柄
 * @return XY_AHT20_OK 成功，其他值失败
 */
int xy_aht20_deinit(xy_aht20_t *aht20);

/**
 * @brief 读取温湿度
 * @param aht20 AHT20 句柄
 * @return XY_AHT20_OK 成功，其他值失败
 */
int xy_aht20_read(xy_aht20_t *aht20);

/**
 * @brief 读取温度
 * @param aht20 AHT20 句柄
 * @param temperature 温度指针 (0.01°C)
 * @return XY_AHT20_OK 成功，其他值失败
 */
int xy_aht20_get_temperature(xy_aht20_t *aht20, int16_t *temperature);

/**
 * @brief 读取湿度
 * @param aht20 AHT20 句柄
 * @param humidity 湿度指针 (0.01%RH)
 * @return XY_AHT20_OK 成功，其他值失败
 */
int xy_aht20_get_humidity(xy_aht20_t *aht20, uint16_t *humidity);

/**
 * @brief 软件复位
 * @param aht20 AHT20 句柄
 * @return XY_AHT20_OK 成功，其他值失败
 */
int xy_aht20_reset(xy_aht20_t *aht20);

#ifdef __cplusplus
}
#endif

#endif
