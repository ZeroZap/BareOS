/**
 * @file sensor_lsm9ds1.c
 * @brief ST LSM9DS1 9轴惯性传感器驱动实现
 */
#include "sensor_lsm9ds1.h"
#include <string.h>

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

/* IMU读写 */
static sensor_err_t lsm9ds1_imu_read(sensor_device_t *sensor, uint8_t reg, uint8_t *data)
{
    lsm9ds1_priv_t *priv = (lsm9ds1_priv_t *)sensor->priv_data;
    return hal_i2c_mem_read(sensor->bus, priv->imu_addr, reg, data, 1);
}

static sensor_err_t lsm9ds1_imu_write(sensor_device_t *sensor, uint8_t reg, uint8_t data)
{
    lsm9ds1_priv_t *priv = (lsm9ds1_priv_t *)sensor->priv_data;
    return hal_i2c_mem_write(sensor->bus, priv->imu_addr, reg, &data, 1);
}

/* Magnetometer读写 */
static sensor_err_t lsm9ds1_mag_read_reg(sensor_device_t *sensor, uint8_t reg, uint8_t *data)
{
    lsm9ds1_priv_t *priv = (lsm9ds1_priv_t *)sensor->priv_data;
    return hal_i2c_mem_read(sensor->bus, priv->mag_addr, reg, data, 1);
}

static sensor_err_t lsm9ds1_mag_write(sensor_device_t *sensor, uint8_t reg, uint8_t data)
{
    lsm9ds1_priv_t *priv = (lsm9ds1_priv_t *)sensor->priv_data;
    return hal_i2c_mem_write(sensor->bus, priv->mag_addr, reg, &data, 1);
}

static sensor_err_t lsm9ds1_init(sensor_device_t *sensor)
{
    uint8_t data;
    SENSOR_LOG("Initializing LSM9DS1");

    if (lsm9ds1_imu_read(sensor, LSM9DS1_REG_WHOAMI_IMU, &data) != SENSOR_EOK) return SENSOR_EIO;
    if (data != LSM9DS1_IMU_WHOAMI_VALUE) {
        SENSOR_LOG("Wrong IMU WHO_AM_I: 0x%02X", data);
        return SENSOR_ERROR;
    }

    if (lsm9ds1_mag_read(sensor, LSM9DS1_REG_WHOAMI_MAG, &data) != SENSOR_EOK) return SENSOR_EIO;
    if (data != LSM9DS1_MAG_WHOAMI_VALUE) {
        SENSOR_LOG("Wrong MAG WHO_AM_I: 0x%02X", data);
        return SENSOR_ERROR;
    }

    /* IMU软复位 */
    lsm9ds1_imu_write(sensor, LSM9DS1_REG_CTRL3_C, 0x01);
    SENSOR_DELAY_MS(10);

    /* 配置加速度计: ±2g, 100Hz */
    data = (0x04 << 4) | LSM9DS1_ACCEL_RANGE_2G;
    lsm9ds1_imu_write(sensor, LSM9DS1_REG_CTRL1_XL, data);

    /* 配置陀螺仪: ±250°/s, 100Hz */
    data = (0x04 << 4) | LSM9DS1_GYRO_RANGE_250DPS;
    lsm9ds1_imu_write(sensor, LSM9DS1_REG_CTRL2_G, data);

    /* 配置磁力计: 100Hz, 高性能 */
    lsm9ds1_mag_write(sensor, LSM9DS1_REG_CTRL_REG1_M, 0x70);

    SENSOR_LOG("LSM9DS1 initialized");
    return SENSOR_EOK;
}

static sensor_err_t lsm9ds1_deinit(sensor_device_t *sensor)
{
    lsm9ds1_imu_write(sensor, LSM9DS1_REG_CTRL1_XL, 0x00);
    lsm9ds1_imu_write(sensor, LSM9DS1_REG_CTRL2_G, 0x00);
    lsm9ds1_mag_write(sensor, LSM9DS1_REG_CTRL_REG1_M, 0x00);
    return SENSOR_EOK;
}

static sensor_err_t lsm9ds1_accel_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint8_t buf[6];
    for (int i = 0; i < 6; i++) {
        if (lsm9ds1_imu_read(sensor, LSM9DS1_REG_OUTX_L_XL + i, &buf[i]) != SENSOR_EOK) return SENSOR_EIO;
    }

    int16_t raw[3];
    raw[0] = (int16_t)((buf[1] << 8) | buf[0]);
    raw[1] = (int16_t)((buf[3] << 8) | buf[2]);
    raw[2] = (int16_t)((buf[5] << 8) | buf[4]);

    lsm9ds1_priv_t *priv = (lsm9ds1_priv_t *)sensor->priv_data;
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

static sensor_err_t lsm9ds1_gyro_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint8_t buf[6];
    for (int i = 0; i < 6; i++) {
        if (lsm9ds1_imu_read(sensor, LSM9DS1_REG_OUTX_L_G + i, &buf[i]) != SENSOR_EOK) return SENSOR_EIO;
    }

    int16_t raw[3];
    raw[0] = (int16_t)((buf[1] << 8) | buf[0]);
    raw[1] = (int16_t)((buf[3] << 8) | buf[2]);
    raw[2] = (int16_t)((buf[5] << 8) | buf[4]);

    lsm9ds1_priv_t *priv = (lsm9ds1_priv_t *)sensor->priv_data;
    int32_t scale = (priv->gyro_range == 250) ? 9 : (priv->gyro_range == 500) ? 17 : (priv->gyro_range == 1000) ? 35 : 70;

    data->type = SENSOR_TYPE_GYROSCOPE;
    data->unit = SENSOR_UNIT_DEGREE_PER_SECOND;
    data->value.val_3axis.x = (int32_t)raw[0] * scale / 1000;
    data->value.val_3axis.y = (int32_t)raw[1] * scale / 1000;
    data->value.val_3axis.z = (int32_t)raw[2] * scale / 1000;
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 95;
    return SENSOR_EOK;
}

static sensor_err_t lsm9ds1_mag_read_data(sensor_device_t *sensor, sensor_data_t *data)
{
    uint8_t buf[6];
    for (int i = 0; i < 6; i++) {
        if (lsm9ds1_mag_read_reg(sensor, LSM9DS1_REG_OUTX_L_M + i, &buf[i]) != SENSOR_EOK) return SENSOR_EIO;
    }

    int16_t raw[3];
    raw[0] = (int16_t)((buf[1] << 8) | buf[0]);
    raw[1] = (int16_t)((buf[3] << 8) | buf[2]);
    raw[2] = (int16_t)((buf[5] << 8) | buf[4]);

    data->type = SENSOR_TYPE_MAGNETIC;
    data->unit = SENSOR_UNIT_MICRO_TESLA;
    data->value.val_3axis.x = (int32_t)raw[0] * 15;  /* 15 mgauss/LSB */
    data->value.val_3axis.y = (int32_t)raw[1] * 15;
    data->value.val_3axis.z = (int32_t)raw[2] * 15;
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 95;
    return SENSOR_EOK;
}

static const sensor_ops_t lsm9ds1_accel_ops = {
    .init = lsm9ds1_init, .deinit = lsm9ds1_deinit, .read = lsm9ds1_accel_read,
};

static const sensor_ops_t lsm9ds1_gyro_ops = {
    .init = lsm9ds1_init, .deinit = lsm9ds1_deinit, .read = lsm9ds1_gyro_read,
};

static const sensor_ops_t lsm9ds1_mag_ops = {
    .init = lsm9ds1_init, .deinit = lsm9ds1_deinit, .read = lsm9ds1_mag_read_data,
};

sensor_device_t *lsm9ds1_create_accel(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    lsm9ds1_priv_t *priv = (lsm9ds1_priv_t *)SENSOR_MALLOC(sizeof(lsm9ds1_priv_t));
    if (!sensor || !priv) { SENSOR_FREE(sensor); SENSOR_FREE(priv); return NULL; }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(lsm9ds1_priv_t));
    priv->imu_addr = LSM9DS1_IMU_ADDR_DEFAULT;
    priv->mag_addr = LSM9DS1_MAG_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor = "STMicro";
    sensor->info.model = "LSM9DS1";
    sensor->info.version = 0x0100;
    sensor->info.type = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit = SENSOR_UNIT_MILLI_G;
    sensor->info.range_max = 2000;
    sensor->info.range_min = -2000;
    sensor->info.resolution = 16;
    sensor->info.max_odr = 1000;
    sensor->info.flags = SENSOR_FLAG_INT_SUPPORT;

    sensor->ops = &lsm9ds1_accel_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 100;
    return sensor;
}

sensor_device_t *lsm9ds1_create_gyro(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    lsm9ds1_priv_t *priv = (lsm9ds1_priv_t *)SENSOR_MALLOC(sizeof(lsm9ds1_priv_t));
    if (!sensor || !priv) { SENSOR_FREE(sensor); SENSOR_FREE(priv); return NULL; }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(lsm9ds1_priv_t));
    priv->imu_addr = LSM9DS1_IMU_ADDR_DEFAULT;
    priv->mag_addr = LSM9DS1_MAG_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor = "STMicro";
    sensor->info.model = "LSM9DS1";
    sensor->info.version = 0x0100;
    sensor->info.type = SENSOR_TYPE_GYROSCOPE;
    sensor->info.unit = SENSOR_UNIT_DEGREE_PER_SECOND;
    sensor->info.range_max = 250;
    sensor->info.range_min = -250;
    sensor->info.resolution = 16;
    sensor->info.max_odr = 1000;
    sensor->info.flags = SENSOR_FLAG_INT_SUPPORT;

    sensor->ops = &lsm9ds1_gyro_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 100;
    return sensor;
}

sensor_device_t *lsm9ds1_create_mag(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    lsm9ds1_priv_t *priv = (lsm9ds1_priv_t *)SENSOR_MALLOC(sizeof(lsm9ds1_priv_t));
    if (!sensor || !priv) { SENSOR_FREE(sensor); SENSOR_FREE(priv); return NULL; }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(lsm9ds1_priv_t));
    priv->imu_addr = LSM9DS1_IMU_ADDR_DEFAULT;
    priv->mag_addr = LSM9DS1_MAG_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor = "STMicro";
    sensor->info.model = "LSM9DS1";
    sensor->info.version = 0x0100;
    sensor->info.type = SENSOR_TYPE_MAGNETIC;
    sensor->info.unit = SENSOR_UNIT_MICRO_TESLA;
    sensor->info.range_max = 4900;
    sensor->info.range_min = -4900;
    sensor->info.resolution = 16;
    sensor->info.max_odr = 100;
    sensor->info.flags = 0;

    sensor->ops = &lsm9ds1_mag_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 100;
    return sensor;
}
