/**
 * @file sensor_mq3.h
 * @brief MQ-3 气体传感器驱动
 */
#ifndef __SENSOR_MQ3_H__
#define __SENSOR_MQ3_H__

#include "sensor_core.h"

typedef struct {
    uint8_t adc_pin;
} mq3_priv_t;

/**
 * @brief Create MQ3 sensor instance
 * @param name Sensor name
 * @param adc_pin ADC pin number
 * @return sensor_device_t* sensor handle
 */
sensor_device_t *mq3_create(const char *name, uint8_t adc_pin);

#endif
