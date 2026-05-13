/**
 * @file xy_coulomb.c
 * @brief Coulomb Counter Fuel Gauge Driver (INA226/INA219)
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#include "xy_coulomb.h"
#include "xy_log.h"
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/**
 * @brief 写入寄存器
 */
static int xy_coulomb_write_reg(xy_coulomb_t *coulomb, uint8_t reg, uint16_t value)
{
    uint8_t buf[3];
    buf[0] = reg;
    buf[1] = (value >> 8) & 0xFF;
    buf[2] = value & 0xFF;
    return xy_i2c_device_write(&coulomb->i2c_dev, buf, 3);
}

/**
 * @brief 读取寄存器
 */
static int xy_coulomb_read_reg(xy_coulomb_t *coulomb, uint8_t reg, uint16_t *value)
{
    uint8_t buf[2];
    int ret = xy_i2c_device_read_reg(&coulomb->i2c_dev, reg, buf, 2);
    if (ret == XY_DEVICE_OK) {
        *value = ((uint16_t)buf[0] << 8) | buf[1];
    }
    return ret;
}

int xy_coulomb_init(xy_coulomb_t *coulomb, void *i2c_handle, uint8_t addr,
                    const xy_coulomb_config_t *config)
{
    int ret;
    uint16_t mfg_id, die_id;
    uint16_t config_reg;
    
    if (!coulomb || !i2c_handle || !config) {
        return XY_COULOMB_INVALID_PARAM;
    }
    
    memset(coulomb, 0, sizeof(*coulomb));
    memcpy(&coulomb->config, config, sizeof(xy_coulomb_config_t));
    
    /* 初始化 I2C */
    xy_i2c_device_init(&coulomb->i2c_dev, i2c_handle, addr, 1000);
    coulomb->addr = addr;
    
    /* 读取 ID 验证设备 */
    ret = xy_coulomb_read_reg(coulomb, INA226_REG_MFG_ID, &mfg_id);
    if (ret != XY_DEVICE_OK || mfg_id != INA226_MFG_ID_VALUE) {
        xy_log_e("INA226 MFG ID mismatch (0x%04X)\n", mfg_id);
        return XY_COULOMB_NOT_FOUND;
    }
    
    ret = xy_coulomb_read_reg(coulomb, INA226_REG_DIE_ID, &die_id);
    if (ret != XY_DEVICE_OK || die_id != INA226_DIE_ID_VALUE) {
        xy_log_e("INA226 DIE ID mismatch (0x%04X)\n", die_id);
        return XY_COULOMB_NOT_FOUND;
    }
    
    xy_log_i("INA226 found at 0x%02X (MFG=0x%04X, DIE=0x%04X)\n", 
             addr, mfg_id, die_id);
    
    /* 计算校准值 */
    /* Current_LSB = Capacity / 2^15 (A) */
    coulomb->current_lsb = config->capacity_mah / 32768.0f / 1000.0f;  /* A */
    
    /* CAL = 0.00512 / (Current_LSB * Rshunt) */
    float rshunt_ohm = config->shunt_resistor_mohm / 1000.0f;
    coulomb->calib_value = (uint16_t)(0.00512f / (coulomb->current_lsb * rshunt_ohm));
    
    xy_log_d("Current_LSB=%.6f A, CAL=%d\n", coulomb->current_lsb, coulomb->calib_value);
    
    /* 写入校准寄存器 */
    ret = xy_coulomb_write_reg(coulomb, INA226_REG_CALIB, coulomb->calib_value);
    if (ret != XY_DEVICE_OK) {
        return XY_COULOMB_ERROR;
    }
    
    /* 配置寄存器 */
    /* Bit 15-13: Mode (111=Shunt+Bus continuous) */
    /* Bit 12-10: VBUS ADC (100=1.1ms) */
    /* Bit 9-7: VSHUNT ADC (100=1.1ms) */
    /* Bit 6-4: AVG (100=16 samples) */
    config_reg = (7 << 13) | (4 << 10) | (4 << 7) | (config->avg_samples << 4);
    
    ret = xy_coulomb_write_reg(coulomb, INA226_REG_CONFIG, config_reg);
    if (ret != XY_DEVICE_OK) {
        return XY_COULOMB_ERROR;
    }
    
    /* 重置累计容量 */
    xy_coulomb_reset_charge(coulomb);
    
    coulomb->initialized = true;
    xy_log_i("INA226 initialized (Capacity=%.0f mAh, Rshunt=%.1f mΩ)\n",
             config->capacity_mah, config->shunt_resistor_mohm);
    
    return XY_COULOMB_OK;
}

int xy_coulomb_deinit(xy_coulomb_t *coulomb)
{
    if (!coulomb) {
        return XY_COULOMB_INVALID_PARAM;
    }
    
    /* 关闭测量 */
    xy_coulomb_write_reg(coulomb, INA226_REG_CONFIG, 0x0000);
    
    coulomb->initialized = false;
    return XY_COULOMB_OK;
}

int xy_coulomb_read(xy_coulomb_t *coulomb)
{
    int ret;
    uint16_t raw_value;
    int16_t signed_value;
    
    if (!coulomb || !coulomb->initialized) {
        return XY_COULOMB_INVALID_PARAM;
    }
    
    /* 读取母线电压 (1.25mV/LSB) */
    ret = xy_coulomb_read_reg(coulomb, INA226_REG_BUS_VOLT, &raw_value);
    if (ret == XY_DEVICE_OK) {
        coulomb->data.voltage_mv = raw_value * 1.25f;
    }
    
    /* 读取分流电压 */
    ret = xy_coulomb_read_reg(coulomb, INA226_REG_SHUNT_VOLT, (uint16_t*)&signed_value);
    if (ret == XY_DEVICE_OK) {
        /* 2.5uV/LSB */
        float shunt_voltage_v = signed_value * 2.5e-6f;
        float rshunt_ohm = coulomb->config.shunt_resistor_mohm / 1000.0f;
        coulomb->data.current_ma = (shunt_voltage_v / rshunt_ohm) * 1000.0f;
    }
    
    /* 读取功率 (25uW/LSB) */
    ret = xy_coulomb_read_reg(coulomb, INA226_REG_POWER, &raw_value);
    if (ret == XY_DEVICE_OK) {
        coulomb->data.power_mw = raw_value * 0.025f;
    }
    
    /* 读取累计容量 (Current_LSB/LSB) */
    ret = xy_coulomb_read_reg(coulomb, INA226_REG_CURRENT, (uint16_t*)&signed_value);
    if (ret == XY_DEVICE_OK) {
        coulomb->data.charge_mah = signed_value * coulomb->current_lsb * 1000.0f;
        
        /* 计算电量百分比 */
        if (coulomb->config.capacity_mah > 0) {
            float used_percentage = (coulomb->data.charge_mah / coulomb->config.capacity_mah) * 100.0f;
            coulomb->data.percentage = 100.0f - used_percentage;
            
            /* 限制在 0-100% */
            if (coulomb->data.percentage < 0) coulomb->data.percentage = 0;
            if (coulomb->data.percentage > 100) coulomb->data.percentage = 100;
        }
    }
    
    coulomb->data.timestamp = xy_hal_sys_get_tick_count();
    
    xy_log_d("INA226: V=%.2fV, I=%.2fmA, P=%.2fmW, SOC=%.1f%%\n",
             coulomb->data.voltage_mv / 1000.0f,
             coulomb->data.current_ma,
             coulomb->data.power_mw,
             coulomb->data.percentage);
    
    return XY_COULOMB_OK;
}

int xy_coulomb_get_voltage(xy_coulomb_t *coulomb, float *voltage_mv)
{
    if (!coulomb || !voltage_mv) {
        return XY_COULOMB_INVALID_PARAM;
    }
    
    int ret = xy_coulomb_read(coulomb);
    if (ret == XY_COULOMB_OK) {
        *voltage_mv = coulomb->data.voltage_mv;
    }
    return ret;
}

int xy_coulomb_get_current(xy_coulomb_t *coulomb, float *current_ma)
{
    if (!coulomb || !current_ma) {
        return XY_COULOMB_INVALID_PARAM;
    }
    
    int ret = xy_coulomb_read(coulomb);
    if (ret == XY_COULOMB_OK) {
        *current_ma = coulomb->data.current_ma;
    }
    return ret;
}

int xy_coulomb_get_power(xy_coulomb_t *coulomb, float *power_mw)
{
    if (!coulomb || !power_mw) {
        return XY_COULOMB_INVALID_PARAM;
    }
    
    int ret = xy_coulomb_read(coulomb);
    if (ret == XY_COULOMB_OK) {
        *power_mw = coulomb->data.power_mw;
    }
    return ret;
}

int xy_coulomb_get_charge(xy_coulomb_t *coulomb, float *charge_mah)
{
    if (!coulomb || !charge_mah) {
        return XY_COULOMB_INVALID_PARAM;
    }
    
    int ret = xy_coulomb_read(coulomb);
    if (ret == XY_COULOMB_OK) {
        *charge_mah = coulomb->data.charge_mah;
    }
    return ret;
}

int xy_coulomb_get_percentage(xy_coulomb_t *coulomb, float *percentage)
{
    if (!coulomb || !percentage) {
        return XY_COULOMB_INVALID_PARAM;
    }
    
    int ret = xy_coulomb_read(coulomb);
    if (ret == XY_COULOMB_OK) {
        *percentage = coulomb->data.percentage;
    }
    return ret;
}

int xy_coulomb_reset_charge(xy_coulomb_t *coulomb)
{
    if (!coulomb) {
        return XY_COULOMB_INVALID_PARAM;
    }
    
    /* 写入 0 到累计容量寄存器 */
    return xy_coulomb_write_reg(coulomb, INA226_REG_CURRENT, 0x0000);
}

int xy_coulomb_set_capacity(xy_coulomb_t *coulomb, float capacity_mah)
{
    if (!coulomb || capacity_mah <= 0) {
        return XY_COULOMB_INVALID_PARAM;
    }
    
    coulomb->config.capacity_mah = capacity_mah;
    return XY_COULOMB_OK;
}

int xy_coulomb_enable_alert(xy_coulomb_t *coulomb, bool enable)
{
    uint16_t alert_reg;
    
    if (!coulomb) {
        return XY_COULOMB_INVALID_PARAM;
    }
    
    if (enable) {
        /* 配置告警引脚功能 */
        alert_reg = (1 << 4);  /* SOL - 累计容量超限告警 */
    } else {
        alert_reg = 0x0000;
    }
    
    return xy_coulomb_write_reg(coulomb, INA226_REG_MASK_EN, alert_reg);
}
