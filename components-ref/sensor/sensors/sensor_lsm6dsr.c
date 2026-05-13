/**
 * @file sensor_lsm6dsr.c
 * @brief ST LSM6DSR 高性能6轴惯性传感器驱动实现
 */
#include "sensor_lsm6dsr.h"
#include <string.h>

/* I2C读写接口 */
extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

/* SPI读写接口 */
extern int hal_spi_recv(void *bus, uint8_t cs, uint8_t *data, uint16_t len);
extern int hal_spi_send(void *bus, uint8_t cs, uint8_t *data, uint16_t len);

static sensor_err_t lsm6dsr_reg_read(sensor_device_t *sensor, uint8_t reg, uint8_t *data)
{
    lsm6dsr_priv_t *priv = (lsm6dsr_priv_t *)sensor->priv_data;
    if (priv->spi_cs != LSM6DSR_SPI_CS_NONE) {
        uint8_t tx = reg | 0x80;
        hal_spi_send(sensor->bus, priv->spi_cs, &tx, 1);
        return hal_spi_recv(sensor->bus, priv->spi_cs, data, 1);
    }
    return hal_i2c_mem_read(sensor->bus, priv->i2c_addr, reg, data, 1);
}

static sensor_err_t lsm6dsr_reg_write(sensor_device_t *sensor, uint8_t reg, uint8_t data)
{
    lsm6dsr_priv_t *priv = (lsm6dsr_priv_t *)sensor->priv_data;
    if (priv->spi_cs != LSM6DSR_SPI_CS_NONE) {
        uint8_t tx[2] = { reg & 0x7F, data };
        return hal_spi_send(sensor->bus, priv->spi_cs, tx, 2);
    }
    return hal_i2c_mem_write(sensor->bus, priv->i2c_addr, reg, &data, 1);
}

static sensor_err_t lsm6dsr_init(sensor_device_t *sensor)
{
    uint8_t data;
    SENSOR_LOG("Initializing LSM6DSR");

    if (lsm6dsr_reg_read(sensor, LSM6DSR_REG_WHOAMI, &data) != SENSOR_EOK) return SENSOR_EIO;
    if (data != LSM6DSR_WHOAMI_VALUE) {
        SENSOR_LOG("Wrong WHO_AM_I: 0x%02X (expected 0x%02X)", data, LSM6DSR_WHOAMI_VALUE);
        return SENSOR_ERROR;
    }

    lsm6dsr_reg_write(sensor, LSM6DSR_REG_CTRL3_C, 0x01);  /* 软复位 */
    SENSOR_DELAY_MS(10);

    lsm6dsr_reg_write(sensor, LSM6DSR_REG_CTRL4_C, 0x00); /* 关闭I3C */

    /* 配置: ±2g, 104Hz */
    data = (LSM6DSR_ACCEL_RATE_104Hz << 4) | LSM6DSR_ACCEL_RANGE_2G;
    lsm6dsr_reg_write(sensor, LSM6DSR_REG_CTRL1_XL, data);
    ((lsm6dsr_priv_t *)sensor->priv_data)->accel_range = 2;
    ((lsm6dsr_priv_t *)sensor->priv_data)->accel_rate = 104;

    /* 配置: ±250°/s, 104Hz */
    data = (LSM6DSR_GYRO_RATE_104Hz << 4) | LSM6DSR_GYRO_RANGE_250DPS;
    lsm6dsr_reg_write(sensor, LSM6DSR_REG_CTRL2_G, data);
    ((lsm6dsr_priv_t *)sensor->priv_data)->gyro_range = 250;
    ((lsm6dsr_priv_t *)sensor->priv_data)->gyro_rate = 104;

    SENSOR_LOG("LSM6DSR initialized");
    return SENSOR_EOK;
}

static sensor_err_t lsm6dsr_deinit(sensor_device_t *sensor)
{
    lsm6dsr_reg_write(sensor, LSM6DSR_REG_CTRL1_XL, 0x00);
    lsm6dsr_reg_write(sensor, LSM6DSR_REG_CTRL2_G, 0x00);
    return SENSOR_EOK;
}

static sensor_err_t lsm6dsr_accel_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint8_t buf[6];
    for (int i = 0; i < 6; i++) {
        if (lsm6dsr_reg_read(sensor, LSM6DSR_REG_OUTX_L_XL + i, &buf[i]) != SENSOR_EOK) return SENSOR_EIO;
    }

    int16_t raw[3];
    raw[0] = (int16_t)((buf[1] << 8) | buf[0]);
    raw[1] = (int16_t)((buf[3] << 8) | buf[2]);
    raw[2] = (int16_t)((buf[5] << 8) | buf[4]);

    lsm6dsr_priv_t *priv = (lsm6dsr_priv_t *)sensor->priv_data;
    int32_t scale = (priv->accel_range == 2) ? 61 : (priv->accel_range == 4) ? 122 : (priv->accel_range == 8) ? 244 : 488;

    data->type = SENSOR_TYPE_ACCELEROMETER;
    data->unit = SENSOR_UNIT_MILLI_G;
    data->value.val_3axis.x = (int32_t)raw[0] * scale / 1000;
    data->value.val_3axis.y = (int32_t)raw[1] * scale / 1000;
    data->value.val_3axis.z = (int32_t)raw[2] * scale / 1000;
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 95;
    return SENSOR_EOK;
}

static sensor_err_t lsm6dsr_gyro_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint8_t buf[6];
    for (int i = 0; i < 6; i++) {
        if (lsm6dsr_reg_read(sensor, LSM6DSR_REG_OUTX_L_G + i, &buf[i]) != SENSOR_EOK) return SENSOR_EIO;
    }

    int16_t raw[3];
    raw[0] = (int16_t)((buf[1] << 8) | buf[0]);
    raw[1] = (int16_t)((buf[3] << 8) | buf[2]);
    raw[2] = (int16_t)((buf[5] << 8) | buf[4]);

    lsm6dsr_priv_t *priv = (lsm6dsr_priv_t *)sensor->priv_data;
    int32_t scale = (priv->gyro_range == 125) ? 4 : (priv->gyro_range == 250) ? 9 : (priv->gyro_range == 500) ? 17 : (priv->gyro_range == 1000) ? 35 : 70;

    data->type = SENSOR_TYPE_GYROSCOPE;
    data->unit = SENSOR_UNIT_DEGREE_PER_SECOND;
    data->value.val_3axis.x = (int32_t)raw[0] * scale / 1000;
    data->value.val_3axis.y = (int32_t)raw[1] * scale / 1000;
    data->value.val_3axis.z = (int32_t)raw[2] * scale / 1000;
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 95;
    return SENSOR_EOK;
}

static const sensor_ops_t lsm6dsr_accel_ops = {
    .init = lsm6dsr_init, .deinit = lsm6dsr_deinit, .read = lsm6dsr_accel_read,
};

static const sensor_ops_t lsm6dsr_gyro_ops = {
    .init = lsm6dsr_init, .deinit = lsm6dsr_deinit, .read = lsm6dsr_gyro_read,
};

sensor_device_t *lsm6dsr_create_accel(const char *name, void *i2c_bus, uint8_t addr)
{
    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    lsm6dsr_priv_t *priv = (lsm6dsr_priv_t *)SENSOR_MALLOC(sizeof(lsm6dsr_priv_t));
    if (!sensor || !priv) { SENSOR_FREE(sensor); SENSOR_FREE(priv); return NULL; }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(lsm6dsr_priv_t));

    priv->i2c_addr = addr ? addr : LSM6DSR_ADDR_DEFAULT;
    priv->spi_cs = LSM6DSR_SPI_CS_NONE;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor = "STMicro";
    sensor->info.model = "LSM6DSR";
    sensor->info.version = 0x0100;
    sensor->info.type = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit = SENSOR_UNIT_MILLI_G;
    sensor->info.range_max = 2000;
    sensor->info.range_min = -2000;
    sensor->info.resolution = 16;
    sensor->info.max_odr = 8000;
    sensor->info.flags = SENSOR_FLAG_INT_SUPPORT | SENSOR_FLAG_CALIBRATION;

    sensor->ops = &lsm6dsr_accel_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 104;
    return sensor;
}

sensor_device_t *lsm6dsr_create_gyro(const char *name, void *i2c_bus, uint8_t addr)
{
    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    lsm6dsr_priv_t *priv = (lsm6dsr_priv_t *)SENSOR_MALLOC(sizeof(lsm6dsr_priv_t));
    if (!sensor || !priv) { SENSOR_FREE(sensor); SENSOR_FREE(priv); return NULL; }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(lsm6dsr_priv_t));

    priv->i2c_addr = addr ? addr : LSM6DSR_ADDR_DEFAULT;
    priv->spi_cs = LSM6DSR_SPI_CS_NONE;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor = "STMicro";
    sensor->info.model = "LSM6DSR";
    sensor->info.version = 0x0100;
    sensor->info.type = SENSOR_TYPE_GYROSCOPE;
    sensor->info.unit = SENSOR_UNIT_DEGREE_PER_SECOND;
    sensor->info.range_max = 250;
    sensor->info.range_min = -250;
    sensor->info.resolution = 16;
    sensor->info.max_odr = 8000;
    sensor->info.flags = SENSOR_FLAG_INT_SUPPORT | SENSOR_FLAG_CALIBRATION;

    sensor->ops = &lsm6dsr_gyro_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 104;
    return sensor;
}

sensor_device_t *lsm6dsr_create_imu(const char *name, void *i2c_bus, uint8_t addr)
{
    return lsm6dsr_create_accel(name, i2c_bus, addr);
}

sensor_device_t *lsm6dsr_create_spi_accel(const char *name, void *spi_bus, uint8_t cs)
{
    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    lsm6dsr_priv_t *priv = (lsm6dsr_priv_t *)SENSOR_MALLOC(sizeof(lsm6dsr_priv_t));
    if (!sensor || !priv) { SENSOR_FREE(sensor); SENSOR_FREE(priv); return NULL; }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(lsm6dsr_priv_t));
    priv->spi_cs = cs;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor = "STMicro";
    sensor->info.model = "LSM6DSR";
    sensor->info.version = 0x0100;
    sensor->info.type = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit = SENSOR_UNIT_MILLI_G;
    sensor->info.range_max = 2000;
    sensor->info.range_min = -2000;
    sensor->info.resolution = 16;
    sensor->info.max_odr = 8000;
    sensor->info.flags = SENSOR_FLAG_INT_SUPPORT | SENSOR_FLAG_CALIBRATION;

    sensor->ops = &lsm6dsr_accel_ops;
    sensor->bus = spi_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 104;
    return sensor;
}

sensor_device_t *lsm6dsr_create_spi_gyro(const char *name, void *spi_bus, uint8_t cs)
{
    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    lsm6dsr_priv_t *priv = (lsm6dsr_priv_t *)SENSOR_MALLOC(sizeof(lsm6dsr_priv_t));
    if (!sensor || !priv) { SENSOR_FREE(sensor); SENSOR_FREE(priv); return NULL; }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(lsm6dsr_priv_t));
    priv->spi_cs = cs;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor = "STMicro";
    sensor->info.model = "LSM6DSR";
    sensor->info.version = 0x0100;
    sensor->info.type = SENSOR_TYPE_GYROSCOPE;
    sensor->info.unit = SENSOR_UNIT_DEGREE_PER_SECOND;
    sensor->info.range_max = 250;
    sensor->info.range_min = -250;
    sensor->info.resolution = 16;
    sensor->info.max_odr = 8000;
    sensor->info.flags = SENSOR_FLAG_INT_SUPPORT | SENSOR_FLAG_CALIBRATION;

    sensor->ops = &lsm6dsr_gyro_ops;
    sensor->bus = spi_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 104;
    return sensor;
}

sensor_device_t *lsm6dsr_create_spi_imu(const char *name, void *spi_bus, uint8_t cs)
{
    return lsm6dsr_create_spi_accel(name, spi_bus, cs);
}

int lsm6dsr_set_accel_range(sensor_device_t *dev, uint8_t range)
{
    lsm6dsr_priv_t *priv = (lsm6dsr_priv_t *)dev->priv_data;
    uint8_t ctrl1;
    lsm6dsr_reg_read(dev, LSM6DSR_REG_CTRL1_XL, &ctrl1);
    ctrl1 = (ctrl1 & 0x0F) | (range << 4);
    lsm6dsr_reg_write(dev, LSM6DSR_REG_CTRL1_XL, ctrl1);
    priv->accel_range = range;
    return 0;
}

int lsm6dsr_set_gyro_range(sensor_device_t *dev, uint8_t range)
{
    lsm6dsr_priv_t *priv = (lsm6dsr_priv_t *)dev->priv_data;
    uint8_t ctrl2;
    lsm6dsr_reg_read(dev, LSM6DSR_REG_CTRL2_G, &ctrl2);
    ctrl2 = (ctrl2 & 0x0F) | (range << 4);
    lsm6dsr_reg_write(dev, LSM6DSR_REG_CTRL2_G, ctrl2);
    priv->gyro_range = range;
    return 0;
}

int lsm6dsr_set_accel_rate(sensor_device_t *dev, uint8_t rate)
{
    lsm6dsr_priv_t *priv = (lsm6dsr_priv_t *)dev->priv_data;
    uint8_t ctrl1;
    lsm6dsr_reg_read(dev, LSM6DSR_REG_CTRL1_XL, &ctrl1);
    ctrl1 = (ctrl1 & 0xF0) | (rate & 0x0F);
    lsm6dsr_reg_write(dev, LSM6DSR_REG_CTRL1_XL, ctrl1);
    priv->accel_rate = rate;
    return 0;
}

int lsm6dsr_set_gyro_rate(sensor_device_t *dev, uint8_t rate)
{
    lsm6dsr_priv_t *priv = (lsm6dsr_priv_t *)dev->priv_data;
    uint8_t ctrl2;
    lsm6dsr_reg_read(dev, LSM6DSR_REG_CTRL2_G, &ctrl2);
    ctrl2 = (ctrl2 & 0xF0) | (rate & 0x0F);
    lsm6dsr_reg_write(dev, LSM6DSR_REG_CTRL2_G, ctrl2);
    priv->gyro_rate = rate;
    return 0;
}
