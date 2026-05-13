#include "sensor_adt7420.h"

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data,
                            uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg,
                             uint8_t *data, uint16_t len);

static sensor_err_t adt7420_init(sensor_device_t *sensor)
{
    adt7420_priv_t *priv = (adt7420_priv_t *)sensor->priv_data;
    uint8_t data;

    SENSOR_LOG("Initializing ADT7420");

    /* 读取ID验证 */
    if (hal_i2c_mem_read(sensor->bus, priv->i2c_addr, ADT7420_REG_ID, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }

    /* 配置16位分辨率 */
    data = ADT7420_CONFIG_RES_16BIT;
    if (hal_i2c_mem_write(
            sensor->bus, priv->i2c_addr, ADT7420_REG_CONFIG, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }

    priv->resolution = 16;

    SENSOR_LOG("ADT7420 initialized (16-bit mode)");

    return SENSOR_EOK;
}

static sensor_err_t adt7420_deinit(sensor_device_t *sensor)
{
    return SENSOR_EOK;
}

static sensor_err_t adt7420_read(sensor_device_t *sensor, sensor_data_t *data)
{
    adt7420_priv_t *priv = (adt7420_priv_t *)sensor->priv_data;
    uint8_t buf[2];
    int16_t raw;

    /* 读取温度数据 */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, ADT7420_REG_TEMP_MSB, buf, 2)
        != 0) {
        return SENSOR_EIO;
    }

    raw = (int16_t)((buf[0] << 8) | buf[1]);

    data->type = SENSOR_TYPE_TEMPERATURE;
    data->unit = SENSOR_UNIT_CELSIUS;

#if SENSOR_USE_FLOAT
    /* 16位: 0.0078125°C/LSB */
    data->value.val_float = raw / 128.0f;
#else
    /* 转换为0.01°C */
    data->value.val_int32 = ((int64_t)raw * 100) / 128;
#endif

    data->timestamp = SENSOR_GET_TICK();
    data->accuracy  = 99; /* ±0.25°C精度 */

    return SENSOR_EOK;
}

static const sensor_ops_t adt7420_ops = {
    .init   = adt7420_init,
    .deinit = adt7420_deinit,
    .read   = adt7420_read,
};

sensor_device_t *adt7420_create(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL)
        return NULL;

    adt7420_priv_t *priv =
        (adt7420_priv_t *)SENSOR_MALLOC(sizeof(adt7420_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(adt7420_priv_t));

    priv->i2c_addr = ADT7420_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "Analog Devices";
    sensor->info.model      = "ADT7420";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_TEMPERATURE;
    sensor->info.unit       = SENSOR_UNIT_CELSIUS;
    sensor->info.range_max  = 150;
    sensor->info.range_min  = -40;
    sensor->info.resolution = 16;
    sensor->info.max_odr    = 4;
    sensor->info.flags      = SENSOR_FLAG_HIGH_PRECISION;

    sensor->ops       = &adt7420_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;

    return sensor;
}