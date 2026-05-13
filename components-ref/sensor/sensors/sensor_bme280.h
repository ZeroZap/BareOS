#ifndef __SENSOR_BME280_H__
#define __SENSOR_BME280_H__

#include "sensor_core.h"

/* BME280 I2C地址 */
#define BME280_ADDR_DEFAULT 0x76
#define BME280_ADDR_ALT     0x77

/* 寄存器定义 */
#define BME280_REG_CHIP_ID   0xD0
#define BME280_REG_RESET     0xE0
#define BME280_REG_CTRL_HUM  0xF2
#define BME280_REG_STATUS    0xF3
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_CONFIG    0xF5
#define BME280_REG_PRESS_MSB 0xF7
#define BME280_REG_CALIB00   0x88
#define BME280_REG_CALIB26   0xE1

/* CHIP_ID */
#define BME280_CHIP_ID 0x60

/* 校准参数 */
typedef struct {
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
    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
} bme280_calib_t;

/* 私有数据 */
typedef struct {
    uint8_t i2c_addr;
    bme280_calib_t calib;
    int32_t t_fine;
} bme280_priv_t;

sensor_device_t *bme280_create_temperature(const char *name, void *i2c_bus);
sensor_device_t *bme280_create_pressure(const char *name, void *i2c_bus);
sensor_device_t *bme280_create_humidity(const char *name, void *i2c_bus);

#endif /* __SENSOR_BME280_H__ */