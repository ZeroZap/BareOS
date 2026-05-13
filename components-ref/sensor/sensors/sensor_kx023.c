#include "sensor_kx023.h"

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data,
                            uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg,
                             uint8_t *data, uint16_t len);

static sensor_err_t kx023_init(sensor_device_t *sensor)
{
    kx023_priv_t *priv = (kx023_priv_t *)sensor->priv_data;
    uint8_t data;

    SENSOR_LOG("Initializing KX023");

    /* 检查WHO_AM_I */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, KX023_REG_WHO_AM_I, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }

    if (data != KX023_WHO_AM_I_VALUE) {
        SENSOR_LOG("Wrong WHO_AM_I: 0x%02X", data);
        return SENSOR_ERROR;
    }

    /* 软复位 */
    data = 0x80;
    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, KX023_REG_SOFT_REST, &data, 1);
    SENSOR_DELAY_MS(10);

    /* 配置CNTL1: 待机模式, ±2g */
    data = 0x00;
    hal_i2c_mem_write(sensor->bus, priv->i2c_addr, KX023_REG_CNTL1, &data, 1);

    /* 配置ODCNTL: 12.5Hz */
    data = KX023_ODR_12_5HZ;
    hal_i2c_mem_write(sensor->bus, priv->i2c_addr, KX023_REG_ODCNTL, &data, 1);
    priv->odr = KX023_ODR_12_5HZ;

    /* 启动低功耗模式 */
    data = KX023_MODE_LOW_POWER;
    hal_i2c_mem_write(sensor->bus, priv->i2c_addr, KX023_REG_CNTL1, &data, 1);
    priv->mode = KX023_MODE_LOW_POWER;

    SENSOR_LOG("KX023 initialized (Ultra Low Power: 0.9μA @ 0.781Hz)");

    return SENSOR_EOK;
}

static sensor_err_t kx023_deinit(sensor_device_t *sensor)
{
    kx023_priv_t *priv = (kx023_priv_t *)sensor->priv_data;
    uint8_t data       = KX023_MODE_STANDBY;

    hal_i2c_mem_write(sensor->bus, priv->i2c_addr, KX023_REG_CNTL1, &data, 1);

    return SENSOR_EOK;
}

static sensor_err_t kx023_read(sensor_device_t *sensor, sensor_data_t *data)
{
    kx023_priv_t *priv = (kx023_priv_t *)sensor->priv_data;
    uint8_t buf[6];
    int16_t raw[3];

    /* 读取6字节加速度数据 */
    if (hal_i2c_mem_read(sensor->bus, priv->i2c_addr, KX023_REG_XOUT_L, buf, 6)
        != 0) {
        return SENSOR_EIO;
    }

    /* 组合数据 */
    raw[0] = (int16_t)((buf[1] << 8) | buf[0]);
    raw[1] = (int16_t)((buf[3] << 8) | buf[2]);
    raw[2] = (int16_t)((buf[5] << 8) | buf[4]);

    /* 转换为mg (±2g范围) */
    data->type              = SENSOR_TYPE_ACCELEROMETER;
    data->unit              = SENSOR_UNIT_MILLI_G;
    data->value.val_3axis.x = (int32_t)raw[0] * 2000 / 32768;
    data->value.val_3axis.y = (int32_t)raw[1] * 2000 / 32768;
    data->value.val_3axis.z = (int32_t)raw[2] * 2000 / 32768;
    data->timestamp         = SENSOR_GET_TICK();
    data->accuracy          = 92;

    return SENSOR_EOK;
}

static const sensor_ops_t kx023_ops = {
    .init   = kx023_init,
    .deinit = kx023_deinit,
    .read   = kx023_read,
};

sensor_device_t *kx023_create(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL)
        return NULL;

    kx023_priv_t *priv = (kx023_priv_t *)SENSOR_MALLOC(sizeof(kx023_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(kx023_priv_t));

    priv->i2c_addr = KX023_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "Kionix";
    sensor->info.model      = "KX023";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit       = SENSOR_UNIT_MILLI_G;
    sensor->info.range_max  = 2000;
    sensor->info.range_min  = -2000;
    sensor->info.resolution = 16;
    sensor->info.max_odr    = 100;
    sensor->info.flags      = SENSOR_FLAG_LOW_POWER | SENSOR_FLAG_INT_SUPPORT;

    sensor->ops       = &kx023_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 12;

    return sensor;
}