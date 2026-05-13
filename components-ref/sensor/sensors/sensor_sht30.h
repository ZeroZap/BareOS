/**
 * @file sensor_sht30.h
 * @brief Sensirion SHT30 温湿度传感器驱动
 */
#ifndef __SENSOR_SHT30_H__
#define __SENSOR_SHT30_H__

#include "sensor_core.h"

#define SHT30_ADDR_DEFAULT 0x44
#define SHT30_ADDR_ALT    0x45

typedef struct {
    uint8_t i2c_addr;
} sht30_priv_t;

sensor_device_t *sht30_create(const char *name, void *i2c_bus, uint8_t addr);

#endif
