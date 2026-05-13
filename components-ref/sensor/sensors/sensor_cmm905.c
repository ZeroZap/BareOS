/**
 * @file sensor_cmm905.c
 * @brief CMMLab MM905 3轴磁力计驱动实现
 */
#include "sensor_cmm905.h"
#include <string.h>

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

static sensor_err_t cmm905_init(sensor_device_t *sensor)
{
    SENSOR_LOG("Initializing CMMLab MM905");
    return SENSOR_EOK;
}

static sensor_err_t cmm905_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint8_t buf[6];
    cmm905_priv_t *priv = (cmm905_priv_t *)sensor->priv_data;
    for (int i = 0; i < 6; i++) {
        if (hal_i2c_mem_read(sensor->bus, priv->i2c_addr, CMM905_REG_DATA + i, &buf[i], 1) != SENSOR_EOK) return SENSOR_EIO;
    }

    int16_t raw[3];
    raw[0] = (int16_t)((buf[1] << 8) | buf[0]);
    raw[1] = (int16_t)((buf[3] << 8) | buf[2]);
    raw[2] = (int16_t)((buf[5] << 8) | buf[4]);

    data->type = SENSOR_TYPE_MAGNETIC;
    data->unit = SENSOR_UNIT_MICRO_TESLA;
    data->value.val_3axis.x = (int32_t)raw[0] * 10;
    data->value.val_3axis.y = (int32_t)raw[1] * 10;
    data->value.val_3axis.z = (int32_t)raw[2] * 10;
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 80;
    return SENSOR_EOK;
}

static const sensor_ops_t cmm905_ops = {
    .init = cmm905_init, .deinit = NULL, .read = cmm905_read,
};

sensor_device_t *cmm905_create(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    cmm905_priv_t *priv = (cmm905_priv_t *)SENSOR_MALLOC(sizeof(cmm905_priv_t));
    if (!sensor || !priv) { SENSOR_FREE(sensor); SENSOR_FREE(priv); return NULL; }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(cmm905_priv_t));
    priv->i2c_addr = CMM905_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor = "CMMLab";
    sensor->info.model = "MM905";
    sensor->info.version = 0x0100;
    sensor->info.type = SENSOR_TYPE_MAGNETIC;
    sensor->info.unit = SENSOR_UNIT_MICRO_TESLA;
    sensor->info.range_max = 4900;
    sensor->info.range_min = -4900;
    sensor->info.resolution = 14;
    sensor->info.max_odr = 100;
    sensor->info.flags = 0;

    sensor->ops = &cmm905_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 100;
    return sensor;
}
