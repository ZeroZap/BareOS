/**
 * @file xy_sensor_aht20.c
 * @brief AHT20 Temperature & Humidity Sensor Driver - Migrated to New Framework
 * @version 2.0.0
 * @date 2026-03-05
 */

#include "xy_sensor.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ==================== AHT20 寄存器定义 ==================== */

#define AHT20_ADDR              0x38
#define AHT20_CMD_INIT          0xBE
#define AHT20_CMD_TRIGGER       0xAC
#define AHT20_CMD_RESET         0xBA
#define AHT20_STATUS_BUSY       0x80
#define AHT20_STATUS_CALIBRATED 0x08

/* ==================== 私有数据结构 ==================== */

typedef struct {
    xy_sensor_bus_t bus;                /* 总线信息 */
    bool initialized;
    bool calibrated;
    xy_sensor_value_t temp_data;        /* 温度缓存 */
    xy_sensor_value_t humidity_data;    /* 湿度缓存 */
} aht20_private_data_t;

/* ==================== 驱动实现 ==================== */

/**
 * @brief AHT20 初始化
 */
static int aht20_init(xy_sensor_device_t *dev)
{
    aht20_private_data_t *priv = (aht20_private_data_t *)dev->data;
    
    if (!priv) {
        return -1;
    }
    
    /* I2C 读取设备状态 */
    uint8_t status;
    if (xy_sensor_i2c_read_reg(&priv->bus, 0x71, &status) != 0) {
        xy_log_e("AHT20: Failed to read status\n");
        return -1;
    }
    
    /* 检查是否需要初始化 */
    if (!(status & AHT20_STATUS_CALIBRATED)) {
        uint8_t init_cmd[] = {AHT20_CMD_INIT, 0x08, 0x00};
        if (xy_sensor_i2c_write(&priv->bus, 0x71, init_cmd, 3) != 0) {
            xy_log_e("AHT20: Init failed\n");
            return -1;
        }
        xy_os_delay(10);
    }
    
    priv->initialized = true;
    xy_log_i("AHT20 initialized\n");
    return 0;
}

/**
 * @brief AHT20 采样获取
 */
static int aht20_sample_fetch(xy_sensor_device_t *dev, xy_sensor_channel_t channel)
{
    aht20_private_data_t *priv = (aht20_private_data_t *)dev->data;
    
    if (!priv->initialized) {
        return -1;
    }
    
    /* 触发测量 */
    uint8_t trigger_cmd[] = {AHT20_CMD_TRIGGER, 0x33, 0x00};
    if (xy_sensor_i2c_write(&priv->bus, 0xAC, trigger_cmd, 3) != 0) {
        return -1;
    }
    
    /* 等待测量完成 (约 75ms) */
    xy_os_delay(80);
    
    /* 检查忙状态 */
    uint8_t status;
    int timeout = 10;
    do {
        xy_sensor_i2c_read_reg(&priv->bus, 0x71, &status);
        xy_os_delay(10);
    } while ((status & AHT20_STATUS_BUSY) && --timeout > 0);
    
    if (timeout == 0) {
        xy_log_e("AHT20: Timeout\n");
        return -1;
    }
    
    /* 读取数据 (6 字节) */
    uint8_t data[6];
    if (xy_sensor_i2c_read(&priv->bus, 0x00, data, 6) != 0) {
        return -1;
    }
    
    /* 解析数据 */
    uint32_t humidity_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    uint32_t temp_raw = (((uint32_t)data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    
    /* 转换为实际值 */
    float humidity = (float)humidity_raw / (1 << 20) * 100.0f;
    float temperature = (float)temp_raw / (1 << 20) * 200.0f - 50.0f;
    
    /* 存储到缓存 */
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->humidity_data, humidity);
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->temp_data, temperature);
    
    xy_log_d("AHT20: T=%.2f°C, H=%.2f%%\n", temperature, humidity);
    return 0;
}

/**
 * @brief AHT20 通道数据获取
 */
static int aht20_channel_get(xy_sensor_device_t *dev, 
                             xy_sensor_channel_t channel, 
                             xy_sensor_value_t *val)
{
    aht20_private_data_t *priv = (aht20_private_data_t *)dev->data;
    
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
 * @brief AHT20 属性设置
 */
static int aht20_attr_set(xy_sensor_device_t *dev,
                          xy_sensor_channel_t channel,
                          xy_sensor_attr_t attr,
                          const xy_sensor_value_t *val)
{
    /* AHT20 不支持属性配置 */
    return 0;
}

/**
 * @brief AHT20 驱动 API
 */
static const xy_sensor_driver_api_t aht20_driver_api = {
    .init = aht20_init,
    .sample_fetch = aht20_sample_fetch,
    .channel_get = aht20_channel_get,
    .attr_set = aht20_attr_set,
    .attr_get = NULL,
    .trigger_set = NULL,
};

/* ==================== 设备注册 ==================== */

static aht20_private_data_t aht20_priv_data;
static xy_sensor_device_t aht20_device = {
    .name = "AHT20",
    .type = XY_SENSOR_TYPE_COMBO,
    .api = &aht20_driver_api,
    .data = &aht20_priv_data,
    .bus = {0},
    .initialized = false,
};

/**
 * @brief 注册 AHT20 传感器
 */
int xy_sensor_aht20_register(void *i2c_handle, uint8_t addr)
{
    /* 配置总线 */
    xy_sensor_bus_config_i2c(&aht20_priv_data.bus, i2c_handle, addr);
    
    /* 注册设备 */
    return xy_sensor_device_register(&aht20_device);
}
