/**
 * @file xy_bq25620.c
 * @brief TI BQ25620 Standalone I2C 1-Cell Li-Ion Battery Charger Driver Implementation
 * @version 1.0.0
 * @date 2026-03-17
 * 
 * @note BQ25620 驱动实现 - 基于 I2C 接口
 */

#include "../inc/xy_bq25620.h"
#include "../inc/xy_charger.h"
#include "xy_hal_i2c.h"
#include <string.h>

/* ==================== Private Functions ==================== */

/**
 * @brief I2C 读取寄存器
 */
static int bq25620_i2c_read(xy_bq25620_t *dev, uint8_t reg, uint8_t *data, uint8_t len)
{
    if (!dev || !dev->i2c_handle || !data) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    void *i2c = dev->i2c_handle;
    
    /* 写入寄存器地址 */
    int32_t ret = xy_hal_i2c_master_transmit(i2c, dev->i2c_addr, &reg, 1, 100);
    if (ret < 0) {
        return XY_DEVICE_ERROR;
    }
    
    /* 读取数据 */
    ret = xy_hal_i2c_master_receive(i2c, dev->i2c_addr, data, len, 100);
    if (ret < 0) {
        return XY_DEVICE_ERROR;
    }
    
    return XY_DEVICE_OK;
}

/**
 * @brief I2C 写入寄存器
 */
static int bq25620_i2c_write(xy_bq25620_t *dev, uint8_t reg, uint8_t data)
{
    if (!dev || !dev->i2c_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    void *i2c = dev->i2c_handle;
    uint8_t tx_buf[2] = {reg, data};
    
    int32_t ret = xy_hal_i2c_master_transmit(i2c, dev->i2c_addr, tx_buf, 2, 100);
    if (ret < 0) {
        return XY_DEVICE_ERROR;
    }
    
    return XY_DEVICE_OK;
}

/**
 * @brief 更新寄存器位
 */
static int bq25620_i2c_update_bits(xy_bq25620_t *dev, uint8_t reg, uint8_t mask, uint8_t value)
{
    uint8_t reg_value;
    int ret = bq25620_i2c_read(dev, reg, &reg_value, 1);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    reg_value = (reg_value & ~mask) | (value & mask);
    
    return bq25620_i2c_write(dev, reg, reg_value);
}

/**
 * @brief 电流转寄存器值
 */
static uint8_t current_to_reg(uint32_t current_mA, uint8_t step, uint8_t min_mA)
{
    if (current_mA < min_mA) {
        return 0;
    }
    
    uint8_t reg_value = (current_mA - min_mA) / step;
    return reg_value;
}

/**
 * @brief 寄存器值转电流
 */
static uint32_t reg_to_current(uint8_t reg_value, uint8_t step, uint8_t min_mA)
{
    return min_mA + (reg_value * step);
}

/**
 * @brief 电压转寄存器值
 */
static uint8_t voltage_to_reg(uint32_t voltage_mV, uint8_t step, uint16_t min_mV)
{
    if (voltage_mV < min_mV) {
        return 0;
    }
    
    uint8_t reg_value = (voltage_mV - min_mV) / step;
    return reg_value;
}

/**
 * @brief 寄存器值转电压
 */
static uint32_t reg_to_voltage(uint8_t reg_value, uint8_t step, uint16_t min_mV)
{
    return min_mV + (reg_value * step);
}

/* ==================== Hardware Operations ==================== */

static int bq25620_hw_init(void *hw_data)
{
    xy_bq25620_t *dev = (xy_bq25620_t *)hw_data;
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 读取设备 ID 验证 */
    uint8_t dev_id;
    int ret = xy_bq25620_read_reg(dev, BQ25620_REG_DEVICE_ID, &dev_id);
    if (ret != XY_DEVICE_OK) {
        return XY_DEVICE_ERROR;
    }
    
    /* 验证型号 */
    if ((dev_id & BQ25620_PART_NUMBER_MASK) != BQ25620_PART_NUMBER) {
        return XY_DEVICE_NOT_SUPPORT;
    }
    
    dev->initialized = true;
    return XY_DEVICE_OK;
}

static int bq25620_hw_read_status(void *hw_data, xy_charger_status_t *status)
{
    xy_bq25620_t *dev = (xy_bq25620_t *)hw_data;
    if (!dev || !status) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    memset(status, 0, sizeof(*status));
    
    /* 读取状态寄存器 */
    uint8_t stat0, stat1;
    if (bq25620_i2c_read(dev, BQ25620_REG_CHG_STAT_0, &stat0, 1) != XY_DEVICE_OK) {
        return XY_DEVICE_ERROR;
    }
    
    if (bq25620_i2c_read(dev, BQ25620_REG_CHG_STAT_1, &stat1, 1) != XY_DEVICE_OK) {
        return XY_DEVICE_ERROR;
    }
    
    /* 解析充电状态 */
    uint8_t chg_stat = stat0 & BQ25620_STAT_CHG_MASK;
    switch (chg_stat) {
        case BQ25620_STAT_CHG_IDLE:
            status->state = XY_CHARGER_STATE_IDLE;
            break;
        case BQ25620_STAT_CHG_PRECHG:
            status->state = XY_CHARGER_STATE_PRE_CHARGE;
            break;
        case BQ25620_STAT_CHG_FAST:
            status->state = XY_CHARGER_STATE_FAST_CHARGE;
            break;
        case BQ25620_STAT_CHG_DONE:
            status->state = XY_CHARGER_STATE_CHARGE_DONE;
            break;
        default:
            status->state = XY_CHARGER_STATE_FAULT;
            break;
    }
    
    /* 解析故障状态 */
    uint8_t fault = stat1 & BQ25620_FAULT_MASK;
    switch (fault) {
        case BQ25620_FAULT_NORMAL:
            status->fault = XY_CHARGER_FAULT_NONE;
            break;
        case BQ25620_FAULT_INPUT_OVP:
            status->fault = XY_CHARGER_FAULT_INPUT_OVP;
            break;
        case BQ25620_FAULT_THERMAL:
            status->fault = XY_CHARGER_FAULT_THERMAL;
            break;
        case BQ25620_FAULT_CHG_TIMEOUT:
            status->fault = XY_CHARGER_FAULT_CHARGE_TIMEOUT;
            break;
        case BQ25620_FAULT_BAT_OVP:
            status->fault = XY_CHARGER_FAULT_BAT_OVP;
            break;
        default:
            status->fault = XY_CHARGER_FAULT_NONE;
            break;
    }
    
    /* Power Good 状态 */
    status->power_good = (stat0 & BQ25620_STAT_PG) ? true : false;
    
    /* 充电中标志 */
    status->charging = (status->state == XY_CHARGER_STATE_PRE_CHARGE ||
                        status->state == XY_CHARGER_STATE_FAST_CHARGE ||
                        status->state == XY_CHARGER_STATE_CONSTANT_VOLT);
    
    /* 充电完成标志 */
    status->done = (status->state == XY_CHARGER_STATE_CHARGE_DONE);
    
    return XY_DEVICE_OK;
}

static int bq25620_hw_set_config(void *hw_data, const xy_charger_config_t *config)
{
    xy_bq25620_t *dev = (xy_bq25620_t *)hw_data;
    if (!dev || !config) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 设置充电电流 */
    uint8_t ichg_reg = current_to_reg(config->charge_current, 
                                       BQ25620_ICHG_STEP_mA, 
                                       BQ25620_ICHG_MIN_mA);
    bq25620_i2c_write(dev, BQ25620_REG_CHG_CTRL_1, ichg_reg & BQ25620_ICHG_MASK);
    
    /* 设置充电电压 */
    uint8_t vreg_reg = voltage_to_reg(config->charge_voltage,
                                       BQ25620_VREG_STEP_mV,
                                       BQ25620_VREG_MIN_mV);
    bq25620_i2c_write(dev, BQ25620_REG_CHG_CTRL_3, vreg_reg & BQ25620_VREG_MASK);
    
    /* 设置输入电流限制 */
    uint8_t ilim_reg = current_to_reg(config->input_current_limit,
                                       BQ25620_ILIM_STEP_mA,
                                       BQ25620_ILIM_MIN_mA);
    uint8_t ilim_value = (ilim_reg & BQ25620_ILIM_MASK) | BQ25620_EN_ILIM;
    bq25620_i2c_write(dev, BQ25620_REG_CHG_CTRL_4, ilim_value);
    
    /* 设置预充电电流和终止电流 */
    uint8_t prechg_reg = current_to_reg(config->precharge_current, 64, 64);
    uint8_t iterm_reg = current_to_reg(config->termination_current, 64, 64);
    uint8_t prechg_iterm = ((iterm_reg & 0x0F) << 4) | (prechg_reg & 0x0F);
    bq25620_i2c_write(dev, BQ25620_REG_CHG_CTRL_2, prechg_iterm);
    
    /* 设置再充电阈值和自动再充电 */
    uint8_t ctrl5 = 0;
    if (config->auto_recharge) {
        ctrl5 |= BQ25620_AUTO_RECHG;
    }
    uint8_t vrechg_reg = config->recharge_threshold / 100; /* 100mV 步进 */
    ctrl5 |= (vrechg_reg & 0x03) << 6;
    bq25620_i2c_write(dev, BQ25620_REG_CHG_CTRL_5, ctrl5);
    
    return XY_DEVICE_OK;
}

static int bq25620_hw_enable(void *hw_data, bool enable)
{
    xy_bq25620_t *dev = (xy_bq25620_t *)hw_data;
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    if (enable) {
        return bq25620_i2c_update_bits(dev, BQ25620_REG_CHG_CTRL_0,
                                        BQ25620_EN_CHG, BQ25620_EN_CHG);
    } else {
        return bq25620_i2c_update_bits(dev, BQ25620_REG_CHG_CTRL_0,
                                        BQ25620_EN_CHG, 0);
    }
}

static int bq25620_hw_read_reg(void *hw_data, uint8_t reg, uint8_t *value)
{
    xy_bq25620_t *dev = (xy_bq25620_t *)hw_data;
    if (!dev || !value) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return bq25620_i2c_read(dev, reg, value, 1);
}

static int bq25620_hw_write_reg(void *hw_data, uint8_t reg, uint8_t value)
{
    xy_bq25620_t *dev = (xy_bq25620_t *)hw_data;
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return bq25620_i2c_write(dev, reg, value);
}

/* ==================== Public API Implementation ==================== */

int xy_bq25620_init(xy_bq25620_t *dev, void *i2c_handle, uint8_t i2c_addr)
{
    if (!dev || !i2c_handle) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    memset(dev, 0, sizeof(*dev));
    
    dev->i2c_handle = i2c_handle;
    dev->i2c_addr = i2c_addr;
    
    /* 设置硬件操作接口 */
    dev->base.hw_init = bq25620_hw_init;
    dev->base.hw_read_status = bq25620_hw_read_status;
    dev->base.hw_set_config = bq25620_hw_set_config;
    dev->base.hw_enable = bq25620_hw_enable;
    dev->base.hw_read_reg = bq25620_hw_read_reg;
    dev->base.hw_write_reg = bq25620_hw_write_reg;
    dev->base.hw_data = dev;
    
    /* 初始化硬件 */
    return bq25620_hw_init(dev);
}

int xy_bq25620_deinit(xy_bq25620_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 停止充电 */
    xy_bq25620_stop_charge(dev);
    
    dev->initialized = false;
    return XY_DEVICE_OK;
}

int xy_bq25620_read_reg(xy_bq25620_t *dev, uint8_t reg, uint8_t *value)
{
    if (!dev || !value) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return bq25620_i2c_read(dev, reg, value, 1);
}

int xy_bq25620_write_reg(xy_bq25620_t *dev, uint8_t reg, uint8_t value)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return bq25620_i2c_write(dev, reg, value);
}

int xy_bq25620_get_device_id(xy_bq25620_t *dev, uint8_t *id)
{
    if (!dev || !id) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return bq25620_i2c_read(dev, BQ25620_REG_DEVICE_ID, id, 1);
}

int xy_bq25620_get_status(xy_bq25620_t *dev, xy_charger_status_t *status)
{
    if (!dev || !status) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return bq25620_hw_read_status(dev, status);
}

int xy_bq25620_set_charge_current(xy_bq25620_t *dev, uint32_t current_mA)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 限制电流范围 */
    if (current_mA < BQ25620_ICHG_MIN_mA) {
        current_mA = BQ25620_ICHG_MIN_mA;
    } else if (current_mA > BQ25620_ICHG_MAX_mA) {
        current_mA = BQ25620_ICHG_MAX_mA;
    }
    
    uint8_t ichg_reg = current_to_reg(current_mA, BQ25620_ICHG_STEP_mA, BQ25620_ICHG_MIN_mA);
    return bq25620_i2c_write(dev, BQ25620_REG_CHG_CTRL_1, ichg_reg & BQ25620_ICHG_MASK);
}

int xy_bq25620_set_charge_voltage(xy_bq25620_t *dev, uint32_t voltage_mV)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 限制电压范围 */
    if (voltage_mV < BQ25620_VREG_MIN_mV) {
        voltage_mV = BQ25620_VREG_MIN_mV;
    } else if (voltage_mV > BQ25620_VREG_MAX_mV) {
        voltage_mV = BQ25620_VREG_MAX_mV;
    }
    
    uint8_t vreg_reg = voltage_to_reg(voltage_mV, BQ25620_VREG_STEP_mV, BQ25620_VREG_MIN_mV);
    return bq25620_i2c_write(dev, BQ25620_REG_CHG_CTRL_3, vreg_reg & BQ25620_VREG_MASK);
}

int xy_bq25620_set_input_limit(xy_bq25620_t *dev, uint32_t current_mA)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 限制电流范围 */
    if (current_mA < BQ25620_ILIM_MIN_mA) {
        current_mA = BQ25620_ILIM_MIN_mA;
    } else if (current_mA > BQ25620_ILIM_MAX_mA) {
        current_mA = BQ25620_ILIM_MAX_mA;
    }
    
    uint8_t ilim_reg = current_to_reg(current_mA, BQ25620_ILIM_STEP_mA, BQ25620_ILIM_MIN_mA);
    uint8_t ilim_value = (ilim_reg & BQ25620_ILIM_MASK) | BQ25620_EN_ILIM;
    return bq25620_i2c_write(dev, BQ25620_REG_CHG_CTRL_4, ilim_value);
}

int xy_bq25620_start_charge(xy_bq25620_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return bq25620_hw_enable(dev, true);
}

int xy_bq25620_stop_charge(xy_bq25620_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    return bq25620_hw_enable(dev, false);
}

/* ==================== End of File ==================== */
