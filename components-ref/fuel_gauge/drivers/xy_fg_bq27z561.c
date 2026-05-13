/**
 * @file xy_fg_bq27z561.c
 * @brief BQ27z561 Fuel Gauge Driver
 * @version 1.0.0
 * @date 2026-03-05
 * 
 * Reference: Zephyr BQ27z561 driver
 */

#include "xy_fuel_gauge.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* BQ27z561 寄存器定义 */
#define BQ27Z561_ADDR           0x55
#define BQ27Z561_REG_CTRL       0x00
#define BQ27Z561_REG_TEMP       0x06
#define BQ27Z561_REG_VOLT       0x08
#define BQ27Z561_REG_FLAGS      0x0A
#define BQ27Z561_REG_NOM_CAP    0x12
#define BQ27Z561_REG_REM_CAP    0x14
#define BQ27Z561_REG_SOC        0x2C
#define BQ27Z561_REG_CURR       0x58
#define BQ27Z561_REG_SOH        0x7A

/* 私有数据 */
typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    xy_fuel_gauge_data_t data;
} bq27z561_private_data_t;

/**
 * @brief BQ27z561 初始化
 */
static int bq27z561_init(xy_fuel_gauge_t *fg)
{
    bq27z561_private_data_t *priv = (bq27z561_private_data_t *)fg->data;
    
    /* 读取设备 ID */
    uint16_t device_id;
    if (xy_sensor_i2c_read_reg16(&priv->bus, 0x0002, &device_id) != 0) {
        return -1;
    }
    
    xy_log_i("BQ27z561 device ID: 0x%04X\n", device_id);
    
    priv->initialized = true;
    xy_log_i("BQ27z561 initialized\n");
    return 0;
}

/**
 * @brief BQ27z561 数据获取
 */
static int bq27z561_fetch(xy_fuel_gauge_t *fg)
{
    bq27z561_private_data_t *priv = (bq27z561_private_data_t *)fg->data;
    
    /* 读取电压 (mV) */
    uint16_t voltage;
    if (xy_sensor_i2c_read_reg16(&priv->bus, BQ27Z561_REG_VOLT, &voltage) != 0) {
        return -1;
    }
    
    /* 读取电流 (mA) */
    int16_t current;
    if (xy_sensor_i2c_read_reg16(&priv->bus, BQ27Z561_REG_CURR, (uint16_t*)&current) != 0) {
        return -1;
    }
    
    /* 读取 SOC (%) */
    uint8_t soc;
    if (xy_sensor_i2c_read_reg(&priv->bus, BQ27Z561_REG_SOC, &soc) != 0) {
        return -1;
    }
    
    /* 读取 SOH (%) */
    uint8_t soh;
    if (xy_sensor_i2c_read_reg(&priv->bus, BQ27Z561_REG_SOH, &soh) != 0) {
        return -1;
    }
    
    /* 读取温度 (0.1K) */
    uint16_t temp;
    if (xy_sensor_i2c_read_reg16(&priv->bus, BQ27Z561_REG_TEMP, &temp) != 0) {
        return -1;
    }
    int16_t temp_c = (temp - 2731) - 2731;  /* 转换为 0.1°C */
    
    /* 读取满充容量 (mAh) */
    uint16_t full_cap;
    if (xy_sensor_i2c_read_reg16(&priv->bus, BQ27Z561_REG_NOM_CAP, &full_cap) != 0) {
        return -1;
    }
    
    /* 读取剩余容量 (mAh) */
    uint16_t rem_cap;
    if (xy_sensor_i2c_read_reg16(&priv->bus, BQ27Z561_REG_REM_CAP, &rem_cap) != 0) {
        return -1;
    }
    
    /* 存储数据 */
    priv->data.voltage_mv = voltage;
    priv->data.current_ma = current;
    priv->data.soc = soc;
    priv->data.soh = soh;
    priv->data.temperature_c = temp_c;
    priv->data.full_capacity_mah = full_cap;
    priv->data.remain_capacity_mah = rem_cap;
    
    return 0;
}

/**
 * @brief BQ27z561 通道数据获取
 */
static int bq27z561_channel_get(xy_fuel_gauge_t *fg,
                                xy_fuel_gauge_data_type_t channel,
                                int32_t *val)
{
    bq27z561_private_data_t *priv = (bq27z561_private_data_t *)fg->data;
    
    if (!val) return -1;
    
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
        default:
            return -1;
    }
    return 0;
}

/**
 * @brief BQ27z561 驱动 API
 */
static const xy_fuel_gauge_api_t bq27z561_driver_api = {
    .init = bq27z561_init,
    .fetch = bq27z561_fetch,
    .channel_get = bq27z561_channel_get,
    .alert_set = NULL,
    .alert_get = NULL,
};

/* 设备实例 */
static bq27z561_private_data_t bq27z561_priv;
static xy_fuel_gauge_t bq27z561_device = {
    .name = "BQ27z561",
    .api = &bq27z561_driver_api,
    .data = &bq27z561_priv,
};

/**
 * @brief 注册 BQ27z561 电量计
 */
int xy_fuel_gauge_bq27z561_register(void *i2c_handle, uint8_t addr)
{
    xy_sensor_bus_config_i2c(&bq27z561_priv.bus, i2c_handle, addr ? addr : BQ27Z561_ADDR);
    return xy_fuel_gauge_device_register(&bq27z561_device);
}
