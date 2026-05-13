#ifndef __SENSOR_AS5048_H__
#define __SENSOR_AS5048_H__
#include "sensor_core.h"
#define AS5048_ADDR 0x40
typedef struct { uint8_t i2c_addr; } as5048_priv_t;
sensor_device_t *as5048_create(const char *name, void *i2c_bus);
#endif
