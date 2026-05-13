#include "sensor_mpu6050.h"

/* I2C读写接口 (需要用户实现) */
extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data,
                            uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg,
                             uint8_t *data, uint16_t len);

/**
 * @brief MPU6050初始化
 */
static sensor_err_t mpu6050_init(sensor_device_t *sensor)
{
    mpu6050_priv_t *priv = (mpu6050_priv_t *)sensor->priv_data;
    uint8_t data;

    SENSOR_LOG("Initializing MPU6050");

    /* 检查WHO_AM_I */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, MPU6050_REG_WHOAMI, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }

    if (data != MPU6050_WHOAMI_VALUE) {
        SENSOR_LOG("Wrong WHO_AM_I: 0x%02X", data);
        return SENSOR_ERROR;
    }

    /* 复位设备 */
    data = 0x80;
    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, MPU6050_REG_PWR_MGMT_1, &data, 1);
    SENSOR_DELAY_MS(100);

    /* 唤醒设备 */
    data = 0x00;
    if (hal_i2c_mem_write(
            sensor->bus, priv->i2c_addr, MPU6050_REG_PWR_MGMT_1, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }

    /* 配置加速度计量程 ±2g */
    data = 0x00;
    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, MPU6050_REG_ACCEL_CONFIG, &data, 1);
    priv->accel_range = 2;

    /* 配置陀螺仪量程 ±250°/s */
    data = 0x00;
    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, MPU6050_REG_GYRO_CONFIG, &data, 1);
    priv->gyro_range = 250;

    /* 配置低通滤波器 */
    data = 0x03; /* ~44Hz */
    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, MPU6050_REG_CONFIG, &data, 1);

    SENSOR_LOG("MPU6050 initialized successfully");

    return SENSOR_EOK;
}

/**
 * @brief MPU6050反初始化
 */
static sensor_err_t mpu6050_deinit(sensor_device_t *sensor)
{
    mpu6050_priv_t *priv = (mpu6050_priv_t *)sensor->priv_data;
    uint8_t data         = 0x40; /* 进入睡眠模式 */

    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, MPU6050_REG_PWR_MGMT_1, &data, 1);

    return SENSOR_EOK;
}

/**
 * @brief 读取加速度数据
 */
static sensor_err_t mpu6050_accel_read(sensor_device_t *sensor,
                                       sensor_data_t *data)
{
    mpu6050_priv_t *priv = (mpu6050_priv_t *)sensor->priv_data;
    uint8_t buf[6];
    int16_t raw[3];

    /* 读取6字节加速度数据 */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, MPU6050_REG_ACCEL_XOUT_H, buf, 6)
        != 0) {
        return SENSOR_EIO;
    }

    /* 组合数据 */
    raw[0] = (int16_t)((buf[0] << 8) | buf[1]);
    raw[1] = (int16_t)((buf[2] << 8) | buf[3]);
    raw[2] = (int16_t)((buf[4] << 8) | buf[5]);

    /* 转换为mg (±2g范围，16位ADC) */
    data->type              = SENSOR_TYPE_ACCELEROMETER;
    data->unit              = SENSOR_UNIT_MILLI_G;
    data->value.val_3axis.x = (int32_t)raw[0] * 2000 / 32768;
    data->value.val_3axis.y = (int32_t)raw[1] * 2000 / 32768;
    data->value.val_3axis.z = (int32_t)raw[2] * 2000 / 32768;
    data->timestamp         = SENSOR_GET_TICK();
    data->accuracy          = 95;

    return SENSOR_EOK;
}

/**
 * @brief 读取陀螺仪数据
 */
static sensor_err_t mpu6050_gyro_read(sensor_device_t *sensor,
                                      sensor_data_t *data)
{
    mpu6050_priv_t *priv = (mpu6050_priv_t *)sensor->priv_data;
    uint8_t buf[6];
    int16_t raw[3];

    /* 读取6字节陀螺仪数据 */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, MPU6050_REG_GYRO_XOUT_H, buf, 6)
        != 0) {
        return SENSOR_EIO;
    }

    /* 组合数据 */
    raw[0] = (int16_t)((buf[0] << 8) | buf[1]);
    raw[1] = (int16_t)((buf[2] << 8) | buf[3]);
    raw[2] = (int16_t)((buf[4] << 8) | buf[5]);

    /* 转换为°/s (±250°/s范围) */
    data->type              = SENSOR_TYPE_GYROSCOPE;
    data->unit              = SENSOR_UNIT_DEGREE_PER_SECOND;
    data->value.val_3axis.x = (int32_t)raw[0] * 250 / 32768;
    data->value.val_3axis.y = (int32_t)raw[1] * 250 / 32768;
    data->value.val_3axis.z = (int32_t)raw[2] * 250 / 32768;
    data->timestamp         = SENSOR_GET_TICK();
    data->accuracy          = 95;

    return SENSOR_EOK;
}

/* 加速度计操作接口 */
static const sensor_ops_t mpu6050_accel_ops = {
    .init   = mpu6050_init,
    .deinit = mpu6050_deinit,
    .read   = mpu6050_accel_read,
};

/* 陀螺仪操作接口 */
static const sensor_ops_t mpu6050_gyro_ops = {
    .init   = mpu6050_init,
    .deinit = mpu6050_deinit,
    .read   = mpu6050_gyro_read,
};

/**
 * @brief 创建MPU6050加速度计设备
 */
sensor_device_t *mpu6050_create_accel(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL) {
        return NULL;
    }

    mpu6050_priv_t *priv =
        (mpu6050_priv_t *)SENSOR_MALLOC(sizeof(mpu6050_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(mpu6050_priv_t));

    /* 初始化私有数据 */
    priv->i2c_addr = MPU6050_ADDR_DEFAULT;

    /* 设置设备信息 */
    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "InvenSense";
    sensor->info.model      = "MPU6050";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit       = SENSOR_UNIT_MILLI_G;
    sensor->info.range_max  = 2000;
    sensor->info.range_min  = -2000;
    sensor->info.resolution = 16;
    sensor->info.max_odr    = 1000;
    sensor->info.flags      = SENSOR_FLAG_INT_SUPPORT | SENSOR_FLAG_CALIBRATION;

    sensor->ops       = &mpu6050_accel_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 100;

    return sensor;
}

/**
 * @brief 创建MPU6050陀螺仪设备
 */
sensor_device_t *mpu6050_create_gyro(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL) {
        return NULL;
    }

    mpu6050_priv_t *priv =
        (mpu6050_priv_t *)SENSOR_MALLOC(sizeof(mpu6050_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(mpu6050_priv_t));

    /* 初始化私有数据 */
    priv->i2c_addr = MPU6050_ADDR_DEFAULT;

    /* 设置设备信息 */
    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "InvenSense";
    sensor->info.model      = "MPU6050";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_GYROSCOPE;
    sensor->info.unit       = SENSOR_UNIT_DEGREE_PER_SECOND;
    sensor->info.range_max  = 250;
    sensor->info.range_min  = -250;
    sensor->info.resolution = 16;
    sensor->info.max_odr    = 8000;
    sensor->info.flags      = SENSOR_FLAG_INT_SUPPORT | SENSOR_FLAG_CALIBRATION;

    sensor->ops       = &mpu6050_gyro_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 100;

    return sensor;
}