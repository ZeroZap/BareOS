#ifndef __SENSOR_QMC5883L_H__
#define __SENSOR_QMC5883L_H__

#include "sensor_core.h"

/* QMC5883L I2C地址 */
#define QMC5883L_ADDR_DEFAULT 0x0D

/* 寄存器定义 */
#define QMC5883L_REG_DATA_X_LSB 0x00
#define QMC5883L_REG_DATA_X_MSB 0x01
#define QMC5883L_REG_DATA_Y_LSB 0x02
#define QMC5883L_REG_DATA_Y_MSB 0x03
#define QMC5883L_REG_DATA_Z_LSB 0x04
#define QMC5883L_REG_DATA_Z_MSB 0x05
#define QMC5883L_REG_STATUS     0x06
#define QMC5883L_REG_TEMP_LSB   0x07
#define QMC5883L_REG_TEMP_MSB   0x08
#define QMC5883L_REG_CONTROL1   0x09
#define QMC5883L_REG_CONTROL2   0x0A
#define QMC5883L_REG_PERIOD     0x0B
#define QMC5883L_REG_CHIP_ID    0x0D

/* CHIP_ID值 */
#define QMC5883L_CHIP_ID 0xFF

/* 私有数据 */
typedef struct {
    uint8_t i2c_addr;
    uint8_t range;     /* 量程 (2/8 gauss) */
    int16_t offset[3]; /* 偏移校准 */
} qmc5883l_priv_t;

/* 创建传感器设备 */
sensor_device_t *qmc5883l_create(const char *name, void *i2c_bus);

#endif /* __SENSOR_QMC5883L_H__ */