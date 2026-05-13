/**
 * @file sensor_iis2iclp.c
 * @brief ST IIS2ICLP 超低功耗3轴加速度计驱动实现
 */
#include "sensor_iis2iclp.h"
#include <string.h>

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_spi_recv(void *bus, uint8_t cs, uint8_t *data, uint16_t len);
extern int hal_spi_send(void *bus, uint8_t cs, uint8_t *data, uint16_t len);

static sensor_err_t iis2iclp_reg_read(sensor_device_t *sensor, uint8_t reg, uint8_t *data)
{
    iis2iclp_priv_t *priv = (iis2iclp_priv_t *)sensor->priv_data;
    if (priv->spi_cs != IIS2ICLP_SPI_CS_NONE) {
        uint8_t tx = reg | 0x80;
        hal_spi_send(sensor->bus, priv->spi_cs, &tx, 1);
        return hal_spi_recv(sensor->bus, priv->spi_cs, data, 1);
    }
    return hal_i2c_mem_read(sensor->bus, priv->i2c_addr, reg, data, 1);
}

static sensor_err_t iis2iclp_reg_write(sensor_device_t *sensor, uint8_t reg, uint8_t data)
{
    iis2iclp_priv_t *priv = (iis2iclp_priv_t *)sensor->priv_data;
    if (priv->spi_cs != IIS2ICLP_SPI_CS_NONE) {
        uint8_t tx[2] = { reg & 0x7F, data };
        return hal_spi_send(sensor->bus, priv->spi_cs, tx, 2);
    }
    return hal_i2c_mem_write(sensor->bus, priv->i2c_addr, reg, &data, 1);
}

static sensor_err_t iis2iclp_init(sensor_device_t *sensor)
{
    uint8_t data;
    SENSOR_LOG("Initializing IIS2ICLP");

    if (iis2iclp_reg_read(sensor, IIS2ICLP_REG_WHOAMI, &data) != SENSOR_EOK) return SENSOR_EIO;
    if (data != IIS2ICLP_WHOAMI_VALUE) {
        SENSOR_LOG("Wrong WHO_AM_I: 0x%02X (expected 0x%02X)", data, IIS2ICLP_WHOAMI_VALUE);
        return SENSOR_ERROR;
    }

    iis2iclp_reg_write(sensor, IIS2ICLP_REG_CTRL3, 0x01);  /* 软复位 */
    SENSOR_DELAY_MS(10);

    /* 配置: 高性能模式, ±2g, 100Hz */
    data = (IIS2ICLP_RATE_100HZ << 4) | IIS2ICLP_RANGE_2G;
    iis2iclp_reg_write(sensor, IIS2ICLP_REG_CTRL1, data);
    ((iis2iclp_priv_t *)sensor->priv_data)->range = 2;

    SENSOR_LOG("IIS2ICLP initialized");
    return SENSOR_EOK;
}

static sensor_err_t iis2iclp_deinit(sensor_device_t *sensor)
{
    iis2iclp_reg_write(sensor, IIS2ICLP_REG_CTRL1, 0x00);
    return SENSOR_EOK;
}

static sensor_err_t iis2iclp_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint8_t buf[6];
    for (int i = 0; i < 6; i++) {
        if (iis2iclp_reg_read(sensor, IIS2ICLP_REG_OUT_X_L + i, &buf[i]) != SENSOR_EOK) return SENSOR_EIO;
    }

    int16_t raw[3];
    raw[0] = (int16_t)((buf[1] << 8) | buf[0]);
    raw[1] = (int16_t)((buf[3] << 8) | buf[2]);
    raw[2] = (int16_t)((buf[5] << 8) | buf[4]);

    iis2iclp_priv_t *priv = (iis2iclp_priv_t *)sensor->priv_data;
    int32_t scale = (priv->range == 2) ? 1 : (priv->range == 4) ? 2 : (priv->range == 8) ? 4 : 8;

    data->type = SENSOR_TYPE_ACCELEROMETER;
    data->unit = SENSOR_UNIT_MILLI_G;
    data->value.val_3axis.x = (int32_t)raw[0] * scale;
    data->value.val_3axis.y = (int32_t)raw[1] * scale;
    data->value.val_3axis.z = (int32_t)raw[2] * scale;
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 95;
    return SENSOR_EOK;
}

static const sensor_ops_t iis2iclp_ops = {
    .init = iis2iclp_init, .deinit = iis2iclp_deinit, .read = iis2iclp_read,
};

sensor_device_t *iis2iclp_create(const char *name, void *i2c_bus, uint8_t addr)
{
    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    iis2iclp_priv_t *priv = (iis2iclp_priv_t *)SENSOR_MALLOC(sizeof(iis2iclp_priv_t));
    if (!sensor || !priv) { SENSOR_FREE(sensor); SENSOR_FREE(priv); return NULL; }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(iis2iclp_priv_t));
    priv->i2c_addr = addr ? addr : IIS2ICLP_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor = "STMicro";
    sensor->info.model = "IIS2ICLP";
    sensor->info.version = 0x0100;
    sensor->info.type = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit = SENSOR_UNIT_MILLI_G;
    sensor->info.range_max = 2000;
    sensor->info.range_min = -2000;
    sensor->info.resolution = 16;
    sensor->info.max_odr = 200;
    sensor->info.flags = SENSOR_FLAG_LOW_POWER;

    sensor->ops = &iis2iclp_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 100;
    return sensor;
}

sensor_device_t *iis2iclp_create_spi(const char *name, void *spi_bus, uint8_t cs)
{
    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    iis2iclp_priv_t *priv = (iis2iclp_priv_t *)SENSOR_MALLOC(sizeof(iis2iclp_priv_t));
    if (!sensor || !priv) { SENSOR_FREE(sensor); SENSOR_FREE(priv); return NULL; }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(iis2iclp_priv_t));
    priv->spi_cs = cs;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor = "STMicro";
    sensor->info.model = "IIS2ICLP";
    sensor->info.version = 0x0100;
    sensor->info.type = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit = SENSOR_UNIT_MILLI_G;
    sensor->info.range_max = 2000;
    sensor->info.range_min = -2000;
    sensor->info.resolution = 16;
    sensor->info.max_odr = 200;
    sensor->info.flags = SENSOR_FLAG_LOW_POWER;

    sensor->ops = &iis2iclp_ops;
    sensor->bus = spi_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 100;
    return sensor;
}

int iis2iclp_set_range(sensor_device_t *dev, uint8_t range)
{
    uint8_t ctrl1;
    iis2iclp_reg_read(dev, IIS2ICLP_REG_CTRL1, &ctrl1);
    ctrl1 = (ctrl1 & 0xFC) | (range & 0x03);
    iis2iclp_reg_write(dev, IIS2ICLP_REG_CTRL1, ctrl1);
    ((iis2iclp_priv_t *)dev->priv_data)->range = range;
    return 0;
}
