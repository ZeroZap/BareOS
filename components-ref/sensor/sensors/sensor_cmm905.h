/**
 * @file sensor_cmm905.h
 * @brief CMMLab MM905 3轴磁力计驱动 (国产)
 */
#ifndef __SENSOR_CMM905_H__
#define __SENSOR_CMM905_H__

#include "sensor_core.h"

#define CMM905_ADDR_DEFAULT 0x30
#define CMM905_REG_STATUS   0x02
#define CMM905_REG_DATA     0x04

typedef struct {
    uint8_t i2c_addr;
} cmm905_priv_t;

sensor_device_t *cmm905_create(const char *name, void *i2c_bus);

#endif
