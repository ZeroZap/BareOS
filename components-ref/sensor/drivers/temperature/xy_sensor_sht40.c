/**
 * @file xy_sensor_sht40.c
 * @brief SHT40 Temperature & Humidity Sensor Driver - Migrated
 * @version 2.0.0
 * @date 2026-03-05
 */

#include "xy_sensor.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* SHT40 寄存器定义 */
#define SHT40_ADDR              0x44
#define SHT40_CMD_MEASURE_HPM   0xFD  /* 高精度模式 */
#define SHT40_CMD_SOFT_RESET    0x94

/* CRC 计算 */
static uint8_t sht40_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) : (crc << 1);
        }
    }
    return crc;
}

/* 私有数据 */
typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    xy_sensor_value_t temp_data;
    xy_sensor_value_t humidity_data;
} sht40_private_data_t;

/* 初始化 */
static int sht40_init(xy_sensor_device_t *dev)
{
    sht40_private_data_t *priv = (sht40_private_data_t *)dev->data;
    
    /* 软件复位 */
    if (xy_sensor_i2c_write_reg(&priv->bus, SHT40_CMD_SOFT_RESET, 0) != 0) {
        return -1;
    }
    xy_os_delay(1);
    
    priv->initialized = true;
    xy_log_i("SHT40 initialized\n");
    return 0;
}

/* 采样获取 */
static int sht40_sample_fetch(xy_sensor_device_t *dev, xy_sensor_channel_t channel)
{
    sht40_private_data_t *priv = (sht40_private_data_t *)dev->data;
    
    /* 触发测量 */
    if (xy_sensor_i2c_write_reg(&priv->bus, SHT40_CMD_MEASURE_HPM, 0) != 0) {
        return -1;
    }
    
    /* 等待测量完成 (约 10ms) */
    xy_os_delay(15);
    
    /* 读取数据 (6 字节) */
    uint8_t data[6];
    if (xy_sensor_i2c_read(&priv->bus, 0, data, 6) != 0) {
        return -1;
    }
    
    /* 验证 CRC */
    if (sht40_crc8(data, 2) != data[2] || sht40_crc8(&data[3], 2) != data[5]) {
        xy_log_e("SHT40: CRC error\n");
        return -1;
    }
    
    /* 解析数据 */
    uint16_t temp_raw = ((uint16_t)data[0] << 8) | data[1];
    uint16_t humidity_raw = ((uint16_t)data[3] << 8) | data[4];
    
    float temperature = -45.0f + 175.0f * (float)temp_raw / 65535.0f;
    float humidity = 100.0f * (float)humidity_raw / 65535.0f;
    
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->temp_data, temperature);
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->humidity_data, humidity);
    
    return 0;
}

/* 通道数据获取 */
static int sht40_channel_get(xy_sensor_device_t *dev, 
                             xy_sensor_channel_t channel, 
                             xy_sensor_value_t *val)
{
    sht40_private_data_t *priv = (sht40_private_data_t *)dev->data;
    
    if (!val) return -1;
    
    switch (channel) {
        case XY_SENSOR_CHAN_AMBIENT_TEMP:
            *val = priv->temp_data;
            break;
        case XY_SENSOR_CHAN_HUMIDITY:
            *val = priv->humidity_data;
            break;
        default:
            return -1;
    }
    return 0;
}

/* 驱动 API */
static const xy_sensor_driver_api_t sht40_driver_api = {
    .init = sht40_init,
    .sample_fetch = sht40_sample_fetch,
    .channel_get = sht40_channel_get,
};

/* 设备实例 */
static sht40_private_data_t sht40_priv;
static xy_sensor_device_t sht40_device = {
    .name = "SHT40",
    .type = XY_SENSOR_TYPE_COMBO,
    .api = &sht40_driver_api,
    .data = &sht40_priv,
};

/* 注册函数 */
int xy_sensor_sht40_register(void *i2c_handle, uint8_t addr)
{
    xy_sensor_bus_config_i2c(&sht40_priv.bus, i2c_handle, addr);
    return xy_sensor_device_register(&sht40_device);
}
