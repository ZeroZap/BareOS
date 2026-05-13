/**
 * @file sensor_qma6100.c
 * @brief QMA6100 3轴加速度计驱动实现
 */
#include "sensor_qma6100.h"
#include <string.h>

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

static sensor_err_t qma6100_reg_read(sensor_device_t *sensor, uint8_t reg, uint8_t *data)
{
    qma6100_priv_t *priv = (qma6100_priv_t *)sensor->priv_data;
    return hal_i2c_mem_read(sensor->bus, priv->i2c_addr, reg, data, 1);
}

static sensor_err_t qma6100_reg_write(sensor_device_t *sensor, uint8_t reg, uint8_t data)
{
    qma6100_priv_t *priv = (qma6100_priv_t *)sensor->priv_data;
    return hal_i2c_mem_write(sensor->bus, priv->i2c_addr, reg, &data, 1);
}

static sensor_err_t qma6100_init(sensor_device_t *sensor)
{
    uint8_t data;
    SENSOR_LOG("Initializing QMA6100");

    if (qma6100_reg_read(sensor, QMA6100_REG_WHOAMI, &data) != SENSOR_EOK) return SENSOR_EIO;
    if (data != QMA6100_WHOAMI_VALUE) {
        SENSOR_LOG("Wrong WHO_AM_I: 0x%02X (expected 0x%02X)", data, QMA6100_WHOAMI_VALUE);
        return SENSOR_ERROR;
    }

    /* 软件复位 */
    qma6100_reg_write(sensor, QMA6100_REG_DSET, 0x20);
    SENSOR_DELAY_MS(10);

    /* 配置: ±2g, 100Hz */
    data = (QMA6100_RATE_100HZ << 4) | QMA6100_RANGE_2G;
    qma6100_reg_write(sensor, QMA6100_REG_CTRL, data);

    ((qma6100_priv_t *)sensor->priv_data)->range = 2;
    ((qma6100_priv_t *)sensor->priv_data)->rate  = 100;

    SENSOR_LOG("QMA6100 initialized");
    return SENSOR_EOK;
}

static sensor_err_t qma6100_deinit(sensor_device_t *sensor)
{
    qma6100_reg_write(sensor, QMA6100_REG_PWRCTL, 0x01); /* 深度省电模式 */
    return SENSOR_EOK;
}

static sensor_err_t qma6100_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint8_t buf[6];
    for (int i = 0; i < 6; i++) {
        if (qma6100_reg_read(sensor, QMA6100_REG_DATA + i, &buf[i]) != SENSOR_EOK) return SENSOR_EIO;
    }

    int16_t raw[3];
    raw[0] = (int16_t)((buf[1] << 8) | buf[0]);
    raw[1] = (int16_t)((buf[3] << 8) | buf[2]);
    raw[2] = (int16_t)((buf[5] << 8) | buf[4]);

    qma6100_priv_t *priv = (qma6100_priv_t *)sensor->priv_data;
    int32_t scale = (priv->range == 2) ? 16 : (priv->range == 4) ? 31 : (priv->range == 8) ? 63 : 127;

    data->type = SENSOR_TYPE_ACCELEROMETER;
    data->unit = SENSOR_UNIT_MILLI_G;
    data->value.val_3axis.x = (int32_t)raw[0] * scale;
    data->value.val_3axis.y = (int32_t)raw[1] * scale;
    data->value.val_3axis.z = (int32_t)raw[2] * scale;
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 95;
    return SENSOR_EOK;
}

static const sensor_ops_t qma6100_ops = {
    .init = qma6100_init, .deinit = qma6100_deinit, .read = qma6100_read,
};

sensor_device_t *qma6100_create(const char *name, void *i2c_bus, uint8_t addr)
{
    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    qma6100_priv_t *priv = (qma6100_priv_t *)SENSOR_MALLOC(sizeof(qma6100_priv_t));
    if (!sensor || !priv) { SENSOR_FREE(sensor); SENSOR_FREE(priv); return NULL; }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(qma6100_priv_t));
    priv->i2c_addr = addr ? addr : QMA6100_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor = "QST";
    sensor->info.model = "QMA6100";
    sensor->info.version = 0x0100;
    sensor->info.type = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit = SENSOR_UNIT_MILLI_G;
    sensor->info.range_max = 2000;
    sensor->info.range_min = -2000;
    sensor->info.resolution = 16;
    sensor->info.max_odr = 800;
    sensor->info.flags = SENSOR_FLAG_LOW_POWER;

    sensor->ops = &qma6100_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 100;
    return sensor;
}

int qma6100_set_range(sensor_device_t *dev, uint8_t range)
{
    uint8_t ctrl;
    qma6100_reg_read(dev, QMA6100_REG_CTRL, &ctrl);
    ctrl = (ctrl & 0xFC) | (range & 0x03);
    qma6100_reg_write(dev, QMA6100_REG_CTRL, ctrl);
    ((qma6100_priv_t *)dev->priv_data)->range = range;
    return 0;
}
