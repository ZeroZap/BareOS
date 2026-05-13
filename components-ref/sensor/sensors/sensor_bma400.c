#include "sensor_bma400.h"

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data,
                            uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg,
                             uint8_t *data, uint16_t len);

static sensor_err_t bma400_init(sensor_device_t *sensor)
{
    bma400_priv_t *priv = (bma400_priv_t *)sensor->priv_data;
    uint8_t data;

    SENSOR_LOG("Initializing BMA400");

    /* 检查CHIP_ID */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, BMA400_REG_CHIPID, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }

    if (data != BMA400_CHIP_ID) {
        SENSOR_LOG("Wrong CHIP_ID: 0x%02X", data);
        return SENSOR_ERROR;
    }

    /* 软复位 */
    data = 0xB6;
    hal_i2c_mem_write(sensor->bus, priv->i2c_addr, BMA400_REG_CMD, &data, 1);
    SENSOR_DELAY_MS(10);

    /* 配置ACC_CONFIG0: 低功耗模式 */
    data = BMA400_POWER_MODE_LOW_POWER;
    if (hal_i2c_mem_write(
            sensor->bus, priv->i2c_addr, BMA400_REG_ACC_CONFIG0, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }
    priv->power_mode = BMA400_POWER_MODE_LOW_POWER;

    /* 配置ACC_CONFIG1: ±2g, OSR=0 (低功耗) */
    data = (BMA400_RANGE_2G << 6) | 0x00;
    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, BMA400_REG_ACC_CONFIG1, &data, 1);
    priv->range   = BMA400_RANGE_2G;
    priv->range_g = 2;

    /* 配置ACC_CONFIG2: 25Hz */
    data = BMA400_ODR_25HZ;
    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, BMA400_REG_ACC_CONFIG2, &data, 1);
    priv->odr = BMA400_ODR_25HZ;

    SENSOR_LOG("BMA400 initialized (Low Power: 3.5μA, 25Hz, ±2g)");

    return SENSOR_EOK;
}

static sensor_err_t bma400_deinit(sensor_device_t *sensor)
{
    bma400_priv_t *priv = (bma400_priv_t *)sensor->priv_data;
    uint8_t data        = BMA400_POWER_MODE_SLEEP;

    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, BMA400_REG_ACC_CONFIG0, &data, 1);

    return SENSOR_EOK;
}

static sensor_err_t bma400_read(sensor_device_t *sensor, sensor_data_t *data)
{
    bma400_priv_t *priv = (bma400_priv_t *)sensor->priv_data;
    uint8_t buf[6];
    int16_t raw[3];

    /* 读取6字节加速度数据 */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, BMA400_REG_ACC_X_LSB, buf, 6)
        != 0) {
        return SENSOR_EIO;
    }

    /* 组合数据 (12位有效) */
    raw[0] = (int16_t)(((buf[1] << 8) | buf[0]) & 0x0FFF);
    raw[1] = (int16_t)(((buf[3] << 8) | buf[2]) & 0x0FFF);
    raw[2] = (int16_t)(((buf[5] << 8) | buf[4]) & 0x0FFF);

    /* 符号扩展 */
    if (raw[0] & 0x0800)
        raw[0] |= 0xF000;
    if (raw[1] & 0x0800)
        raw[1] |= 0xF000;
    if (raw[2] & 0x0800)
        raw[2] |= 0xF000;

    /* 转换为mg */
    int32_t sensitivity = priv->range_g * 1000 / 2048;

    data->type              = SENSOR_TYPE_ACCELEROMETER;
    data->unit              = SENSOR_UNIT_MILLI_G;
    data->value.val_3axis.x = (int32_t)raw[0] * sensitivity;
    data->value.val_3axis.y = (int32_t)raw[1] * sensitivity;
    data->value.val_3axis.z = (int32_t)raw[2] * sensitivity;
    data->timestamp         = SENSOR_GET_TICK();
    data->accuracy          = 93;

    return SENSOR_EOK;
}

#if SENSOR_ENABLE_POWER_MGMT
static sensor_err_t bma400_set_power_mode(sensor_device_t *sensor,
                                          sensor_power_mode_t mode)
{
    bma400_priv_t *priv = (bma400_priv_t *)sensor->priv_data;
    uint8_t data;

    switch (mode) {
    case SENSOR_POWER_MODE_SHUTDOWN:
    case SENSOR_POWER_MODE_SLEEP:
        data = BMA400_POWER_MODE_SLEEP;
        break;

    case SENSOR_POWER_MODE_LOW_POWER:
    case SENSOR_POWER_MODE_STANDBY:
        data = BMA400_POWER_MODE_LOW_POWER;
        break;

    case SENSOR_POWER_MODE_NORMAL:
    case SENSOR_POWER_MODE_HIGH_PERFORMANCE:
        data = BMA400_POWER_MODE_NORMAL;
        break;

    default:
        return SENSOR_EINVAL;
    }

    if (hal_i2c_mem_write(
            sensor->bus, priv->i2c_addr, BMA400_REG_ACC_CONFIG0, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }

    priv->power_mode = data;

    return SENSOR_EOK;
}
#endif

static const sensor_ops_t bma400_ops = {
    .init   = bma400_init,
    .deinit = bma400_deinit,
    .read   = bma400_read,
#if SENSOR_ENABLE_POWER_MGMT
    .set_power_mode = bma400_set_power_mode,
#endif
};

sensor_device_t *bma400_create(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL)
        return NULL;

    bma400_priv_t *priv = (bma400_priv_t *)SENSOR_MALLOC(sizeof(bma400_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(bma400_priv_t));

    priv->i2c_addr = BMA400_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "Bosch";
    sensor->info.model      = "BMA400";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit       = SENSOR_UNIT_MILLI_G;
    sensor->info.range_max  = 2000;
    sensor->info.range_min  = -2000;
    sensor->info.resolution = 12;
    sensor->info.max_odr    = 800;
    sensor->info.flags      = SENSOR_FLAG_LOW_POWER | SENSOR_FLAG_INT_SUPPORT
                         | SENSOR_FLAG_FIFO_SUPPORT;

    sensor->ops       = &bma400_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 25;

    return sensor;
}