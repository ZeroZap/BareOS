/**
 * @file xy_sensor_hdc1080.c
 * @brief HDC1080 Temperature & Humidity Sensor - Migrated
 * @version 2.0.0
 * @date 2026-03-05
 */

#include "xy_sensor.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* HDC1080 寄存器定义 */
#define HDC1080_ADDR            0x40
#define HDC1080_REG_TEMP        0x00
#define HDC1080_REG_HUMI        0x01
#define HDC1080_REG_CONFIG      0x02
#define HDC1080_CONFIG_RST      (1 << 15)
#define HDC1080_CONFIG_MODE     (1 << 12)

/* 私有数据 */
typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    xy_sensor_value_t temp_data;
    xy_sensor_value_t humidity_data;
} hdc1080_private_data_t;

/* 初始化 */
static int hdc1080_init(xy_sensor_device_t *dev)
{
    hdc1080_private_data_t *priv = (hdc1080_private_data_t *)dev->data;
    
    /* 软件复位 */
    uint16_t config = HDC1080_CONFIG_RST;
    xy_sensor_i2c_write_reg16(&priv->bus, HDC1080_REG_CONFIG, config);
    xy_os_delay(15);
    
    /* 配置：温湿度测量，14 位分辨率 */
    config = HDC1080_CONFIG_MODE;  /* 独立测量模式 */
    xy_sensor_i2c_write_reg16(&priv->bus, HDC1080_REG_CONFIG, config);
    xy_os_delay(15);
    
    priv->initialized = true;
    xy_log_i("HDC1080 initialized\n");
    return 0;
}

/* 采样获取 */
static int hdc1080_sample_fetch(xy_sensor_device_t *dev, xy_sensor_channel_t channel)
{
    hdc1080_private_data_t *priv = (hdc1080_private_data_t *)dev->data;
    
    /* 触发温度测量 */
    xy_sensor_i2c_write_reg(&priv->bus, HDC1080_REG_TEMP, 0);
    xy_os_delay(50);
    
    /* 读取温度 (2 字节) */
    uint8_t temp_buf[2];
    if (xy_sensor_i2c_read(&priv->bus, 0, temp_buf, 2) != 0) {
        return -1;
    }
    uint16_t temp_raw = ((uint16_t)temp_buf[0] << 8) | temp_buf[1];
    float temperature = (float)temp_raw / 65536.0f * 165.0f - 40.0f;
    
    /* 触发湿度测量 */
    xy_sensor_i2c_write_reg(&priv->bus, HDC1080_REG_HUMI, 0);
    xy_os_delay(50);
    
    /* 读取湿度 (2 字节) */
    uint8_t humi_buf[2];
    if (xy_sensor_i2c_read(&priv->bus, 0, humi_buf, 2) != 0) {
        return -1;
    }
    uint16_t humi_raw = ((uint16_t)humi_buf[0] << 8) | humi_buf[1];
    float humidity = (float)humi_raw / 65536.0f * 100.0f;
    
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->temp_data, temperature);
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->humidity_data, humidity);
    
    return 0;
}

/* 通道数据获取 */
static int hdc1080_channel_get(xy_sensor_device_t *dev, 
                               xy_sensor_channel_t channel, 
                               xy_sensor_value_t *val)
{
    hdc1080_private_data_t *priv = (hdc1080_private_data_t *)dev->data;
    if (!val) return -1;
    
    switch (channel) {
        case XY_SENSOR_CHAN_AMBIENT_TEMP: *val = priv->temp_data; break;
        case XY_SENSOR_CHAN_HUMIDITY: *val = priv->humidity_data; break;
        default: return -1;
    }
    return 0;
}

/* 驱动 API */
static const xy_sensor_driver_api_t hdc1080_driver_api = {
    .init = hdc1080_init,
    .sample_fetch = hdc1080_sample_fetch,
    .channel_get = hdc1080_channel_get,
};

static hdc1080_private_data_t hdc1080_priv;
static xy_sensor_device_t hdc1080_device = {
    .name = "HDC1080",
    .type = XY_SENSOR_TYPE_COMBO,
    .api = &hdc1080_driver_api,
    .data = &hdc1080_priv,
};

int xy_sensor_hdc1080_register(void *i2c_handle, uint8_t addr)
{
    xy_sensor_bus_config_i2c(&hdc1080_priv.bus, i2c_handle, addr ? addr : HDC1080_ADDR);
    return xy_sensor_device_register(&hdc1080_device);
}
