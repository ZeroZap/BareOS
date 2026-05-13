/**
 * @file sensor_bmp390.c
 * @brief Bosch BMP390 气压计驱动
 */
#include "sensor_bmp390.h"
#include <string.h>

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

static sensor_err_t bmp390_init(sensor_device_t *sensor)
{
    uint8_t data;
    bmp390_priv_t *priv = (bmp390_priv_t *)sensor->priv_data;
    SENSOR_LOG("Initializing BMP390");

    if (hal_i2c_mem_read(sensor->bus, priv->i2c_addr, 0x00, &data, 1) != SENSOR_EOK) return SENSOR_EIO;
    if (data != 0x50) { SENSOR_LOG("Wrong CHIP_ID: 0x%02X", data); return SENSOR_ERROR; }

    hal_i2c_mem_write(sensor->bus, priv->i2c_addr, 0x1A, (uint8_t[]){0x33}, 1);
    SENSOR_LOG("BMP390 initialized");
    return SENSOR_EOK;
}

static sensor_err_t bmp390_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint8_t buf[6];
    bmp390_priv_t *priv = (bmp390_priv_t *)sensor->priv_data;
    for (int i = 0; i < 6; i++) {
        if (hal_i2c_mem_read(sensor->bus, priv->i2c_addr, 0x1C + i, &buf[i], 1) != SENSOR_EOK) return SENSOR_EIO;
    }

    uint32_t press = ((uint32_t)buf[2] << 16) | ((uint32_t)buf[1] << 8) | buf[0];
    data->type = SENSOR_TYPE_PRESSURE;
    data->unit = SENSOR_UNIT_HECTOPASCAL;
    data->value.val_float = (float)press / 256.0f;
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 95;
    return SENSOR_EOK;
}

static const sensor_ops_t bmp390_ops = {
    .init = bmp390_init, .deinit = NULL, .read = bmp390_read,
};

sensor_device_t *bmp390_create(const char *name, void *i2c_bus, uint8_t addr)
{
    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    bmp390_priv_t *priv = (bmp390_priv_t *)SENSOR_MALLOC(sizeof(bmp390_priv_t));
    if (!sensor || !priv) { SENSOR_FREE(sensor); SENSOR_FREE(priv); return NULL; }

    memset(sensor, 0, sizeof(sensor_device_t));
    priv->i2c_addr = addr ? addr : BMP390_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor = "Bosch";
    sensor->info.model = "BMP390";
    sensor->info.type = SENSOR_TYPE_PRESSURE;
    sensor->info.max_odr = 200;

    sensor->ops = &bmp390_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 100;
    return sensor;
}
