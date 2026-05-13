/**
 * @file sensor_aht10.c
 * @brief 国产 AHT10 温湿度传感器驱动
 */
#include "sensor_aht10.h"
#include <string.h>

extern int hal_i2c_master_send(void *bus, uint8_t addr, uint8_t *data, uint16_t len);
extern int hal_i2c_master_recv(void *bus, uint8_t addr, uint8_t *data, uint16_t len);

static sensor_err_t aht10_init(sensor_device_t *sensor)
{
    aht10_priv_t *priv = (aht10_priv_t *)sensor->priv_data;
    SENSOR_LOG("Initializing AHT10");
    uint8_t cmd[3] = {0xE1, 0x08, 0x00};
    hal_i2c_master_send(sensor->bus, priv->i2c_addr, cmd, 3);
    SENSOR_DELAY_MS(10);
    SENSOR_LOG("AHT10 initialized");
    return SENSOR_EOK;
}

static sensor_err_t aht10_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint8_t buf[6];
    aht10_priv_t *priv = (aht10_priv_t *)sensor->priv_data;
    uint8_t cmd[3] = {0xAC, 0x33, 0x00};
    hal_i2c_master_send(sensor->bus, priv->i2c_addr, cmd, 3);
    SENSOR_DELAY_MS(80);
    if (hal_i2c_master_recv(sensor->bus, priv->i2c_addr, buf, 6) != SENSOR_EOK) return SENSOR_EIO;

    uint32_t hum = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | (buf[3] >> 4);
    uint32_t temp = (((uint32_t)buf[3] & 0x0F) << 16) | ((uint32_t)buf[4] << 8) | buf[5];

    data->type = SENSOR_TYPE_RELATIVE_HUMIDITY;
    data->unit = SENSOR_UNIT_PERCENT;
    data->value.val_float = (float)hum / 1048576.0f * 100.0f;
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 90;
    return SENSOR_EOK;
}

static const sensor_ops_t aht10_ops = {
    .init = aht10_init, .deinit = NULL, .read = aht10_read,
};

sensor_device_t *aht10_create(const char *name, void *i2c_bus, uint8_t addr)
{
    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    aht10_priv_t *priv = (aht10_priv_t *)SENSOR_MALLOC(sizeof(aht10_priv_t));
    if (!sensor || !priv) { SENSOR_FREE(sensor); SENSOR_FREE(priv); return NULL; }

    memset(sensor, 0, sizeof(sensor_device_t));
    priv->i2c_addr = addr ? addr : AHT10_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor = "Aosong";
    sensor->info.model = "AHT10";
    sensor->info.type = SENSOR_TYPE_RELATIVE_HUMIDITY;
    sensor->info.max_odr = 10;

    sensor->ops = &aht10_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 10;
    return sensor;
}
