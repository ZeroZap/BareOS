/**
 * @file sensor_qma6100.h
 * @brief QMA6100 3轴加速度计驱动 (台湾QST/全志旗下)
 */
#ifndef __SENSOR_QMA6100_H__
#define __SENSOR_QMA6100_H__

#include "sensor_core.h"

#define QMA6100_ADDR_DEFAULT 0x12
#define QMA6100_ADDR_ALT     0x13
#define QMA6100_SPI_CS_NONE  0xFF

#define QMA6100_REG_WHOAMI   0x00
#define QMA6100_REG_DSET     0x01
#define QMA6100_REG_CTRL     0x0C
#define QMA6100_REG_BW       0x0D
#define QMA6100_REG_PWRCTL   0x11
#define QMA6100_REG_DATA     0x18

#define QMA6100_WHOAMI_VALUE 0x61

#define QMA6100_RANGE_2G   0x00
#define QMA6100_RANGE_4G   0x01
#define QMA6100_RANGE_8G   0x02
#define QMA6100_RANGE_16G  0x03

#define QMA6100_RATE_OFF   0x00
#define QMA6100_RATE_1_56HZ  0x01
#define QMA6100_RATE_6_25HZ  0x02
#define QMA6100_RATE_12_5HZ  0x03
#define QMA6100_RATE_25HZ   0x04
#define QMA6100_RATE_50HZ   0x05
#define QMA6100_RATE_100HZ  0x06
#define QMA6100_RATE_200HZ  0x07
#define QMA6100_RATE_400HZ  0x08
#define QMA6100_RATE_800HZ  0x09

typedef struct {
    uint8_t i2c_addr;
    uint8_t range;
    uint8_t rate;
} qma6100_priv_t;

sensor_device_t *qma6100_create(const char *name, void *i2c_bus, uint8_t addr);
int qma6100_set_range(sensor_device_t *dev, uint8_t range);

#endif
