#ifndef __SENSOR_MLX90393_H__
#define __SENSOR_MLX90393_H__
#include "sensor_core.h"
#define MLX90393_ADDR 0x0C
typedef struct { uint8_t i2c_addr; } mlx90393_priv_t;
sensor_device_t *mlx90393_create(const char *name, void *i2c_bus);
#endif
