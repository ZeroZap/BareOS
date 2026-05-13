/**
 * @file xy_fg_bq27z746.c
 * @brief BQ27Z746 Fuel Gauge Driver
 * @version 1.0.0
 * @date 2026-03-05
 * 
 * Reference: Texas Instruments BQ27Z746 Technical Reference
 * https://www.ti.com/product/BQ27Z746
 */

#include "xy_fuel_gauge.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* BQ27Z746 寄存器定义 */
#define BQ27Z746_ADDR           0x55

/* 标准命令寄存器 */
#define BQ27Z746_REG_CTRL       0x00
#define BQ27Z746_REG_TEMP       0x06
#define BQ27Z746_REG_VOLT       0x08
#define BQ27Z746_REG_FLAGS      0x0A
#define BQ27Z746_REG_NOM_CAP    0x12
#define BQ27Z746_REG_REM_CAP    0x14
#define BQ27Z746_REG_SOC        0x2C
#define BQ27Z746_REG_SOH        0x7A
#define BQ27Z746_REG_CURR       0x58
#define BQ27Z746_REG_AVG_CURR   0x5A
#define BQ27Z746_REG_CYCLE_CNT  0x2A
#define BQ27Z746_REG_FULL_CAP   0x12

/* 控制子命令 */
#define BQ27Z746_CTRL_STATUS    0x00
#define BQ27Z746_CTRL_DEVICE_TYPE 0x0002
#define BQ27Z746_CTRL_FW_VER    0x0004

/* 标志位 */
#define BQ27Z746_FLAG_CHG       (1 << 0)    /* 充电中 */
#define BQ27Z746_FLAG_DISCHG    (1 << 1)    /* 放电中 */
#define BQ27Z746_FLAG_FC        (1 << 3)    /* 充满 */
#define BQ27Z746_FLAG_OT        (1 << 2)    /* 过温 */
#define BQ27Z746_FLAG_OCHG      (1 << 9)    /* 过充 */
#define BQ27Z746_FLAG_OCUR      (1 << 10)   /* 过流 */

/* 私有数据 */
typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    xy_fuel_gauge_data_t data;
    uint16_t flags;
    uint8_t device_type[2];
} bq27z746_private_data_t;

/**
 * @brief 读取寄存器 (16 位)
 */
static int bq27z746_read_reg16(bq27z746_private_data_t *priv, 
                                uint8_t reg, uint16_t *value)
{
    uint8_t buf[2];
    int ret;
    
    ret = xy_sensor_i2c_read(&priv->bus, reg, buf, 2);
    if (ret != 0) {
        return -1;
    }
    
    /* BQ27Z746 使用小端格式 */
    *value = ((uint16_t)buf[1] << 8) | buf[0];
    
    return 0;
}

/**
 * @brief BQ27Z746 初始化
 */
static int bq27z746_init(xy_fuel_gauge_t *fg)
{
    bq27z746_private_data_t *priv = (bq27z746_private_data_t *)fg->data;
    uint16_t device_type;
    
    /* 读取设备类型验证连接 */
    if (bq27z746_read_reg16(priv, BQ27Z746_REG_CTRL, &device_type) != 0) {
        xy_log_e("BQ27Z746: Failed to read device type\n");
        return -1;
    }
    
    xy_log_i("BQ27Z746 device type: 0x%04X\n", device_type);
    
    /* 读取标志位 */
    if (bq27z746_read_reg16(priv, BQ27Z746_REG_FLAGS, &priv->flags) != 0) {
        xy_log_w("BQ27Z746: Failed to read flags\n");
    }
    
    priv->initialized = true;
    xy_log_i("BQ27Z746 initialized\n");
    return 0;
}

/**
 * @brief BQ27Z746 数据获取
 */
static int bq27z746_fetch(xy_fuel_gauge_t *fg)
{
    bq27z746_private_data_t *priv = (bq27z746_private_data_t *)fg->data;
    uint16_t value;
    
    if (!priv->initialized) {
        return -1;
    }
    
    /* 读取电压 (mV) */
    if (bq27z746_read_reg16(priv, BQ27Z746_REG_VOLT, &value) == 0) {
        priv->data.voltage_mv = value;
    }
    
    /* 读取电流 (mA) */
    if (bq27z746_read_reg16(priv, BQ27Z746_REG_CURR, &value) == 0) {
        /* BQ27Z746 电流为有符号数，正=充电，负=放电 */
        priv->data.current_ma = (int16_t)value;
    }
    
    /* 读取 SOC (%) */
    if (bq27z746_read_reg16(priv, BQ27Z746_REG_SOC, &value) == 0) {
        priv->data.soc = (uint8_t)value;
    }
    
    /* 读取 SOH (%) */
    if (bq27z746_read_reg16(priv, BQ27Z746_REG_SOH, &value) == 0) {
        priv->data.soh = (uint8_t)value;
    }
    
    /* 读取温度 (0.1K) */
    if (bq27z746_read_reg16(priv, BQ27Z746_REG_TEMP, &value) == 0) {
        /* 转换为 0.1°C */
        priv->data.temperature_c = (int16_t)(value - 2731);
    }
    
    /* 读取满充容量 (mAh) */
    if (bq27z746_read_reg16(priv, BQ27Z746_REG_FULL_CAP, &value) == 0) {
        priv->data.full_capacity_mah = value;
    }
    
    /* 读取剩余容量 (mAh) */
    if (bq27z746_read_reg16(priv, BQ27Z746_REG_REM_CAP, &value) == 0) {
        priv->data.remain_capacity_mah = value;
    }
    
    /* 读取循环次数 */
    if (bq27z746_read_reg16(priv, BQ27Z746_REG_CYCLE_CNT, &value) == 0) {
        priv->data.cycle_count = value;
    }
    
    /* 更新标志位 */
    bq27z746_read_reg16(priv, BQ27Z746_REG_FLAGS, &priv->flags);
    
    xy_log_d("BQ27Z746: V=%dmV, I=%dmA, SOC=%d%%, SOH=%d%%\n",
             priv->data.voltage_mv, priv->data.current_ma,
             priv->data.soc, priv->data.soh);
    
    return 0;
}

/**
 * @brief BQ27Z746 通道数据获取
 */
static int bq27z746_channel_get(xy_fuel_gauge_t *fg,
                                xy_fuel_gauge_data_type_t channel,
                                int32_t *val)
{
    bq27z746_private_data_t *priv = (bq27z746_private_data_t *)fg->data;
    
    if (!val) {
        return -1;
    }
    
    switch (channel) {
        case XY_FG_DATA_VOLTAGE:
            *val = priv->data.voltage_mv;
            break;
        case XY_FG_DATA_CURRENT:
            *val = priv->data.current_ma;
            break;
        case XY_FG_DATA_SOC:
            *val = priv->data.soc;
            break;
        case XY_FG_DATA_SOH:
            *val = priv->data.soh;
            break;
        case XY_FG_DATA_TEMPERATURE:
            *val = priv->data.temperature_c;
            break;
        case XY_FG_DATA_FULL_CAPACITY:
            *val = priv->data.full_capacity_mah;
            break;
        case XY_FG_DATA_REMAIN_CAPACITY:
            *val = priv->data.remain_capacity_mah;
            break;
        case XY_FG_DATA_CYCLE_COUNT:
            *val = priv->data.cycle_count;
            break;
        default:
            return -1;
    }
    return 0;
}

/**
 * @brief 获取充电状态
 */
bool xy_fuel_gauge_bq27z746_is_charging(xy_fuel_gauge_t *fg)
{
    bq27z746_private_data_t *priv = (bq27z746_private_data_t *)fg->data;
    return (priv->flags & BQ27Z746_FLAG_CHG) != 0;
}

/**
 * @brief 获取充满状态
 */
bool xy_fuel_gauge_bq27z746_is_full(xy_fuel_gauge_t *fg)
{
    bq27z746_private_data_t *priv = (bq27z746_private_data_t *)fg->data;
    return (priv->flags & BQ27Z746_FLAG_FC) != 0;
}

/**
 * @brief 获取告警状态
 */
uint16_t xy_fuel_gauge_bq27z746_get_flags(xy_fuel_gauge_t *fg)
{
    bq27z746_private_data_t *priv = (bq27z746_private_data_t *)fg->data;
    return priv->flags;
}

/**
 * @brief BQ27Z746 驱动 API
 */
static const xy_fuel_gauge_api_t bq27z746_driver_api = {
    .init = bq27z746_init,
    .fetch = bq27z746_fetch,
    .channel_get = bq27z746_channel_get,
    .alert_set = NULL,
    .alert_get = NULL,
};

/* 设备实例 */
static bq27z746_private_data_t bq27z746_priv;
static xy_fuel_gauge_t bq27z746_device = {
    .name = "BQ27Z746",
    .api = &bq27z746_driver_api,
    .data = &bq27z746_priv,
};

/**
 * @brief 注册 BQ27Z746 电量计
 */
int xy_fuel_gauge_bq27z746_register(void *i2c_handle, uint8_t addr)
{
    xy_sensor_bus_config_i2c(&bq27z746_priv.bus, i2c_handle, 
                             addr ? addr : BQ27Z746_ADDR);
    return xy_fuel_gauge_device_register(&bq27z746_device);
}
