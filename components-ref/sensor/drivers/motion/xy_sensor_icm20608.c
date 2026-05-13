/**
 * @file xy_sensor_icm20608.c
 * @brief ICM20608 6-Axis Motion Tracking - Migrated
 * @version 2.0.0
 * @date 2026-03-05
 */

#include "xy_sensor.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ICM20608 寄存器定义 */
#define ICM20608_ADDR           0x68
#define ICM20608_REG_WHO_AM_I   0x75
#define ICM20608_REG_SMPLRT_DIV 0x19
#define ICM20608_REG_CONFIG     0x1A
#define ICM20608_REG_GYRO_CONFIG 0x1B
#define ICM20608_REG_ACCEL_CONFIG 0x1C
#define ICM20608_REG_PWR_MGMT_1 0x6B
#define ICM20608_REG_ACCEL_XOUT_H 0x3B

#define ICM20608_ID             0xAF

/* 私有数据 */
typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    xy_sensor_value_t accel_x, accel_y, accel_z;
    xy_sensor_value_t gyro_x, gyro_y, gyro_z;
    xy_sensor_value_t temp_data;
} icm20608_private_data_t;

/* 初始化 */
static int icm20608_init(xy_sensor_device_t *dev)
{
    icm20608_private_data_t *priv = (icm20608_private_data_t *)dev->data;
    
    /* 检查设备 ID */
    uint8_t id;
    if (xy_sensor_i2c_read_reg(&priv->bus, ICM20608_REG_WHO_AM_I, &id) != 0) {
        return -1;
    }
    
    if (id != ICM20608_ID) {
        xy_log_e("ICM20608: Wrong ID (0x%02X)\n", id);
        return -1;
    }
    
    /* 唤醒 */
    xy_sensor_i2c_write_reg(&priv->bus, ICM20608_REG_PWR_MGMT_1, 0x00);
    xy_os_delay(10);
    
    /* 复位 */
    xy_sensor_i2c_write_reg(&priv->bus, ICM20608_REG_PWR_MGMT_1, 0x80);
    xy_os_delay(100);
    
    /* 配置：采样率 1kHz, DLPF 184Hz */
    xy_sensor_i2c_write_reg(&priv->bus, ICM20608_REG_SMPLRT_DIV, 0x07);
    xy_sensor_i2c_write_reg(&priv->bus, ICM20608_REG_CONFIG, 0x03);
    
    /* 配置量程：±2g, ±250dps */
    xy_sensor_i2c_write_reg(&priv->bus, ICM20608_REG_ACCEL_CONFIG, 0x00);
    xy_sensor_i2c_write_reg(&priv->bus, ICM20608_REG_GYRO_CONFIG, 0x00);
    
    priv->initialized = true;
    xy_log_i("ICM20608 initialized\n");
    return 0;
}

/* 采样获取 */
static int icm20608_sample_fetch(xy_sensor_device_t *dev, xy_sensor_channel_t channel)
{
    icm20608_private_data_t *priv = (icm20608_private_data_t *)dev->data;
    
    /* 读取 14 字节数据 */
    uint8_t buf[14];
    if (xy_sensor_i2c_read(&priv->bus, ICM20608_REG_ACCEL_XOUT_H, buf, 14) != 0) {
        return -1;
    }
    
    /* 解析加速度计 */
    int16_t accel_x_raw = (buf[0] << 8) | buf[1];
    int16_t accel_y_raw = (buf[2] << 8) | buf[3];
    int16_t accel_z_raw = (buf[4] << 8) | buf[5];
    
    /* 解析陀螺仪 */
    int16_t gyro_x_raw = (buf[8] << 8) | buf[9];
    int16_t gyro_y_raw = (buf[10] << 8) | buf[11];
    int16_t gyro_z_raw = (buf[12] << 8) | buf[13];
    
    /* 转换 */
    float accel_scale = 9.80665f / 16384.0f;
    float gyro_scale = 250.0f / 131.0f;
    
    float accel_x = accel_x_raw * accel_scale;
    float accel_y = accel_y_raw * accel_scale;
    float accel_z = accel_z_raw * accel_scale;
    
    float gyro_x = gyro_x_raw * gyro_scale;
    float gyro_y = gyro_y_raw * gyro_scale;
    float gyro_z = gyro_z_raw * gyro_scale;
    
    /* 温度 */
    int16_t temp_raw = (buf[6] << 8) | buf[7];
    float temperature = temp_raw / 326.8f + 25.0f;
    
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->accel_x, accel_x * 1000);
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->accel_y, accel_y * 1000);
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->accel_z, accel_z * 1000);
    
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->gyro_x, gyro_x);
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->gyro_y, gyro_y);
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->gyro_z, gyro_z);
    
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->temp_data, temperature);
    
    return 0;
}

/* 通道数据获取 */
static int icm20608_channel_get(xy_sensor_device_t *dev, 
                                xy_sensor_channel_t channel, 
                                xy_sensor_value_t *val)
{
    icm20608_private_data_t *priv = (icm20608_private_data_t *)dev->data;
    if (!val) return -1;
    
    switch (channel) {
        case XY_SENSOR_CHAN_ACCEL_X: *val = priv->accel_x; break;
        case XY_SENSOR_CHAN_ACCEL_Y: *val = priv->accel_y; break;
        case XY_SENSOR_CHAN_ACCEL_Z: *val = priv->accel_z; break;
        case XY_SENSOR_CHAN_GYRO_X: *val = priv->gyro_x; break;
        case XY_SENSOR_CHAN_GYRO_Y: *val = priv->gyro_y; break;
        case XY_SENSOR_CHAN_GYRO_Z: *val = priv->gyro_z; break;
        case XY_SENSOR_CHAN_DIE_TEMP: *val = priv->temp_data; break;
        default: return -1;
    }
    return 0;
}

/* 驱动 API */
static const xy_sensor_driver_api_t icm20608_driver_api = {
    .init = icm20608_init,
    .sample_fetch = icm20608_sample_fetch,
    .channel_get = icm20608_channel_get,
};

static icm20608_private_data_t icm20608_priv;
static xy_sensor_device_t icm20608_device = {
    .name = "ICM20608",
    .type = XY_SENSOR_TYPE_ACCEL,
    .api = &icm20608_driver_api,
    .data = &icm20608_priv,
};

int xy_sensor_icm20608_register(void *i2c_handle, uint8_t addr)
{
    xy_sensor_bus_config_i2c(&icm20608_priv.bus, i2c_handle, addr ? addr : ICM20608_ADDR);
    return xy_sensor_device_register(&icm20608_device);
}
