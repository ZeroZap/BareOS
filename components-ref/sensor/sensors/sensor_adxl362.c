#include "sensor_adxl362.h"

extern int hal_spi_transfer(void *bus, uint8_t *tx, uint8_t *rx, uint16_t len);

/**
 * @brief 读取寄存器
 */
static int adxl362_read_reg(void *spi_bus, uint8_t reg, uint8_t *data,
                            uint16_t len)
{
    uint8_t tx[256] = { ADXL362_CMD_READ, reg };
    uint8_t rx[256];

    if (hal_spi_transfer(spi_bus, tx, rx, len + 2) != 0) {
        return -1;
    }

    memcpy(data, &rx[2], len);
    return 0;
}

/**
 * @brief 写入寄存器
 */
static int adxl362_write_reg(void *spi_bus, uint8_t reg, uint8_t *data,
                             uint16_t len)
{
    uint8_t tx[256] = { ADXL362_CMD_WRITE, reg };
    uint8_t rx[256];

    memcpy(&tx[2], data, len);

    return hal_spi_transfer(spi_bus, tx, rx, len + 2);
}

static sensor_err_t adxl362_init(sensor_device_t *sensor)
{
    adxl362_priv_t *priv = (adxl362_priv_t *)sensor->priv_data;
    uint8_t data;

    SENSOR_LOG("Initializing ADXL362");

    /* 检查Device ID */
    if (adxl362_read_reg(priv->spi_bus, ADXL362_REG_DEVID_AD, &data, 1) != 0) {
        return SENSOR_EIO;
    }

    if (data != ADXL362_DEVID_AD) {
        SENSOR_LOG("Wrong Device ID: 0x%02X", data);
        return SENSOR_ERROR;
    }

    /* 配置滤波器: ODR=100Hz, ±2g */
    data = (ADXL362_ODR_100HZ << 3) | ADXL362_RANGE_2G;
    adxl362_write_reg(priv->spi_bus, ADXL362_REG_FILTER_CTL, &data, 1);
    priv->odr     = ADXL362_ODR_100HZ;
    priv->range   = ADXL362_RANGE_2G;
    priv->range_g = 2;

    /* 启动测量模式 */
    data = ADXL362_MODE_MEASUREMENT;
    adxl362_write_reg(priv->spi_bus, ADXL362_REG_POWER_CTL, &data, 1);
    priv->mode = ADXL362_MODE_MEASUREMENT;

    SENSOR_LOG("ADXL362 initialized (Ultra Low Power: 1.8μA @ 100Hz)");

    return SENSOR_EOK;
}

static sensor_err_t adxl362_deinit(sensor_device_t *sensor)
{
    adxl362_priv_t *priv = (adxl362_priv_t *)sensor->priv_data;
    uint8_t data         = ADXL362_MODE_STANDBY;

    adxl362_write_reg(priv->spi_bus, ADXL362_REG_POWER_CTL, &data, 1);

    return SENSOR_EOK;
}

static sensor_err_t adxl362_read(sensor_device_t *sensor, sensor_data_t *data)
{
    adxl362_priv_t *priv = (adxl362_priv_t *)sensor->priv_data;
    uint8_t buf[6];
    int16_t raw[3];

    /* 读取6字节加速度数据 */
    if (adxl362_read_reg(priv->spi_bus, ADXL362_REG_XDATA, buf, 6) != 0) {
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

    /* 转换为mg (1 LSB = 1mg @ ±2g) */
    data->type              = SENSOR_TYPE_ACCELEROMETER;
    data->unit              = SENSOR_UNIT_MILLI_G;
    data->value.val_3axis.x = raw[0];
    data->value.val_3axis.y = raw[1];
    data->value.val_3axis.z = raw[2];
    data->timestamp         = SENSOR_GET_TICK();
    data->accuracy          = 95;

    return SENSOR_EOK;
}

static const sensor_ops_t adxl362_ops = {
    .init   = adxl362_init,
    .deinit = adxl362_deinit,
    .read   = adxl362_read,
};

sensor_device_t *adxl362_create(const char *name, void *spi_bus)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL)
        return NULL;

    adxl362_priv_t *priv =
        (adxl362_priv_t *)SENSOR_MALLOC(sizeof(adxl362_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(adxl362_priv_t));

    priv->spi_bus = spi_bus;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "Analog Devices";
    sensor->info.model      = "ADXL362";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit       = SENSOR_UNIT_MILLI_G;
    sensor->info.range_max  = 2000;
    sensor->info.range_min  = -2000;
    sensor->info.resolution = 12;
    sensor->info.max_odr    = 400;
    sensor->info.flags      = SENSOR_FLAG_LOW_POWER | SENSOR_FLAG_INT_SUPPORT
                         | SENSOR_FLAG_FIFO_SUPPORT;

    sensor->ops       = &adxl362_ops;
    sensor->bus       = spi_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 100;

    return sensor;
}