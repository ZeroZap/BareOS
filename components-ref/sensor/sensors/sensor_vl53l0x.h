#ifndef __SENSOR_VL53L0X_H__
#define __SENSOR_VL53L0X_H__

#include "sensor_core.h"

/* VL53L0X I2C地址 */
#define VL53L0X_ADDR_DEFAULT 0x29

/* 寄存器定义 */
#define VL53L0X_REG_IDENTIFICATION_MODEL_ID    0xC0
#define VL53L0X_REG_IDENTIFICATION_REVISION_ID 0xC2
#define VL53L0X_REG_SYSRANGE_START             0x00
#define VL53L0X_REG_RESULT_RANGE_STATUS        0x14

/* 私有数据 */
typedef struct {
    uint8_t i2c_addr;
    uint8_t measurement_timing_budget_us;
} vl53l0x_priv_t;

sensor_device_t *vl53l0x_create(const char *name, void *i2c_bus);

#endif /* __SENSOR_VL53L0X_H__ */