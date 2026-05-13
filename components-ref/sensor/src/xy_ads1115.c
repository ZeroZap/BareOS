/**
 * @file xy_ads1115.c
 * @brief ADS1115 4-Channel 16-Bit ADC Driver Implementation
 * @version 1.0.0
 * @date 2026-03-01
 */

#include "xy_ads1115.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/**
 * @brief 增益对应的 LSB (mV)
 */
static const float g_lsb_per_mv[] = {
    0.187500F,  /* ±6.144V */
    0.125000F,  /* ±4.096V */
    0.062500F,  /* ±2.048V */
    0.031250F,  /* ±1.024V */
    0.015625F,  /* ±0.512V */
    0.007812F,  /* ±0.256V */
};

/**
 * @brief 写入配置寄存器
 */
static int xy_ads1115_write_config(xy_ads1115_t *dev, uint16_t config)
{
    uint8_t buf[3];
    
    buf[0] = ADS1115_REG_CONFIG;
    buf[1] = (config >> 8) & 0xFF;
    buf[2] = config & 0xFF;
    
    return xy_i2c_device_write_reg(&dev->i2c_dev, ADS1115_REG_CONFIG, buf, 3);
}

/**
 * @brief 读取配置寄存器
 */
static int xy_ads1115_read_config(xy_ads1115_t *dev, uint16_t *config)
{
    uint8_t buf[2];
    int ret;
    
    ret = xy_i2c_device_read_reg(&dev->i2c_dev, ADS1115_REG_CONFIG, buf, 2);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    *config = ((uint16_t)buf[0] << 8) | buf[1];
    return XY_DEVICE_OK;
}

/**
 * @brief 读取转换寄存器
 */
static int xy_ads1115_read_conversion(xy_ads1115_t *dev, int16_t *value)
{
    uint8_t buf[2];
    int ret;
    
    ret = xy_i2c_device_read_reg(&dev->i2c_dev, ADS1115_REG_CONVERT, buf, 2);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    *value = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    return XY_DEVICE_OK;
}

int xy_ads1115_init(xy_ads1115_t *dev, void *i2c_handle, uint8_t addr)
{
    uint16_t config;
    int ret;
    
    if (!dev || !i2c_handle) {
        return XY_ADS1115_INVALID_PARAM;
    }
    
    memset(dev, 0, sizeof(*dev));
    
    /* 初始化 I2C 设备 */
    xy_i2c_device_init(&dev->i2c_dev, i2c_handle, addr, 1000);
    dev->addr = addr;
    
    /* 默认设置 */
    dev->pga = ADS1115_PGA_2_048V;
    dev->dr = ADS1115_DR_128SPS;
    
    /* 读取配置寄存器 */
    ret = xy_ads1115_read_config(dev, &config);
    if (ret != XY_DEVICE_OK) {
        xy_log_e("Failed to read config from ADS1115\n");
        return XY_ADS1115_NOT_FOUND;
    }
    
    xy_log_i("ADS1115 found at 0x%02X, config=0x%04X\n", addr, config);
    
    dev->initialized = 1;
    return XY_ADS1115_OK;
}

int xy_ads1115_deinit(xy_ads1115_t *dev)
{
    if (!dev) {
        return XY_ADS1115_INVALID_PARAM;
    }
    
    dev->initialized = 0;
    return XY_ADS1115_OK;
}

int xy_ads1115_read_single(xy_ads1115_t *dev, uint8_t channel, int16_t *value)
{
    uint16_t config;
    int ret;
    
    if (!dev || !value || !dev->initialized || channel > 3) {
        return XY_ADS1115_INVALID_PARAM;
    }
    
    /* 构建配置 */
    config = ADS1115_CONFIG_OS_SINGLE;              /* 单次转换 */
    config |= (4 + channel) << 12;                  /* 单端通道 */
    config |= dev->pga << 9;                        /* PGA 增益 */
    config |= ADS1115_CONFIG_MODE_SINGLE;           /* 单次模式 */
    config |= dev->dr << 5;                         /* 数据速率 */
    config |= ADS1115_CONFIG_COMP_DISABLE;          /* 禁用比较器 */
    
    /* 写入配置并启动转换 */
    ret = xy_ads1115_write_config(dev, config);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 等待转换完成 (128SPS = ~8ms) */
    xy_os_delay(10);
    
    /* 读取转换结果 */
    ret = xy_ads1115_read_conversion(dev, value);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    dev->last_value = *value;
    xy_log_d("ADS1115 CH%d: %d\n", channel, *value);
    return XY_ADS1115_OK;
}

int xy_ads1115_read_diff(xy_ads1115_t *dev, uint8_t channel_p, uint8_t channel_n, int16_t *value)
{
    uint16_t config;
    int ret;
    
    if (!dev || !value || !dev->initialized) {
        return XY_ADS1115_INVALID_PARAM;
    }
    
    if (channel_p > 2 || channel_n > 3 || channel_p >= channel_n) {
        return XY_ADS1115_INVALID_PARAM;
    }
    
    /* 构建配置 */
    config = ADS1115_CONFIG_OS_SINGLE;              /* 单次转换 */
    
    /* 差分通道选择 */
    if (channel_p == 0 && channel_n == 1) {
        config |= ADS1115_CONFIG_MUX_DIFF_0_1;
    } else if (channel_p == 0 && channel_n == 3) {
        config |= ADS1115_CONFIG_MUX_DIFF_0_3;
    } else if (channel_p == 1 && channel_n == 3) {
        config |= ADS1115_CONFIG_MUX_DIFF_1_3;
    } else if (channel_p == 2 && channel_n == 3) {
        config |= ADS1115_CONFIG_MUX_DIFF_2_3;
    }
    
    config |= dev->pga << 9;                        /* PGA 增益 */
    config |= ADS1115_CONFIG_MODE_SINGLE;           /* 单次模式 */
    config |= dev->dr << 5;                         /* 数据速率 */
    config |= ADS1115_CONFIG_COMP_DISABLE;          /* 禁用比较器 */
    
    /* 写入配置并启动转换 */
    ret = xy_ads1115_write_config(dev, config);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 等待转换完成 */
    xy_os_delay(10);
    
    /* 读取转换结果 */
    ret = xy_ads1115_read_conversion(dev, value);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    dev->last_value = *value;
    return XY_ADS1115_OK;
}

int xy_ads1115_read_voltage(xy_ads1115_t *dev, uint8_t channel, int32_t *voltage_mv)
{
    int16_t adc_value;
    int ret;
    float voltage;
    
    if (!dev || !voltage_mv || channel > 3) {
        return XY_ADS1115_INVALID_PARAM;
    }
    
    ret = xy_ads1115_read_single(dev, channel, &adc_value);
    if (ret != XY_ADS1115_OK) {
        return ret;
    }
    
    /* 转换为电压 (mV) */
    voltage = adc_value * g_lsb_per_mv[dev->pga];
    *voltage_mv = (int32_t)voltage;
    
    return XY_ADS1115_OK;
}

int xy_ads1115_set_pga(xy_ads1115_t *dev, xy_ads1115_pga_t pga)
{
    if (!dev || pga > ADS1115_PGA_0_256V) {
        return XY_ADS1115_INVALID_PARAM;
    }
    
    dev->pga = pga;
    return XY_ADS1115_OK;
}

int xy_ads1115_set_dr(xy_ads1115_t *dev, xy_ads1115_dr_t dr)
{
    if (!dev || dr > ADS1115_DR_860SPS) {
        return XY_ADS1115_INVALID_PARAM;
    }
    
    dev->dr = dr;
    return XY_ADS1115_OK;
}
