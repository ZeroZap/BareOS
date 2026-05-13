/**
 * @file xy_ltc2945.c
 * @brief LTC2945 Power Monitor Driver
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#include "xy_ltc2945.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/**
 * @brief 写入寄存器
 */
static int xy_ltc2945_write_reg(xy_ltc2945_t *ltc2945, uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    return xy_i2c_device_write(&ltc2945->i2c_dev, buf, 2);
}

/**
 * @brief 读取寄存器
 */
static int xy_ltc2945_read_reg(xy_ltc2945_t *ltc2945, uint8_t reg, uint8_t *value)
{
    return xy_i2c_device_read_reg(&ltc2945->i2c_dev, reg, value, 1);
}

/**
 * @brief 读取 24 位寄存器
 */
static int xy_ltc2945_read24(xy_ltc2945_t *ltc2945, uint8_t reg, uint32_t *value)
{
    uint8_t buf[3];
    int ret;
    
    ret = xy_i2c_device_read_reg(&ltc2945->i2c_dev, reg, buf, 3);
    if (ret == XY_DEVICE_OK) {
        *value = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
    }
    return ret;
}

int xy_ltc2945_init(xy_ltc2945_t *ltc2945, void *i2c_handle, uint8_t addr,
                    const xy_ltc2945_config_t *config)
{
    int ret;
    uint8_t status;
    
    if (!ltc2945 || !i2c_handle || !config) {
        return XY_LTC2945_INVALID_PARAM;
    }
    
    memset(ltc2945, 0, sizeof(*ltc2945));
    memcpy(&ltc2945->config, config, sizeof(xy_ltc2945_config_t));
    ltc2945->addr = addr;
    
    /* 初始化 I2C */
    xy_i2c_device_init(&ltc2945->i2c_dev, i2c_handle, addr, 400);
    
    /* 读取状态寄存器验证设备 */
    ret = xy_ltc2945_read_reg(ltc2945, LTC2945_REG_STATUS, &status);
    if (ret != XY_DEVICE_OK) {
        xy_log_e("LTC2945 not found\n");
        return XY_LTC2945_NOT_FOUND;
    }
    
    xy_log_i("LTC2945 found at 0x%02X (STATUS=0x%02X)\n", addr, status);
    
    /* 计算 LSB 值 */
    float rshunt_ohm = config->shunt_resistor_mohm / 1000.0f;
    
    /* 功率 LSB = 25uV * 25mV / Rshunt (W) */
    ltc2945->power_lsb = 25e-6f * 25e-3f / rshunt_ohm;
    
    /* 电荷 LSB = 25uV / Rshunt * 1s (C) */
    ltc2945->charge_lsb = 25e-6f / rshunt_ohm;
    
    /* 能量 LSB = 功率 LSB * 1s (J) */
    ltc2945->energy_lsb = ltc2945->power_lsb;
    
    xy_log_d("Power_LSB=%.6f W, Charge_LSB=%.6f C, Energy_LSB=%.6f J\n",
             ltc2945->power_lsb, ltc2945->charge_lsb, ltc2945->energy_lsb);
    
    /* 配置控制寄存器 */
    uint8_t ctrl = 0x00;
    if (config->auto_convert) {
        ctrl |= 0x08;  /* 自动转换 */
    }
    xy_ltc2945_write_reg(ltc2945, LTC2945_REG_CONTROL, ctrl);
    
    /* 配置 ALERT GPIO */
    xy_ltc2945_write_reg(ltc2945, LTC2945_REG_CTRL_GPIO, config->alert_gpio_config);
    
    ltc2945->initialized = true;
    xy_log_i("LTC2945 initialized (Rshunt=%.1f mΩ)\n", config->shunt_resistor_mohm);
    
    return XY_LTC2945_OK;
}

int xy_ltc2945_deinit(xy_ltc2945_t *ltc2945)
{
    if (!ltc2945) {
        return XY_LTC2945_INVALID_PARAM;
    }
    
    ltc2945->initialized = false;
    return XY_LTC2945_OK;
}

int xy_ltc2945_read(xy_ltc2945_t *ltc2945)
{
    int ret;
    uint32_t raw_value;
    uint16_t raw16;
    
    if (!ltc2945 || !ltc2945->initialized) {
        return XY_LTC2945_INVALID_PARAM;
    }
    
    /* 读取输入电压 (25mV/LSB) */
    ret = xy_i2c_device_read_reg(&ltc2945->i2c_dev, LTC2945_REG_VIN_MSB, 
                                  (uint8_t*)&raw16, 2);
    if (ret == XY_DEVICE_OK) {
        raw16 = ((uint16_t)((uint8_t*)&raw16)[0] << 8) | ((uint8_t*)&raw16)[1];
        ltc2945->data.voltage_v = (raw16 >> 4) * 0.025f;
    }
    
    /* 读取分流电压 (25uV/LSB) */
    ret = xy_i2c_device_read_reg(&ltc2945->i2c_dev, LTC2945_REG_VSENSE_MSB,
                                  (uint8_t*)&raw16, 2);
    if (ret == XY_DEVICE_OK) {
        raw16 = ((uint16_t)((uint8_t*)&raw16)[0] << 8) | ((uint8_t*)&raw16)[1];
        int16_t signed_value = (int16_t)raw16;
        ltc2945->data.shunt_voltage_v = (signed_value >> 4) * 25e-6f;
        
        /* 计算电流 */
        float rshunt_ohm = ltc2945->config.shunt_resistor_mohm / 1000.0f;
        ltc2945->data.current_a = ltc2945->data.shunt_voltage_v / rshunt_ohm;
    }
    
    /* 读取功率 (24 位) */
    ret = xy_ltc2945_read24(ltc2945, LTC2945_REG_POWER_MSB, &raw_value);
    if (ret == XY_DEVICE_OK) {
        ltc2945->data.power_w = raw_value * ltc2945->power_lsb;
    }
    
    /* 读取电荷 (24 位) */
    ret = xy_ltc2945_read24(ltc2945, LTC2945_REG_CHARGE_MSB, &raw_value);
    if (ret == XY_DEVICE_OK) {
        ltc2945->data.charge_c = raw_value * ltc2945->charge_lsb;
    }
    
    /* 读取能量 (24 位) */
    ret = xy_ltc2945_read24(ltc2945, LTC2945_REG_ENERGY_MSB, &raw_value);
    if (ret == XY_DEVICE_OK) {
        ltc2945->data.energy_j = raw_value * ltc2945->energy_lsb;
    }
    
    ltc2945->data.timestamp = xy_hal_sys_get_tick_count();
    
    xy_log_d("LTC2945: V=%.2fV, I=%.2fA, P=%.2fW, Q=%.2fC, E=%.2fJ\n",
             ltc2945->data.voltage_v,
             ltc2945->data.current_a,
             ltc2945->data.power_w,
             ltc2945->data.charge_c,
             ltc2945->data.energy_j);
    
    return XY_LTC2945_OK;
}

int xy_ltc2945_get_voltage(xy_ltc2945_t *ltc2945, float *voltage_v)
{
    if (!ltc2945 || !voltage_v) {
        return XY_LTC2945_INVALID_PARAM;
    }
    
    int ret = xy_ltc2945_read(ltc2945);
    if (ret == XY_LTC2945_OK) {
        *voltage_v = ltc2945->data.voltage_v;
    }
    return ret;
}

int xy_ltc2945_get_current(xy_ltc2945_t *ltc2945, float *current_a)
{
    if (!ltc2945 || !current_a) {
        return XY_LTC2945_INVALID_PARAM;
    }
    
    int ret = xy_ltc2945_read(ltc2945);
    if (ret == XY_LTC2945_OK) {
        *current_a = ltc2945->data.current_a;
    }
    return ret;
}

int xy_ltc2945_get_power(xy_ltc2945_t *ltc2945, float *power_w)
{
    if (!ltc2945 || !power_w) {
        return XY_LTC2945_INVALID_PARAM;
    }
    
    int ret = xy_ltc2945_read(ltc2945);
    if (ret == XY_LTC2945_OK) {
        *power_w = ltc2945->data.power_w;
    }
    return ret;
}

int xy_ltc2945_get_charge(xy_ltc2945_t *ltc2945, float *charge_c)
{
    if (!ltc2945 || !charge_c) {
        return XY_LTC2945_INVALID_PARAM;
    }
    
    int ret = xy_ltc2945_read(ltc2945);
    if (ret == XY_LTC2945_OK) {
        *charge_c = ltc2945->data.charge_c;
    }
    return ret;
}

int xy_ltc2945_get_energy(xy_ltc2945_t *ltc2945, float *energy_j)
{
    if (!ltc2945 || !energy_j) {
        return XY_LTC2945_INVALID_PARAM;
    }
    
    int ret = xy_ltc2945_read(ltc2945);
    if (ret == XY_LTC2945_OK) {
        *energy_j = ltc2945->data.energy_j;
    }
    return ret;
}

int xy_ltc2945_reset_counters(xy_ltc2945_t *ltc2945)
{
    if (!ltc2945) {
        return XY_LTC2945_INVALID_PARAM;
    }
    
    /* 通过配置寄存器复位计数器 */
    uint8_t ctrl = ltc2945->config.auto_convert ? 0x08 : 0x00;
    xy_ltc2945_write_reg(ltc2945, LTC2945_REG_CONTROL, ctrl);
    
    return XY_LTC2945_OK;
}

int xy_ltc2945_enable_alert(xy_ltc2945_t *ltc2945, bool enable)
{
    if (!ltc2945) {
        return XY_LTC2945_INVALID_PARAM;
    }
    
    uint8_t alert = enable ? 0x01 : 0x00;
    return xy_ltc2945_write_reg(ltc2945, LTC2945_REG_ALERT, alert);
}
