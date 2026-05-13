/**
 * @file sensor_mq7.h
 * @brief MQ7 气体传感器驱动
 */
#ifndef __SENSOR_MQ7_H__
#define __SENSOR_MQ7_H__

#include "sensor_core.h"

typedef struct {
    uint8_t adc_pin;
} mq7_priv_t;

sensor_device_t *mq7_create(const char *name, uint8_t adc_pin);

#endif
