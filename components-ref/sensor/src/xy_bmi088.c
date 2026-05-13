#include "xy_ret.h"
/**
 * @file xy_bmi088.c
 * @brief BMI088 6-Axis IMU Sensor Driver Implementation
 * @version 1.0.0
 * @date 2026-03-14
 * 
 * BMI088 是 Bosch 出品的高性能 6 轴 IMU，包含独立的加速度计和陀螺仪
 * 适用于无人机、机器人等需要高精度和抗振动的应用
 */

#include "xy_bmi088.h"
#include <string.h>

#ifndef XY_NULL
#define XY_NULL NULL
#endif

#ifndef XY_OK
#define XY_OK 0
#endif

#ifndef XY_ERROR
#define XY_ERROR -1
#endif

/*============================================================================
 * 内部辅助函数
 *===========================================================================*/

/**
 * @brief 写入加速度计寄存器
 */
static xy_ret_t bmi088_acc_write_reg(xy_bmi088_dev_t *dev, uint8_t reg_addr, const uint8_t *data, uint16_t len)
{
    if (dev == XY_NULL || dev->spi == XY_NULL || data == XY_NULL) {
        return XY_ERROR;
    }
    return xy_spi_write_reg(dev->spi, 0, reg_addr, data, len);
}

/**
 * @brief 读取加速度计寄存器
 */
static xy_ret_t bmi088_acc_read_reg(xy_bmi088_dev_t *dev, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    if (dev == XY_NULL || dev->spi == XY_NULL || data == XY_NULL) {
        return XY_ERROR;
    }
    return xy_spi_read_reg(dev->spi, 0, reg_addr, data, len);
}

/**
 * @brief 写入陀螺仪寄存器
 */
static xy_ret_t bmi088_gyro_write_reg(xy_bmi088_dev_t *dev, uint8_t reg_addr, const uint8_t *data, uint16_t len)
{
    if (dev == XY_NULL || dev->spi == XY_NULL || data == XY_NULL) {
        return XY_ERROR;
    }
    return xy_spi_write_reg(dev->spi, 1, reg_addr, data, len);
}

/**
 * @brief 读取陀螺仪寄存器
 */
static xy_ret_t bmi088_gyro_read_reg(xy_bmi088_dev_t *dev, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    if (dev == XY_NULL || dev->spi == XY_NULL || data == XY_NULL) {
        return XY_ERROR;
    }
    return xy_spi_read_reg(dev->spi, 1, reg_addr, data, len);
}

/**
 * @brief 获取加速度计灵敏度
 */
static float bmi088_get_acc_sensitivity(xy_bmi088_acc_range_t range)
{
    switch (range) {
        case XY_BMI088_ACC_RANGE_3G:  return 10920.0f;  /* 10920 LSB/g */
        case XY_BMI088_ACC_RANGE_6G:  return 5460.0f;   /* 5460 LSB/g */
        case XY_BMI088_ACC_RANGE_12G: return 2730.0f;   /* 2730 LSB/g */
        case XY_BMI088_ACC_RANGE_24G: return 1365.0f;   /* 1365 LSB/g */
        default: return 5460.0f;
    }
}

/**
 * @brief 获取陀螺仪灵敏度
 */
static float bmi088_get_gyro_sensitivity(xy_bmi088_gyro_range_t range)
{
    switch (range) {
        case XY_BMI088_GYRO_RANGE_125:   return 262.4f;   /* 262.4 LSB/°/s */
        case XY_BMI088_GYRO_RANGE_250:   return 131.2f;   /* 131.2 LSB/°/s */
        case XY_BMI088_GYRO_RANGE_500:   return 65.6f;    /* 65.6 LSB/°/s */
        case XY_BMI088_GYRO_RANGE_1000:  return 32.8f;    /* 32.8 LSB/°/s */
        case XY_BMI088_GYRO_RANGE_2000:  return 16.4f;    /* 16.4 LSB/°/s */
        default: return 131.2f;
    }
}

/*============================================================================
 * 公开 API 实现
 *===========================================================================*/

xy_ret_t xy_bmi088_init(xy_bmi088_dev_t *dev, xy_spi_dev_t *spi, xy_bmi088_config_t *config)
{
    if (dev == XY_NULL || spi == XY_NULL) {
        return XY_ERROR;
    }
    
    memset(dev, 0, sizeof(xy_bmi088_dev_t));
    dev->spi = spi;
    
    if (config != XY_NULL) {
        dev->config = *config;
    } else {
        /* 默认配置 */
        dev->config.acc_range = XY_BMI088_ACC_RANGE_6G;
        dev->config.gyro_range = XY_BMI088_GYRO_RANGE_2000;
        dev->config.acc_odr = XY_BMI088_ACC_ODR_200HZ;
        dev->config.gyro_odr = XY_BMI088_GYRO_ODR_200HZ;
        dev->config.bw = XY_BMI088_BW_NORMAL;
        dev->config.enable_interrupt = false;
    }
    
    /* 软件复位 */
    xy_ret_t ret = xy_bmi088_soft_reset(dev);
    if (ret != XY_OK) {
        return ret;
    }
    xy_delay_ms(10);
    
    /* 验证芯片 ID */
    uint8_t acc_id = 0, gyro_id = 0;
    ret = xy_bmi088_read_chip_id(dev, &acc_id, &gyro_id);
    if (ret != XY_OK) {
        return ret;
    }
    
    if (acc_id != BMI088_ACC_CHIP_ID_VALUE) {
        return XY_ERROR;  /* 加速度计 ID 错误 */
    }
    if (gyro_id != BMI088_GYRO_CHIP_ID_VALUE) {
        return XY_ERROR;  /* 陀螺仪 ID 错误 */
    }
    
    /* 配置加速度计 */
    uint8_t reg_data[2];
    
    /* 设置量程 */
    reg_data[0] = (uint8_t)dev->config.acc_range;
    ret = bmi088_acc_write_reg(dev, BMI088_ACC_RANGE_ADDR, reg_data, 1);
    if (ret != XY_OK) return ret;
    
    /* 设置带宽和 ODR */
    reg_data[0] = (uint8_t)dev->config.acc_odr;
    ret = bmi088_acc_write_reg(dev, BMI088_ACC_BW_ADDR, reg_data, 1);
    if (ret != XY_OK) return ret;
    
    /* 配置电源模式 */
    reg_data[0] = 0x00;  /* 正常模式 */
    ret = bmi088_acc_write_reg(dev, BMI088_ACC_PWR_CONF_ADDR, reg_data, 1);
    if (ret != XY_OK) return ret;
    
    xy_delay_ms(5);
    
    /* 使能加速度计 */
    reg_data[0] = 0x04;  /* 使能 */
    ret = bmi088_acc_write_reg(dev, BMI088_ACC_PWR_CTRL_ADDR, reg_data, 1);
    if (ret != XY_OK) return ret;
    
    xy_delay_ms(10);
    
    /* 配置陀螺仪 */
    
    /* 设置量程 */
    reg_data[0] = (uint8_t)dev->config.gyro_range;
    ret = bmi088_gyro_write_reg(dev, BMI088_GYRO_RANGE_ADDR, reg_data, 1);
    if (ret != XY_OK) return ret;
    
    /* 设置带宽和 ODR */
    reg_data[0] = (uint8_t)dev->config.gyro_odr;
    ret = bmi088_gyro_write_reg(dev, BMI088_GYRO_BANDWIDTH_ADDR, reg_data, 1);
    if (ret != XY_OK) return ret;
    
    /* 使能陀螺仪 */
    reg_data[0] = 0x80;  /* 正常模式 */
    ret = bmi088_gyro_write_reg(dev, BMI088_GYRO_LPM1_ADDR, reg_data, 1);
    if (ret != XY_OK) return ret;
    
    xy_delay_ms(30);  /* 等待陀螺仪启动 */
    
    /* 设置灵敏度 */
    dev->acc_sensitivity = bmi088_get_acc_sensitivity(dev->config.acc_range);
    dev->gyro_sensitivity = bmi088_get_gyro_sensitivity(dev->config.gyro_range);
    
    /* 初始化校准参数 */
    memset(dev->acc_offset, 0, sizeof(dev->acc_offset));
    memset(dev->gyro_offset, 0, sizeof(dev->gyro_offset));
    
    dev->is_initialized = true;
    
    return XY_OK;
}

xy_ret_t xy_bmi088_deinit(xy_bmi088_dev_t *dev)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    /* 关闭加速度计 */
    uint8_t reg_data = 0x00;
    bmi088_acc_write_reg(dev, BMI088_ACC_PWR_CTRL_ADDR, &reg_data, 1);
    
    /* 关闭陀螺仪 */
    reg_data = 0x03;  /* 深度睡眠 */
    bmi088_gyro_write_reg(dev, BMI088_GYRO_LPM1_ADDR, &reg_data, 1);
    
    dev->is_initialized = false;
    
    return XY_OK;
}

xy_ret_t xy_bmi088_read_chip_id(xy_bmi088_dev_t *dev, uint8_t *acc_id, uint8_t *gyro_id)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    xy_ret_t ret = XY_OK;
    
    if (acc_id != XY_NULL) {
        ret = bmi088_acc_read_reg(dev, BMI088_ACC_CHIP_ID, acc_id, 1);
        if (ret != XY_OK) return ret;
    }
    
    if (gyro_id != XY_NULL) {
        ret = bmi088_gyro_read_reg(dev, BMI088_GYRO_CHIP_ID, gyro_id, 1);
        if (ret != XY_OK) return ret;
    }
    
    return XY_OK;
}

xy_ret_t xy_bmi088_read_raw_data(xy_bmi088_dev_t *dev, xy_bmi088_raw_data_t *raw_data)
{
    if (dev == XY_NULL || !dev->is_initialized || raw_data == XY_NULL) {
        return XY_ERROR;
    }
    
    uint8_t buffer[6];
    xy_ret_t ret;
    
    /* 读取加速度计数据 */
    ret = bmi088_acc_read_reg(dev, BMI088_ACC_X_LSB_ADDR, buffer, 6);
    if (ret != XY_OK) return ret;
    
    raw_data->acc_x = (int16_t)((uint16_t)buffer[1] << 8 | buffer[0]);
    raw_data->acc_y = (int16_t)((uint16_t)buffer[3] << 8 | buffer[2]);
    raw_data->acc_z = (int16_t)((uint16_t)buffer[5] << 8 | buffer[4]);
    
    /* 读取陀螺仪数据 */
    ret = bmi088_gyro_read_reg(dev, BMI088_GYRO_X_LSB_ADDR, buffer, 6);
    if (ret != XY_OK) return ret;
    
    raw_data->gyro_x = (int16_t)((uint16_t)buffer[1] << 8 | buffer[0]);
    raw_data->gyro_y = (int16_t)((uint16_t)buffer[3] << 8 | buffer[2]);
    raw_data->gyro_z = (int16_t)((uint16_t)buffer[5] << 8 | buffer[4]);
    
    /* 读取温度 (可选) */
    ret = bmi088_acc_read_reg(dev, BMI088_ACC_TEMP_LSB_ADDR, buffer, 2);
    if (ret == XY_OK) {
        raw_data->temperature = (int16_t)((uint16_t)buffer[1] << 8 | buffer[0]);
    }
    
    return XY_OK;
}

xy_ret_t xy_bmi088_read_data(xy_bmi088_dev_t *dev, xy_bmi088_data_t *data)
{
    if (dev == XY_NULL || !dev->is_initialized || data == XY_NULL) {
        return XY_ERROR;
    }
    
    xy_bmi088_raw_data_t raw;
    xy_ret_t ret = xy_bmi088_read_raw_data(dev, &raw);
    if (ret != XY_OK) return ret;
    
    /* 应用零偏校准 */
    float acc_x = (float)raw.acc_x - dev->acc_offset[0];
    float acc_y = (float)raw.acc_y - dev->acc_offset[1];
    float acc_z = (float)raw.acc_z - dev->acc_offset[2];
    
    float gyro_x = (float)raw.gyro_x - dev->gyro_offset[0];
    float gyro_y = (float)raw.gyro_y - dev->gyro_offset[1];
    float gyro_z = (float)raw.gyro_z - dev->gyro_offset[2];
    
    /* 转换为物理单位 */
    data->acc_x = acc_x / dev->acc_sensitivity * 9.80665f;  /* m/s² */
    data->acc_y = acc_y / dev->acc_sensitivity * 9.80665f;
    data->acc_z = acc_z / dev->acc_sensitivity * 9.80665f;
    
    data->gyro_x = gyro_x / dev->gyro_sensitivity * 0.01745329252f;  /* rad/s */
    data->gyro_y = gyro_y / dev->gyro_sensitivity * 0.01745329252f;
    data->gyro_z = gyro_z / dev->gyro_sensitivity * 0.01745329252f;
    
    /* 温度转换 (BMI088 温度公式) */
    data->temperature = (float)raw.temperature / 128.0f + 23.0f;  /* °C */
    
    return XY_OK;
}

xy_ret_t xy_bmi088_soft_reset(xy_bmi088_dev_t *dev)
{
    if (dev == XY_NULL) {
        return XY_ERROR;
    }
    
    uint8_t reg_data;
    
    /* 复位加速度计 */
    reg_data = 0xB6;
    xy_ret_t ret = bmi088_acc_write_reg(dev, BMI088_ACC_SOFTRESET_ADDR, &reg_data, 1);
    if (ret != XY_OK) return ret;
    
    xy_delay_ms(10);
    
    /* 复位陀螺仪 */
    reg_data = 0xB6;
    ret = bmi088_gyro_write_reg(dev, BMI088_GYRO_SOFTRESET_ADDR, &reg_data, 1);
    if (ret != XY_OK) return ret;
    
    xy_delay_ms(30);
    
    return XY_OK;
}

xy_ret_t xy_bmi088_set_acc_range(xy_bmi088_dev_t *dev, xy_bmi088_acc_range_t range)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    uint8_t reg_data = (uint8_t)range;
    xy_ret_t ret = bmi088_acc_write_reg(dev, BMI088_ACC_RANGE_ADDR, &reg_data, 1);
    if (ret != XY_OK) return ret;
    
    dev->config.acc_range = range;
    dev->acc_sensitivity = bmi088_get_acc_sensitivity(range);
    
    return XY_OK;
}

xy_ret_t xy_bmi088_set_gyro_range(xy_bmi088_dev_t *dev, xy_bmi088_gyro_range_t range)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    uint8_t reg_data = (uint8_t)range;
    xy_ret_t ret = bmi088_gyro_write_reg(dev, BMI088_GYRO_RANGE_ADDR, &reg_data, 1);
    if (ret != XY_OK) return ret;
    
    dev->config.gyro_range = range;
    dev->gyro_sensitivity = bmi088_get_gyro_sensitivity(range);
    
    return XY_OK;
}

xy_ret_t xy_bmi088_calibrate(xy_bmi088_dev_t *dev, uint16_t samples)
{
    if (dev == XY_NULL || !dev->is_initialized || samples == 0) {
        return XY_ERROR;
    }
    
    /* 校准前确保设备静止放置 */
    
    int32_t acc_sum[3] = {0, 0, 0};
    int32_t gyro_sum[3] = {0, 0, 0};
    
    /* 采样 */
    for (uint16_t i = 0; i < samples; i++) {
        xy_bmi088_raw_data_t raw;
        xy_ret_t ret = xy_bmi088_read_raw_data(dev, &raw);
        if (ret != XY_OK) return ret;
        
        acc_sum[0] += raw.acc_x;
        acc_sum[1] += raw.acc_y;
        acc_sum[2] += raw.acc_z;
        
        gyro_sum[0] += raw.gyro_x;
        gyro_sum[1] += raw.gyro_y;
        gyro_sum[2] += raw.gyro_z;
        
        xy_delay_ms(2);
    }
    
    /* 计算平均值 */
    dev->acc_offset[0] = (float)acc_sum[0] / samples;
    dev->acc_offset[1] = (float)acc_sum[1] / samples;
    dev->acc_offset[2] = (float)acc_sum[2] / samples - dev->acc_sensitivity;  /* 减去 1g */
    
    dev->gyro_offset[0] = (float)gyro_sum[0] / samples;
    dev->gyro_offset[1] = (float)gyro_sum[1] / samples;
    dev->gyro_offset[2] = (float)gyro_sum[2] / samples;
    
    return XY_OK;
}

void xy_bmi088_set_calibration(xy_bmi088_dev_t *dev, float acc_offset[3], float gyro_offset[3])
{
    if (dev == XY_NULL) return;
    
    if (acc_offset != XY_NULL) {
        memcpy(dev->acc_offset, acc_offset, sizeof(dev->acc_offset));
    }
    
    if (gyro_offset != XY_NULL) {
        memcpy(dev->gyro_offset, gyro_offset, sizeof(dev->gyro_offset));
    }
}

bool xy_bmi088_is_ready(xy_bmi088_dev_t *dev)
{
    if (dev == XY_NULL) return false;
    return dev->is_initialized;
}
