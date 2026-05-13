/**
 * @file sensor_fsr.h
 * @brief FSR (Force Sensitive Resistor) 压力传感器驱动
 */
#ifndef __SENSOR_FSR_H__
#define __SENSOR_FSR_H__

#include "sensor_core.h"

typedef struct {
    uint8_t adc_pin;
} fsr_priv_t;

/**
 * @brief Create FSR sensor instance
 * @param name Sensor name
 * @param adc_pin ADC pin number
 * @return sensor_device_t* sensor handle
 */
sensor_device_t *fsr_create(const char *name, uint8_t adc_pin);

#endif
