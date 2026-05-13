#include "sensor_ap3216c.h"

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data,
                            uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg,
                             uint8_t *data, uint16_t len);

/**
 * @brief AP3216C初始化
 */
static sensor_err_t ap3216c_init(sensor_device_t *sensor)
{
    ap3216c_priv_t *priv = (ap3216c_priv_t *)sensor->priv_data;
    uint8_t data;

    SENSOR_LOG("Initializing AP3216C");

    /* 复位 */
    data = 0x04;
    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, AP3216C_REG_SYS_CONFIG, &data, 1);
    SENSOR_DELAY_MS(50);

    /* 设置工作模式 */
    data = priv->mode;
    if (hal_i2c_mem_write(
            sensor->bus, priv->i2c_addr, AP3216C_REG_SYS_CONFIG, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }

    SENSOR_DELAY_MS(50);

    SENSOR_LOG("AP3216C initialized successfully");

    return SENSOR_EOK;
}

/**
 * @brief AP3216C反初始化
 */
static sensor_err_t ap3216c_deinit(sensor_device_t *sensor)
{
    ap3216c_priv_t *priv = (ap3216c_priv_t *)sensor->priv_data;
    uint8_t data         = AP3216C_MODE_POWER_DOWN;

    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, AP3216C_REG_SYS_CONFIG, &data, 1);

    return SENSOR_EOK;
}

/**
 * @brief 读取环境光数据
 */
static sensor_err_t ap3216c_light_read(sensor_device_t *sensor,
                                       sensor_data_t *data)
{
    ap3216c_priv_t *priv = (ap3216c_priv_t *)sensor->priv_data;
    uint8_t buf[2];

    /* 读取ALS数据 */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, AP3216C_REG_ALS_DATA_L, buf, 2)
        != 0) {
        return SENSOR_EIO;
    }

    /* 组合数据 (16位) */
    uint16_t als_raw = (buf[1] << 8) | buf[0];

    /* 转换为lux */
    uint32_t lux = (uint32_t)als_raw * 35 / 100; /* 0.35 lux/count */

    data->type             = SENSOR_TYPE_AMBIENT_LIGHT;
    data->unit             = SENSOR_UNIT_LUX;
    data->value.val_uint32 = lux;
    data->timestamp        = SENSOR_GET_TICK();
    data->accuracy         = 90;

    return SENSOR_EOK;
}

/**
 * @brief 读取接近传感器数据
 */
static sensor_err_t ap3216c_proximity_read(sensor_device_t *sensor,
                                           sensor_data_t *data)
{
    ap3216c_priv_t *priv = (ap3216c_priv_t *)sensor->priv_data;
    uint8_t buf[2];

    /* 读取PS数据 */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, AP3216C_REG_PS_DATA_L, buf, 2)
        != 0) {
        return SENSOR_EIO;
    }

    /* 检查数据有效性 */
    if (buf[0] & 0x40) {
        /* IR溢出 */
        return SENSOR_ERROR;
    }

    if (buf[0] & 0x80) {
        /* 物体接近 */
        data->value.val_uint32 = 1;
    } else {
        data->value.val_uint32 = 0;
    }

    /* PS原始值 (10位) */
    uint16_t ps_raw = ((buf[1] & 0x3F) << 4) | (buf[0] & 0x0F);

    data->type            = SENSOR_TYPE_PROXIMITY;
    data->unit            = SENSOR_UNIT_NONE;
    data->value.val_int32 = ps_raw;
    data->timestamp       = SENSOR_GET_TICK();
    data->accuracy        = 85;

    return SENSOR_EOK;
}

/**
 * @brief 读取红外数据
 */
static sensor_err_t ap3216c_ir_read(sensor_device_t *sensor,
                                    sensor_data_t *data)
{
    ap3216c_priv_t *priv = (ap3216c_priv_t *)sensor->priv_data;
    uint8_t buf[2];

    /* 读取IR数据 */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, AP3216C_REG_IR_DATA_L, buf, 2)
        != 0) {
        return SENSOR_EIO;
    }

    /* 组合数据 (10位) */
    uint16_t ir_raw = ((buf[1] & 0x03) << 8) | buf[0];

    data->type             = SENSOR_TYPE_IR;
    data->unit             = SENSOR_UNIT_NONE;
    data->value.val_uint32 = ir_raw;
    data->timestamp        = SENSOR_GET_TICK();
    data->accuracy         = 85;

    return SENSOR_EOK;
}

/* 环境光传感器操作接口 */
static const sensor_ops_t ap3216c_light_ops = {
    .init   = ap3216c_init,
    .deinit = ap3216c_deinit,
    .read   = ap3216c_light_read,
};

/* 接近传感器操作接口 */
static const sensor_ops_t ap3216c_proximity_ops = {
    .init   = ap3216c_init,
    .deinit = ap3216c_deinit,
    .read   = ap3216c_proximity_read,
};

/* 红外传感器操作接口 */
static const sensor_ops_t ap3216c_ir_ops = {
    .init   = ap3216c_init,
    .deinit = ap3216c_deinit,
    .read   = ap3216c_ir_read,
};

/**
 * @brief 创建AP3216C环境光传感器
 */
sensor_device_t *ap3216c_create_light(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL) {
        return NULL;
    }

    ap3216c_priv_t *priv =
        (ap3216c_priv_t *)SENSOR_MALLOC(sizeof(ap3216c_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(ap3216c_priv_t));

    priv->i2c_addr = AP3216C_ADDR_DEFAULT;
    priv->mode     = AP3216C_MODE_ALS_PS_IR;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "Liteon";
    sensor->info.model      = "AP3216C";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_AMBIENT_LIGHT;
    sensor->info.unit       = SENSOR_UNIT_LUX;
    sensor->info.range_max  = 20000;
    sensor->info.range_min  = 0;
    sensor->info.resolution = 16;
    sensor->info.max_odr    = 10;
    sensor->info.flags      = 0;

    sensor->ops       = &ap3216c_light_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 1;

    return sensor;
}

/**
 * @brief 创建AP3216C接近传感器
 */
sensor_device_t *ap3216c_create_proximity(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL) {
        return NULL;
    }

    ap3216c_priv_t *priv =
        (ap3216c_priv_t *)SENSOR_MALLOC(sizeof(ap3216c_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(ap3216c_priv_t));

    priv->i2c_addr = AP3216C_ADDR_DEFAULT;
    priv->mode     = AP3216C_MODE_ALS_PS_IR;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "Liteon";
    sensor->info.model      = "AP3216C";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_PROXIMITY;
    sensor->info.unit       = SENSOR_UNIT_NONE;
    sensor->info.range_max  = 1023;
    sensor->info.range_min  = 0;
    sensor->info.resolution = 10;
    sensor->info.max_odr    = 10;
    sensor->info.flags      = 0;

    sensor->ops       = &ap3216c_proximity_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 1;

    return sensor;
}

/**
 * @brief 创建AP3216C红外传感器
 */
sensor_device_t *ap3216c_create_ir(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL) {
        return NULL;
    }

    ap3216c_priv_t *priv =
        (ap3216c_priv_t *)SENSOR_MALLOC(sizeof(ap3216c_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(ap3216c_priv_t));

    priv->i2c_addr = AP3216C_ADDR_DEFAULT;
    priv->mode     = AP3216C_MODE_ALS_PS_IR;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "Liteon";
    sensor->info.model      = "AP3216C";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_IR;
    sensor->info.unit       = SENSOR_UNIT_NONE;
    sensor->info.range_max  = 1023;
    sensor->info.range_min  = 0;
    sensor->info.resolution = 10;
    sensor->info.max_odr    = 10;
    sensor->info.flags      = 0;

    sensor->ops       = &ap3216c_ir_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 1;

    return sensor;
}