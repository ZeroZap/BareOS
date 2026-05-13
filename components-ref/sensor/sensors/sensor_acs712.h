#ifndef __SENSOR_ACS712_H__
#define __SENSOR_ACS712_H__
#include "sensor_core.h"
typedef struct { uint8_t adc_pin; } acs712_priv_t;
sensor_device_t *acs712_create(const char *name, uint8_t adc_pin);
#endif
