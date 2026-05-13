#ifndef __SENSOR_ADXL362_H__
#define __SENSOR_ADXL362_H__

#include "sensor_core.h"

/* ADXL362 SPI寄存器 */
#define ADXL362_REG_DEVID_AD   0x00
#define ADXL362_REG_DEVID_MST  0x01
#define ADXL362_REG_PARTID     0x02
#define ADXL362_REG_STATUS     0x0B
#define ADXL362_REG_XDATA      0x0E
#define ADXL362_REG_POWER_CTL  0x2D
#define ADXL362_REG_FILTER_CTL 0x2C

/* SPI命令 */
#define ADXL362_CMD_WRITE 0x0A
#define ADXL362_CMD_READ  0x0B
#define ADXL362_CMD_FIFO  0x0D

/* ID值 */
#define ADXL362_DEVID_AD  0xAD
#define ADXL362_DEVID_MST 0x1D
#define ADXL362_PARTID    0xF2

/* 功耗模式 */
typedef enum {
    ADXL362_MODE_STANDBY     = 0x00, /* 0.01μA */
    ADXL362_MODE_MEASUREMENT = 0x02, /* 1.8μA @ 100Hz */
} adxl362_mode_t;

/* 数据速率 */
typedef enum {
    ADXL362_ODR_12_5HZ = 0x00,
    ADXL362_ODR_25HZ   = 0x01,
    ADXL362_ODR_50HZ   = 0x02,
    ADXL362_ODR_100HZ  = 0x03,
    ADXL362_ODR_200HZ  = 0x04,
    ADXL362_ODR_400HZ  = 0x05,
} adxl362_odr_t;

/* 量程 */
typedef enum {
    ADXL362_RANGE_2G = 0x00,
    ADXL362_RANGE_4G = 0x01,
    ADXL362_RANGE_8G = 0x02,
} adxl362_range_t;

/* 私有数据 */
typedef struct {
    void *spi_bus;
    adxl362_mode_t mode;
    adxl362_odr_t odr;
    adxl362_range_t range;
    uint8_t range_g;
} adxl362_priv_t;

sensor_device_t *adxl362_create(const char *name, void *spi_bus);

#endif /* __SENSOR_ADXL362_H__ */