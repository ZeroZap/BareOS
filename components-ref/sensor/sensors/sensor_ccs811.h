#ifndef __SENSOR_CCS811_H__
#define __SENSOR_CCS811_H__

#include "sensor_core.h"

/* CCS811 I2C地址 */
#define CCS811_ADDR_DEFAULT 0x5A
#define CCS811_ADDR_ALT     0x5B

/* 寄存器定义 */
#define CCS811_REG_STATUS     0x00
#define CCS811_REG_MEAS_MODE  0x01
#define CCS811_REG_ALG_RESULT 0x02
#define CCS811_REG_HW_ID      0x20
#define CCS811_REG_APP_START  0xF4

/* HW_ID */
#define CCS811_HW_ID 0x81

/* 私有数据 */
typedef struct {
    uint8_t i2c_addr;
    uint16_t eco2; /* CO2等效浓度 (ppm) */
    uint16_t tvoc; /* TVOC浓度 (ppb) */
} ccs811_priv_t;

sensor_device_t *ccs811_create_co2(const char *name, void *i2c_bus);
sensor_device_t *ccs811_create_tvoc(const char *name, void *i2c_bus);

#endif /* __SENSOR_CCS811_H__ */