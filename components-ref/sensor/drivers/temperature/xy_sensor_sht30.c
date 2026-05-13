/**
 * @file xy_sensor_sht30.c
 * @brief SHT30 Temperature & Humidity Sensor Driver - Migrated to New Framework
 * @version 2.0.0
 * @date 2026-03-05
 */

#include "xy_sensor.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ==================== SHT30 寄存器定义 ==================== */

#define SHT30_ADDR_DEFAULT      0x44
#define SHT30_ADDR_ALT          0x45
#define SHT30_CMD_MEASURE_H     0x2400  /* 高重复性 */
#define SHT30_CMD_MEASURE_M     0x240B  /* 中重复性 */
#define SHT30_CMD_SOFT_RESET    0x30A2

/* CRC 计算 */
static uint8_t sht30_crc8(const uint8_t *data, size_t len)
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

/* ==================== 私有数据结构 ==================== */

typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    xy_sensor_value_t temp_data;
    xy_sensor_value_t humidity_data;
} sht30_private_data_t;

/* ==================== 驱动实现 ==================== */

/**
 * @brief SHT30 初始化
 */
static int sht30_init(xy_sensor_device_t *dev)
{
    sht30_private_data_t *priv = (sht30_private_data_t *)dev->data;
    
    /* 软件复位 */
    if (xy_sensor_i2c_write_reg(&priv->bus, SHT30_CMD_SOFT_RESET, 0) != 0) {
        xy_log_e("SHT30: Reset failed\n");
        return -1;
    }
    xy_os_delay(15);
    
    priv->initialized = true;
    xy_log_i("SHT30 initialized\n");
    return 0;
}

/**
 * @brief SHT30 采样获取
 */
static int sht30_sample_fetch(xy_sensor_device_t *dev, xy_sensor_channel_t channel)
{
    sht30_private_data_t *priv = (sht30_private_data_t *)dev->data;
    
    if (!priv->initialized) {
        return -1;
    }
    
    /* 触发测量 (高重复性) */
    uint8_t cmd[2] = {SHT30_CMD_MEASURE_H >> 8, SHT30_CMD_MEASURE_H & 0xFF};
    if (xy_sensor_i2c_write(&priv->bus, 0, cmd, 2) != 0) {
        return -1;
    }
    
    /* 等待测量完成 (约 15ms) */
    xy_os_delay(20);
    
    /* 读取数据 (6 字节：2 字节温度+CRC + 2 字节湿度+CRC) */
    uint8_t data[6];
    if (xy_sensor_i2c_read(&priv->bus, 0, data, 6) != 0) {
        return -1;
    }
    
    /* 验证 CRC */
    if (sht30_crc8(data, 2) != data[2] || sht30_crc8(&data[3], 2) != data[5]) {
        xy_log_e("SHT30: CRC error\n");
        return -1;
    }
    
    /* 解析数据 */
    uint16_t temp_raw = ((uint16_t)data[0] << 8) | data[1];
    uint16_t humidity_raw = ((uint16_t)data[3] << 8) | data[4];
    
    float temperature = -45.0f + 175.0f * (float)temp_raw / 65535.0f;
    float humidity = 100.0f * (float)humidity_raw / 65535.0f;
    
    /* 存储到缓存 */
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->temp_data, temperature);
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->humidity_data, humidity);
    
    xy_log_d("SHT30: T=%.2f°C, H=%.2f%%\n", temperature, humidity);
    return 0;
}

/**
 * @brief SHT30 通道数据获取
 */
static int sht30_channel_get(xy_sensor_device_t *dev, 
                             xy_sensor_channel_t channel, 
                             xy_sensor_value_t *val)
{
    sht30_private_data_t *priv = (sht30_private_data_t *)dev->data;
    
    if (!val) {
        return -1;
    }
    
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

/**
 * @brief SHT30 驱动 API
 */
static const xy_sensor_driver_api_t sht30_driver_api = {
    .init = sht30_init,
    .sample_fetch = sht30_sample_fetch,
    .channel_get = sht30_channel_get,
    .attr_set = NULL,
    .attr_get = NULL,
    .trigger_set = NULL,
};

/* ==================== 设备注册 ==================== */

static sht30_private_data_t sht30_priv_data;
static xy_sensor_device_t sht30_device = {
    .name = "SHT30",
    .type = XY_SENSOR_TYPE_COMBO,
    .api = &sht30_driver_api,
    .data = &sht30_priv_data,
    .bus = {0},
    .initialized = false,
};

/**
 * @brief 注册 SHT30 传感器
 */
int xy_sensor_sht30_register(void *i2c_handle, uint8_t addr)
{
    xy_sensor_bus_config_i2c(&sht30_priv_data.bus, i2c_handle, addr);
    return xy_sensor_device_register(&sht30_device);
}
