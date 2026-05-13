/**
 * @file sensor_mg811.h
 * @brief MG811 CO2 传感器驱动
 */
#ifndef __SENSOR_MG811_H__
#define __SENSOR_MG811_H__

#include "sensor_core.h"

typedef struct {
    uint8_t adc_pin;
} mg811_priv_t;

sensor_device_t *mg811_create(const char *name, uint8_t adc_pin);

#endif
