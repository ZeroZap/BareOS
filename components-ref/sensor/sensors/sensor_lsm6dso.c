/**
 * @file sensor_lsm6dso.c
 * @brief ST LSM6DSO 6轴惯性传感器驱动实现
 * @description I2C/SPI 双接口，支持加速度计+陀螺仪
 */
#include "sensor_lsm6dso.h"
#include <string.h>

/* I2C读写接口 */
extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data,
                            uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg,
                             uint8_t *data, uint16_t len);

/* SPI读写接口 */
extern int hal_spi_recv(void *bus, uint8_t cs, uint8_t *data, uint16_t len);
extern int hal_spi_send(void *bus, uint8_t cs, uint8_t *data, uint16_t len);

/**
 * @brief I2C单字节读取
 */
static sensor_err_t lsm6dso_i2c_read(sensor_device_t *sensor, uint8_t reg,
                                    uint8_t *data)
{
    return hal_i2c_mem_read(sensor->bus, ((lsm6dso_priv_t *)sensor->priv_data)->i2c_addr,
                            reg, data, 1);
}

/**
 * @brief I2C单字节写入
 */
static sensor_err_t lsm6dso_i2c_write(sensor_device_t *sensor, uint8_t reg,
                                      uint8_t data)
{
    return hal_i2c_mem_write(sensor->bus, ((lsm6dso_priv_t *)sensor->priv_data)->i2c_addr,
                             reg, &data, 1);
}

/**
 * @brief SPI单字节读取
 */
static sensor_err_t lsm6dso_spi_read(sensor_device_t *sensor, uint8_t reg,
                                    uint8_t *data)
{
    uint8_t tx_data = reg | 0x80;  /* 读命令 */
    uint8_t rx_data;
    
    if (hal_spi_send(sensor->bus, ((lsm6dso_priv_t *)sensor->priv_data)->spi_cs,
                     &tx_data, 1) != 0) {
        return SENSOR_EIO;
    }
    if (hal_spi_recv(sensor->bus, ((lsm6dso_priv_t *)sensor->priv_data)->spi_cs,
                     &rx_data, 1) != 0) {
        return SENSOR_EIO;
    }
    *data = rx_data;
    return SENSOR_EOK;
}

/**
 * @brief SPI单字节写入
 */
static sensor_err_t lsm6dso_spi_write(sensor_device_t *sensor, uint8_t reg,
                                      uint8_t data)
{
    uint8_t tx_data[2] = { reg & 0x7F, data };  /* 写命令 */
    return hal_spi_send(sensor->bus, ((lsm6dso_priv_t *)sensor->priv_data)->spi_cs,
                        tx_data, 2);
}

/**
 * @brief 根据接口类型选择读写函数
 */
static inline sensor_err_t lsm6dso_reg_read(sensor_device_t *sensor,
                                            uint8_t reg, uint8_t *data)
{
    lsm6dso_priv_t *priv = (lsm6dso_priv_t *)sensor->priv_data;
    if (priv->spi_cs != LSM6DSO_SPI_CS_NONE) {
        return lsm6dso_spi_read(sensor, reg, data);
    }
    return lsm6dso_i2c_read(sensor, reg, data);
}

static inline sensor_err_t lsm6dso_reg_write(sensor_device_t *sensor,
                                             uint8_t reg, uint8_t data)
{
    lsm6dso_priv_t *priv = (lsm6dso_priv_t *)sensor->priv_data;
    if (priv->spi_cs != LSM6DSO_SPI_CS_NONE) {
        return lsm6dso_spi_write(sensor, reg, data);
    }
    return lsm6dso_i2c_write(sensor, reg, data);
}

/**
 * @brief LSM6DSO初始化
 */
static sensor_err_t lsm6dso_init(sensor_device_t *sensor)
{
    uint8_t data;

    SENSOR_LOG("Initializing LSM6DSO");

    /* 检查WHO_AM_I */
    if (lsm6dso_reg_read(sensor, LSM6DSO_REG_WHOAMI, &data) != SENSOR_EOK) {
        return SENSOR_EIO;
    }

    if (data != LSM6DSO_WHOAMI_VALUE) {
        SENSOR_LOG("Wrong WHO_AM_I: 0x%02X (expected 0x%02X)", data,
                   LSM6DSO_WHOAMI_VALUE);
        return SENSOR_ERROR;
    }

    /* 复位设备 */
    lsm6dso_reg_write(sensor, LSM6DSO_REG_CTRL3_C, 0x01);
    SENSOR_DELAY_MS(10);

    /* 关闭I3C接口 */
    data = 0x00;
    lsm6dso_reg_write(sensor, LSM6DSO_REG_CTRL4_C, data);

    /* 配置加速度计: ±2g, 104Hz */
    data = (LSM6DSO_ACCEL_RATE_104Hz << 4) | LSM6DSO_ACCEL_RANGE_2G;
    lsm6dso_reg_write(sensor, LSM6DSO_REG_CTRL1_XL, data);
    ((lsm6dso_priv_t *)sensor->priv_data)->accel_range = 2;
    ((lsm6dso_priv_t *)sensor->priv_data)->accel_rate  = 104;

    /* 配置陀螺仪: ±250°/s, 104Hz */
    data = (LSM6DSO_GYRO_RATE_104Hz << 4) | LSM6DSO_GYRO_RANGE_250DPS;
    lsm6dso_reg_write(sensor, LSM6DSO_REG_CTRL2_G, data);
    ((lsm6dso_priv_t *)sensor->priv_data)->gyro_range = 250;
    ((lsm6dso_priv_t *)sensor->priv_data)->gyro_rate   = 104;

    SENSOR_LOG("LSM6DSO initialized successfully");

    return SENSOR_EOK;
}

/**
 * @brief LSM6DSO反初始化
 */
static sensor_err_t lsm6dso_deinit(sensor_device_t *sensor)
{
    /* 进入关闭模式 */
    lsm6dso_reg_write(sensor, LSM6DSO_REG_CTRL1_XL, 0x00);
    lsm6dso_reg_write(sensor, LSM6DSO_REG_CTRL2_G, 0x00);

    return SENSOR_EOK;
}

/**
 * @brief 读取加速度数据
 */
static sensor_err_t lsm6dso_accel_read(sensor_device_t *sensor,
                                       sensor_data_t *data)
{
    uint8_t buf[6];
    int16_t raw[3];

    /* 读取6字节加速度数据 */
    if (lsm6dso_reg_read(sensor, LSM6DSO_REG_OUTX_L_XL, &buf[0]) != SENSOR_EOK ||
        lsm6dso_reg_read(sensor, LSM6DSO_REG_OUTX_L_XL + 1, &buf[1]) != SENSOR_EOK ||
        lsm6dso_reg_read(sensor, LSM6DSO_REG_OUTX_L_XL + 2, &buf[2]) != SENSOR_EOK ||
        lsm6dso_reg_read(sensor, LSM6DSO_REG_OUTX_L_XL + 3, &buf[3]) != SENSOR_EOK ||
        lsm6dso_reg_read(sensor, LSM6DSO_REG_OUTX_L_XL + 4, &buf[4]) != SENSOR_EOK ||
        lsm6dso_reg_read(sensor, LSM6DSO_REG_OUTX_L_XL + 5, &buf[5]) != SENSOR_EOK) {
        return SENSOR_EIO;
    }

    /* 组合数据 (小端模式) */
    raw[0] = (int16_t)((buf[1] << 8) | buf[0]);
    raw[1] = (int16_t)((buf[3] << 8) | buf[2]);
    raw[2] = (int16_t)((buf[5] << 8) | buf[4]);

    /* 转换为mg */
    lsm6dso_priv_t *priv = (lsm6dso_priv_t *)sensor->priv_data;
    int32_t scale        = 0;
    switch (priv->accel_range) {
    case 2:
        scale = 61;   /* 2g: 61ug/LSB */
        break;
    case 4:
        scale = 122;
        break;
    case 8:
        scale = 244;
        break;
    case 16:
        scale = 488;
        break;
    default:
        scale = 61;
    }

    data->type              = SENSOR_TYPE_ACCELEROMETER;
    data->unit              = SENSOR_UNIT_MILLI_G;
    data->value.val_3axis.x = (int32_t)raw[0] * scale / 1000;
    data->value.val_3axis.y = (int32_t)raw[1] * scale / 1000;
    data->value.val_3axis.z = (int32_t)raw[2] * scale / 1000;
    data->timestamp         = SENSOR_GET_TICK();
    data->accuracy          = 95;

    return SENSOR_EOK;
}

/**
 * @brief 读取陀螺仪数据
 */
static sensor_err_t lsm6dso_gyro_read(sensor_device_t *sensor,
                                      sensor_data_t *data)
{
    uint8_t buf[6];
    int16_t raw[3];

    /* 读取6字节陀螺仪数据 */
    if (lsm6dso_reg_read(sensor, LSM6DSO_REG_OUTX_L_G, &buf[0]) != SENSOR_EOK ||
        lsm6dso_reg_read(sensor, LSM6DSO_REG_OUTX_L_G + 1, &buf[1]) != SENSOR_EOK ||
        lsm6dso_reg_read(sensor, LSM6DSO_REG_OUTX_L_G + 2, &buf[2]) != SENSOR_EOK ||
        lsm6dso_reg_read(sensor, LSM6DSO_REG_OUTX_L_G + 3, &buf[3]) != SENSOR_EOK ||
        lsm6dso_reg_read(sensor, LSM6DSO_REG_OUTX_L_G + 4, &buf[4]) != SENSOR_EOK ||
        lsm6dso_reg_read(sensor, LSM6DSO_REG_OUTX_L_G + 5, &buf[5]) != SENSOR_EOK) {
        return SENSOR_EIO;
    }

    /* 组合数据 */
    raw[0] = (int16_t)((buf[1] << 8) | buf[0]);
    raw[1] = (int16_t)((buf[3] << 8) | buf[2]);
    raw[2] = (int16_t)((buf[5] << 8) | buf[4]);

    /* 转换为°/s */
    lsm6dso_priv_t *priv = (lsm6dso_priv_t *)sensor->priv_data;
    int32_t scale        = 0;
    switch (priv->gyro_range) {
    case 125:
        scale = 4;    /* 4.375 mdps/LSB */
        break;
    case 250:
        scale = 9;    /* 8.75 mdps/LSB */
        break;
    case 500:
        scale = 17;
        break;
    case 1000:
        scale = 35;
        break;
    case 2000:
        scale = 70;
        break;
    default:
        scale = 9;
    }

    data->type              = SENSOR_TYPE_GYROSCOPE;
    data->unit              = SENSOR_UNIT_DEGREE_PER_SECOND;
    data->value.val_3axis.x = (int32_t)raw[0] * scale / 1000;
    data->value.val_3axis.y = (int32_t)raw[1] * scale / 1000;
    data->value.val_3axis.z = (int32_t)raw[2] * scale / 1000;
    data->timestamp         = SENSOR_GET_TICK();
    data->accuracy          = 95;

    return SENSOR_EOK;
}

/* 加速度计操作接口 */
static const sensor_ops_t lsm6dso_accel_ops = {
    .init   = lsm6dso_init,
    .deinit = lsm6dso_deinit,
    .read   = lsm6dso_accel_read,
};

/* 陀螺仪操作接口 */
static const sensor_ops_t lsm6dso_gyro_ops = {
    .init   = lsm6dso_init,
    .deinit = lsm6dso_deinit,
    .read   = lsm6dso_gyro_read,
};

/**
 * @brief 创建LSM6DSO加速度计设备 (I2C)
 */
sensor_device_t *lsm6dso_create_accel(const char *name, void *i2c_bus,
                                      uint8_t addr)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL) {
        return NULL;
    }

    lsm6dso_priv_t *priv =
        (lsm6dso_priv_t *)SENSOR_MALLOC(sizeof(lsm6dso_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(lsm6dso_priv_t));

    /* 初始化私有数据 */
    priv->i2c_addr = (addr != 0) ? addr : LSM6DSO_ADDR_DEFAULT;
    priv->spi_cs   = LSM6DSO_SPI_CS_NONE;

    /* 设置设备信息 */
    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "STMicro";
    sensor->info.model      = "LSM6DSO";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit       = SENSOR_UNIT_MILLI_G;
    sensor->info.range_max  = 2000;
    sensor->info.range_min  = -2000;
    sensor->info.resolution = 16;
    sensor->info.max_odr    = 6660;
    sensor->info.flags      = SENSOR_FLAG_INT_SUPPORT | SENSOR_FLAG_CALIBRATION;

    sensor->ops       = &lsm6dso_accel_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 104;

    return sensor;
}

/**
 * @brief 创建LSM6DSO陀螺仪设备 (I2C)
 */
sensor_device_t *lsm6dso_create_gyro(const char *name, void *i2c_bus,
                                     uint8_t addr)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL) {
        return NULL;
    }

    lsm6dso_priv_t *priv =
        (lsm6dso_priv_t *)SENSOR_MALLOC(sizeof(lsm6dso_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(lsm6dso_priv_t));

    /* 初始化私有数据 */
    priv->i2c_addr = (addr != 0) ? addr : LSM6DSO_ADDR_DEFAULT;
    priv->spi_cs   = LSM6DSO_SPI_CS_NONE;

    /* 设置设备信息 */
    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "STMicro";
    sensor->info.model      = "LSM6DSO";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_GYROSCOPE;
    sensor->info.unit       = SENSOR_UNIT_DEGREE_PER_SECOND;
    sensor->info.range_max  = 250;
    sensor->info.range_min  = -250;
    sensor->info.resolution = 16;
    sensor->info.max_odr    = 6660;
    sensor->info.flags      = SENSOR_FLAG_INT_SUPPORT | SENSOR_FLAG_CALIBRATION;

    sensor->ops       = &lsm6dso_gyro_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 104;

    return sensor;
}

/**
 * @brief 创建LSM6DSO IMU设备 (I2C)
 */
sensor_device_t *lsm6dso_create_imu(const char *name, void *i2c_bus, uint8_t addr)
{
    /* IMU本质上是同一个物理设备,这里返回加速度计 */
    return lsm6dso_create_accel(name, i2c_bus, addr);
}

/**
 * @brief 创建LSM6DSO加速度计设备 (SPI)
 */
sensor_device_t *lsm6dso_create_spi_accel(const char *name, void *spi_bus,
                                          uint8_t cs)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL) {
        return NULL;
    }

    lsm6dso_priv_t *priv =
        (lsm6dso_priv_t *)SENSOR_MALLOC(sizeof(lsm6dso_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(lsm6dso_priv_t));

    priv->spi_cs = cs;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "STMicro";
    sensor->info.model      = "LSM6DSO";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit       = SENSOR_UNIT_MILLI_G;
    sensor->info.range_max  = 2000;
    sensor->info.range_min  = -2000;
    sensor->info.resolution = 16;
    sensor->info.max_odr    = 6660;
    sensor->info.flags      = SENSOR_FLAG_INT_SUPPORT | SENSOR_FLAG_CALIBRATION;

    sensor->ops       = &lsm6dso_accel_ops;
    sensor->bus       = spi_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 104;

    return sensor;
}

/**
 * @brief 创建LSM6DSO陀螺仪设备 (SPI)
 */
sensor_device_t *lsm6dso_create_spi_gyro(const char *name, void *spi_bus,
                                         uint8_t cs)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL) {
        return NULL;
    }

    lsm6dso_priv_t *priv =
        (lsm6dso_priv_t *)SENSOR_MALLOC(sizeof(lsm6dso_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(lsm6dso_priv_t));

    priv->spi_cs = cs;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "STMicro";
    sensor->info.model      = "LSM6DSO";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_GYROSCOPE;
    sensor->info.unit       = SENSOR_UNIT_DEGREE_PER_SECOND;
    sensor->info.range_max  = 250;
    sensor->info.range_min  = -250;
    sensor->info.resolution = 16;
    sensor->info.max_odr    = 6660;
    sensor->info.flags      = SENSOR_FLAG_INT_SUPPORT | SENSOR_FLAG_CALIBRATION;

    sensor->ops       = &lsm6dso_gyro_ops;
    sensor->bus       = spi_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 104;

    return sensor;
}

/**
 * @brief 创建LSM6DSO IMU设备 (SPI)
 */
sensor_device_t *lsm6dso_create_spi_imu(const char *name, void *spi_bus,
                                        uint8_t cs)
{
    return lsm6dso_create_spi_accel(name, spi_bus, cs);
}

/**
 * @brief 设置加速度量程
 */
int lsm6dso_set_accel_range(sensor_device_t *dev, uint8_t range)
{
    lsm6dso_priv_t *priv = (lsm6dso_priv_t *)dev->priv_data;
    uint8_t ctrl1       = 0;

    lsm6dso_reg_read(dev, LSM6DSO_REG_CTRL1_XL, &ctrl1);
    ctrl1 &= 0x0F;
    ctrl1 |= (range << 4);
    lsm6dso_reg_write(dev, LSM6DSO_REG_CTRL1_XL, ctrl1);

    priv->accel_range = range;

    return 0;
}

/**
 * @brief 设置陀螺仪量程
 */
int lsm6dso_set_gyro_range(sensor_device_t *dev, uint8_t range)
{
    lsm6dso_priv_t *priv = (lsm6dso_priv_t *)dev->priv_data;
    uint8_t ctrl2       = 0;

    lsm6dso_reg_read(dev, LSM6DSO_REG_CTRL2_G, &ctrl2);
    ctrl2 &= 0x0F;
    ctrl2 |= (range << 4);
    lsm6dso_reg_write(dev, LSM6DSO_REG_CTRL2_G, ctrl2);

    priv->gyro_range = range;

    return 0;
}

/**
 * @brief 设置加速度采样率
 */
int lsm6dso_set_accel_rate(sensor_device_t *dev, uint8_t rate)
{
    lsm6dso_priv_t *priv = (lsm6dso_priv_t *)dev->priv_data;
    uint8_t ctrl1       = 0;

    lsm6dso_reg_read(dev, LSM6DSO_REG_CTRL1_XL, &ctrl1);
    ctrl1 &= 0xF0;
    ctrl1 |= (rate & 0x0F);
    lsm6dso_reg_write(dev, LSM6DSO_REG_CTRL1_XL, ctrl1);

    priv->accel_rate = rate;

    return 0;
}

/**
 * @brief 设置陀螺仪采样率
 */
int lsm6dso_set_gyro_rate(sensor_device_t *dev, uint8_t rate)
{
    lsm6dso_priv_t *priv = (lsm6dso_priv_t *)dev->priv_data;
    uint8_t ctrl2       = 0;

    lsm6dso_reg_read(dev, LSM6DSO_REG_CTRL2_G, &ctrl2);
    ctrl2 &= 0xF0;
    ctrl2 |= (rate & 0x0F);
    lsm6dso_reg_write(dev, LSM6DSO_REG_CTRL2_G, ctrl2);

    priv->gyro_rate = rate;

    return 0;
}
