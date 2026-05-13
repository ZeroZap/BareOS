/**
 * @file xy_sensor_tsl2561.c
 * @brief TSL2561 Digital Light Sensor - Migrated
 * @version 2.0.0
 * @date 2026-03-05
 */

#include "xy_sensor.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* TSL2561 寄存器定义 */
#define TSL2561_ADDR_LOW      0x29
#define TSL2561_ADDR_FLOAT    0x39
#define TSL2561_ADDR_HIGH     0x49

#define TSL2561_REG_CONTROL   0x00
#define TSL2561_REG_TIMING    0x01
#define TSL2561_REG_THRESH_L  0x02
#define TSL2561_REG_THRESH_H  0x04
#define TSL2561_REG_INTERRUPT 0x06
#define TSL2561_REG_BYTE      0x08
#define TSL2561_REG_ID        0x0A
#define TSL2561_REG_DATA0_L   0x0C
#define TSL2561_REG_DATA1_L   0x0E

#define TSL2561_ID            0x50

/* 控制命令 */
#define TSL2561_CMD_POWER_ON  0x03
#define TSL2561_CMD_POWER_OFF 0x00

/* 增益 */
typedef enum {
    TSL2561_GAIN_1X = 0x00,
    TSL2561_GAIN_16X = 0x10,
} tsl2561_gain_t;

/* 积分时间 */
typedef enum {
    TSL2561_INTEG_13_7MS = 0x00,
    TSL2561_INTEG_101MS = 0x01,
    TSL2561_INTEG_402MS = 0x02,
} tsl2561_integ_t;

/* 私有数据 */
typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    tsl2561_gain_t gain;
    tsl2561_integ_t integ_time;
    xy_sensor_value_t light_data;
} tsl2561_private_data_t;

/* 初始化 */
static int tsl2561_init(xy_sensor_device_t *dev)
{
    tsl2561_private_data_t *priv = (tsl2561_private_data_t *)dev->data;
    
    /* 检查设备 ID */
    uint8_t id;
    if (xy_sensor_i2c_read_reg(&priv->bus, TSL2561_REG_ID, &id) != 0) {
        return -1;
    }
    
    if ((id & 0xF0) != TSL2561_ID) {
        xy_log_e("TSL2561: Wrong ID (0x%02X)\n", id);
        return -1;
    }
    
    /* 上电 */
    xy_sensor_i2c_write_reg(&priv->bus, TSL2561_REG_CONTROL, TSL2561_CMD_POWER_ON);
    xy_os_delay(10);
    
    /* 配置：402ms 积分时间，16x 增益 */
    priv->gain = TSL2561_GAIN_16X;
    priv->integ_time = TSL2561_INTEG_402MS;
    xy_sensor_i2c_write_reg(&priv->bus, TSL2561_REG_TIMING, priv->gain | priv->integ_time);
    
    priv->initialized = true;
    xy_log_i("TSL2561 initialized\n");
    return 0;
}

/* 计算照度 */
static float tsl2561_calculate_lux(uint16_t broadband, uint16_t ir)
{
    unsigned long channel0, channel1;
    unsigned long temp1, temp2, ratio;
    unsigned long lux;
    
    channel0 = broadband;
    channel1 = ir;
    
    /* 计算比率 */
    ratio = (channel1 << (TSL2561_INTEG_402MS + 1)) / channel0;
    
    /* 根据比率计算照度 */
    if ((ratio >= 0) && (ratio <= 320)) {
        temp1 = 0x0040 * channel0 - 0x005c * channel1;
        temp2 = 0x0096;
    } else if ((ratio >= 321) && (ratio <= 490)) {
        temp1 = 0x0069 * channel0 - 0x00a0 * channel1;
        temp2 = 0x00c9;
    } else if ((ratio >= 491) && (ratio <= 630)) {
        temp1 = 0x0084 * channel0 - 0x00c8 * channel1;
        temp2 = 0x00fa;
    } else {
        temp1 = 0x00be * channel0 - 0x0138 * channel1;
        temp2 = 0x017c;
    }
    
    lux = ((temp1 + (temp2 >> 1)) / temp2);
    return (float)lux;
}

/* 采样获取 */
static int tsl2561_sample_fetch(xy_sensor_device_t *dev, xy_sensor_channel_t channel)
{
    tsl2561_private_data_t *priv = (tsl2561_private_data_t *)dev->data;
    
    /* 等待转换完成 (402ms) */
    xy_os_delay(450);
    
    /* 读取通道 0 和通道 1 数据 */
    uint8_t buf[4];
    if (xy_sensor_i2c_read(&priv->bus, TSL2561_REG_DATA0_L, buf, 4) != 0) {
        return -1;
    }
    
    uint16_t broadband = (buf[1] << 8) | buf[0];
    uint16_t ir = (buf[3] << 8) | buf[2];
    
    /* 计算照度 */
    float lux = tsl2561_calculate_lux(broadband, ir);
    
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->light_data, lux);
    
    return 0;
}

/* 通道数据获取 */
static int tsl2561_channel_get(xy_sensor_device_t *dev, 
                               xy_sensor_channel_t channel, 
                               xy_sensor_value_t *val)
{
    tsl2561_private_data_t *priv = (tsl2561_private_data_t *)dev->data;
    if (!val) return -1;
    
    if (channel == XY_SENSOR_CHAN_LIGHT) {
        *val = priv->light_data;
    } else {
        return -1;
    }
    return 0;
}

/* 驱动 API */
static const xy_sensor_driver_api_t tsl2561_driver_api = {
    .init = tsl2561_init,
    .sample_fetch = tsl2561_sample_fetch,
    .channel_get = tsl2561_channel_get,
};

static tsl2561_private_data_t tsl2561_priv;
static xy_sensor_device_t tsl2561_device = {
    .name = "TSL2561",
    .type = XY_SENSOR_TYPE_LIGHT,
    .api = &tsl2561_driver_api,
    .data = &tsl2561_priv,
};

int xy_sensor_tsl2561_register(void *i2c_handle, uint8_t addr)
{
    xy_sensor_bus_config_i2c(&tsl2561_priv.bus, i2c_handle, addr ? addr : TSL2561_ADDR_FLOAT);
    return xy_sensor_device_register(&tsl2561_device);
}
