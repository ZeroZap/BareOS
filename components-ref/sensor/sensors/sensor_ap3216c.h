#ifndef __SENSOR_AP3216C_H__
#define __SENSOR_AP3216C_H__

#include "sensor_core.h"

/* AP3216C I2C地址 */
#define AP3216C_ADDR_DEFAULT 0x1E

/* 寄存器定义 */
#define AP3216C_REG_SYS_CONFIG 0x00
#define AP3216C_REG_INT_STATUS 0x01
#define AP3216C_REG_INT_CLEAR  0x02
#define AP3216C_REG_IR_DATA_L  0x0A
#define AP3216C_REG_IR_DATA_H  0x0B
#define AP3216C_REG_ALS_DATA_L 0x0C
#define AP3216C_REG_ALS_DATA_H 0x0D
#define AP3216C_REG_PS_DATA_L  0x0E
#define AP3216C_REG_PS_DATA_H  0x0F

/* 工作模式 */
#define AP3216C_MODE_POWER_DOWN 0x00
#define AP3216C_MODE_ALS        0x01
#define AP3216C_MODE_PS         0x02
#define AP3216C_MODE_ALS_PS     0x03
#define AP3216C_MODE_IR         0x04
#define AP3216C_MODE_ALS_IR     0x05
#define AP3216C_MODE_PS_IR      0x06
#define AP3216C_MODE_ALS_PS_IR  0x07

/* 私有数据 */
typedef struct {
    uint8_t i2c_addr;
    uint8_t mode;
} ap3216c_priv_t;

/* 创建传感器设备 */
sensor_device_t *ap3216c_create_light(const char *name, void *i2c_bus);
sensor_device_t *ap3216c_create_proximity(const char *name, void *i2c_bus);
sensor_device_t *ap3216c_create_ir(const char *name, void *i2c_bus);

#endif /* __SENSOR_AP3216C_H__ */