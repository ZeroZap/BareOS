#ifndef __SENSOR_ICM20608_H__
#define __SENSOR_ICM20608_H__

#include "sensor_core.h"

/* ICM20608 SPI/I2C地址 */
#define ICM20608_ADDR_DEFAULT 0x68
#define ICM20608_ADDR_ALT     0x69

/* 寄存器定义 */
#define ICM20608_REG_WHOAMI        0x75
#define ICM20608_REG_PWR_MGMT_1    0x6B
#define ICM20608_REG_PWR_MGMT_2    0x6C
#define ICM20608_REG_CONFIG        0x1A
#define ICM20608_REG_GYRO_CONFIG   0x1B
#define ICM20608_REG_ACCEL_CONFIG  0x1C
#define ICM20608_REG_ACCEL_CONFIG2 0x1D
#define ICM20608_REG_ACCEL_XOUT_H  0x3B
#define ICM20608_REG_GYRO_XOUT_H   0x43
#define ICM20608_REG_TEMP_OUT_H    0x41

/* WHO_AM_I值 */
#define ICM20608_WHOAMI_VALUE 0xAF

/* 私有数据 */
typedef struct {
    uint8_t addr;        /* I2C地址或SPI片选 */
    uint8_t accel_range; /* 加速度量程 */
    uint8_t gyro_range;  /* 陀螺仪量程 */
    bool use_spi;        /* 是否使用SPI */
    int16_t accel_offset[3];
    int16_t gyro_offset[3];
} icm20608_priv_t;

/* 创建传感器设备 */
sensor_device_t *icm20608_create_accel(const char *name, void *bus,
                                       bool use_spi);
sensor_device_t *icm20608_create_gyro(const char *name, void *bus,
                                      bool use_spi);
sensor_device_t *icm20608_create_temp(const char *name, void *bus,
                                      bool use_spi);

#endif /* __SENSOR_ICM20608_H__ */