/**
 * @file sensor_silan_sc7a20.c
 * @brief 士兰微 SC7A20 3轴加速度计驱动实现
 */
#include "sensor_silan_sc7a20.h"
#include <string.h>

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

static sensor_err_t silan_sc7a20_reg_read(sensor_device_t *sensor, uint8_t reg, uint8_t *data)
{
    silan_sc7a20_priv_t *priv = (silan_sc7a20_priv_t *)sensor->priv_data;
    return hal_i2c_mem_read(sensor->bus, priv->i2c_addr, reg, data, 1);
}

static sensor_err_t silan_sc7a20_reg_write(sensor_device_t *sensor, uint8_t reg, uint8_t data)
{
    silan_sc7a20_priv_t *priv = (silan_sc7a20_priv_t *)sensor->priv_data;
    return hal_i2c_mem_write(sensor->bus, priv->i2c_addr, reg, &data, 1);
}

static sensor_err_t silan_sc7a20_init(sensor_device_t *sensor)
{
    uint8_t data;
    SENSOR_LOG("Initializing Silan SC7A20");

    /* 检查WHO_AM_I */
    if (silan_sc7a20_reg_read(sensor, SILAN_SC7A20_REG_WHOAMI, &data) != SENSOR_EOK) return SENSOR_EIO;
    if (data != SILAN_SC7A20_WHOAMI_VALUE) {
        SENSOR_LOG("Wrong WHO_AM_I: 0x%02X (expected 0x%02X)", data, SILAN_SC7A20_WHOAMI_VALUE);
        return SENSOR_ERROR;
    }

    /* 软复位 */
    silan_sc7a20_reg_write(sensor, SILAN_SC7A20_REG_CTRL2, 0x80);
    SENSOR_DELAY_MS(10);

    /* 配置: ±2g, 100Hz */
    data = SILAN_SC7A20_RATE_100HZ | SILAN_SC7A20_RANGE_2G | 0x07;  /* 使能 XYZ */
    silan_sc7a20_reg_write(sensor, SILAN_SC7A20_REG_CTRL1, data);

    ((silan_sc7a20_priv_t *)sensor->priv_data)->range = 2;
    ((silan_sc7a20_priv_t *)sensor->priv_data)->rate  = 100;

    SENSOR_LOG("Silan SC7A20 initialized (100Hz, ±2g)");
    return SENSOR_EOK;
}

static sensor_err_t silan_sc7a20_deinit(sensor_device_t *sensor)
{
    /* 进入掉电模式 */
    silan_sc7a20_reg_write(sensor, SILAN_SC7A20_REG_CTRL1, SILAN_SC7A20_RATE_POWER_DOWN);
    return SENSOR_EOK;
}

static sensor_err_t silan_sc7a20_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint8_t buf[6];
    
    /* 读取6字节加速度数据 */
    for (int i = 0; i < 6; i++) {
        if (silan_sc7a20_reg_read(sensor, SILAN_SC7A20_REG_OUT_X_L + i, &buf[i]) != SENSOR_EOK) {
            return SENSOR_EIO;
        }
    }

    /* 小端模式组合 */
    int16_t raw[3];
    raw[0] = (int16_t)((buf[1] << 8) | buf[0]);
    raw[1] = (int16_t)((buf[3] << 8) | buf[2]);
    raw[2] = (int16_t)((buf[5] << 8) | buf[4]);

    /* 转换为mg */
    silan_sc7a20_priv_t *priv = (silan_sc7a20_priv_t *)sensor->priv_data;
    int32_t scale = 0;
    switch (priv->range) {
    case 2:  scale = 1; break;    /* 1mg/LSB */
    case 4:  scale = 2; break;    /* 2mg/LSB */
    case 8:  scale = 4; break;    /* 4mg/LSB */
    case 16: scale = 12; break;   /* 12mg/LSB */
    default: scale = 1;
    }

    data->type              = SENSOR_TYPE_ACCELEROMETER;
    data->unit              = SENSOR_UNIT_MILLI_G;
    data->value.val_3axis.x = (int32_t)raw[0] * scale;
    data->value.val_3axis.y = (int32_t)raw[1] * scale;
    data->value.val_3axis.z = (int32_t)raw[2] * scale;
    data->timestamp         = SENSOR_GET_TICK();
    data->accuracy          = 95;

    return SENSOR_EOK;
}

/* 操作接口 */
static const sensor_ops_t silan_sc7a20_ops = {
    .init   = silan_sc7a20_init,
    .deinit = silan_sc7a20_deinit,
    .read   = silan_sc7a20_read,
};

/**
 * @brief 创建SC7A20加速度计设备
 */
sensor_device_t *silan_sc7a20_create(const char *name, void *i2c_bus, uint8_t addr)
{
    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    silan_sc7a20_priv_t *priv = (silan_sc7a20_priv_t *)SENSOR_MALLOC(sizeof(silan_sc7a20_priv_t));
    if (!sensor || !priv) { SENSOR_FREE(sensor); SENSOR_FREE(priv); return NULL; }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(silan_sc7a20_priv_t));

    priv->i2c_addr = addr ? addr : SILAN_SC7A20_ADDR_DEFAULT;

    /* 设置设备信息 */
    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "Silan";
    sensor->info.model      = "SC7A20";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit       = SENSOR_UNIT_MILLI_G;
    sensor->info.range_max  = 2000;
    sensor->info.range_min  = -2000;
    sensor->info.resolution = 12;
    sensor->info.max_odr    = 400;
    sensor->info.flags      = SENSOR_FLAG_INT_SUPPORT | SENSOR_FLAG_FIFO_SUPPORT;

    sensor->ops       = &silan_sc7a20_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 100;

    return sensor;
}

/**
 * @brief 设置量程
 */
int silan_sc7a20_set_range(sensor_device_t *dev, uint8_t range)
{
    uint8_t ctrl1;
    silan_sc7a20_reg_read(dev, SILAN_SC7A20_REG_CTRL1, &ctrl1);
    ctrl1 = (ctrl1 & 0xC7) | (range & 0x18);  /* 保留ODR位 */
    silan_sc7a20_reg_write(dev, SILAN_SC7A20_REG_CTRL1, ctrl1);
    ((silan_sc7a20_priv_t *)dev->priv_data)->range = (range == SILAN_SC7A20_RANGE_2G) ? 2 :
                                                       (range == SILAN_SC7A20_RANGE_4G) ? 4 :
                                                       (range == SILAN_SC7A20_RANGE_8G) ? 8 : 16;
    return 0;
}

/**
 * @brief 设置采样率
 */
int silan_sc7a20_set_rate(sensor_device_t *dev, uint8_t rate)
{
    uint8_t ctrl1;
    silan_sc7a20_reg_read(dev, SILAN_SC7A20_REG_CTRL1, &ctrl1);
    ctrl1 = (ctrl1 & 0x0F) | (rate & 0xF0);  /* 保留量程位 */
    silan_sc7a20_reg_write(dev, SILAN_SC7A20_REG_CTRL1, ctrl1);
    ((silan_sc7a20_priv_t *)dev->priv_data)->rate = rate;
    return 0;
}

/**
 * @brief 使能高通滤波器
 */
int silan_sc7a20_enable_high_pass(sensor_device_t *dev, uint8_t enable)
{
    uint8_t ctrl2;
    silan_sc7a20_reg_read(dev, SILAN_SC7A20_REG_CTRL2, &ctrl2);
    ctrl2 = enable ? (ctrl2 | 0x01) : (ctrl2 & 0xFE);
    silan_sc7a20_reg_write(dev, SILAN_SC7A20_REG_CTRL2, ctrl2);
    return 0;
}
