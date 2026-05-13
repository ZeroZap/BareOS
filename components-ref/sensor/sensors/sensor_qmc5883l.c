#include "sensor_qmc5883l.h"

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data,
                            uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg,
                             uint8_t *data, uint16_t len);

/**
 * @brief QMC5883L初始化
 */
static sensor_err_t qmc5883l_init(sensor_device_t *sensor)
{
    qmc5883l_priv_t *priv = (qmc5883l_priv_t *)sensor->priv_data;
    uint8_t data;

    SENSOR_LOG("Initializing QMC5883L");

    /* 软复位 */
    data = 0x80;
    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, QMC5883L_REG_CONTROL2, &data, 1);
    SENSOR_DELAY_MS(10);

    /* 配置CONTROL1: 连续模式, ODR=200Hz, RNG=±2G, OSR=512 */
    data = 0x0D;
    if (hal_i2c_mem_write(
            sensor->bus, priv->i2c_addr, QMC5883L_REG_CONTROL1, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }

    /* 设置SET/RESET周期 */
    data = 0x01;
    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, QMC5883L_REG_PERIOD, &data, 1);

    priv->range = 2;

    SENSOR_LOG("QMC5883L initialized successfully");

    return SENSOR_EOK;
}

/**
 * @brief QMC5883L反初始化
 */
static sensor_err_t qmc5883l_deinit(sensor_device_t *sensor)
{
    qmc5883l_priv_t *priv = (qmc5883l_priv_t *)sensor->priv_data;
    uint8_t data          = 0x00; /* 待机模式 */

    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, QMC5883L_REG_CONTROL1, &data, 1);

    return SENSOR_EOK;
}

/**
 * @brief 读取QMC5883L数据
 */
static sensor_err_t qmc5883l_read(sensor_device_t *sensor, sensor_data_t *data)
{
    qmc5883l_priv_t *priv = (qmc5883l_priv_t *)sensor->priv_data;
    uint8_t buf[6];
    int16_t raw[3];
    uint8_t status;

    /* 检查数据就绪 */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, QMC5883L_REG_STATUS, &status, 1)
        != 0) {
        return SENSOR_EIO;
    }

    if (!(status & 0x01)) {
        return SENSOR_EBUSY; /* 数据未就绪 */
    }

    /* 读取6字节磁场数据 */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, QMC5883L_REG_DATA_X_LSB, buf, 6)
        != 0) {
        return SENSOR_EIO;
    }

    /* 组合数据 */
    raw[0] = (int16_t)((buf[1] << 8) | buf[0]);
    raw[1] = (int16_t)((buf[3] << 8) | buf[2]);
    raw[2] = (int16_t)((buf[5] << 8) | buf[4]);

    /* 转换为μT (±2G范围, 1 LSB = 12 nT) */
    data->type              = SENSOR_TYPE_MAGNETOMETER;
    data->unit              = SENSOR_UNIT_MICRO_TESLA;
    data->value.val_3axis.x = (int32_t)raw[0] * 12 / 1000; /* nT to μT */
    data->value.val_3axis.y = (int32_t)raw[1] * 12 / 1000;
    data->value.val_3axis.z = (int32_t)raw[2] * 12 / 1000;
    data->timestamp         = SENSOR_GET_TICK();
    data->accuracy          = 92;

    return SENSOR_EOK;
}

/* QMC5883L操作接口 */
static const sensor_ops_t qmc5883l_ops = {
    .init   = qmc5883l_init,
    .deinit = qmc5883l_deinit,
    .read   = qmc5883l_read,
};

/**
 * @brief 创建QMC5883L磁力计
 */
sensor_device_t *qmc5883l_create(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL) {
        return NULL;
    }

    qmc5883l_priv_t *priv =
        (qmc5883l_priv_t *)SENSOR_MALLOC(sizeof(qmc5883l_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(qmc5883l_priv_t));

    priv->i2c_addr = QMC5883L_ADDR_DEFAULT;
    priv->range    = 2;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "QST";
    sensor->info.model      = "QMC5883L";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_MAGNETOMETER;
    sensor->info.unit       = SENSOR_UNIT_MICRO_TESLA;
    sensor->info.range_max  = 200; /* ±2 gauss = ±200 μT */
    sensor->info.range_min  = -200;
    sensor->info.resolution = 16;
    sensor->info.max_odr    = 200;
    sensor->info.flags      = SENSOR_FLAG_CALIBRATION;

    sensor->ops       = &qmc5883l_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 100;

    return sensor;
}