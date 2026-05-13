/**
 * @file xy_mpu6050.c
 * @brief MPU6050 6-Axis IMU Driver Implementation
 * @version 1.0.0
 * @date 2026-03-01 凌晨
 */

#include "xy_mpu6050.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/**
 * @brief 加速度计灵敏度 (LSB/g)
 */
static const float g_accel_sensitivity[] = {
    16384.0F,  /* ±2g */
    8192.0F,   /* ±4g */
    4096.0F,   /* ±8g */
    2048.0F,   /* ±16g */
};

/**
 * @brief 陀螺仪灵敏度 (LSB/°/s)
 */
static const float g_gyro_sensitivity[] = {
    131.0F,    /* ±250°/s */
    65.5F,     /* ±500°/s */
    32.8F,     /* ±1000°/s */
    16.4F,     /* ±2000°/s */
};

/**
 * @brief 写入寄存器
 */
static int xy_mpu6050_write_reg(xy_mpu6050_t *dev, uint8_t reg, uint8_t value)
{
    return xy_i2c_device_write_reg(&dev->i2c_dev, reg, &value, 1);
}

/**
 * @brief 读取寄存器
 */
static int xy_mpu6050_read_reg(xy_mpu6050_t *dev, uint8_t reg, uint8_t *value)
{
    return xy_i2c_device_read_reg(&dev->i2c_dev, reg, value, 1);
}

/**
 * @brief 读取多字节寄存器
 */
static int xy_mpu6050_read_regs(xy_mpu6050_t *dev, uint8_t reg, uint8_t *data, uint8_t len)
{
    return xy_i2c_device_read_reg(&dev->i2c_dev, reg, data, len);
}

int xy_mpu6050_init(xy_mpu6050_t *dev, void *i2c_handle, uint8_t addr)
{
    int ret;
    uint8_t who_am_i;
    
    if (!dev || !i2c_handle) {
        return XY_MPU6050_INVALID_PARAM;
    }
    
    memset(dev, 0, sizeof(*dev));
    
    /* 初始化 I2C */
    xy_i2c_device_init(&dev->i2c_dev, i2c_handle, addr, 1000);
    dev->addr = addr;
    
    /* 检查 WHO_AM_I */
    ret = xy_mpu6050_read_reg(dev, MPU6050_REG_WHO_AM_I, &who_am_i);
    if (ret != XY_DEVICE_OK) {
        xy_log_e("Failed to read WHO_AM_I\n");
        return XY_MPU6050_NOT_FOUND;
    }
    
    if (who_am_i != MPU6050_WHO_AM_I_VALUE) {
        xy_log_e("WHO_AM_I mismatch: expected 0x%02X, got 0x%02X\n",
                 MPU6050_WHO_AM_I_VALUE, who_am_i);
        return XY_MPU6050_ID_ERROR;
    }
    
    xy_log_i("MPU6050 found at 0x%02X\n", addr);
    
    /* 唤醒设备 */
    ret = xy_mpu6050_write_reg(dev, MPU6050_REG_PWR_MGMT_1, 0x00);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    xy_os_delay(100);
    
    /* 设置采样率 (1kHz) */
    xy_mpu6050_write_reg(dev, MPU6050_REG_SMPLRT_DIV, 0x00);
    
    /* 配置 DLPF */
    dev->dlpf = MPU6050_DLPF_44HZ;
    xy_mpu6050_write_reg(dev, MPU6050_REG_CONFIG, dev->dlpf);
    
    /* 设置默认量程 */
    dev->accel_range = MPU6050_ACCEL_2G;
    dev->gyro_range = MPU6050_GYRO_250DPS;
    xy_mpu6050_set_accel_range(dev, dev->accel_range);
    xy_mpu6050_set_gyro_range(dev, dev->gyro_range);
    
    dev->initialized = 1;
    xy_log_i("MPU6050 initialized\n");
    return XY_MPU6050_OK;
}

int xy_mpu6050_deinit(xy_mpu6050_t *dev)
{
    if (!dev) {
        return XY_MPU6050_INVALID_PARAM;
    }
    
    /* 进入睡眠模式 */
    xy_mpu6050_write_reg(dev, MPU6050_REG_PWR_MGMT_1, 0x40);
    
    dev->initialized = 0;
    return XY_MPU6050_OK;
}

int xy_mpu6050_read_raw(xy_mpu6050_t *dev)
{
    uint8_t buf[14];
    int ret;
    
    if (!dev || !dev->initialized) {
        return XY_MPU6050_INVALID_PARAM;
    }
    
    /* 读取 14 字节数据 */
    ret = xy_mpu6050_read_regs(dev, MPU6050_REG_ACCEL_XOUT_H, buf, 14);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 解析数据 */
    dev->raw.accel_x = (int16_t)((buf[0] << 8) | buf[1]);
    dev->raw.accel_y = (int16_t)((buf[2] << 8) | buf[3]);
    dev->raw.accel_z = (int16_t)((buf[4] << 8) | buf[5]);
    dev->raw.temperature = (int16_t)((buf[6] << 8) | buf[7]);
    dev->raw.gyro_x = (int16_t)((buf[8] << 8) | buf[9]);
    dev->raw.gyro_y = (int16_t)((buf[10] << 8) | buf[11]);
    dev->raw.gyro_z = (int16_t)((buf[12] << 8) | buf[13]);
    
    /* 转换为物理量 */
    float sensitivity = g_accel_sensitivity[dev->accel_range];
    dev->accel_g[0] = (dev->raw.accel_x - dev->calib.accel_offset_x) / sensitivity;
    dev->accel_g[1] = (dev->raw.accel_y - dev->calib.accel_offset_y) / sensitivity;
    dev->accel_g[2] = (dev->raw.accel_z - dev->calib.accel_offset_z) / sensitivity;
    
    sensitivity = g_gyro_sensitivity[dev->gyro_range];
    dev->gyro_dps[0] = (dev->raw.gyro_x - dev->calib.gyro_offset_x) / sensitivity;
    dev->gyro_dps[1] = (dev->raw.gyro_y - dev->calib.gyro_offset_y) / sensitivity;
    dev->gyro_dps[2] = (dev->raw.gyro_z - dev->calib.gyro_offset_z) / sensitivity;
    
    /* 温度：°C = (raw / 340) + 36.53 */
    dev->temperature_c = ((float)dev->raw.temperature / 340.0F) + 36.53F;
    
    return XY_MPU6050_OK;
}

int xy_mpu6050_read_accel(xy_mpu6050_t *dev, float *ax, float *ay, float *az)
{
    int ret;
    
    if (!dev || !ax || !ay || !az) {
        return XY_MPU6050_INVALID_PARAM;
    }
    
    ret = xy_mpu6050_read_raw(dev);
    if (ret != XY_MPU6050_OK) {
        return ret;
    }
    
    *ax = dev->accel_g[0];
    *ay = dev->accel_g[1];
    *az = dev->accel_g[2];
    
    return XY_MPU6050_OK;
}

int xy_mpu6050_read_gyro(xy_mpu6050_t *dev, float *gx, float *gy, float *gz)
{
    int ret;
    
    if (!dev || !gx || !gy || !gz) {
        return XY_MPU6050_INVALID_PARAM;
    }
    
    ret = xy_mpu6050_read_raw(dev);
    if (ret != XY_MPU6050_OK) {
        return ret;
    }
    
    *gx = dev->gyro_dps[0];
    *gy = dev->gyro_dps[1];
    *gz = dev->gyro_dps[2];
    
    return XY_MPU6050_OK;
}

int xy_mpu6050_read_temperature(xy_mpu6050_t *dev, float *temp)
{
    int ret;
    
    if (!dev || !temp) {
        return XY_MPU6050_INVALID_PARAM;
    }
    
    ret = xy_mpu6050_read_raw(dev);
    if (ret != XY_MPU6050_OK) {
        return ret;
    }
    
    *temp = dev->temperature_c;
    return XY_MPU6050_OK;
}

int xy_mpu6050_set_accel_range(xy_mpu6050_t *dev, xy_mpu6050_accel_range_t range)
{
    if (!dev || range > MPU6050_ACCEL_16G) {
        return XY_MPU6050_INVALID_PARAM;
    }
    
    dev->accel_range = range;
    return xy_mpu6050_write_reg(dev, MPU6050_REG_ACCEL_CONFIG, range << 3);
}

int xy_mpu6050_set_gyro_range(xy_mpu6050_t *dev, xy_mpu6050_gyro_range_t range)
{
    if (!dev || range > MPU6050_GYRO_2000DPS) {
        return XY_MPU6050_INVALID_PARAM;
    }
    
    dev->gyro_range = range;
    return xy_mpu6050_write_reg(dev, MPU6050_REG_GYRO_CONFIG, range << 3);
}

int xy_mpu6050_calibrate(xy_mpu6050_t *dev, uint16_t samples)
{
    int32_t accel_sum[3] = {0, 0, 0};
    int32_t gyro_sum[3] = {0, 0, 0};
    uint16_t i;
    
    if (!dev || samples == 0) {
        return XY_MPU6050_INVALID_PARAM;
    }
    
    xy_log_i("Calibrating MPU6050 (%d samples)...\n", samples);
    
    /* 采集样本 */
    for (i = 0; i < samples; i++) {
        xy_mpu6050_read_raw(dev);
        
        accel_sum[0] += dev->raw.accel_x;
        accel_sum[1] += dev->raw.accel_y;
        accel_sum[2] += dev->raw.accel_z;
        
        gyro_sum[0] += dev->raw.gyro_x;
        gyro_sum[1] += dev->raw.gyro_y;
        gyro_sum[2] += dev->raw.gyro_z;
        
        xy_os_delay(10);
    }
    
    /* 计算偏移 (假设水平放置，Z 轴应为 1g) */
    float sensitivity = g_accel_sensitivity[dev->accel_range];
    dev->calib.accel_offset_x = (float)accel_sum[0] / samples;
    dev->calib.accel_offset_y = (float)accel_sum[1] / samples;
    dev->calib.accel_offset_z = (float)accel_sum[2] / samples - sensitivity;
    
    sensitivity = g_gyro_sensitivity[dev->gyro_range];
    dev->calib.gyro_offset_x = (float)gyro_sum[0] / samples;
    dev->calib.gyro_offset_y = (float)gyro_sum[1] / samples;
    dev->calib.gyro_offset_z = (float)gyro_sum[2] / samples;
    
    xy_log_i("Calibration complete\n");
    xy_log_d("Accel offset: %.2f, %.2f, %.2f\n", 
             dev->calib.accel_offset_x, dev->calib.accel_offset_y, dev->calib.accel_offset_z);
    xy_log_d("Gyro offset: %.2f, %.2f, %.2f\n",
             dev->calib.gyro_offset_x, dev->calib.gyro_offset_y, dev->calib.gyro_offset_z);
    
    return XY_MPU6050_OK;
}
