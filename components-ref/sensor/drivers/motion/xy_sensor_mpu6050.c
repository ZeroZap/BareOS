/**
 * @file xy_sensor_mpu6050.c
 * @brief MPU6050 6-Axis Motion Tracking Device - Migrated to New Framework
 * @version 2.0.0
 * @date 2026-03-05
 */

#include "xy_sensor.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ==================== MPU6050 寄存器定义 ==================== */

#define MPU6050_ADDR            0x68
#define MPU6050_REG_SMPLRT_DIV  0x19
#define MPU6050_REG_CONFIG      0x1A
#define MPU6050_REG_GYRO_CONFIG 0x1B
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_PWR_MGMT_1  0x6B
#define MPU6050_REG_PWR_MGMT_2  0x6C
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_WHO_AM_I    0x75
#define MPU6050_ID              0x68

#define MPU6050_SOFT_RESET      0x80

/* 量程配置 */
#define ACCEL_RANGE_2G          0x00
#define ACCEL_RANGE_4G          0x08
#define ACCEL_RANGE_8G          0x10
#define ACCEL_RANGE_16G         0x18

#define GYRO_RANGE_250          0x00
#define GYRO_RANGE_500          0x08
#define GYRO_RANGE_1000         0x10
#define GYRO_RANGE_2000         0x18

/* ==================== 私有数据结构 ==================== */

typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    uint8_t accel_range;
    uint8_t gyro_range;
    xy_sensor_value_t accel_x, accel_y, accel_z;
    xy_sensor_value_t gyro_x, gyro_y, gyro_z;
    xy_sensor_value_t temp_data;
} mpu6050_private_data_t;

/* ==================== 驱动实现 ==================== */

/**
 * @brief MPU6050 初始化
 */
static int mpu6050_init(xy_sensor_device_t *dev)
{
    mpu6050_private_data_t *priv = (mpu6050_private_data_t *)dev->data;
    
    /* 检查设备 ID */
    uint8_t id;
    if (xy_sensor_i2c_read_reg(&priv->bus, MPU6050_REG_WHO_AM_I, &id) != 0) {
        xy_log_e("MPU6050: Failed to read ID\n");
        return -1;
    }
    
    if (id != MPU6050_ID) {
        xy_log_e("MPU6050: Wrong ID (0x%02X)\n", id);
        return -1;
    }
    
    /* 唤醒设备 */
    xy_sensor_i2c_write_reg(&priv->bus, MPU6050_REG_PWR_MGMT_1, 0x00);
    xy_os_delay(10);
    
    /* 软件复位 */
    xy_sensor_i2c_write_reg(&priv->bus, MPU6050_REG_PWR_MGMT_1, MPU6050_SOFT_RESET);
    xy_os_delay(100);
    
    /* 配置：采样率 1kHz, DLPF 184Hz */
    xy_sensor_i2c_write_reg(&priv->bus, MPU6050_REG_SMPLRT_DIV, 0x07);
    xy_sensor_i2c_write_reg(&priv->bus, MPU6050_REG_CONFIG, 0x03);
    
    /* 配置加速度计量程：±2g */
    priv->accel_range = ACCEL_RANGE_2G;
    xy_sensor_i2c_write_reg(&priv->bus, MPU6050_REG_ACCEL_CONFIG, priv->accel_range);
    
    /* 配置陀螺仪量程：±250dps */
    priv->gyro_range = GYRO_RANGE_250;
    xy_sensor_i2c_write_reg(&priv->bus, MPU6050_REG_GYRO_CONFIG, priv->gyro_range);
    
    priv->initialized = true;
    xy_log_i("MPU6050 initialized\n");
    return 0;
}

/**
 * @brief MPU6050 采样获取
 */
static int mpu6050_sample_fetch(xy_sensor_device_t *dev, xy_sensor_channel_t channel)
{
    mpu6050_private_data_t *priv = (mpu6050_private_data_t *)dev->data;
    
    if (!priv->initialized) {
        return -1;
    }
    
    /* 读取原始数据 (14 字节) */
    uint8_t buf[14];
    if (xy_sensor_i2c_read(&priv->bus, MPU6050_REG_ACCEL_XOUT_H, buf, 14) != 0) {
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
    
    /* 转换为实际值 */
    float accel_scale = 9.80665f / 16384.0f;  /* ±2g */
    float gyro_scale = 250.0f / 131.0f;        /* ±250dps */
    
    float accel_x = accel_x_raw * accel_scale;
    float accel_y = accel_y_raw * accel_scale;
    float accel_z = accel_z_raw * accel_scale;
    
    float gyro_x = gyro_x_raw * gyro_scale;
    float gyro_y = gyro_y_raw * gyro_scale;
    float gyro_z = gyro_z_raw * gyro_scale;
    
    /* 解析温度 */
    int16_t temp_raw = (buf[6] << 8) | buf[7];
    float temperature = temp_raw / 340.0f + 36.53f;
    
    /* 存储到缓存 */
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->accel_x, accel_x * 1000);  /* mG */
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->accel_y, accel_y * 1000);
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->accel_z, accel_z * 1000);
    
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->gyro_x, gyro_x);  /* mdps */
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->gyro_y, gyro_y);
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->gyro_z, gyro_z);
    
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->temp_data, temperature);
    
    xy_log_d("MPU6050: A(%.2f,%.2f,%.2f) G(%.2f,%.2f,%.2f)\n",
             accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z);
    
    return 0;
}

/**
 * @brief MPU6050 通道数据获取
 */
static int mpu6050_channel_get(xy_sensor_device_t *dev, 
                               xy_sensor_channel_t channel, 
                               xy_sensor_value_t *val)
{
    mpu6050_private_data_t *priv = (mpu6050_private_data_t *)dev->data;
    
    if (!val) {
        return -1;
    }
    
    switch (channel) {
        case XY_SENSOR_CHAN_ACCEL_X:
            *val = priv->accel_x;
            break;
        case XY_SENSOR_CHAN_ACCEL_Y:
            *val = priv->accel_y;
            break;
        case XY_SENSOR_CHAN_ACCEL_Z:
            *val = priv->accel_z;
            break;
        case XY_SENSOR_CHAN_GYRO_X:
            *val = priv->gyro_x;
            break;
        case XY_SENSOR_CHAN_GYRO_Y:
            *val = priv->gyro_y;
            break;
        case XY_SENSOR_CHAN_GYRO_Z:
            *val = priv->gyro_z;
            break;
        case XY_SENSOR_CHAN_DIE_TEMP:
            *val = priv->temp_data;
            break;
        default:
            return -1;
    }
    
    return 0;
}

/**
 * @brief MPU6050 驱动 API
 */
static const xy_sensor_driver_api_t mpu6050_driver_api = {
    .init = mpu6050_init,
    .sample_fetch = mpu6050_sample_fetch,
    .channel_get = mpu6050_channel_get,
    .attr_set = NULL,
    .attr_get = NULL,
    .trigger_set = NULL,
};

/* ==================== 设备注册 ==================== */

static mpu6050_private_data_t mpu6050_priv_data;
static xy_sensor_device_t mpu6050_device = {
    .name = "MPU6050",
    .type = XY_SENSOR_TYPE_ACCEL,
    .api = &mpu6050_driver_api,
    .data = &mpu6050_priv_data,
    .bus = {0},
    .initialized = false,
};

/**
 * @brief 注册 MPU6050 传感器
 */
int xy_sensor_mpu6050_register(void *i2c_handle, uint8_t addr)
{
    xy_sensor_bus_config_i2c(&mpu6050_priv_data.bus, i2c_handle, addr);
    return xy_sensor_device_register(&mpu6050_device);
}
