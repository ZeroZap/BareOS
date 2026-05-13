#include "sensor_apds9960.h"

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data,
                            uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg,
                             uint8_t *data, uint16_t len);

static sensor_err_t apds9960_init(sensor_device_t *sensor)
{
    apds9960_priv_t *priv = (apds9960_priv_t *)sensor->priv_data;
    uint8_t data;

    SENSOR_LOG("Initializing APDS9960");

    /* 读取ID */
    if (hal_i2c_mem_read(sensor->bus, priv->i2c_addr, APDS9960_REG_ID, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }

    if (data != 0xAB && data != 0x9C) {
        SENSOR_LOG("Wrong ID: 0x%02X", data);
        return SENSOR_ERROR;
    }

    /* 使能功能: PON, AEN, PEN, GEN */
    data = 0x4F;
    if (hal_i2c_mem_write(
            sensor->bus, priv->i2c_addr, APDS9960_REG_ENABLE, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }

    SENSOR_LOG("APDS9960 initialized");

    return SENSOR_EOK;
}

static sensor_err_t apds9960_deinit(sensor_device_t *sensor)
{
    apds9960_priv_t *priv = (apds9960_priv_t *)sensor->priv_data;
    uint8_t data          = 0x00;

    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, APDS9960_REG_ENABLE, &data, 1);

    return SENSOR_EOK;
}

/* RGB传感器读取 */
static sensor_err_t apds9960_rgb_read(sensor_device_t *sensor,
                                      sensor_data_t *data)
{
    apds9960_priv_t *priv = (apds9960_priv_t *)sensor->priv_data;
    uint8_t buf[8];

    /* 读取RGBC数据 */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, APDS9960_REG_CDATAL, buf, 8)
        != 0) {
        return SENSOR_EIO;
    }

    apds9960_rgb_t rgb;
    rgb.c = (buf[1] << 8) | buf[0];
    rgb.r = (buf[3] << 8) | buf[2];
    rgb.g = (buf[5] << 8) | buf[4];
    rgb.b = (buf[7] << 8) | buf[6];

    data->type = SENSOR_TYPE_RGB;
    data->unit = SENSOR_UNIT_NONE;

    /* 存储在val_bytes中 */
    memcpy(data->value.val_bytes, &rgb, sizeof(apds9960_rgb_t));

    data->timestamp = SENSOR_GET_TICK();
    data->accuracy  = 90;

    return SENSOR_EOK;
}

/* 接近传感器读取 */
static sensor_err_t apds9960_proximity_read(sensor_device_t *sensor,
                                            sensor_data_t *data)
{
    apds9960_priv_t *priv = (apds9960_priv_t *)sensor->priv_data;
    uint8_t proximity;

    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, APDS9960_REG_PDATA, &proximity, 1)
        != 0) {
        return SENSOR_EIO;
    }

    data->type             = SENSOR_TYPE_PROXIMITY;
    data->unit             = SENSOR_UNIT_NONE;
    data->value.val_uint32 = proximity;
    data->timestamp        = SENSOR_GET_TICK();
    data->accuracy         = 85;

    return SENSOR_EOK;
}

/* 手势识别读取 */
static sensor_err_t apds9960_gesture_read(sensor_device_t *sensor,
                                          sensor_data_t *data)
{
    apds9960_priv_t *priv = (apds9960_priv_t *)sensor->priv_data;
    uint8_t gstatus;

    /* 读取手势状态 */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, APDS9960_REG_GSTATUS, &gstatus, 1)
        != 0) {
        return SENSOR_EIO;
    }

    if (gstatus & 0x01) {
        /* 有手势数据 */
        uint8_t fifo_data[128];
        uint8_t fifo_level = (gstatus >> 1) & 0x7F;

        if (fifo_level > 0) {
            /* 读取FIFO数据并解析手势 */
            hal_i2c_mem_read(sensor->bus, priv->i2c_addr, APDS9960_REG_GFIFO_U,
                             fifo_data, fifo_level * 4);

            /* 简化的手势识别算法 */
            /* 实际应用需要更复杂的算法 */
            priv->last_gesture = APDS9960_GESTURE_NONE;
        }
    }

    data->type             = SENSOR_TYPE_CUSTOM;
    data->unit             = SENSOR_UNIT_NONE;
    data->value.val_uint32 = priv->last_gesture;
    data->timestamp        = SENSOR_GET_TICK();
    data->accuracy         = 80;

    return SENSOR_EOK;
}

static const sensor_ops_t apds9960_rgb_ops = {
    .init   = apds9960_init,
    .deinit = apds9960_deinit,
    .read   = apds9960_rgb_read,
};

static const sensor_ops_t apds9960_proximity_ops = {
    .init   = apds9960_init,
    .deinit = apds9960_deinit,
    .read   = apds9960_proximity_read,
};

static const sensor_ops_t apds9960_gesture_ops = {
    .init   = apds9960_init,
    .deinit = apds9960_deinit,
    .read   = apds9960_gesture_read,
};

sensor_device_t *apds9960_create_rgb(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL)
        return NULL;

    apds9960_priv_t *priv =
        (apds9960_priv_t *)SENSOR_MALLOC(sizeof(apds9960_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(apds9960_priv_t));

    priv->i2c_addr = APDS9960_ADDR;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "Avago";
    sensor->info.model      = "APDS9960";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_RGB;
    sensor->info.unit       = SENSOR_UNIT_NONE;
    sensor->info.range_max  = 65535;
    sensor->info.range_min  = 0;
    sensor->info.resolution = 16;
    sensor->info.max_odr    = 10;
    sensor->info.flags      = SENSOR_FLAG_INT_SUPPORT;

    sensor->ops       = &apds9960_rgb_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;

    return sensor;
}

sensor_device_t *apds9960_create_proximity(const char *name, void *i2c_bus)
{
    /* 类似实现 */
    return NULL; /* 完整实现省略 */
}

sensor_device_t *apds9960_create_gesture(const char *name, void *i2c_bus)
{
    /* 类似实现 */
    return NULL; /* 完整实现省略 */
}