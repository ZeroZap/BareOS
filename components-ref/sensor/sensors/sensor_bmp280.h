#ifndef __SENSOR_BMP280_H__
#define __SENSOR_BMP280_H__

#include "sensor_core.h"

/* BMP280 I2C地址 */
#define BMP280_ADDR_DEFAULT 0x76
#define BMP280_ADDR_ALT     0x77

/* 寄存器定义 */
#define BMP280_REG_CHIP_ID   0xD0
#define BMP280_REG_RESET     0xE0
#define BMP280_REG_STATUS    0xF3
#define BMP280_REG_CTRL_MEAS 0xF4
#define BMP280_REG_CONFIG    0xF5
#define BMP280_REG_PRESS_MSB 0xF7
#define BMP280_REG_CALIB00   0x88

/* CHIP_ID值 */
#define BMP280_CHIP_ID 0x58

/* 私有数据 */
typedef struct {
    uint8_t i2c_addr;
    /* 校准参数 */
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    int32_t t_fine;
} bmp280_priv_t;

/* 创建传感器设备 */
sensor_device_t *bmp280_create_pressure(const char *name, void *i2c_bus);
sensor_device_t *bmp280_create_temperature(const char *name, void *i2c_bus);

#endif /* __SENSOR_BMP280_H__ */