/**
 * @file sensor_vl53l1x.c
 * @brief ST VL53L1X ToF 激光测距传感器驱动
 */
#include "sensor_vl53l1x.h"
#include <string.h>

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

static sensor_err_t vl53l1x_init(sensor_device_t *sensor)
{
    vl53l1x_priv_t *priv = (vl53l1x_priv_t *)sensor->priv_data;
    SENSOR_LOG("Initializing VL53L1X");
    uint8_t data = 0x00;
    hal_i2c_mem_write(sensor->bus, priv->i2c_addr, 0x2D, &data, 1);
    SENSOR_DELAY_MS(100);
    data = 0x01;
    hal_i2c_mem_write(sensor->bus, priv->i2c_addr, 0x2D, &data, 1);
    SENSOR_LOG("VL53L1X initialized");
    return SENSOR_EOK;
}

static sensor_err_t vl53l1x_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint8_t buf[2];
    vl53l1x_priv_t *priv = (vl53l1x_priv_t *)sensor->priv_data;
    if (hal_i2c_mem_read(sensor->bus, priv->i2c_addr, 0x6E, buf, 2) != SENSOR_EOK) return SENSOR_EIO;

    uint16_t dist = (buf[0] << 8) | buf[1];
    data->type = SENSOR_TYPE_PROXIMITY;
    data->unit = SENSOR_UNIT_MILLIMETER;
    data->value.val_float = (float)dist;
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 90;
    return SENSOR_EOK;
}

static const sensor_ops_t vl53l1x_ops = {
    .init = vl53l1x_init, .deinit = NULL, .read = vl53l1x_read,
};

sensor_device_t *vl53l1x_create(const char *name, void *i2c_bus, uint8_t addr)
{
    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    vl53l1x_priv_t *priv = (vl53l1x_priv_t *)SENSOR_MALLOC(sizeof(vl53l1x_priv_t));
    if (!sensor || !priv) { SENSOR_FREE(sensor); SENSOR_FREE(priv); return NULL; }

    memset(sensor, 0, sizeof(sensor_device_t));
    priv->i2c_addr = addr ? addr : VL53L1X_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor = "STMicro";
    sensor->info.model = "VL53L1X";
    sensor->info.type = SENSOR_TYPE_PROXIMITY;
    sensor->info.max_odr = 100;

    sensor->ops = &vl53l1x_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 50;
    return sensor;
}
