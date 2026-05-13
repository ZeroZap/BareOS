/**
 * @file xy_hdc1080.h
 * @brief HDC1080 Temperature & Humidity Sensor Driver
 * @version 1.0.0
 * @date 2026-03-01 自主任务
 */

#ifndef XY_HDC1080_H
#define XY_HDC1080_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_dev_i2c.h"
#include <stdint.h>

/**
 * @brief HDC1080 I2C 地址
 */
#define HDC1080_ADDR            0x40

/**
 * @brief HDC1080 寄存器
 */
#define HDC1080_REG_TEMP        0x00
#define HDC1080_REG_HUMI        0x01
#define HDC1080_REG_CONFIG      0x02
#define HDC1080_REG_SERIAL1     0xFB
#define HDC1080_REG_SERIAL2     0xFC
#define HDC1080_REG_SERIAL3     0xFD

/**
 * @brief 配置寄存器位
 */
#define HDC1080_CONFIG_RST      (1 << 15)
#define HDC1080_CONFIG_HEATER   (1 << 13)
#define HDC1080_CONFIG_MODE     (1 << 12)
#define HDC1080_CONFIG_TRES_14  (0 << 10)
#define HDC1080_CONFIG_TRES_11  (1 << 10)
#define HDC1080_CONFIG_HRES_14  (0 << 8)
#define HDC1080_CONFIG_HRES_11  (1 << 8)
#define HDC1080_CONFIG_HRES_8   (2 << 8)

/**
 * @brief 错误码
 */
#define XY_HDC1080_OK           0
#define XY_HDC1080_ERROR        (-1)
#define XY_HDC1080_INVALID_PARAM (-2)
#define XY_HDC1080_NOT_FOUND    (-3)

/**
 * @brief HDC1080 设备结构
 */
typedef struct {
    xy_i2c_device_t i2c_dev;
    uint8_t addr;
    int16_t temperature;    /* 0.01°C */
    uint16_t humidity;      /* 0.01%RH */
    uint8_t initialized;
} xy_hdc1080_t;

/**
 * @brief 初始化 HDC1080
 */
int xy_hdc1080_init(xy_hdc1080_t *dev, void *i2c_handle, uint8_t addr);

/**
 * @brief 反初始化
 */
int xy_hdc1080_deinit(xy_hdc1080_t *dev);

/**
 * @brief 读取温湿度
 */
int xy_hdc1080_read(xy_hdc1080_t *dev);

/**
 * @brief 读取温度
 */
int xy_hdc1080_read_temperature(xy_hdc1080_t *dev, int16_t *temp);

/**
 * @brief 读取湿度
 */
int xy_hdc1080_read_humidity(xy_hdc1080_t *dev, uint16_t *humi);

/**
 * @brief 开启加热器 (防结露)
 */
int xy_hdc1080_heater_on(xy_hdc1080_t *dev);

/**
 * @brief 关闭加热器
 */
int xy_hdc1080_heater_off(xy_hdc1080_t *dev);

#ifdef __cplusplus
}
#endif

#endif
