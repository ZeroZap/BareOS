#ifndef __SENSOR_AHT20_H__
#define __SENSOR_AHT20_H__

#include "sensor_core.h"

/* AHT20 I2C地址 */
#define AHT20_ADDR_DEFAULT 0x38

/* 命令定义 */
#define AHT20_CMD_INIT       0xBE
#define AHT20_CMD_TRIGGER    0xAC
#define AHT20_CMD_SOFT_RESET 0xBA

/* 私有数据 */
typedef struct {
    uint8_t i2c_addr;
    bool initialized;
} aht20_priv_t;

/* 创建传感器设备 */
sensor_device_t *aht20_create_temperature(const char *name, void *i2c_bus);
sensor_device_t *aht20_create_humidity(const char *name, void *i2c_bus);

#endif /* __SENSOR_AHT20_H__ */