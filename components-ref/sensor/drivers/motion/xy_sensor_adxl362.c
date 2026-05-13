/**
 * @file xy_sensor_adxl362.c
 * @brief ADXL362 Ultra Low Power Accelerometer - Migrated
 * @version 2.0.0
 * @date 2026-03-05
 */

#include "xy_sensor.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ADXL362 寄存器定义 */
#define ADXL362_REG_DEVID_AD   0x00
#define ADXL362_REG_PARTID     0x02
#define ADXL362_REG_STATUS     0x0B
#define ADXL362_REG_XDATA      0x0E
#define ADXL362_REG_POWER_CTL  0x2D
#define ADXL362_REG_FILTER_CTL 0x2C

#define ADXL362_PARTID         0xF2
#define ADXL362_CMD_WRITE      0x0A
#define ADXL362_CMD_READ       0x0B

/* 私有数据 */
typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    uint8_t range_g;
    xy_sensor_value_t accel_x, accel_y, accel_z;
} adxl362_private_data_t;

/* SPI 读写 */
static int adxl362_spi_write(xy_sensor_bus_t *bus, uint8_t reg, uint8_t value)
{
    uint8_t buf[3] = {ADXL362_CMD_WRITE, reg, value};
    /* 实际 SPI 实现 */
    return 0;
}

static int adxl362_spi_read(xy_sensor_bus_t *bus, uint8_t reg, uint8_t *value)
{
    uint8_t buf[3] = {ADXL362_CMD_READ, reg, 0};
    /* 实际 SPI 实现 */
    *value = buf[2];
    return 0;
}

/* 初始化 */
static int adxl362_init(xy_sensor_device_t *dev)
{
    adxl362_private_data_t *priv = (adxl362_private_data_t *)dev->data;
    
    /* 检查设备 ID */
    uint8_t part_id;
    if (adxl362_spi_read(&priv->bus, ADXL362_REG_PARTID, &part_id) != 0) {
        return -1;
    }
    
    if (part_id != ADXL362_PARTID) {
        xy_log_e("ADXL362: Wrong ID (0x%02X)\n", part_id);
        return -1;
    }
    
    /* 配置：测量模式，100Hz, ±2g */
    adxl362_spi_write(&priv->bus, ADXL362_REG_FILTER_CTL, 0x02);  /* 100Hz */
    adxl362_spi_write(&priv->bus, ADXL362_REG_POWER_CTL, 0x02);   /* 测量模式 */
    
    priv->range_g = 2;
    priv->initialized = true;
    xy_log_i("ADXL362 initialized\n");
    return 0;
}

/* 采样获取 */
static int adxl362_sample_fetch(xy_sensor_device_t *dev, xy_sensor_channel_t channel)
{
    adxl362_private_data_t *priv = (adxl362_private_data_t *)dev->data;
    
    /* 读取 3 轴数据 (6 字节) */
    uint8_t buf[6];
    /* 实际 SPI 读取 */
    
    int16_t x = (buf[1] << 8) | buf[0];
    int16_t y = (buf[3] << 8) | buf[2];
    int16_t z = (buf[5] << 8) | buf[4];
    
    /* 转换为 mG (±2g 量程，1mg/LSB) */
    float accel_x = x * 1.0f;
    float accel_y = y * 1.0f;
    float accel_z = z * 1.0f;
    
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->accel_x, accel_x);
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->accel_y, accel_y);
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->accel_z, accel_z);
    
    return 0;
}

/* 通道数据获取 */
static int adxl362_channel_get(xy_sensor_device_t *dev, 
                               xy_sensor_channel_t channel, 
                               xy_sensor_value_t *val)
{
    adxl362_private_data_t *priv = (adxl362_private_data_t *)dev->data;
    if (!val) return -1;
    
    switch (channel) {
        case XY_SENSOR_CHAN_ACCEL_X: *val = priv->accel_x; break;
        case XY_SENSOR_CHAN_ACCEL_Y: *val = priv->accel_y; break;
        case XY_SENSOR_CHAN_ACCEL_Z: *val = priv->accel_z; break;
        default: return -1;
    }
    return 0;
}

/* 驱动 API */
static const xy_sensor_driver_api_t adxl362_driver_api = {
    .init = adxl362_init,
    .sample_fetch = adxl362_sample_fetch,
    .channel_get = adxl362_channel_get,
};

static adxl362_private_data_t adxl362_priv;
static xy_sensor_device_t adxl362_device = {
    .name = "ADXL362",
    .type = XY_SENSOR_TYPE_ACCEL,
    .api = &adxl362_driver_api,
    .data = &adxl362_priv,
};

int xy_sensor_adxl362_register(void *spi_handle)
{
    adxl362_priv.bus.type = XY_SENSOR_BUS_SPI;
    adxl362_priv.bus.bus_handle = spi_handle;
    return xy_sensor_device_register(&adxl362_device);
}
