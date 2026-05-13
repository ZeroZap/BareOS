#ifndef __SENSOR_AEAT8800_H__
#define __SENSOR_AEAT8800_H__
#include "sensor_core.h"
typedef struct { uint8_t spi_bus; } aeat8800_priv_t;
sensor_device_t *aeat8800_create(const char *name, void *spi_bus);
#endif
