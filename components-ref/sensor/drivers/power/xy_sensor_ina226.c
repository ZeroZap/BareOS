/**
 * @file xy_sensor_ina226.c
 * @brief INA226 Power Monitor Driver - Migrated
 * @version 2.0.0
 * @date 2026-03-05
 */

#include "xy_sensor.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* INA226 寄存器定义 */
#define INA226_ADDR             0x40
#define INA226_REG_CONFIG       0x00
#define INA226_REG_SHUNT_VOLT   0x01
#define INA226_REG_BUS_VOLT     0x02
#define INA226_REG_CURRENT      0x04
#define INA226_REG_CALIB        0x05
#define INA226_REG_MFG_ID       0xFE
#define INA226_MFG_ID_VALUE     0x5449  /* TI */

/* 配置值 */
#define INA226_CONFIG_DEFAULT   0x4127  /* 平均 16 次，电压 1.1ms */
#define INA226_SHUNT_RESISTOR   1000    /* 分流电阻 1mΩ (单位：微欧) */

/* 私有数据 */
typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    xy_sensor_value_t voltage_data;
    xy_sensor_value_t current_data;
    xy_sensor_value_t power_data;
    float current_lsb;
} ina226_private_data_t;

/* 初始化 */
static int ina226_init(xy_sensor_device_t *dev)
{
    ina226_private_data_t *priv = (ina226_private_data_t *)dev->data;
    
    /* 检查设备 ID */
    uint16_t id;
    if (xy_sensor_i2c_read_reg16(&priv->bus, INA226_REG_MFG_ID, &id) != 0) {
        return -1;
    }
    
    if (id != INA226_MFG_ID_VALUE) {
        xy_log_e("INA226: Wrong ID (0x%04X)\n", id);
        return -1;
    }
    
    /* 计算校准值 */
    priv->current_lsb = 0.0025f;  /* 2.5mA/LSB */
    uint16_t calib = (uint16_t)(0.00512f / (priv->current_lsb * INA226_SHUNT_RESISTOR / 1000000.0f));
    xy_sensor_i2c_write_reg16(&priv->bus, INA226_REG_CALIB, calib);
    
    /* 配置 */
    xy_sensor_i2c_write_reg16(&priv->bus, INA226_REG_CONFIG, INA226_CONFIG_DEFAULT);
    
    priv->initialized = true;
    xy_log_i("INA226 initialized\n");
    return 0;
}

/* 采样获取 */
static int ina226_sample_fetch(xy_sensor_device_t *dev, xy_sensor_channel_t channel)
{
    ina226_private_data_t *priv = (ina226_private_data_t *)dev->data;
    
    /* 读取总线电压 (mV) */
    uint16_t bus_volt_raw;
    if (xy_sensor_i2c_read_reg16(&priv->bus, INA226_REG_BUS_VOLT, &bus_volt_raw) != 0) {
        return -1;
    }
    float voltage = bus_volt_raw * 1.25f;  /* 1.25mV/LSB */
    
    /* 读取电流 */
    int16_t current_raw;
    if (xy_sensor_i2c_read_reg16(&priv->bus, INA226_REG_CURRENT, (uint16_t*)&current_raw) != 0) {
        return -1;
    }
    float current = priv->current_lsb * current_raw;  /* A */
    
    /* 计算功率 */
    float power = voltage * current / 1000.0f;  /* W */
    
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->voltage_data, voltage);
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->current_data, current * 1000);  /* mA */
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->power_data, power * 1000);      /* mW */
    
    return 0;
}

/* 通道数据获取 */
static int ina226_channel_get(xy_sensor_device_t *dev, 
                              xy_sensor_channel_t channel, 
                              xy_sensor_value_t *val)
{
    ina226_private_data_t *priv = (ina226_private_data_t *)dev->data;
    
    if (!val) return -1;
    
    switch (channel) {
        case XY_SENSOR_CHAN_VOLTAGE:
            *val = priv->voltage_data;
            break;
        case XY_SENSOR_CHAN_CURRENT:
            *val = priv->current_data;
            break;
        case XY_SENSOR_CHAN_POWER:
            *val = priv->power_data;
            break;
        default:
            return -1;
    }
    return 0;
}

/* 驱动 API */
static const xy_sensor_driver_api_t ina226_driver_api = {
    .init = ina226_init,
    .sample_fetch = ina226_sample_fetch,
    .channel_get = ina226_channel_get,
};

/* 设备实例 */
static ina226_private_data_t ina226_priv;
static xy_sensor_device_t ina226_device = {
    .name = "INA226",
    .type = XY_SENSOR_TYPE_POWER,
    .api = &ina226_driver_api,
    .data = &ina226_priv,
};

/* 注册函数 */
int xy_sensor_ina226_register(void *i2c_handle, uint8_t addr)
{
    xy_sensor_bus_config_i2c(&ina226_priv.bus, i2c_handle, addr ? addr : INA226_ADDR);
    return xy_sensor_device_register(&ina226_device);
}
