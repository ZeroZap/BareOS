/**
 * @file xy_sensor_bh1750.c
 * @brief BH1750 Ambient Light Sensor Driver - Migrated
 * @version 2.0.0
 * @date 2026-03-05
 */

#include "xy_sensor.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* BH1750 寄存器定义 */
#define BH1750_ADDR_LOW         0x23
#define BH1750_CMD_POWER_DOWN   0x00
#define BH1750_CMD_POWER_ON     0x01
#define BH1750_CMD_RESET        0x07
#define BH1750_CMD_CONT_H       0x10  /* 连续高分辨率 */

/* 私有数据 */
typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    xy_sensor_value_t light_data;
} bh1750_private_data_t;

/* 初始化 */
static int bh1750_init(xy_sensor_device_t *dev)
{
    bh1750_private_data_t *priv = (bh1750_private_data_t *)dev->data;
    
    /* 上电 */
    xy_sensor_i2c_write_reg(&priv->bus, BH1750_CMD_POWER_ON, 0);
    xy_os_delay(10);
    
    /* 复位 */
    xy_sensor_i2c_write_reg(&priv->bus, BH1750_CMD_RESET, 0);
    xy_os_delay(10);
    
    /* 配置连续高分辨率模式 */
    xy_sensor_i2c_write_reg(&priv->bus, BH1750_CMD_CONT_H, 0);
    xy_os_delay(180);
    
    priv->initialized = true;
    xy_log_i("BH1750 initialized\n");
    return 0;
}

/* 采样获取 */
static int bh1750_sample_fetch(xy_sensor_device_t *dev, xy_sensor_channel_t channel)
{
    bh1750_private_data_t *priv = (bh1750_private_data_t *)dev->data;
    
    /* 读取数据 (2 字节) */
    uint8_t data[2];
    if (xy_sensor_i2c_read(&priv->bus, 0, data, 2) != 0) {
        return -1;
    }
    
    /* 转换为照度 (lux) */
    uint16_t raw = ((uint16_t)data[0] << 8) | data[1];
    float lux = raw / 1.2f;
    
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->light_data, lux);
    
    return 0;
}

/* 通道数据获取 */
static int bh1750_channel_get(xy_sensor_device_t *dev, 
                              xy_sensor_channel_t channel, 
                              xy_sensor_value_t *val)
{
    bh1750_private_data_t *priv = (bh1750_private_data_t *)dev->data;
    
    if (!val) return -1;
    
    if (channel == XY_SENSOR_CHAN_LIGHT) {
        *val = priv->light_data;
    } else {
        return -1;
    }
    return 0;
}

/* 驱动 API */
static const xy_sensor_driver_api_t bh1750_driver_api = {
    .init = bh1750_init,
    .sample_fetch = bh1750_sample_fetch,
    .channel_get = bh1750_channel_get,
};

/* 设备实例 */
static bh1750_private_data_t bh1750_priv;
static xy_sensor_device_t bh1750_device = {
    .name = "BH1750",
    .type = XY_SENSOR_TYPE_LIGHT,
    .api = &bh1750_driver_api,
    .data = &bh1750_priv,
};

/* 注册函数 */
int xy_sensor_bh1750_register(void *i2c_handle, uint8_t addr)
{
    xy_sensor_bus_config_i2c(&bh1750_priv.bus, i2c_handle, addr ? addr : BH1750_ADDR_LOW);
    return xy_sensor_device_register(&bh1750_device);
}
