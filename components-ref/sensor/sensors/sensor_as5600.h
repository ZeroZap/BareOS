#ifndef __SENSOR_AS5600_H__
#define __SENSOR_AS5600_H__
#include "sensor_core.h"
#define AS5600_ADDR 0x36
typedef struct { uint8_t i2c_addr; } as5600_priv_t;
sensor_device_t *as5600_create(const char *name, void *i2c_bus);
#endif
