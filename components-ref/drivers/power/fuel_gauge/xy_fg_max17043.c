/**
 * @file xy_fg_max17043.c
 * @brief MAX17043 Fuel Gauge Driver
 * @version 1.0.0
 * @date 2026-03-05
 * 
 * Reference: Zephyr MAX17043 driver
 */

#include "xy_fuel_gauge.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* MAX17043 寄存器定义 */
#define MAX17043_ADDR           0x36
#define MAX17043_REG_VCELL      0x02
#define MAX17043_REG_SOC        0x04
#define MAX17043_REG_MODE       0x06
#define MAX17043_REG_VER        0x08
#define MAX17043_REG_HIBRT      0x0A
#define MAX17043_REG_CONFIG     0x0C
#define MAX17043_REG_VALRT      0x14
#define MAX17043_REG_CRATE      0x16
#define MAX17043_REG_VRESET     0x18
#define MAX17043_REG_STATUS     0x1A
#define MAX17043_REG_UNLOCK     0x3E
#define MAX17043_REG_COMMAND    0xFF

/* 私有数据 */
typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    xy_fuel_gauge_data_t data;
} max17043_private_data_t;

/**
 * @brief MAX17043 初始化
 */
static int max17043_init(xy_fuel_gauge_t *fg)
{
    max17043_private_data_t *priv = (max17043_private_data_t *)fg->data;
    
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

/**
 * @brief MAX17043 数据获取
 */
static int max17043_fetch(xy_fuel_gauge_t *fg)
{
    max17043_private_data_t *priv = (max17043_private_data_t *)fg->data;
    
    /* 读取电压 (mV) */
    uint16_t vcell;
    if (xy_sensor_i2c_read_reg16(&priv->bus, MAX17043_REG_VCELL, &vcell) != 0) {
        return -1;
    }
    float voltage = (vcell >> 4) * 0.0625f * 1000;  /* 1.25mV/LSB */
    
    /* 读取 SOC (%) */
    uint16_t soc_reg;
    if (xy_sensor_i2c_read_reg16(&priv->bus, MAX17043_REG_SOC, &soc_reg) != 0) {
        return -1;
    }
    float soc = (soc_reg >> 8) + (soc_reg & 0xFF) / 256.0f;
    
    /* 读取充放电率 */
    uint16_t crate_reg;
    if (xy_sensor_i2c_read_reg16(&priv->bus, MAX17043_REG_CRATE, &crate_reg) != 0) {
        return -1;
    }
    int16_t crate = (int16_t)crate_reg;
    float current = crate * 0.208f * 1000;  /* 0.208mA/LSB */
    
    /* 存储数据 */
    priv->data.voltage_mv = (uint16_t)voltage;
    priv->data.current_ma = (int16_t)current;
    priv->data.soc = (uint8_t)soc;
    priv->data.temperature_c = 0;  /* MAX17043 不支持温度 */
    
    return 0;
}

/**
 * @brief MAX17043 通道数据获取
 */
static int max17043_channel_get(xy_fuel_gauge_t *fg,
                                xy_fuel_gauge_data_type_t channel,
                                int32_t *val)
{
    max17043_private_data_t *priv = (max17043_private_data_t *)fg->data;
    
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
        default:
            return -1;
    }
    return 0;
}

/**
 * @brief MAX17043 驱动 API
 */
static const xy_fuel_gauge_api_t max17043_driver_api = {
    .init = max17043_init,
    .fetch = max17043_fetch,
    .channel_get = max17043_channel_get,
    .alert_set = NULL,
    .alert_get = NULL,
};

/* 设备实例 */
static max17043_private_data_t max17043_priv;
static xy_fuel_gauge_t max17043_device = {
    .name = "MAX17043",
    .api = &max17043_driver_api,
    .data = &max17043_priv,
};

/**
 * @brief 注册 MAX17043 电量计
 */
int xy_fuel_gauge_max17043_register(void *i2c_handle, uint8_t addr)
{
    xy_sensor_bus_config_i2c(&max17043_priv.bus, i2c_handle, addr ? addr : MAX17043_ADDR);
    return xy_fuel_gauge_device_register(&max17043_device);
}
