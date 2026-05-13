/**
 * @file xy_sensor_bq25620.c
 * @brief BQ25620 Battery Charger Driver - Migrated
 * @version 2.0.0
 * @date 2026-03-05
 */

#include "xy_sensor.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* BQ25620 寄存器定义 */
#define BQ25620_ADDR            0x6B
#define BQ25620_REG_CHG_STAT    0x00
#define BQ25620_REG_PART_ID     0x0B
#define BQ25620_REG_DEV_ID      0x0C
#define BQ25620_PART_ID_VALUE   0x16

/* 私有数据 */
typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    xy_sensor_value_t voltage_data;
    xy_sensor_value_t current_data;
    xy_sensor_value_t soc_data;
    uint8_t chg_state;
} bq25620_private_data_t;

/* 初始化 */
static int bq25620_init(xy_sensor_device_t *dev)
{
    bq25620_private_data_t *priv = (bq25620_private_data_t *)dev->data;
    
    /* 检查设备 ID */
    uint8_t part_id;
    if (xy_sensor_i2c_read_reg(&priv->bus, BQ25620_REG_PART_ID, &part_id) != 0) {
        return -1;
    }
    
    if (part_id != BQ25620_PART_ID_VALUE) {
        xy_log_e("BQ25620: Wrong ID (0x%02X)\n", part_id);
        return -1;
    }
    
    priv->initialized = true;
    xy_log_i("BQ25620 initialized\n");
    return 0;
}

/* 采样获取 */
static int bq25620_sample_fetch(xy_sensor_device_t *dev, xy_sensor_channel_t channel)
{
    bq25620_private_data_t *priv = (bq25620_private_data_t *)dev->data;
    
    /* 读取充电状态 */
    uint8_t stat;
    if (xy_sensor_i2c_read_reg(&priv->bus, BQ25620_REG_CHG_STAT, &stat) != 0) {
        return -1;
    }
    
    priv->chg_state = (stat >> 4) & 0x03;
    
    /* 简化实现：实际需读取具体电压/电流寄存器 */
    float voltage = 4.2f;  /* 示例值 */
    float current = 0.5f;  /* 示例值 */
    float soc = 75.0f;     /* 示例值 */
    
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->voltage_data, voltage * 1000);  /* mV */
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->current_data, current * 1000);  /* mA */
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->soc_data, soc);                 /* % */
    
    return 0;
}

/* 通道数据获取 */
static int bq25620_channel_get(xy_sensor_device_t *dev, 
                               xy_sensor_channel_t channel, 
                               xy_sensor_value_t *val)
{
    bq25620_private_data_t *priv = (bq25620_private_data_t *)dev->data;
    if (!val) return -1;
    
    switch (channel) {
        case XY_SENSOR_CHAN_VOLTAGE: *val = priv->voltage_data; break;
        case XY_SENSOR_CHAN_CURRENT: *val = priv->current_data; break;
        case XY_SENSOR_CHAN_SOC: *val = priv->soc_data; break;
        default: return -1;
    }
    return 0;
}

/* 驱动 API */
static const xy_sensor_driver_api_t bq25620_driver_api = {
    .init = bq25620_init,
    .sample_fetch = bq25620_sample_fetch,
    .channel_get = bq25620_channel_get,
};

static bq25620_private_data_t bq25620_priv;
static xy_sensor_device_t bq25620_device = {
    .name = "BQ25620",
    .type = XY_SENSOR_TYPE_POWER,
    .api = &bq25620_driver_api,
    .data = &bq25620_priv,
};

int xy_sensor_bq25620_register(void *i2c_handle, uint8_t addr)
{
    xy_sensor_bus_config_i2c(&bq25620_priv.bus, i2c_handle, addr ? addr : BQ25620_ADDR);
    return xy_sensor_device_register(&bq25620_device);
}
