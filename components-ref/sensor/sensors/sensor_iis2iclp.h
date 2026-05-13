/**
 * @file sensor_iis2iclp.h
 * @brief ST IIS2ICLP 超低功耗3轴加速度计驱动
 */
#ifndef __SENSOR_IIS2ICLP_H__
#define __SENSOR_IIS2ICLP_H__

#include "sensor_core.h"

#define IIS2ICLP_ADDR_DEFAULT 0x23
#define IIS2ICLP_ADDR_ALT     0x23
#define IIS2ICLP_SPI_CS_NONE  0xFF

#define IIS2ICLP_REG_WHOAMI     0x0F
#define IIS2ICLP_REG_CTRL1      0x20
#define IIS2ICLP_REG_CTRL2      0x21
#define IIS2ICLP_REG_CTRL3      0x22
#define IIS2ICLP_REG_OUT_X_L    0x28
#define IIS2ICLP_WHOAMI_VALUE  0x6D

#define IIS2ICLP_RANGE_2G   0x00
#define IIS2ICLP_RANGE_4G   0x01
#define IIS2ICLP_RANGE_8G   0x02
#define IIS2ICLP_RANGE_16G  0x03

#define IIS2ICLP_RATE_OFF     0x00
#define IIS2ICLP_RATE_1_6HZ   0x01
#define IIS2ICLP_RATE_12_5HZ  0x02
#define IIS2ICLP_RATE_25HZ    0x03
#define IIS2ICLP_RATE_50HZ    0x04
#define IIS2ICLP_RATE_100HZ   0x05
#define IIS2ICLP_RATE_200HZ   0x06

typedef struct {
    uint8_t i2c_addr;
    uint8_t spi_cs;
    uint8_t range;
} iis2iclp_priv_t;

sensor_device_t *iis2iclp_create(const char *name, void *i2c_bus, uint8_t addr);
sensor_device_t *iis2iclp_create_spi(const char *name, void *spi_bus, uint8_t cs);
int iis2iclp_set_range(sensor_device_t *dev, uint8_t range);

#endif
