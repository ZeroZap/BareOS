/**
 * @file xy_max17043.c
 * @brief MAX17043/MAX17044 Fuel Gauge Driver
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#include "xy_max17043.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/**
 * @brief 写入寄存器
 */
static int xy_max17043_write_reg(xy_max17043_t *max17043, uint8_t reg, uint16_t value)
{
    uint8_t buf[3];
    buf[0] = reg;
    buf[1] = (value >> 8) & 0xFF;
    buf[2] = value & 0xFF;
    return xy_i2c_device_write(&max17043->i2c_dev, buf, 3);
}

/**
 * @brief 读取寄存器
 */
static int xy_max17043_read_reg(xy_max17043_t *max17043, uint8_t reg, uint16_t *value)
{
    uint8_t buf[2];
    int ret = xy_i2c_device_read_reg(&max17043->i2c_dev, reg, buf, 2);
    if (ret == XY_DEVICE_OK) {
        *value = ((uint16_t)buf[0] << 8) | buf[1];
    }
    return ret;
}

int xy_max17043_init(xy_max17043_t *max17043, void *i2c_handle,
                     const xy_max17043_config_t *config)
{
    int ret;
    uint16_t version;
    
    if (!max17043 || !i2c_handle || !config) {
        return XY_MAX17043_INVALID_PARAM;
    }
    
    memset(max17043, 0, sizeof(*max17043));
    memcpy(&max17043->config, config, sizeof(xy_max17043_config_t));
    
    /* 初始化 I2C */
    xy_i2c_device_init(&max17043->i2c_dev, i2c_handle, MAX17043_ADDR, 400);
    
    /* 读取版本寄存器验证设备 */
    ret = xy_max17043_read_reg(max17043, MAX17043_REG_VER, &version);
    if (ret != XY_DEVICE_OK) {
        xy_log_e("MAX17043 not found\n");
        return XY_MAX17043_NOT_FOUND;
    }
    
    xy_log_i("MAX17043 found (VER=0x%04X)\n", version);
    max17043->data.version = version & 0xFF;
    
    /* 配置告警电压 */
    if (config->alert_voltage_mv > 0) {
        /* VCELL 每格 20mV */
        uint16_t valrt = (uint16_t)(config->alert_voltage_mv / 20.0f);
        xy_max17043_write_reg(max17043, MAX17043_REG_VALRT, valrt);
    }
    
    /* 配置休眠模式 */
    if (config->enable_hibernate) {
        xy_max17043_enable_hibernate(max17043, true);
    }
    
    max17043->initialized = true;
    xy_log_i("MAX17043 initialized (Capacity=%.0f mAh)\n", config->capacity_mah);
    
    return XY_MAX17043_OK;
}

int xy_max17043_deinit(xy_max17043_t *max17043)
{
    if (!max17043) {
        return XY_MAX17043_INVALID_PARAM;
    }
    
    max17043->initialized = false;
    return XY_MAX17043_OK;
}

int xy_max17043_read(xy_max17043_t *max17043)
{
    int ret;
    uint16_t raw_value;
    
    if (!max17043 || !max17043->initialized) {
        return XY_MAX17043_INVALID_PARAM;
    }
    
    /* 读取电池电压 (78.125uV/LSB) */
    ret = xy_max17043_read_reg(max17043, MAX17043_REG_VCELL, &raw_value);
    if (ret == XY_DEVICE_OK) {
        max17043->data.voltage_mv = (raw_value >> 4) * 0.078125f;
    }
    
    /* 读取电量百分比 (1/256 %/LSB) */
    ret = xy_max17043_read_reg(max17043, MAX17043_REG_SOC, &raw_value);
    if (ret == XY_DEVICE_OK) {
        max17043->data.percentage = (raw_value >> 8) + (raw_value & 0xFF) / 256.0f;
    }
    
    /* 读取充放电率 (0.208%/h /LSB) */
    ret = xy_max17043_read_reg(max17043, MAX17043_REG_CRATE, &raw_value);
    if (ret == XY_DEVICE_OK) {
        /* 有符号数 */
        int16_t signed_value = (int16_t)raw_value;
        max17043->data.crate = signed_value * 0.208f / 100.0f;  /* 转换为 C 率 */
    }
    
    /* 读取状态寄存器 */
    ret = xy_max17043_read_reg(max17043, MAX17043_REG_STATUS, &raw_value);
    if (ret == XY_DEVICE_OK) {
        max17043->data.low_battery = (raw_value & 0x02) ? true : false;
        max17043->data.reset_triggered = (raw_value & 0x10) ? true : false;
    }
    
    max17043->data.timestamp = xy_hal_sys_get_tick_count();
    
    xy_log_d("MAX17043: V=%.2fV, SOC=%.1f%%, C-rate=%.3fC\n",
             max17043->data.voltage_mv / 1000.0f,
             max17043->data.percentage,
             max17043->data.crate);
    
    return XY_MAX17043_OK;
}

int xy_max17043_get_voltage(xy_max17043_t *max17043, float *voltage_mv)
{
    if (!max17043 || !voltage_mv) {
        return XY_MAX17043_INVALID_PARAM;
    }
    
    int ret = xy_max17043_read(max17043);
    if (ret == XY_MAX17043_OK) {
        *voltage_mv = max17043->data.voltage_mv;
    }
    return ret;
}

int xy_max17043_get_percentage(xy_max17043_t *max17043, float *percentage)
{
    if (!max17043 || !percentage) {
        return XY_MAX17043_INVALID_PARAM;
    }
    
    int ret = xy_max17043_read(max17043);
    if (ret == XY_MAX17043_OK) {
        *percentage = max17043->data.percentage;
    }
    return ret;
}

int xy_max17043_get_crate(xy_max17043_t *max17043, float *crate)
{
    if (!max17043 || !crate) {
        return XY_MAX17043_INVALID_PARAM;
    }
    
    int ret = xy_max17043_read(max17043);
    if (ret == XY_MAX17043_OK) {
        *crate = max17043->data.crate;
    }
    return ret;
}

int xy_max17043_set_capacity(xy_max17043_t *max17043, float capacity_mah)
{
    if (!max17043 || capacity_mah <= 0) {
        return XY_MAX17043_INVALID_PARAM;
    }
    
    max17043->config.capacity_mah = capacity_mah;
    return XY_MAX17043_OK;
}

int xy_max17043_enable_hibernate(xy_max17043_t *max17043, bool enable)
{
    uint16_t hibrt;
    
    if (!max17043) {
        return XY_MAX17043_INVALID_PARAM;
    }
    
    if (enable) {
        /* 配置休眠阈值 */
        hibrt = 0x4000;  /* 典型值 */
    } else {
        hibrt = 0x0000;
    }
    
    return xy_max17043_write_reg(max17043, MAX17043_REG_HIBRT, hibrt);
}

int xy_max17043_reset(xy_max17043_t *max17043)
{
    if (!max17043) {
        return XY_MAX17043_INVALID_PARAM;
    }
    
    /* 写入解锁序列 */
    xy_max17043_write_reg(max17043, MAX17043_REG_UNLOCK, 0x0090);
    
    /* 写入复位命令 */
    xy_max17043_write_reg(max17043, MAX17043_REG_COMMAND, 0x0002);
    
    xy_hal_delay_ms(100);
    
    return XY_MAX17043_OK;
}
