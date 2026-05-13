/**
 * @file xy_fg_bq40z50.c
 * @brief BQ40Z50 Fuel Gauge Driver
 * @version 1.0.0
 * @date 2026-03-05
 * 
 * Reference: Texas Instruments BQ40Z50 Technical Reference
 * https://www.ti.com/product/BQ40Z50
 * 
 * BQ40Z50 特性:
 * - 支持 2-4 节串联电池
 * - Impedance Track™ 技术
 * - 集成保护功能 (过压/欠压/过流/短路)
 * - 电池平衡功能
 */

#include "xy_fuel_gauge.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* BQ40Z50 寄存器定义 */
#define BQ40Z50_ADDR            0x0B

/* 标准命令寄存器 */
#define BQ40Z50_REG_CTRL        0x00
#define BQ40Z50_REG_TEMP        0x06
#define BQ40Z50_REG_VOLT        0x08
#define BQ40Z50_REG_CURR        0x0A
#define BQ40Z50_REG_SOC         0x2C
#define BQ40Z50_REG_REM_CAP     0x2E
#define BQ40Z50_REG_FULL_CAP    0x30
#define BQ40Z50_REG_CYCLE_CNT   0x2A

/* 扩展命令寄存器 */
#define BQ40Z50_REG_CELL1_VOLT  0x3C
#define BQ40Z50_REG_CELL2_VOLT  0x3E
#define BQ40Z50_REG_CELL3_VOLT  0x40
#define BQ40Z50_REG_CELL4_VOLT  0x42
#define BQ40Z50_REG_BAT_STATUS  0x64
#define BQ40Z50_REG_PROT_STATUS 0x66
#define BQ40Z50_REG_BAL_STATUS  0x68

/* 电池状态标志 */
#define BQ40Z50_BAT_STAT_CHG    (1 << 0)    /* 充电中 */
#define BQ40Z50_BAT_STAT_DISCHG (1 << 1)    /* 放电中 */
#define BQ40Z50_BAT_STAT_FC     (1 << 3)    /* 充满 */
#define BQ40Z50_BAT_STAT_OTD    (1 << 4)    /* 放电过温 */
#define BQ40Z50_BAT_STAT_OTC    (1 << 5)    /* 充电过温 */

/* 保护状态标志 */
#define BQ40Z50_PROT_OVP        (1 << 0)    /* 过压保护 */
#define BQ40Z50_PROT_UVP        (1 << 1)    /* 欠压保护 */
#define BQ40Z50_PROT_OCC        (1 << 2)    /* 充电过流 */
#define BQ40Z50_PROT_ODC        (1 << 3)    /* 放电过流 */
#define BQ40Z50_PROT_SCD        (1 << 4)    /* 短路保护 */
#define BQ40Z50_PROT_OTC        (1 << 5)    /* 充电过温 */
#define BQ40Z50_PROT_OTD        (1 << 6)    /* 放电过温 */

/* 私有数据 */
typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    xy_fuel_gauge_data_t data;
    uint16_t bat_status;
    uint32_t prot_status;
    uint8_t cell_count;
    uint16_t cell_voltages[4];
} bq40z50_private_data_t;

/**
 * @brief 读取寄存器 (16 位)
 */
static int bq40z50_read_reg16(bq40z50_private_data_t *priv, 
                              uint8_t reg, uint16_t *value)
{
    uint8_t buf[2];
    int ret;
    
    ret = xy_sensor_i2c_read(&priv->bus, reg, buf, 2);
    if (ret != 0) {
        return -1;
    }
    
    /* BQ40Z50 使用小端格式 */
    *value = ((uint16_t)buf[1] << 8) | buf[0];
    
    return 0;
}

/**
 * @brief 读取寄存器 (32 位)
 */
static int bq40z50_read_reg32(bq40z50_private_data_t *priv, 
                              uint8_t reg, uint32_t *value)
{
    uint8_t buf[4];
    int ret;
    
    ret = xy_sensor_i2c_read(&priv->bus, reg, buf, 4);
    if (ret != 0) {
        return -1;
    }
    
    /* 小端格式 */
    *value = ((uint32_t)buf[3] << 24) | ((uint32_t)buf[2] << 16) |
             ((uint32_t)buf[1] << 8) | buf[0];
    
    return 0;
}

/**
 * @brief BQ40Z50 初始化
 */
static int bq40z50_init(xy_fuel_gauge_t *fg)
{
    bq40z50_private_data_t *priv = (bq40z50_private_data_t *)fg->data;
    uint16_t device_type;
    
    /* 读取设备类型验证连接 */
    if (bq40z50_read_reg16(priv, BQ40Z50_REG_CTRL, &device_type) != 0) {
        xy_log_e("BQ40Z50: Failed to read device type\n");
        return -1;
    }
    
    xy_log_i("BQ40Z50 device type: 0x%04X\n", device_type);
    
    /* 读取电池状态 */
    if (bq40z50_read_reg16(priv, BQ40Z50_REG_BAT_STATUS, &priv->bat_status) != 0) {
        xy_log_w("BQ40Z50: Failed to read battery status\n");
    }
    
    /* 读取保护状态 */
    if (bq40z50_read_reg32(priv, BQ40Z50_REG_PROT_STATUS, &priv->prot_status) != 0) {
        xy_log_w("BQ40Z50: Failed to read protection status\n");
    }
    
    /* 检测电池数量 */
    priv->cell_count = 4;  /* 默认 4 节 */
    
    priv->initialized = true;
    xy_log_i("BQ40Z50 initialized (%d cells)\n", priv->cell_count);
    return 0;
}

/**
 * @brief BQ40Z50 数据获取
 */
static int bq40z50_fetch(xy_fuel_gauge_t *fg)
{
    bq40z50_private_data_t *priv = (bq40z50_private_data_t *)fg->data;
    uint16_t value;
    
    if (!priv->initialized) {
        return -1;
    }
    
    /* 读取电池组电压 (mV) */
    if (bq40z50_read_reg16(priv, BQ40Z50_REG_VOLT, &value) == 0) {
        priv->data.voltage_mv = value;
    }
    
    /* 读取电流 (mA) */
    if (bq40z50_read_reg16(priv, BQ40Z50_REG_CURR, &value) == 0) {
        /* BQ40Z50 电流为有符号数，正=充电，负=放电 */
        priv->data.current_ma = (int16_t)value;
    }
    
    /* 读取 SOC (0.01%) */
    if (bq40z50_read_reg16(priv, BQ40Z50_REG_SOC, &value) == 0) {
        priv->data.soc = (uint8_t)(value / 100);
    }
    
    /* 读取剩余容量 (mAh) */
    if (bq40z50_read_reg16(priv, BQ40Z50_REG_REM_CAP, &value) == 0) {
        priv->data.remain_capacity_mah = value;
    }
    
    /* 读取满充容量 (mAh) */
    if (bq40z50_read_reg16(priv, BQ40Z50_REG_FULL_CAP, &value) == 0) {
        priv->data.full_capacity_mah = value;
    }
    
    /* 读取循环次数 */
    if (bq40z50_read_reg16(priv, BQ40Z50_REG_CYCLE_CNT, &value) == 0) {
        priv->data.cycle_count = value;
    }
    
    /* 读取温度 (0.1K) */
    if (bq40z50_read_reg16(priv, BQ40Z50_REG_TEMP, &value) == 0) {
        /* 转换为 0.1°C */
        priv->data.temperature_c = (int16_t)(value - 2731);
    }
    
    /* 读取单体电压 */
    if (bq40z50_read_reg16(priv, BQ40Z50_REG_CELL1_VOLT, &value) == 0) {
        priv->cell_voltages[0] = value;
    }
    if (bq40z50_read_reg16(priv, BQ40Z50_REG_CELL2_VOLT, &value) == 0) {
        priv->cell_voltages[1] = value;
    }
    if (bq40z50_read_reg16(priv, BQ40Z50_REG_CELL3_VOLT, &value) == 0) {
        priv->cell_voltages[2] = value;
    }
    if (bq40z50_read_reg16(priv, BQ40Z50_REG_CELL4_VOLT, &value) == 0) {
        priv->cell_voltages[3] = value;
    }
    
    /* 更新状态 */
    bq40z50_read_reg16(priv, BQ40Z50_REG_BAT_STATUS, &priv->bat_status);
    bq40z50_read_reg32(priv, BQ40Z50_REG_PROT_STATUS, &priv->prot_status);
    
    /* SOH 估算 (简化实现) */
    if (priv->data.full_capacity_mah > 0) {
        /* 假设设计容量为满充容量的 100% */
        priv->data.soh = 100;
    }
    
    xy_log_d("BQ40Z50: V=%dmV, I=%dmA, SOC=%d%%, Cells=[%d,%d,%d,%d]mV\n",
             priv->data.voltage_mv, priv->data.current_ma,
             priv->data.soc,
             priv->cell_voltages[0], priv->cell_voltages[1],
             priv->cell_voltages[2], priv->cell_voltages[3]);
    
    return 0;
}

/**
 * @brief BQ40Z50 通道数据获取
 */
static int bq40z50_channel_get(xy_fuel_gauge_t *fg,
                               xy_fuel_gauge_data_type_t channel,
                               int32_t *val)
{
    bq40z50_private_data_t *priv = (bq40z50_private_data_t *)fg->data;
    
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
 * @brief 获取电池组电压
 */
int xy_fuel_gauge_bq40z50_get_battery_voltage(xy_fuel_gauge_t *fg, 
                                              uint16_t *voltage_mv)
{
    bq40z50_private_data_t *priv = (bq40z50_private_data_t *)fg->data;
    
    if (!voltage_mv) {
        return -1;
    }
    
    *voltage_mv = priv->data.voltage_mv;
    return 0;
}

/**
 * @brief 获取单体电压
 */
int xy_fuel_gauge_bq40z50_get_cell_voltage(xy_fuel_gauge_t *fg, 
                                           uint8_t cell_index,
                                           uint16_t *voltage_mv)
{
    bq40z50_private_data_t *priv = (bq40z50_private_data_t *)fg->data;
    
    if (!voltage_mv || cell_index == 0 || cell_index > priv->cell_count) {
        return -1;
    }
    
    *voltage_mv = priv->cell_voltages[cell_index - 1];
    return 0;
}

/**
 * @brief 获取平衡状态
 */
uint8_t xy_fuel_gauge_bq40z50_get_balance_status(xy_fuel_gauge_t *fg)
{
    bq40z50_private_data_t *priv = (bq40z50_private_data_t *)fg->data;
    uint16_t balance_status;
    
    if (bq40z50_read_reg16(priv, BQ40Z50_REG_BAL_STATUS, &balance_status) != 0) {
        return 0;
    }
    
    return (uint8_t)balance_status;
}

/**
 * @brief 获取保护状态
 */
uint32_t xy_fuel_gauge_bq40z50_get_protection_status(xy_fuel_gauge_t *fg)
{
    bq40z50_private_data_t *priv = (bq40z50_private_data_t *)fg->data;
    return priv->prot_status;
}

/**
 * @brief 检查充电状态
 */
bool xy_fuel_gauge_bq40z50_is_charging(xy_fuel_gauge_t *fg)
{
    bq40z50_private_data_t *priv = (bq40z50_private_data_t *)fg->data;
    return (priv->bat_status & BQ40Z50_BAT_STAT_CHG) != 0;
}

/**
 * @brief 检查充满状态
 */
bool xy_fuel_gauge_bq40z50_is_full(xy_fuel_gauge_t *fg)
{
    bq40z50_private_data_t *priv = (bq40z50_private_data_t *)fg->data;
    return (priv->bat_status & BQ40Z50_BAT_STAT_FC) != 0;
}

/**
 * @brief 检查保护状态
 */
bool xy_fuel_gauge_bq40z50_is_protected(xy_fuel_gauge_t *fg)
{
    bq40z50_private_data_t *priv = (bq40z50_private_data_t *)fg->data;
    return priv->prot_status != 0;
}

/**
 * @brief BQ40Z50 驱动 API
 */
static const xy_fuel_gauge_api_t bq40z50_driver_api = {
    .init = bq40z50_init,
    .fetch = bq40z50_fetch,
    .channel_get = bq40z50_channel_get,
    .alert_set = NULL,
    .alert_get = NULL,
};

/* 设备实例 */
static bq40z50_private_data_t bq40z50_priv;
static xy_fuel_gauge_t bq40z50_device = {
    .name = "BQ40Z50",
    .api = &bq40z50_driver_api,
    .data = &bq40z50_priv,
};

/**
 * @brief 注册 BQ40Z50 电量计
 */
int xy_fuel_gauge_bq40z50_register(void *i2c_handle, uint8_t addr)
{
    xy_sensor_bus_config_i2c(&bq40z50_priv.bus, i2c_handle, 
                             addr ? addr : BQ40Z50_ADDR);
    return xy_fuel_gauge_device_register(&bq40z50_device);
}
