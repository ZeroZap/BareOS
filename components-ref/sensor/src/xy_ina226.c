/**
 * @file xy_ina226.c
 * @brief INA226/INA229 Power Monitor Driver
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#include "xy_ina226.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/**
 * @brief 写入寄存器
 */
static int xy_ina_write_reg(xy_ina_t *ina, uint8_t reg, uint16_t value)
{
    uint8_t buf[3];
    buf[0] = reg;
    buf[1] = (value >> 8) & 0xFF;
    buf[2] = value & 0xFF;
    return xy_i2c_device_write(&ina->i2c_dev, buf, 3);
}

/**
 * @brief 读取寄存器
 */
static int xy_ina_read_reg(xy_ina_t *ina, uint8_t reg, uint16_t *value)
{
    uint8_t buf[2];
    int ret = xy_i2c_device_read_reg(&ina->i2c_dev, reg, buf, 2);
    if (ret == XY_DEVICE_OK) {
        *value = ((uint16_t)buf[0] << 8) | buf[1];
    }
    return ret;
}

int xy_ina_init(xy_ina_t *ina, void *i2c_handle, uint8_t addr, const xy_ina_config_t *config)
{
    int ret;
    uint16_t mfg_id, die_id;
    uint16_t config_reg;
    
    if (!ina || !i2c_handle || !config) {
        return XY_INA_INVALID_PARAM;
    }
    
    memset(ina, 0, sizeof(*ina));
    memcpy(&ina->config, config, sizeof(xy_ina_config_t));
    ina->addr = addr;
    
    /* 初始化 I2C */
    xy_i2c_device_init(&ina->i2c_dev, i2c_handle, addr, 1000);
    
    /* 读取 ID */
    ret = xy_ina_read_reg(ina, INA226_REG_MFG_ID, &mfg_id);
    if (ret != XY_DEVICE_OK || mfg_id != INA226_MFG_ID_VALUE) {
        xy_log_e("INA MFG ID mismatch (0x%04X)\n", mfg_id);
        return XY_INA_NOT_FOUND;
    }
    
    ret = xy_ina_read_reg(ina, INA226_REG_DIE_ID, &die_id);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 识别设备类型 */
    if (die_id == INA226_DIE_ID_VALUE) {
        ina->device = XY_INA_DEVICE_INA226;
        xy_log_i("INA226 found at 0x%02X\n", addr);
    } else if (die_id == INA229_DIE_ID_VALUE) {
        ina->device = XY_INA_DEVICE_INA229;
        xy_log_i("INA229 found at 0x%02X\n", addr);
    } else {
        xy_log_e("INA DIE ID unknown (0x%04X)\n", die_id);
        return XY_INA_NOT_FOUND;
    }
    
    /* 计算校准值 */
    /* Current_LSB = Max_Current / 2^15 */
    float max_current = 81.92f;  /* INA226 最大 81.92A */
    ina->current_lsb = max_current / 32768.0f;
    
    /* CAL = 0.00512 / (Current_LSB * Rshunt) */
    float rshunt_ohm = config->shunt_resistor_mohm / 1000.0f;
    ina->calib_value = (uint16_t)(0.00512f / (ina->current_lsb * rshunt_ohm));
    
    xy_log_d("Current_LSB=%.6f A, CAL=%d\n", ina->current_lsb, ina->calib_value);
    
    /* 写入校准寄存器 */
    ret = xy_ina_write_reg(ina, INA226_REG_CALIB, ina->calib_value);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 配置寄存器 */
    /* Bit 15-13: Mode (111=Shunt+Bus continuous) */
    /* Bit 12-10: VBUS ADC (100=1.1ms) */
    /* Bit 9-7: VSHUNT ADC (100=1.1ms) */
    /* Bit 6-4: AVG */
    config_reg = (7 << 13) | (4 << 10) | (4 << 7) | (config->avg_samples << 4);
    
    ret = xy_ina_write_reg(ina, INA226_REG_CONFIG, config_reg);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    ina->initialized = true;
    xy_log_i("INA initialized (Rshunt=%.1f mΩ, AVG=%d)\n",
             config->shunt_resistor_mohm, config->avg_samples);
    
    return XY_INA_OK;
}

int xy_ina_deinit(xy_ina_t *ina)
{
    if (!ina) {
        return XY_INA_INVALID_PARAM;
    }
    
    /* 关闭测量 */
    xy_ina_write_reg(ina, INA226_REG_CONFIG, 0x0000);
    
    ina->initialized = false;
    return XY_INA_OK;
}

int xy_ina_read(xy_ina_t *ina)
{
    int ret;
    uint16_t raw_value;
    int16_t signed_value;
    
    if (!ina || !ina->initialized) {
        return XY_INA_INVALID_PARAM;
    }
    
    /* 读取母线电压 (1.25mV/LSB) */
    ret = xy_ina_read_reg(ina, INA226_REG_BUS_VOLT, &raw_value);
    if (ret == XY_DEVICE_OK) {
        ina->data.voltage_mv = raw_value * 1.25f;
    }
    
    /* 读取分流电压 (2.5uV/LSB) */
    ret = xy_ina_read_reg(ina, INA226_REG_SHUNT_VOLT, (uint16_t*)&signed_value);
    if (ret == XY_DEVICE_OK) {
        ina->data.shunt_voltage_uv = signed_value * 2.5f;
        
        /* 计算电流 */
        float rshunt_ohm = ina->config.shunt_resistor_mohm / 1000.0f;
        ina->data.current_ma = (ina->data.shunt_voltage_uv / 1e6f / rshunt_ohm) * 1000.0f;
    }
    
    /* 读取功率 (25uW/LSB) */
    ret = xy_ina_read_reg(ina, INA226_REG_POWER, &raw_value);
    if (ret == XY_DEVICE_OK) {
        ina->data.power_mw = raw_value * 0.025f;
    }
    
    ina->data.timestamp = xy_os_tick_get();
    
    xy_log_d("INA: V=%.2fV, I=%.2fmA, P=%.2fmW\n",
             ina->data.voltage_mv / 1000.0f,
             ina->data.current_ma,
             ina->data.power_mw);
    
    return XY_INA_OK;
}

int xy_ina_get_voltage(xy_ina_t *ina, float *voltage_mv)
{
    if (!ina || !voltage_mv) {
        return XY_INA_INVALID_PARAM;
    }
    
    int ret = xy_ina_read(ina);
    if (ret == XY_INA_OK) {
        *voltage_mv = ina->data.voltage_mv;
    }
    return ret;
}

int xy_ina_get_current(xy_ina_t *ina, float *current_ma)
{
    if (!ina || !current_ma) {
        return XY_INA_INVALID_PARAM;
    }
    
    int ret = xy_ina_read(ina);
    if (ret == XY_INA_OK) {
        *current_ma = ina->data.current_ma;
    }
    return ret;
}

int xy_ina_get_power(xy_ina_t *ina, float *power_mw)
{
    if (!ina || !power_mw) {
        return XY_INA_INVALID_PARAM;
    }
    
    int ret = xy_ina_read(ina);
    if (ret == XY_INA_OK) {
        *power_mw = ina->data.power_mw;
    }
    return ret;
}

int xy_ina_get_shunt_voltage(xy_ina_t *ina, float *voltage_uv)
{
    if (!ina || !voltage_uv) {
        return XY_INA_INVALID_PARAM;
    }
    
    int ret = xy_ina_read(ina);
    if (ret == XY_INA_OK) {
        *voltage_uv = ina->data.shunt_voltage_uv;
    }
    return ret;
}

int xy_ina_enable_alert(xy_ina_t *ina, bool enable)
{
    uint16_t alert_reg;
    
    if (!ina) {
        return XY_INA_INVALID_PARAM;
    }
    
    if (enable) {
        alert_reg = (1 << 4);  /* SOL - 过流告警 */
    } else {
        alert_reg = 0x0000;
    }
    
    return xy_ina_write_reg(ina, INA226_REG_MASK_EN, alert_reg);
}
