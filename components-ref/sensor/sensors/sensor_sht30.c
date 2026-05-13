/**
 * @file sensor_sht30.c
 * @brief Sensirion SHT30 温湿度传感器驱动
 */
#include "sensor_sht30.h"
#include <string.h>

extern int hal_i2c_master_send(void *bus, uint8_t addr, uint8_t *data, uint16_t len);
extern int hal_i2c_master_recv(void *bus, uint8_t addr, uint8_t *data, uint16_t len);

static sensor_err_t sht30_init(sensor_device_t *sensor)
{
    sht30_priv_t *priv = (sht30_priv_t *)sensor->priv_data;
    SENSOR_LOG("Initializing SHT30");
    uint8_t cmd[2] = {0x30, 0xA2};
    hal_i2c_master_send(sensor->bus, priv->i2c_addr, cmd, 2);
    SENSOR_LOG("SHT30 initialized");
    return SENSOR_EOK;
}

static sensor_err_t sht30_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint8_t buf[6];
    sht30_priv_t *priv = (sht30_priv_t *)sensor->priv_data;
    uint8_t cmd[2] = {0x24, 0x00};
    hal_i2c_master_send(sensor->bus, priv->i2c_addr, cmd, 2);
    SENSOR_DELAY_MS(10);
    if (hal_i2c_master_recv(sensor->bus, priv->i2c_addr, buf, 6) != SENSOR_EOK) return SENSOR_EIO;

    uint16_t temp = (buf[0] << 8) | buf[1];
    uint16_t hum = (buf[3] << 8) | buf[4];

    data->type = SENSOR_TYPE_RELATIVE_HUMIDITY;
    data->unit = SENSOR_UNIT_PERCENT;
    data->value.val_float = (float)hum / 65535.0f * 100.0f;
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy = 95;
    return SENSOR_EOK;
}

static const sensor_ops_t sht30_ops = {
    .init = sht30_init, .deinit = NULL, .read = sht30_read,
};

sensor_device_t *sht30_create(const char *name, void *i2c_bus, uint8_t addr)
{
    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    sht30_priv_t *priv = (sht30_priv_t *)SENSOR_MALLOC(sizeof(sht30_priv_t));
    if (!sensor || !priv) { SENSOR_FREE(sensor); SENSOR_FREE(priv); return NULL; }

    memset(sensor, 0, sizeof(sensor_device_t));
    priv->i2c_addr = addr ? addr : SHT30_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor = "Sensirion";
    sensor->info.model = "SHT30";
    sensor->info.type = SENSOR_TYPE_RELATIVE_HUMIDITY;
    sensor->info.max_odr = 10;

    sensor->ops = &sht30_ops;
    sensor->bus = i2c_bus;
    sensor->priv_data = priv;
    sensor->status = SENSOR_STATUS_IDLE;
    sensor->odr = 10;
    return sensor;
}
