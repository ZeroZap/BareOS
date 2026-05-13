#include "sensor_lis2dh12.h"

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data,
                            uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg,
                             uint8_t *data, uint16_t len);

/**
 * @brief LIS2DH12初始化
 */
static sensor_err_t lis2dh12_init(sensor_device_t *sensor)
{
    lis2dh12_priv_t *priv = (lis2dh12_priv_t *)sensor->priv_data;
    uint8_t data;

    SENSOR_LOG("Initializing LIS2DH12");

    /* 检查WHO_AM_I */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, LIS2DH12_REG_WHOAMI, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }

    if (data != LIS2DH12_WHOAMI_VALUE) {
        SENSOR_LOG("Wrong WHO_AM_I: 0x%02X", data);
        return SENSOR_ERROR;
    }

    /* 配置CTRL1: 低功耗模式, 10Hz, 使能XYZ */
    data = LIS2DH12_ODR_10HZ | 0x07;
    if (hal_i2c_mem_write(
            sensor->bus, priv->i2c_addr, LIS2DH12_REG_CTRL1, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }
    priv->odr = LIS2DH12_ODR_10HZ;

    /* 配置CTRL4: ±2g, 高分辨率模式 */
    data = LIS2DH12_RANGE_2G | 0x08;
    if (hal_i2c_mem_write(
            sensor->bus, priv->i2c_addr, LIS2DH12_REG_CTRL4, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }
    priv->range   = LIS2DH12_RANGE_2G;
    priv->range_g = 2;

    /* 使能温度传感器 */
    data = 0xC0;
    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, LIS2DH12_REG_TEMP_CFG, &data, 1);

    SENSOR_LOG("LIS2DH12 initialized (Low Power Mode, 10Hz, ±2g)");

    return SENSOR_EOK;
}

/**
 * @brief LIS2DH12反初始化
 */
static sensor_err_t lis2dh12_deinit(sensor_device_t *sensor)
{
    lis2dh12_priv_t *priv = (lis2dh12_priv_t *)sensor->priv_data;
    uint8_t data          = LIS2DH12_ODR_POWER_DOWN;

    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, LIS2DH12_REG_CTRL1, &data, 1);

    return SENSOR_EOK;
}

/**
 * @brief 读取LIS2DH12数据
 */
static sensor_err_t lis2dh12_read(sensor_device_t *sensor, sensor_data_t *data)
{
    lis2dh12_priv_t *priv = (lis2dh12_priv_t *)sensor->priv_data;
    uint8_t buf[6];
    int16_t raw[3];

    /* 读取6字节加速度数据 (自动递增) */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, LIS2DH12_REG_OUT_X_L | 0x80, buf, 6)
        != 0) {
        return SENSOR_EIO;
    }

    /* 组合数据 (12位有效，左对齐到16位) */
    raw[0] = (int16_t)((buf[1] << 8) | buf[0]) >> 4;
    raw[1] = (int16_t)((buf[3] << 8) | buf[2]) >> 4;
    raw[2] = (int16_t)((buf[5] << 8) | buf[4]) >> 4;

    /* 转换为mg */
    int32_t sensitivity = priv->range_g * 1000 / 2048; /* mg/LSB */

    data->type              = SENSOR_TYPE_ACCELEROMETER;
    data->unit              = SENSOR_UNIT_MILLI_G;
    data->value.val_3axis.x = (int32_t)raw[0] * sensitivity;
    data->value.val_3axis.y = (int32_t)raw[1] * sensitivity;
    data->value.val_3axis.z = (int32_t)raw[2] * sensitivity;
    data->timestamp         = SENSOR_GET_TICK();
    data->accuracy          = 94;

    return SENSOR_EOK;
}

#if SENSOR_ENABLE_POWER_MGMT
/**
 * @brief 设置功耗模式
 */
static sensor_err_t lis2dh12_set_power_mode(sensor_device_t *sensor,
                                            sensor_power_mode_t mode)
{
    lis2dh12_priv_t *priv = (lis2dh12_priv_t *)sensor->priv_data;
    uint8_t data;

    switch (mode) {
    case SENSOR_POWER_MODE_SHUTDOWN:
        data = LIS2DH12_ODR_POWER_DOWN | 0x07;
        break;

    case SENSOR_POWER_MODE_LOW_POWER:
        /* 1Hz, 低功耗模式 */
        data = LIS2DH12_ODR_1HZ | 0x07;
        break;

    case SENSOR_POWER_MODE_NORMAL:
        /* 10Hz, 正常模式 */
        data = LIS2DH12_ODR_10HZ | 0x07;
        break;

    case SENSOR_POWER_MODE_HIGH_PERFORMANCE:
        /* 100Hz, 高分辨率模式 */
        data = LIS2DH12_ODR_100HZ | 0x07;
        break;

    default:
        return SENSOR_EINVAL;
    }

    if (hal_i2c_mem_write(
            sensor->bus, priv->i2c_addr, LIS2DH12_REG_CTRL1, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }

    SENSOR_LOG("LIS2DH12 power mode changed");

    return SENSOR_EOK;
}
#endif

/**
 * @brief 配置传感器
 */
static sensor_err_t lis2dh12_config(sensor_device_t *sensor,
                                    sensor_config_type_t cfg, void *value)
{
    lis2dh12_priv_t *priv = (lis2dh12_priv_t *)sensor->priv_data;
    uint8_t data;

    switch (cfg) {
    case SENSOR_CFG_RANGE: {
        uint32_t range = *(uint32_t *)value;
        uint8_t range_reg;

        if (range <= 2) {
            range_reg     = LIS2DH12_RANGE_2G;
            priv->range_g = 2;
        } else if (range <= 4) {
            range_reg     = LIS2DH12_RANGE_4G;
            priv->range_g = 4;
        } else if (range <= 8) {
            range_reg     = LIS2DH12_RANGE_8G;
            priv->range_g = 8;
        } else {
            range_reg     = LIS2DH12_RANGE_16G;
            priv->range_g = 16;
        }

        data = range_reg | 0x08;
        if (hal_i2c_mem_write(
                sensor->bus, priv->i2c_addr, LIS2DH12_REG_CTRL4, &data, 1)
            != 0) {
            return SENSOR_EIO;
        }

        priv->range = range_reg;
        break;
    }

    case SENSOR_CFG_ODR: {
        uint32_t odr = *(uint32_t *)value;
        uint8_t odr_reg;

        if (odr <= 1)
            odr_reg = LIS2DH12_ODR_1HZ;
        else if (odr <= 10)
            odr_reg = LIS2DH12_ODR_10HZ;
        else if (odr <= 25)
            odr_reg = LIS2DH12_ODR_25HZ;
        else if (odr <= 50)
            odr_reg = LIS2DH12_ODR_50HZ;
        else if (odr <= 100)
            odr_reg = LIS2DH12_ODR_100HZ;
        else if (odr <= 200)
            odr_reg = LIS2DH12_ODR_200HZ;
        else
            odr_reg = LIS2DH12_ODR_400HZ;

        data = odr_reg | 0x07;
        if (hal_i2c_mem_write(
                sensor->bus, priv->i2c_addr, LIS2DH12_REG_CTRL1, &data, 1)
            != 0) {
            return SENSOR_EIO;
        }

        priv->odr   = odr_reg;
        sensor->odr = odr;
        break;
    }

    default:
        return SENSOR_ENOSYS;
    }

    return SENSOR_EOK;
}

static const sensor_ops_t lis2dh12_ops = {
    .init   = lis2dh12_init,
    .deinit = lis2dh12_deinit,
    .read   = lis2dh12_read,
    .config = lis2dh12_config,
#if SENSOR_ENABLE_POWER_MGMT
    .set_power_mode = lis2dh12_set_power_mode,
#endif
};

sensor_device_t *lis2dh12_create(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL)
        return NULL;

    lis2dh12_priv_t *priv =
        (lis2dh12_priv_t *)SENSOR_MALLOC(sizeof(lis2dh12_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(lis2dh12_priv_t));

    priv->i2c_addr = LIS2DH12_ADDR_DEFAULT;
    priv->mode     = LIS2DH12_MODE_LOW_POWER;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "STMicroelectronics";
    sensor->info.model      = "LIS2DH12";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit       = SENSOR_UNIT_MILLI_G;
    sensor->info.range_max  = 2000;
    sensor->info.range_min  = -2000;
    sensor->info.resolution = 12;
    sensor->info.max_odr    = 400;
    sensor->info.flags      = SENSOR_FLAG_LOW_POWER | SENSOR_FLAG_INT_SUPPORT
                         | SENSOR_FLAG_FIFO_SUPPORT;

    sensor->ops       = &lis2dh12_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 10;

    return sensor;
}