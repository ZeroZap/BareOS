#include "sensor_sc7a20.h"

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data,
                            uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg,
                             uint8_t *data, uint16_t len);

/**
 * @brief SC7A20初始化
 */
static sensor_err_t sc7a20_init(sensor_device_t *sensor)
{
    sc7a20_priv_t *priv = (sc7a20_priv_t *)sensor->priv_data;
    uint8_t data;

    SENSOR_LOG("Initializing SC7A20");

    /* 检查WHO_AM_I */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, SC7A20_REG_WHOAMI, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }

    if (data != SC7A20_WHOAMI_VALUE) {
        SENSOR_LOG("Wrong WHO_AM_I: 0x%02X", data);
        return SENSOR_ERROR;
    }

    /* 配置CTRL_REG1: 100Hz, 正常模式, 使能XYZ */
    data = SC7A20_ODR_100HZ | 0x07;
    if (hal_i2c_mem_write(
            sensor->bus, priv->i2c_addr, SC7A20_REG_CTRL_REG1, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }
    priv->odr_reg = data;

    /* 配置CTRL_REG4: ±2g, 高分辨率模式 */
    data = SC7A20_RANGE_2G | 0x08;
    if (hal_i2c_mem_write(
            sensor->bus, priv->i2c_addr, SC7A20_REG_CTRL_REG4, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }
    priv->range = 2;

    SENSOR_LOG("SC7A20 initialized successfully");

    return SENSOR_EOK;
}

/**
 * @brief SC7A20反初始化
 */
static sensor_err_t sc7a20_deinit(sensor_device_t *sensor)
{
    sc7a20_priv_t *priv = (sc7a20_priv_t *)sensor->priv_data;
    uint8_t data        = SC7A20_ODR_POWER_DOWN;

    /* 进入低功耗模式 */
    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, SC7A20_REG_CTRL_REG1, &data, 1);

    return SENSOR_EOK;
}

/**
 * @brief 读取SC7A20数据
 */
static sensor_err_t sc7a20_read(sensor_device_t *sensor, sensor_data_t *data)
{
    sc7a20_priv_t *priv = (sc7a20_priv_t *)sensor->priv_data;
    uint8_t buf[6];
    int16_t raw[3];

    /* 读取6字节加速度数据 (从0x28开始，自动递增) */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, SC7A20_REG_OUT_X_L | 0x80, buf, 6)
        != 0) {
        return SENSOR_EIO;
    }

    /* 组合数据 (12位有效，左对齐到16位) */
    raw[0] = (int16_t)((buf[1] << 8) | buf[0]) >> 4;
    raw[1] = (int16_t)((buf[3] << 8) | buf[2]) >> 4;
    raw[2] = (int16_t)((buf[5] << 8) | buf[4]) >> 4;

    /* 转换为mg */
    int32_t sensitivity = priv->range * 1000 / 2048; /* LSB/g */

    data->type              = SENSOR_TYPE_ACCELEROMETER;
    data->unit              = SENSOR_UNIT_MILLI_G;
    data->value.val_3axis.x = (int32_t)raw[0] * sensitivity;
    data->value.val_3axis.y = (int32_t)raw[1] * sensitivity;
    data->value.val_3axis.z = (int32_t)raw[2] * sensitivity;
    data->timestamp         = SENSOR_GET_TICK();
    data->accuracy          = 95;

    return SENSOR_EOK;
}

#if SENSOR_ENABLE_POWER_MGMT
/**
 * @brief 设置功耗模式
 */
static sensor_err_t sc7a20_set_power_mode(sensor_device_t *sensor,
                                          sensor_power_mode_t mode)
{
    sc7a20_priv_t *priv = (sc7a20_priv_t *)sensor->priv_data;
    uint8_t data;

    switch (mode) {
    case SENSOR_POWER_MODE_SHUTDOWN:
        data = SC7A20_ODR_POWER_DOWN;
        break;

    case SENSOR_POWER_MODE_LOW_POWER:
        data = SC7A20_ODR_10HZ | 0x07;
        break;

    case SENSOR_POWER_MODE_NORMAL:
        data = SC7A20_ODR_100HZ | 0x07;
        break;

    case SENSOR_POWER_MODE_HIGH_PERFORMANCE:
        data = SC7A20_ODR_400HZ | 0x07;
        break;

    default:
        return SENSOR_EINVAL;
    }

    if (hal_i2c_mem_write(
            sensor->bus, priv->i2c_addr, SC7A20_REG_CTRL_REG1, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }

    priv->odr_reg = data;

    return SENSOR_EOK;
}
#endif

/**
 * @brief 配置传感器
 */
static sensor_err_t sc7a20_config(sensor_device_t *sensor,
                                  sensor_config_type_t cfg, void *value)
{
    sc7a20_priv_t *priv = (sc7a20_priv_t *)sensor->priv_data;
    uint8_t data;

    switch (cfg) {
    case SENSOR_CFG_RANGE: {
        uint32_t range = *(uint32_t *)value;
        uint8_t range_reg;

        if (range <= 2) {
            range_reg   = SC7A20_RANGE_2G;
            priv->range = 2;
        } else if (range <= 4) {
            range_reg   = SC7A20_RANGE_4G;
            priv->range = 4;
        } else if (range <= 8) {
            range_reg   = SC7A20_RANGE_8G;
            priv->range = 8;
        } else {
            range_reg   = SC7A20_RANGE_16G;
            priv->range = 16;
        }

        data = range_reg | 0x08;
        if (hal_i2c_mem_write(
                sensor->bus, priv->i2c_addr, SC7A20_REG_CTRL_REG4, &data, 1)
            != 0) {
            return SENSOR_EIO;
        }

        SENSOR_LOG("SC7A20 range set to ±%dg", priv->range);
        break;
    }

    case SENSOR_CFG_ODR: {
        uint32_t odr = *(uint32_t *)value;
        uint8_t odr_reg;

        if (odr <= 1)
            odr_reg = SC7A20_ODR_1HZ;
        else if (odr <= 10)
            odr_reg = SC7A20_ODR_10HZ;
        else if (odr <= 25)
            odr_reg = SC7A20_ODR_25HZ;
        else if (odr <= 50)
            odr_reg = SC7A20_ODR_50HZ;
        else if (odr <= 100)
            odr_reg = SC7A20_ODR_100HZ;
        else if (odr <= 200)
            odr_reg = SC7A20_ODR_200HZ;
        else
            odr_reg = SC7A20_ODR_400HZ;

        data = odr_reg | 0x07;
        if (hal_i2c_mem_write(
                sensor->bus, priv->i2c_addr, SC7A20_REG_CTRL_REG1, &data, 1)
            != 0) {
            return SENSOR_EIO;
        }

        priv->odr_reg = data;
        sensor->odr   = odr;

        SENSOR_LOG("SC7A20 ODR set to %u Hz", odr);
        break;
    }

    default:
        return SENSOR_ENOSYS;
    }

    return SENSOR_EOK;
}

/* SC7A20操作接口 */
static const sensor_ops_t sc7a20_ops = {
    .init   = sc7a20_init,
    .deinit = sc7a20_deinit,
    .read   = sc7a20_read,
    .config = sc7a20_config,
#if SENSOR_ENABLE_POWER_MGMT
    .set_power_mode = sc7a20_set_power_mode,
#endif
};

/**
 * @brief 创建SC7A20加速度计
 */
sensor_device_t *sc7a20_create(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL) {
        return NULL;
    }

    sc7a20_priv_t *priv = (sc7a20_priv_t *)SENSOR_MALLOC(sizeof(sc7a20_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(sc7a20_priv_t));

    priv->i2c_addr = SC7A20_ADDR_DEFAULT;
    priv->range    = 2;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "Silan";
    sensor->info.model      = "SC7A20";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit       = SENSOR_UNIT_MILLI_G;
    sensor->info.range_max  = 2000;
    sensor->info.range_min  = -2000;
    sensor->info.resolution = 12;
    sensor->info.max_odr    = 400;
    sensor->info.flags      = SENSOR_FLAG_CALIBRATION | SENSOR_FLAG_LOW_POWER;

    sensor->ops       = &sc7a20_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 100;

    return sensor;
}