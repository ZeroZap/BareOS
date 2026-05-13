/**
 * @file xy_sensor_max17043.c
 * @brief MAX17043 Fuel Gauge Driver - Migrated
 * @version 2.0.0
 * @date 2026-03-05
 */

#include "xy_sensor.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* MAX17043 寄存器定义 */
#define MAX17043_ADDR           0x36
#define MAX17043_REG_VCELL      0x02
#define MAX17043_REG_SOC        0x04
#define MAX17043_REG_CONFIG     0x0C
#define MAX17043_REG_VERSION    0x08

/* 私有数据 */
typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    xy_sensor_value_t voltage_data;
    xy_sensor_value_t soc_data;
} max17043_private_data_t;

/* 初始化 */
static int max17043_init(xy_sensor_device_t *dev)
{
    max17043_private_data_t *priv = (max17043_private_data_t *)dev->data;
    
    /* 读取版本 */
    uint16_t version;
    if (xy_sensor_i2c_read_reg16(&priv->bus, MAX17043_REG_VERSION, &version) != 0) {
        return -1;
    }
    
    xy_log_i("MAX17043 version: 0x%04X\n", version);
    
    /* 配置：默认配置 */
    xy_sensor_i2c_write_reg16(&priv->bus, MAX17043_REG_CONFIG, 0x0000);
    
    priv->initialized = true;
    xy_log_i("MAX17043 initialized\n");
    return 0;
}

/* 采样获取 */
static int max17043_sample_fetch(xy_sensor_device_t *dev, xy_sensor_channel_t channel)
{
    max17043_private_data_t *priv = (max17043_private_data_t *)dev->data;
    
    /* 读取电压 (mV) */
    uint16_t vcell;
    if (xy_sensor_i2c_read_reg16(&priv->bus, MAX17043_REG_VCELL, &vcell) != 0) {
        return -1;
    }
    float voltage = (vcell >> 4) * 0.0625f;  /* 1.25mV/LSB */
    
    /* 读取 SOC (%) */
    uint16_t soc_reg;
    if (xy_sensor_i2c_read_reg16(&priv->bus, MAX17043_REG_SOC, &soc_reg) != 0) {
        return -1;
    }
    float soc = (soc_reg >> 8) + (soc_reg & 0xFF) / 256.0f;
    
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->voltage_data, voltage * 1000);  /* mV */
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->soc_data, soc);                  /* % */
    
    return 0;
}

/* 通道数据获取 */
static int max17043_channel_get(xy_sensor_device_t *dev, 
                                xy_sensor_channel_t channel, 
                                xy_sensor_value_t *val)
{
    max17043_private_data_t *priv = (max17043_private_data_t *)dev->data;
    if (!val) return -1;
    
    switch (channel) {
        case XY_SENSOR_CHAN_VOLTAGE: *val = priv->voltage_data; break;
        case XY_SENSOR_CHAN_SOC: *val = priv->soc_data; break;
        default: return -1;
    }
    return 0;
}

/* 驱动 API */
static const xy_sensor_driver_api_t max17043_driver_api = {
    .init = max17043_init,
    .sample_fetch = max17043_sample_fetch,
    .channel_get = max17043_channel_get,
};

static max17043_private_data_t max17043_priv;
static xy_sensor_device_t max17043_device = {
    .name = "MAX17043",
    .type = XY_SENSOR_TYPE_POWER,
    .api = &max17043_driver_api,
    .data = &max17043_priv,
};

int xy_sensor_max17043_register(void *i2c_handle, uint8_t addr)
{
    xy_sensor_bus_config_i2c(&max17043_priv.bus, i2c_handle, addr ? addr : MAX17043_ADDR);
    return xy_sensor_device_register(&max17043_device);
}
