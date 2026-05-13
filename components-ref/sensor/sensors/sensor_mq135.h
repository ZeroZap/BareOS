/**
 * @file sensor_mq135.h
 * @brief MQ-135 气体传感器驱动
 */
#ifndef __SENSOR_MQ135_H__
#define __SENSOR_MQ135_H__

#include "sensor_core.h"

typedef struct {
    uint8_t adc_pin;
} mq135_priv_t;

/**
 * @brief Create MQ135 sensor instance
 * @param name Sensor name
 * @param adc_pin ADC pin number
 * @return sensor_device_t* sensor handle
 */
sensor_device_t *mq135_create(const char *name, uint8_t adc_pin);

#endif
