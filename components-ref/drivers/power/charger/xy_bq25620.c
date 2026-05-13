/**
 * @file xy_bq25620.c
 * @brief TI BQ25620 Standalone I2C 1-Cell Li-Ion Battery Charger Driver Implementation
 * @version 1.0.0
 * @date 2026-03-18
 * 
 * @note BQ25620 是一款独立的 I2C 控制的 1 节锂离子电池充电器
 * 
 * 主要特性:
 * - 输入电压范围：3.5V - 13.5V
 * - 充电电流：最高 2A (实际可达 5A)
 * - 充电电压：4.2V (默认，可配置 3.5V-4.47V)
 * - I2C 接口配置 (地址 0x6A)
 * - 热调节和过温保护
 * - 输入过压保护
 * - 电池温度监测 (TS 引脚)
 * 
 * 使用示例:
 * @code
 * xy_bq25620_t bq25620;
 * xy_bq25620_init(&bq25620, i2c_handle, 0x6A);
 * 
 * // 设置充电参数
 * xy_bq25620_set_charge_current(&bq25620, 1000);  // 1A
 * xy_bq25620_set_charge_voltage(&bq25620, 4200);  // 4.2V
 * xy_bq25620_set_input_limit(&bq25620, 2000);     // 2A 输入限制
 * 
 * // 启动充电
 * xy_bq25620_start_charge(&bq25620);
 * 
 * // 读取状态
 * xy_charger_status_t status;
 * xy_bq25620_get_status(&bq25620, &status);
 * @endcode
 */

#include "xy_bq25620.h"
#include "xy_device.h"
#include "xy_log.h"
#include <string.h>

#define XY_LOG_TAG "BQ25620"

/* ==================== Helper Macros ==================== */

/**
 * @brief I2C 写寄存器
 */
#define BQ25620_I2C_WRITE(dev, reg, val) \
    xy_i2c_device_write_reg((xy_i2c_device_t*)(dev)->i2c_handle, (reg), &(val), 1)

/**
 * @brief I2C 读寄存器
 */
#define BQ25620_I2C_READ(dev, reg, val) \
    xy_i2c_device_read_reg((xy_i2c_device_t*)(dev)->i2c_handle, (reg), &(val), 1)

/**
 * @brief 钳位值到范围
 */
#define CLAMP(val, min, max) \
    ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

/* ==================== Private Functions ==================== */

/**
 * @brief 将充电电流转换为寄存器值
 * @param current_mA 充电电流 (mA)
 * @return 寄存器值
 */
static uint8_t bq25620_current_to_reg(uint32_t current_mA)
{
    uint32_t clamped = CLAMP(current_mA, BQ25620_ICHG_MIN_mA, BQ25620_ICHG_MAX_mA);
    uint32_t steps = (clamped - BQ25620_ICHG_MIN_mA) / BQ25620_ICHG_STEP_mA;
    return (uint8_t)(steps & BQ25620_ICHG_MASK);
}

/**
 * @brief 将寄存器值转换为充电电流
 * @param reg 寄存器值
 * @return 充电电流 (mA)
 */
static uint32_t bq25620_reg_to_current(uint8_t reg)
{
    uint32_t steps = reg & BQ25620_ICHG_MASK;
    return BQ25620_ICHG_MIN_mA + (steps * BQ25620_ICHG_STEP_mA);
}

/**
 * @brief 将充电电压转换为寄存器值
 * @param voltage_mV 充电电压 (mV)
 * @return 寄存器值
 */
static uint8_t bq25620_voltage_to_reg(uint32_t voltage_mV)
{
    uint32_t clamped = CLAMP(voltage_mV, BQ25620_VREG_MIN_mV, BQ25620_VREG_MAX_mV);
    uint32_t steps = (clamped - BQ25620_VREG_MIN_mV) / BQ25620_VREG_STEP_mV;
    return (uint8_t)(steps & BQ25620_VREG_MASK);
}

/**
 * @brief 将寄存器值转换为充电电压
 * @param reg 寄存器值
 * @return 充电电压 (mV)
 */
static uint32_t bq25620_reg_to_voltage(uint8_t reg)
{
    uint32_t steps = reg & BQ25620_VREG_MASK;
    return BQ25620_VREG_MIN_mV + (steps * BQ25620_VREG_STEP_mV);
}

/**
 * @brief 将输入电流限制转换为寄存器值
 * @param current_mA 输入电流限制 (mA)
 * @return 寄存器值
 */
static uint8_t bq25620_ilim_to_reg(uint32_t current_mA)
{
    uint32_t clamped = CLAMP(current_mA, BQ25620_ILIM_MIN_mA, BQ25620_ILIM_MAX_mA);
    uint32_t steps = (clamped - BQ25620_ILIM_MIN_mA) / BQ25620_ILIM_STEP_mA;
    return (uint8_t)(steps & BQ25620_ILIM_MASK);
}

/* ==================== Public API Implementation ==================== */

int xy_bq25620_init(xy_bq25620_t *dev, void *i2c_handle, uint8_t i2c_addr)
{
    if (!dev || !i2c_handle) {
        XY_LOG_ERROR("Invalid parameters");
        return XY_ERROR;
    }

    /* 初始化设备结构 */
    memset(dev, 0, sizeof(xy_bq25620_t));
    
    /* 初始化 I2C 设备 */
    xy_i2c_device_t *i2c_dev = (xy_i2c_device_t *)i2c_handle;
    xy_i2c_device_init(i2c_dev, i2c_dev->i2c_handle, i2c_addr, 1000);
    
    dev->i2c_handle = i2c_dev;
    dev->i2c_addr = i2c_addr;
    dev->initialized = false;

    XY_LOG_INFO("Initializing BQ25620 at I2C address 0x%02X", i2c_addr);

    /* 读取设备 ID 验证通信 */
    uint8_t device_id = 0;
    int ret = xy_bq25620_get_device_id(dev, &device_id);
    if (ret != XY_OK) {
        XY_LOG_ERROR("Failed to read device ID: %d", ret);
        return ret;
    }

    /* 验证 Part Number */
    uint8_t part_number = (device_id & BQ25620_PART_NUMBER_MASK) >> 2;
    if (part_number != (BQ25620_PART_NUMBER >> 2)) {
        XY_LOG_ERROR("Invalid part number: 0x%02X (expected 0x%02X)", 
                     part_number, BQ25620_PART_NUMBER >> 2);
        return XY_ERROR;
    }

    XY_LOG_INFO("BQ25620 found, part number: 0x%02X", part_number);

    /* 默认配置：充电使能，终止检测使能，自动再充电 */
    uint8_t ctrl0 = BQ25620_EN_CHG | BQ25620_EN_TERM | BQ25620_AUTO_RECHG;
    ret = xy_bq25620_write_reg(dev, BQ25620_REG_CHG_CTRL_0, ctrl0);
    if (ret != XY_OK) {
        XY_LOG_ERROR("Failed to write CHG_CTRL_0: %d", ret);
        return ret;
    }

    /* 默认充电电流：512mA */
    ret = xy_bq25620_set_charge_current(dev, 512);
    if (ret != XY_OK) {
        XY_LOG_ERROR("Failed to set default charge current: %d", ret);
        return ret;
    }

    /* 默认充电电压：4.2V */
    ret = xy_bq25620_set_charge_voltage(dev, 4200);
    if (ret != XY_OK) {
        XY_LOG_ERROR("Failed to set default charge voltage: %d", ret);
        return ret;
    }

    /* 默认输入电流限制：2A */
    ret = xy_bq25620_set_input_limit(dev, 2000);
    if (ret != XY_OK) {
        XY_LOG_ERROR("Failed to set default input limit: %d", ret);
        return ret;
    }

    dev->initialized = true;
    XY_LOG_INFO("BQ25620 initialized successfully");

    return XY_OK;
}

int xy_bq25620_deinit(xy_bq25620_t *dev)
{
    if (!dev) {
        return XY_ERROR;
    }

    if (dev->initialized) {
        /* 停止充电 */
        xy_bq25620_stop_charge(dev);
        dev->initialized = false;
        XY_LOG_INFO("BQ25620 deinitialized");
    }

    return XY_OK;
}

int xy_bq25620_read_reg(xy_bq25620_t *dev, uint8_t reg, uint8_t *value)
{
    if (!dev || !value || !dev->initialized) {
        return XY_ERROR;
    }

    int ret = BQ25620_I2C_READ(dev, reg, value);
    if (ret != XY_OK) {
        XY_LOG_ERROR("I2C read failed: reg=0x%02X, ret=%d", reg, ret);
        return XY_ERROR;
    }

    return XY_OK;
}

int xy_bq25620_write_reg(xy_bq25620_t *dev, uint8_t reg, uint8_t value)
{
    if (!dev || !dev->initialized) {
        return XY_ERROR;
    }

    int ret = BQ25620_I2C_WRITE(dev, reg, value);
    if (ret != XY_OK) {
        XY_LOG_ERROR("I2C write failed: reg=0x%02X, ret=%d", reg, ret);
        return XY_ERROR;
    }

    return XY_OK;
}

int xy_bq25620_get_device_id(xy_bq25620_t *dev, uint8_t *id)
{
    if (!dev || !id) {
        return XY_ERROR;
    }

    return xy_bq25620_read_reg(dev, BQ25620_REG_DEVICE_ID, id);
}

int xy_bq25620_get_status(xy_bq25620_t *dev, xy_charger_status_t *status)
{
    if (!dev || !status) {
        return XY_ERROR;
    }

    if (!dev->initialized) {
        return XY_ERROR;
    }

    /* 读取充电状态寄存器 */
    uint8_t stat0 = 0, stat1 = 0;
    int ret = xy_bq25620_read_reg(dev, BQ25620_REG_CHG_STAT_0, &stat0);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    ret = xy_bq25620_read_reg(dev, BQ25620_REG_CHG_STAT_1, &stat1);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    /* 解析充电状态 */
    uint8_t charge_state = (stat0 & BQ25620_STAT_CHG_MASK);
    switch (charge_state) {
        case BQ25620_STAT_CHG_IDLE:
            status->state = XY_CHARGER_STATE_IDLE;
            status->charging = false;
            status->done = false;
            break;
        case BQ25620_STAT_CHG_PRECHG:
            status->state = XY_CHARGER_STATE_PRE_CHARGE;
            status->charging = true;
            status->done = false;
            break;
        case BQ25620_STAT_CHG_FAST:
            status->state = XY_CHARGER_STATE_FAST_CHARGE;
            status->charging = true;
            status->done = false;
            break;
        case BQ25620_STAT_CHG_DONE:
            status->state = XY_CHARGER_STATE_CHARGE_DONE;
            status->charging = false;
            status->done = true;
            break;
        default:
            status->state = XY_CHARGER_STATE_FAULT;
            status->charging = false;
            status->done = false;
            break;
    }

    /* 解析故障状态 */
    uint8_t fault = (stat1 & BQ25620_FAULT_MASK);
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

    /* 温度状态 (简化处理) */
    uint8_t therm = (stat0 & BQ25620_STAT_THERM_MASK);
    if (therm == 0x01) {
        status->temperature = 0;  /* 冷 */
    } else if (therm == 0x02) {
        status->temperature = 100; /* 热 */
    } else {
        status->temperature = 50;  /* 正常 */
    }

    /* 读取 ADC 数据 (可选) */
    /* 注意：需要启用 ADC 才能读取电压/电流值 */
    status->input_voltage = 0;
    status->bat_voltage = 0;
    status->charge_current = 0;
    status->input_current = 0;

    return XY_OK;
}

int xy_bq25620_set_charge_current(xy_bq25620_t *dev, uint32_t current_mA)
{
    if (!dev || !dev->initialized) {
        return XY_ERROR;
    }

    uint8_t reg_val = bq25620_current_to_reg(current_mA);
    int ret = xy_bq25620_write_reg(dev, BQ25620_REG_CHG_CTRL_1, reg_val);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Failed to set charge current: %d", ret);
        return ret;
    }

    XY_LOG_INFO("Charge current set to %lu mA", current_mA);
    return XY_OK;
}

int xy_bq25620_set_charge_voltage(xy_bq25620_t *dev, uint32_t voltage_mV)
{
    if (!dev || !dev->initialized) {
        return XY_ERROR;
    }

    uint8_t reg_val = bq25620_voltage_to_reg(voltage_mV);
    int ret = xy_bq25620_write_reg(dev, BQ25620_REG_CHG_CTRL_3, reg_val);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Failed to set charge voltage: %d", ret);
        return ret;
    }

    XY_LOG_INFO("Charge voltage set to %lu mV", voltage_mV);
    return XY_OK;
}

int xy_bq25620_set_input_limit(xy_bq25620_t *dev, uint32_t current_mA)
{
    if (!dev || !dev->initialized) {
        return XY_ERROR;
    }

    uint8_t reg_val = bq25620_ilim_to_reg(current_mA);
    /* 使能输入电流限制 */
    reg_val |= BQ25620_EN_ILIM;
    
    int ret = xy_bq25620_write_reg(dev, BQ25620_REG_CHG_CTRL_4, reg_val);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Failed to set input limit: %d", ret);
        return ret;
    }

    XY_LOG_INFO("Input current limit set to %lu mA", current_mA);
    return XY_OK;
}

int xy_bq25620_start_charge(xy_bq25620_t *dev)
{
    if (!dev || !dev->initialized) {
        return XY_ERROR;
    }

    /* 读取当前 CHG_CTRL_0 寄存器 */
    uint8_t ctrl0 = 0;
    int ret = xy_bq25620_read_reg(dev, BQ25620_REG_CHG_CTRL_0, &ctrl0);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    /* 使能充电 */
    ctrl0 |= BQ25620_EN_CHG;
    
    ret = xy_bq25620_write_reg(dev, BQ25620_REG_CHG_CTRL_0, ctrl0);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Failed to start charging: %d", ret);
        return ret;
    }

    XY_LOG_INFO("Charging started");
    return XY_OK;
}

int xy_bq25620_stop_charge(xy_bq25620_t *dev)
{
    if (!dev || !dev->initialized) {
        return XY_ERROR;
    }

    /* 读取当前 CHG_CTRL_0 寄存器 */
    uint8_t ctrl0 = 0;
    int ret = xy_bq25620_read_reg(dev, BQ25620_REG_CHG_CTRL_0, &ctrl0);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    /* 禁用充电 */
    ctrl0 &= ~BQ25620_EN_CHG;
    
    ret = xy_bq25620_write_reg(dev, BQ25620_REG_CHG_CTRL_0, ctrl0);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Failed to stop charging: %d", ret);
        return ret;
    }

    XY_LOG_INFO("Charging stopped");
    return XY_OK;
}

/* ==================== Charger Framework Integration ==================== */

/**
 * @brief 充电器框架回调：初始化
 */
static int bq25620_hw_init(void *hw_data)
{
    xy_bq25620_t *dev = (xy_bq25620_t *)hw_data;
    if (!dev) {
        return XY_ERROR;
    }
    
    /* I2C 句柄和地址已在 xy_bq25620_init 中设置 */
    return dev->initialized ? XY_DEVICE_OK : XY_DEVICE_EINVAL;
}

/**
 * @brief 充电器框架回调：读取状态
 */
static int bq25620_hw_read_status(void *hw_data, xy_charger_status_t *status)
{
    xy_bq25620_t *dev = (xy_bq25620_t *)hw_data;
    if (!dev || !status) {
        return XY_ERROR;
    }
    
    return xy_bq25620_get_status(dev, status);
}

/**
 * @brief 充电器框架回调：设置配置
 */
static int bq25620_hw_set_config(void *hw_data, const xy_charger_config_t *config)
{
    xy_bq25620_t *dev = (xy_bq25620_t *)hw_data;
    if (!dev || !config) {
        return XY_ERROR;
    }
    
    int ret;
    
    /* 设置输入电流限制 */
    ret = xy_bq25620_set_input_limit(dev, config->input_current_limit);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 设置充电电流 */
    ret = xy_bq25620_set_charge_current(dev, config->charge_current);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 设置充电电压 */
    ret = xy_bq25620_set_charge_voltage(dev, config->charge_voltage);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    return XY_OK;
}

/**
 * @brief 充电器框架回调：使能/禁用
 */
static int bq25620_hw_enable(void *hw_data, bool enable)
{
    xy_bq25620_t *dev = (xy_bq25620_t *)hw_data;
    if (!dev) {
        return XY_ERROR;
    }
    
    return enable ? xy_bq25620_start_charge(dev) : xy_bq25620_stop_charge(dev);
}

/**
 * @brief 充电器框架回调：读寄存器
 */
static int bq25620_hw_read_reg(void *hw_data, uint8_t reg, uint8_t *value)
{
    xy_bq25620_t *dev = (xy_bq25620_t *)hw_data;
    if (!dev || !value) {
        return XY_ERROR;
    }
    
    return xy_bq25620_read_reg(dev, reg, value);
}

/**
 * @brief 充电器框架回调：写寄存器
 */
static int bq25620_hw_write_reg(void *hw_data, uint8_t reg, uint8_t value)
{
    xy_bq25620_t *dev = (xy_bq25620_t *)hw_data;
    if (!dev) {
        return XY_ERROR;
    }
    
    return xy_bq25620_write_reg(dev, reg, value);
}

/**
 * @brief 将 BQ25620 设备注册到充电器框架
 * @param charger 充电器框架句柄
 * @param bq25620 BQ25620 设备句柄
 * @param config 充电器配置
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bq25620_register_charger(xy_charger_t *charger, xy_bq25620_t *bq25620, 
                                 const xy_charger_config_t *config)
{
    if (!charger || !bq25620 || !config) {
        return XY_ERROR;
    }
    
    /* 初始化充电器基类 */
    memset(charger, 0, sizeof(xy_charger_t));
    
    /* 设置硬件操作接口 */
    charger->hw_init = bq25620_hw_init;
    charger->hw_read_status = bq25620_hw_read_status;
    charger->hw_set_config = bq25620_hw_set_config;
    charger->hw_enable = bq25620_hw_enable;
    charger->hw_read_reg = bq25620_hw_read_reg;
    charger->hw_write_reg = bq25620_hw_write_reg;
    charger->hw_data = bq25620;
    charger->config = config;
    
    /* 初始化充电器框架 */
    return xy_charger_init(charger, config);
}
