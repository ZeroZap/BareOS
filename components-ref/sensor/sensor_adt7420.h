#ifndef __SENSOR_ADT7420_H__
#define __SENSOR_ADT7420_H__

#include "sensor_core.h"

/* ADT7420 I2C地址 */
#define ADT7420_ADDR_DEFAULT 0x48
#define ADT7420_ADDR_ALT1    0x49
#define ADT7420_ADDR_ALT2    0x4A
#define ADT7420_ADDR_ALT3    0x4B

/* 寄存器定义 */
#define ADT7420_REG_TEMP_MSB 0x00
#define ADT7420_REG_TEMP_LSB 0x01
#define ADT7420_REG_STATUS   0x02
#define ADT7420_REG_CONFIG   0x03
#define ADT7420_REG_ID       0x0B
#define ADT7420_REG_RESET    0x2F

/* 配置位 */
#define ADT7420_CONFIG_RES_13BIT 0x00
#define ADT7420_CONFIG_RES_16BIT 0x80

/* 私有数据 */
typedef struct {
    uint8_t i2c_addr;
    uint8_t resolution; /* 13位或16位 */
} adt7420_priv_t;

sensor_device_t *adt7420_create(const char *name, void *i2c_bus);

#endif /* __SENSOR_ADT7420_H__ */