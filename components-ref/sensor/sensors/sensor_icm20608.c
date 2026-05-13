#include "sensor_icm20608.h"

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data,
                            uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg,
                             uint8_t *data, uint16_t len);
extern int hal_spi_read_reg(void *bus, uint8_t reg, uint8_t *data,
                            uint16_t len);
extern int hal_spi_write_reg(void *bus, uint8_t reg, uint8_t *data,
                             uint16_t len);

/**
 * @brief 读寄存器
 */
static int icm20608_read_reg(sensor_device_t *sensor, uint8_t reg,
                             uint8_t *data, uint16_t len)
{
    icm20608_priv_t *priv = (icm20608_priv_t *)sensor->priv_data;

    if (priv->use_spi) {
        return hal_spi_read_reg(sensor->bus, reg, data, len);
    } else {
        return hal_i2c_mem_read(sensor->bus, priv->addr, reg, data, len);
    }
}

/**
 * @brief 写寄存器
 */
static int icm20608_write_reg(sensor_device_t *sensor, uint8_t reg,
                              uint8_t *data, uint16_t len)
{
    icm20608_priv_t *priv = (icm20608_priv_t *)sensor->priv_data;

    if (priv->use_spi) {
        return hal_spi_write_reg(sensor->bus, reg, data, len);
    } else {
        return hal_i2c_mem_write(sensor->bus, priv->addr, reg, data, len);
    }
}

/**
 * @brief ICM20608初始化
 */
static sensor_err_t icm20608_init(sensor_device_t *sensor)
{
    uint8_t data;

    SENSOR_LOG("Initializing ICM20608");

    /* 检查WHO_AM_I */
    if (icm20608_read_reg(sensor, ICM20608_REG_WHOAMI, &data, 1) != 0) {
        return SENSOR_EIO;
    }

    if (data != ICM20608_WHOAMI_VALUE) {
        SENSOR_LOG("Wrong WHO_AM_I: 0x%02X", data);
        return SENSOR_ERROR;
    }

    /* 复位设备 */
    data = 0x80;
    icm20608_write_reg(sensor, ICM20608_REG_PWR_MGMT_1, &data, 1);
    SENSOR_DELAY_MS(100);

    /* 唤醒设备，选择最佳时钟源 */
    data = 0x01;
    if (icm20608_write_reg(sensor, ICM20608_REG_PWR_MGMT_1, &data, 1) != 0) {
        return SENSOR_EIO;
    }

    /* 使能加速度计和陀螺仪 */
    data = 0x00;
    icm20608_write_reg(sensor, ICM20608_REG_PWR_MGMT_2, &data, 1);

    /* 配置陀螺仪: ±500°/s */
    data = 0x08;
    icm20608_write_reg(sensor, ICM20608_REG_GYRO_CONFIG, &data, 1);

    /* 配置加速度计: ±4g */
    data = 0x08;
    icm20608_write_reg(sensor, ICM20608_REG_ACCEL_CONFIG, &data, 1);

    /* 配置低通滤波器 */
    data = 0x04; /* 20Hz */
    icm20608_write_reg(sensor, ICM20608_REG_CONFIG, &data, 1);

    data = 0x04;
    icm20608_write_reg(sensor, ICM20608_REG_ACCEL_CONFIG2, &data, 1);

    SENSOR_LOG("ICM20608 initialized successfully");

    return SENSOR_EOK;
}

/**
 * @brief ICM20608反初始化
 */
static sensor_err_t icm20608_deinit(sensor_device_t *sensor)
{
    uint8_t data = 0x40; /* 睡眠模式 */

    icm20608_write_reg(sensor, ICM20608_REG_PWR_MGMT_1, &data, 1);

    return SENSOR_EOK;
}

/**
 * @brief 读取加速度数据
 */
static sensor_err_t icm20608_accel_read(sensor_device_t *sensor,
                                        sensor_data_t *data)
{
    icm20608_priv_t *priv = (icm20608_priv_t *)sensor->priv_data;
    uint8_t buf[6];
    int16_t raw[3];

    /* 读取6字节加速度数据 */
    if (icm20608_read_reg(sensor, ICM20608_REG_ACCEL_XOUT_H, buf, 6) != 0) {
        return SENSOR_EIO;
    }

    /* 组合数据 */
    raw[0] = (int16_t)((buf[0] << 8) | buf[1]);
    raw[1] = (int16_t)((buf[2] << 8) | buf[3]);
    raw[2] = (int16_t)((buf[4] << 8) | buf[5]);

    /* 转换为mg (±4g范围) */
    data->type              = SENSOR_TYPE_ACCELEROMETER;
    data->unit              = SENSOR_UNIT_MILLI_G;
    data->value.val_3axis.x = (int32_t)raw[0] * 4000 / 32768;
    data->value.val_3axis.y = (int32_t)raw[1] * 4000 / 32768;
    data->value.val_3axis.z = (int32_t)raw[2] * 4000 / 32768;
    data->timestamp         = SENSOR_GET_TICK();
    data->accuracy          = 95;

    return SENSOR_EOK;
}

/**
 * @brief 读取陀螺仪数据
 */
static sensor_err_t icm20608_gyro_read(sensor_device_t *sensor,
                                       sensor_data_t *data)
{
    icm20608_priv_t *priv = (icm20608_priv_t *)sensor->priv_data;
    uint8_t buf[6];
    int16_t raw[3];

    /* 读取6字节陀螺仪数据 */
    if (icm20608_read_reg(sensor, ICM20608_REG_GYRO_XOUT_H, buf, 6) != 0) {
        return SENSOR_EIO;
    }

    /* 组合数据 */
    raw[0] = (int16_t)((buf[0] << 8) | buf[1]);
    raw[1] = (int16_t)((buf[2] << 8) | buf[3]);
    raw[2] = (int16_t)((buf[4] << 8) | buf[5]);

    /* 转换为°/s (±500°/s范围) */
    data->type              = SENSOR_TYPE_GYROSCOPE;
    data->unit              = SENSOR_UNIT_DEGREE_PER_SECOND;
    data->value.val_3axis.x = (int32_t)raw[0] * 500 / 32768;
    data->value.val_3axis.y = (int32_t)raw[1] * 500 / 32768;
    data->value.val_3axis.z = (int32_t)raw[2] * 500 / 32768;
    data->timestamp         = SENSOR_GET_TICK();
    data->accuracy          = 95;

    return SENSOR_EOK;
}

/**
 * @brief 读取温度数据
 */
static sensor_err_t icm20608_temp_read(sensor_device_t *sensor,
                                       sensor_data_t *data)
{
    uint8_t buf[2];
    int16_t raw;

    /* 读取2字节温度数据 */
    if (icm20608_read_reg(sensor, ICM20608_REG_TEMP_OUT_H, buf, 2) != 0) {
        return SENSOR_EIO;
    }

    raw = (int16_t)((buf[0] << 8) | buf[1]);

    /* 转换为°C */
    data->type = SENSOR_TYPE_TEMPERATURE;
    data->unit = SENSOR_UNIT_CELSIUS;

#if SENSOR_USE_FLOAT
    float temperature     = (raw / 326.8f) + 25.0f;
    data->value.val_float = temperature;
#else
    /* 使用整数运算 (0.01°C) */
    int32_t temperature   = ((int64_t)raw * 100 * 10 / 3268) + 2500;
    data->value.val_int32 = temperature;
#endif

    data->timestamp = SENSOR_GET_TICK();
    data->accuracy  = 90;

    return SENSOR_EOK;
}

/* 加速度计操作接口 */
static const sensor_ops_t icm20608_accel_ops = {
    .init   = icm20608_init,
    .deinit = icm20608_deinit,
    .read   = icm20608_accel_read,
};

/* 陀螺仪操作接口 */
static const sensor_ops_t icm20608_gyro_ops = {
    .init   = icm20608_init,
    .deinit = icm20608_deinit,
    .read   = icm20608_gyro_read,
};

/* 温度传感器操作接口 */
static const sensor_ops_t icm20608_temp_ops = {
    .init   = icm20608_init,
    .deinit = icm20608_deinit,
    .read   = icm20608_temp_read,
};

/**
 * @brief 创建ICM20608加速度计
 */
sensor_device_t *icm20608_create_accel(const char *name, void *bus,
                                       bool use_spi)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL) {
        return NULL;
    }

    icm20608_priv_t *priv =
        (icm20608_priv_t *)SENSOR_MALLOC(sizeof(icm20608_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(icm20608_priv_t));

    priv->addr        = ICM20608_ADDR_DEFAULT;
    priv->use_spi     = use_spi;
    priv->accel_range = 4;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "TDK InvenSense";
    sensor->info.model      = "ICM20608";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit       = SENSOR_UNIT_MILLI_G;
    sensor->info.range_max  = 4000;
    sensor->info.range_min  = -4000;
    sensor->info.resolution = 16;
    sensor->info.max_odr    = 1000;
    sensor->info.flags = SENSOR_FLAG_INT_SUPPORT | SENSOR_FLAG_FIFO_SUPPORT;

    sensor->ops       = &icm20608_accel_ops;
    sensor->bus       = bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 100;

    return sensor;
}

/**
 * @brief 创建ICM20608陀螺仪
 */
sensor_device_t *icm20608_create_gyro(const char *name, void *bus, bool use_spi)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL) {
        return NULL;
    }

    icm20608_priv_t *priv =
        (icm20608_priv_t *)SENSOR_MALLOC(sizeof(icm20608_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(icm20608_priv_t));

    priv->addr       = ICM20608_ADDR_DEFAULT;
    priv->use_spi    = use_spi;
    priv->gyro_range = 500;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "TDK InvenSense";
    sensor->info.model      = "ICM20608";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_GYROSCOPE;
    sensor->info.unit       = SENSOR_UNIT_DEGREE_PER_SECOND;
    sensor->info.range_max  = 500;
    sensor->info.range_min  = -500;
    sensor->info.resolution = 16;
    sensor->info.max_odr    = 1000;
    sensor->info.flags = SENSOR_FLAG_INT_SUPPORT | SENSOR_FLAG_FIFO_SUPPORT;

    sensor->ops       = &icm20608_gyro_ops;
    sensor->bus       = bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 100;

    return sensor;
}

/**
 * @brief 创建ICM20608温度传感器
 */
sensor_device_t *icm20608_create_temp(const char *name, void *bus, bool use_spi)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL) {
        return NULL;
    }

    icm20608_priv_t *priv =
        (icm20608_priv_t *)SENSOR_MALLOC(sizeof(icm20608_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(icm20608_priv_t));

    priv->addr    = ICM20608_ADDR_DEFAULT;
    priv->use_spi = use_spi;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "TDK InvenSense";
    sensor->info.model      = "ICM20608";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_TEMPERATURE;
    sensor->info.unit       = SENSOR_UNIT_CELSIUS;
    sensor->info.range_max  = 85;
    sensor->info.range_min  = -40;
    sensor->info.resolution = 16;
    sensor->info.max_odr    = 1000;
    sensor->info.flags      = 0;

    sensor->ops       = &icm20608_temp_ops;
    sensor->bus       = bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 10;

    return sensor;
}