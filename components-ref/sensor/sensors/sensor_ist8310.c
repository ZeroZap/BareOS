#include "sensor_ist8310.h"
#include <string.h>

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

static sensor_err_t ist8310_init(sensor_device_t *sensor)
{
    uint8_t data;
    ist8310_priv_t *priv = (ist8310_priv_t *)sensor->priv_data;
    SENSOR_LOG("Initializing IST8310");

    if (hal_i2c_mem_read(sensor->bus, priv->i2c_addr, IST8310_REG_WHOAMI, &data, 1) != SENSOR_EOK) return SENSOR_EIO;
    if (data != IST8310_WHOAMI_VALUE) {
        SENSOR_LOG("Wrong WHO_AM_I: 0x%02X", data);
        return SENSOR_ERROR;
    }

    hal_i2c_mem_write(sensor->bus, priv->i2c_addr, IST8310_REG_CTRL1, (uint8_t[]){0x1A}, 1);
    hal_i2c_mem_write(sensor->bus, priv->i2c_addr, IST8310_REG_CTRL2, (uint8_t[]){0x40}, 1);
    SENSOR_LOG("IST8310 initialized");
    return SENSOR_EOK;
}

static sensor_err_t ist8310_deinit(sensor_device_t *sensor)
{
    ist8310_priv_t *priv = (ist8310_priv_t *)sensor->priv_data;
    hal_i2c_mem_write(sensor->bus, priv->i2c_addr, IST8310_REG_CTRL1, (uint8_t[]){0x00}, 1);
    return SENSOR_EOK;
}

static sensor_err_t ist8310_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint8_t buf[6];
    ist8310_priv_t *priv = (ist8310_priv_t *)sensor->priv_data;
    for (int i = 0; i < 6; i++) {
        if (hal_i2c_mem_read(sensor->bus, priv->i2c_addr, IST8310_REG_DATA + i, &buf[i], 1) != SENSOR_EOK) return SENSOR_EIO;
    }

    int16_t raw[3];
    raw[0] = (int16_t)((buf[1] << 8) | buf[0]);
    raw[1] = (int16_t)((buf[3] << 8) | buf[2]);
    raw[2] = (int16_t)((buf[5] << 8) | buf[4]);

    data->type = SENSOR_TYPE_MAGNETIC;
    data->unit = SENSOR_UNIT_MICRO_TESLA;
    data->value.val_3axis.x = (int32_t)raw[0] * 15;
    data->value.val_3axis.y = (int32_t)raw[1] * 15;
    data->value.val_3axis.z = (int32_t)raw[2] * 15;
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 85;
    return SENSOR_EOK;
}

static const sensor_ops_t ist8310_ops = {
    .init = ist8310_init, .deinit = ist8310_deinit, .read = ist8310_read,
};

sensor_device_t *ist8310_create(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    ist8310_priv_t *priv = (ist8310_priv_t *)SENSOR_MALLOC(sizeof(ist8310_priv_t));
    if (!sensor || !priv) { SENSOR_FREE(sensor); SENSOR_FREE(priv); return NULL; }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(ist8310_priv_t));
    priv->i2c_addr = IST8310_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor = "iSentek";
    sensor->info.model = "IST8310";
    sensor->info.version = 0x0100;
    sensor->info.type = SENSOR_TYPE_MAGNETIC;
    sensor->info.unit = SENSOR_UNIT_MICRO_TESLA;
    sensor->info.range_max = 4900;
    sensor->info.range_min = -4900;
    sensor->info.resolution = 16;
    sensor->info.max_odr = 100;
    sensor->info.flags = 0;

    sensor->ops = &ist8310_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 100;
    return sensor;
}
