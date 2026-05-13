/**
 * @file xy_bq25620.c
 * @brief BQ25620 Li-Ion Battery Charger Driver
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#include "xy_bq25620.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* 寄存器位定义 */
#define BQ25620_STAT_CHG_MASK       (3 << 4)
#define BQ25620_STAT_PG_MASK        (1 << 3)
#define BQ25620_STAT_DPM_MASK       (1 << 2)
#define BQ25620_FAULT_MASK          (7 << 5)
#define BQ25620_CHG_EN_MASK         (1 << 7)

/**
 * @brief 写入寄存器
 */
static int xy_bq25620_write_reg(xy_bq25620_t *bq, uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    return xy_i2c_device_write(&bq->i2c_dev, buf, 2);
}

/**
 * @brief 读取寄存器
 */
static int xy_bq25620_read_reg(xy_bq25620_t *bq, uint8_t reg, uint8_t *value)
{
    return xy_i2c_device_read_reg(&bq->i2c_dev, reg, value, 1);
}

/**
 * @brief 更新寄存器位
 */
static int xy_bq25620_update_bits(xy_bq25620_t *bq, uint8_t reg, uint8_t mask, uint8_t value)
{
    uint8_t reg_value;
    int ret;
    
    ret = xy_bq25620_read_reg(bq, reg, &reg_value);
    if (ret != XY_BQ_OK) {
        return ret;
    }
    
    reg_value = (reg_value & ~mask) | (value & mask);
    
    return xy_bq25620_write_reg(bq, reg, reg_value);
}

int xy_bq25620_init(xy_bq25620_t *bq, void *i2c_handle, const xy_bq25620_config_t *config)
{
    int ret;
    uint8_t part_id, dev_id;
    
    if (!bq || !i2c_handle || !config) {
        return XY_BQ_INVALID_PARAM;
    }
    
    memset(bq, 0, sizeof(*bq));
    memcpy(&bq->config, config, sizeof(xy_bq25620_config_t));
    
    /* 初始化 I2C */
    xy_i2c_device_init(&bq->i2c_dev, i2c_handle, BQ25620_ADDR, 400);
    
    /* 读取 ID 验证设备 */
    ret = xy_bq25620_read_reg(bq, BQ25620_REG_PART_ID, &part_id);
    if (ret != XY_BQ_OK || part_id != BQ25620_PART_ID_VALUE) {
        xy_log_e("BQ25620 PART ID mismatch (0x%02X)\n", part_id);
        return XY_BQ_NOT_FOUND;
    }
    
    ret = xy_bq25620_read_reg(bq, BQ25620_REG_DEV_ID, &dev_id);
    if (ret != XY_BQ_OK || dev_id != BQ25620_DEV_ID_VALUE) {
        xy_log_e("BQ25620 DEV ID mismatch (0x%02X)\n", dev_id);
        return XY_BQ_NOT_FOUND;
    }
    
    xy_log_i("BQ25620 found (PART=0x%02X, DEV=0x%02X)\n", part_id, dev_id);
    
    /* 配置充电参数 */
    
    /* REG02: 充电控制 0 */
    /* ICHG (位 7-3): 充电电流设置 */
    uint8_t ichg_bits = ((config->ichg_ma - 100) / 100) & 0x1F;
    ret = xy_bq25620_update_bits(bq, BQ25620_REG_CHG_CTRL0, 0xF8, ichg_bits << 3);
    if (ret != XY_BQ_OK) {
        return ret;
    }
    
    /* REG03: 充电控制 1 */
    /* IPRECHG (位 7-4): 预充电电流 */
    uint8_t ipre_bits = ((config->iprecharge_ma - 50) / 50) & 0x0F;
    /* ITERMC (位 3-0): 终止电流 */
    uint8_t iterm_bits = ((config->iterm_ma - 50) / 50) & 0x0F;
    ret = xy_bq25620_write_reg(bq, BQ25620_REG_CHG_CTRL1, (ipre_bits << 4) | iterm_bits);
    if (ret != XY_BQ_OK) {
        return ret;
    }
    
    /* REG04: 充电控制 2 */
    /* VREG (位 7-2): 电池调节电压 */
    uint8_t vreg_bits = ((config->vbat_reg_mv - 3600) / 10) & 0x3F;
    ret = xy_bq25620_update_bits(bq, BQ25620_REG_CHG_CTRL2, 0xFC, vreg_bits << 2);
    if (ret != XY_BQ_OK) {
        return ret;
    }
    
    /* REG05: 充电控制 3 */
    /* VINDPM (位 7-4): VINDPM 电压 */
    uint8_t vindpm_bits = ((config->vindpm_mv - 3900) / 100) & 0x0F;
    /* IVLIM (位 3-0): 输入电流限制 */
    uint8_t ivlim_bits = ((config->ivlim_ma - 500) / 100) & 0x0F;
    ret = xy_bq25620_write_reg(bq, BQ25620_REG_CHG_CTRL3, (vindpm_bits << 4) | ivlim_bits);
    if (ret != XY_BQ_OK) {
        return ret;
    }
    
    /* REG06: 充电控制 4 */
    /* 使能自动再充电 */
    if (config->enable_auto_recharge) {
        ret = xy_bq25620_update_bits(bq, BQ25620_REG_CHG_CTRL4, 0x80, 0x80);
    } else {
        ret = xy_bq25620_update_bits(bq, BQ25620_REG_CHG_CTRL4, 0x80, 0x00);
    }
    if (ret != XY_BQ_OK) {
        return ret;
    }
    
    /* REG07: 充电控制 5 */
    /* 再充电阈值 */
    uint8_t recharge_bits = (config->recharge_mv / 100) & 0x03;
    ret = xy_bq25620_update_bits(bq, BQ25620_REG_CHG_CTRL5, 0x03, recharge_bits);
    if (ret != XY_BQ_OK) {
        return ret;
    }
    
    /* 使能充电 */
    xy_bq25620_enable_charge(bq, true);
    
    bq->initialized = true;
    xy_log_i("BQ25620 initialized (Vbat=%dmV, Ichg=%dmA)\n",
             config->vbat_reg_mv, config->ichg_ma);
    
    return XY_BQ_OK;
}

int xy_bq25620_deinit(xy_bq25620_t *bq)
{
    if (!bq) {
        return XY_BQ_INVALID_PARAM;
    }
    
    /* 禁用充电 */
    xy_bq25620_enable_charge(bq, false);
    
    bq->initialized = false;
    return XY_BQ_OK;
}

int xy_bq25620_read(xy_bq25620_t *bq)
{
    int ret;
    uint8_t stat_reg, fault_reg;
    
    if (!bq || !bq->initialized) {
        return XY_BQ_INVALID_PARAM;
    }
    
    /* 读取充电状态寄存器 */
    ret = xy_bq25620_read_reg(bq, BQ25620_REG_CHG_STAT, &stat_reg);
    if (ret != XY_BQ_OK) {
        return ret;
    }
    
    /* 解析充电状态 */
    uint8_t chg_stat = (stat_reg & BQ25620_STAT_CHG_MASK) >> 4;
    switch (chg_stat) {
        case 0:
            bq->data.chg_state = XY_BQ_CHG_STATE_IDLE;
            break;
        case 1:
            bq->data.chg_state = XY_BQ_CHG_STATE_PRECHARGE;
            break;
        case 2:
            bq->data.chg_state = XY_BQ_CHG_STATE_FAST_CHARGE;
            break;
        case 3:
            bq->data.chg_state = XY_BQ_CHG_STATE_CHARGE_DONE;
            break;
    }
    
    /* VBUS 电源状态 */
    bq->data.vbus_present = (stat_reg & BQ25620_STAT_PG_MASK) ? true : false;
    
    /* DPM 状态 */
    bq->data.dpm_active = (stat_reg & BQ25620_STAT_DPM_MASK) ? true : false;
    
    /* 充电使能状态 */
    bq->data.chg_enabled = (stat_reg & BQ25620_CHG_EN_MASK) ? true : false;
    
    /* 读取故障寄存器 */
    ret = xy_bq25620_read_reg(bq, BQ25620_REG_FAULT, &fault_reg);
    if (ret == XY_BQ_OK) {
        uint8_t fault = (fault_reg & BQ25620_FAULT_MASK) >> 5;
        switch (fault) {
            case 0:
                bq->data.fault = XY_BQ_FAULT_NONE;
                break;
            case 1:
                bq->data.fault = XY_BQ_FAULT_INPUT_OVP;
                break;
            case 2:
                bq->data.fault = XY_BQ_FAULT_THERMAL;
                break;
            case 3:
                bq->data.fault = XY_BQ_FAULT_CHG_TIMEOUT;
                break;
            case 4:
                bq->data.fault = XY_BQ_FAULT_BAT_OVP;
                break;
            default:
                bq->data.fault = XY_BQ_FAULT_NONE;
                break;
        }
    }
    
    bq->data.timestamp = xy_os_tick_get();
    
    xy_log_d("BQ25620: State=%d, VBUS=%d, Fault=%d\n",
             bq->data.chg_state, bq->data.vbus_present, bq->data.fault);
    
    return XY_BQ_OK;
}

int xy_bq25620_get_chg_state(xy_bq25620_t *bq, xy_bq_chg_state_t *state)
{
    if (!bq || !state) {
        return XY_BQ_INVALID_PARAM;
    }
    
    int ret = xy_bq25620_read(bq);
    if (ret == XY_BQ_OK) {
        *state = bq->data.chg_state;
    }
    return ret;
}

int xy_bq25620_get_fault(xy_bq25620_t *bq, xy_bq_fault_t *fault)
{
    if (!bq || !fault) {
        return XY_BQ_INVALID_PARAM;
    }
    
    int ret = xy_bq25620_read(bq);
    if (ret == XY_BQ_OK) {
        *fault = bq->data.fault;
    }
    return ret;
}

bool xy_bq25620_is_vbus_present(xy_bq25620_t *bq)
{
    if (!bq) {
        return false;
    }
    
    xy_bq25620_read(bq);
    return bq->data.vbus_present;
}

int xy_bq25620_enable_charge(xy_bq25620_t *bq, bool enable)
{
    if (!bq) {
        return XY_BQ_INVALID_PARAM;
    }
    
    uint8_t value = enable ? 0x80 : 0x00;
    return xy_bq25620_update_bits(bq, BQ25620_REG_CHG_STAT, 0x80, value);
}

int xy_bq25620_set_charge_current(xy_bq25620_t *bq, uint16_t current_ma)
{
    if (!bq || current_ma < 100 || current_ma > 3200) {
        return XY_BQ_INVALID_PARAM;
    }
    
    uint8_t ichg_bits = ((current_ma - 100) / 100) & 0x1F;
    return xy_bq25620_update_bits(bq, BQ25620_REG_CHG_CTRL0, 0xF8, ichg_bits << 3);
}

int xy_bq25620_set_battery_voltage(xy_bq25620_t *bq, uint16_t voltage_mv)
{
    if (!bq || voltage_mv < 3600 || voltage_mv > 4200) {
        return XY_BQ_INVALID_PARAM;
    }
    
    uint8_t vreg_bits = ((voltage_mv - 3600) / 10) & 0x3F;
    return xy_bq25620_update_bits(bq, BQ25620_REG_CHG_CTRL2, 0xFC, vreg_bits << 2);
}

int xy_bq25620_start_charging(xy_bq25620_t *bq)
{
    return xy_bq25620_enable_charge(bq, true);
}

int xy_bq25620_stop_charging(xy_bq25620_t *bq)
{
    return xy_bq25620_enable_charge(bq, false);
}

int xy_bq25620_reset_fault(xy_bq25620_t *bq)
{
    if (!bq) {
        return XY_BQ_INVALID_PARAM;
    }
    
    /* 通过复位充电控制寄存器来清除故障 */
    uint8_t reg_value;
    int ret = xy_bq25620_read_reg(bq, BQ25620_REG_FAULT, &reg_value);
    if (ret != XY_BQ_OK) {
        return ret;
    }
    
    /* 故障位是只读的，读取后自动清除 */
    return XY_BQ_OK;
}
