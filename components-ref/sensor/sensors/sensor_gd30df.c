/**
 * @file sensor_gd30df.c
 * @brief 兆易创新 GD30DF 3轴加速度计驱动
 */
#include "sensor_gd30df.h"
#include <string.h>

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

static sensor_err_t gd30df_init(sensor_device_t *sensor)
{
    uint8_t data; gd30df_priv_t *priv = (gd30df_priv_t *)sensor->priv_data;
    SENSOR_LOG("Initializing GD30DF");
    if (hal_i2c_mem_read(sensor->bus, priv->i2c_addr, 0x0F, &data, 1) != SENSOR_EOK) return SENSOR_EIO;
    hal_i2c_mem_write(sensor->bus, priv->i2c_addr, 0x20, (uint8_t[]){0x57}, 1);
    return SENSOR_EOK;
}

static sensor_err_t gd30df_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint8_t buf[6]; gd30df_priv_t *priv = (gd30df_priv_t *)sensor->priv_data;
    for (int i = 0; i < 6; i++) hal_i2c_mem_read(sensor->bus, priv->i2c_addr, 0x28 + i, &buf[i], 1);
    int16_t raw[3] = { (int16_t)((buf[1]<<8)|buf[0]), (int16_t)((buf[3]<<8)|buf[2]), (int16_t)((buf[5]<<8)|buf[4]) };
    data->type = SENSOR_TYPE_ACCELEROMETER; data->unit = SENSOR_UNIT_MILLI_G;
    data->value.val_3axis.x = raw[0]; data->value.val_3axis.y = raw[1]; data->value.val_3axis.z = raw[2];
    data->timestamp = SENSOR_GET_TICK(); data->accuracy = 90;
    return SENSOR_EOK;
}

static const sensor_ops_t gd30df_ops = { .init = gd30df_init, .read = gd30df_read, };

sensor_device_t *gd30df_create(const char *name, void *i2c_bus, uint8_t addr)
{
    sensor_device_t *s = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    gd30df_priv_t *p = (gd30df_priv_t *)SENSOR_MALLOC(sizeof(gd30df_priv_t));
    if (!s || !p) { SENSOR_FREE(s); SENSOR_FREE(p); return NULL; }
    memset(s, 0, sizeof(sensor_device_t)); p->i2c_addr = addr ? addr : 0x18;
    strncpy(s->info.name, name, SENSOR_NAME_MAX_LEN-1);
    s->info.vendor = "GigaDevice"; s->info.model = "GD30DF"; s->info.type = SENSOR_TYPE_ACCELEROMETER;
    s->ops = &gd30df_ops; s->bus = i2c_bus; s->priv_data = p; s->status = SENSOR_STATUS_IDLE;
    return s;
}
