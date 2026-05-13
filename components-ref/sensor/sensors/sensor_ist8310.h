/**
 * @file sensor_ist8310.h
 * @brief IST8310 3轴磁力计驱动 (国产)
 */
#ifndef __SENSOR_IST8310_H__
#define __SENSOR_IST8310_H__

#include "sensor_core.h"

#define IST8310_ADDR_DEFAULT 0x0C
#define IST8310_REG_WHOAMI   0x00
#define IST8310_REG_CTRL1    0x01
#define IST8310_REG_CTRL2    0x02
#define IST8310_REG_DATA    0x03
#define IST8310_WHOAMI_VALUE 0x10

typedef struct {
    uint8_t i2c_addr;
} ist8310_priv_t;

sensor_device_t *ist8310_create(const char *name, void *i2c_bus);

#endif
