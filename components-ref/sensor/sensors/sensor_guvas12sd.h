/**
 * @file sensor_guvas12sd.h
 * @brief 国产 GUVA-S12SD 紫外线传感器驱动
 */
#ifndef __SENSOR_GUVAS12SD_H__
#define __SENSOR_GUVAS12SD_H__

#include "sensor_core.h"

typedef struct {
    uint8_t adc_pin;
} guvas12sd_priv_t;

sensor_device_t *guvas12sd_create(const char *name, uint8_t adc_pin);

#endif
