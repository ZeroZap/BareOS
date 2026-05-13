/**
 * @file sensor_lis2dw12.c
 * @brief ST LIS2DW12 低功耗3轴加速度计驱动实现
 */
#include "sensor_lis2dw12.h"
#include <string.h>

/* I2C读写接口 */
extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg, uint8_t *data, uint16_t len);

/* SPI读写接口 */
extern int hal_spi_recv(void *bus, uint8_t cs, uint8_t *data, uint16_t len);
extern int hal_spi_send(void *bus, uint8_t cs, uint8_t *data, uint16_t len);

/**
 * @brief 寄存器读写辅助函数
 */
static sensor_err_t lis2dw12_reg_read(sensor_device_t *sensor, uint8_t reg, uint8_t *data)
{
    lis2dw12_priv_t *priv = (lis2dw12_priv_t *)sensor->priv_data;
    if (priv->spi_cs != LIS2DW12_SPI_CS_NONE) {
        uint8_t tx = reg | 0x80;
        hal_spi_send(sensor->bus, priv->spi_cs, &tx, 1);
        return hal_spi_recv(sensor->bus, priv->spi_cs, data, 1);
    }
    return hal_i2c_mem_read(sensor->bus, priv->i2c_addr, reg, data, 1);
}

static sensor_err_t lis2dw12_reg_write(sensor_device_t *sensor, uint8_t reg, uint8_t data)
{
    lis2dw12_priv_t *priv = (lis2dw12_priv_t *)sensor->priv_data;
    if (priv->spi_cs != LIS2DW12_SPI_CS_NONE) {
        uint8_t tx[2] = { reg & 0x7F, data };
        return hal_spi_send(sensor->bus, priv->spi_cs, tx, 2);
    }
    return hal_i2c_mem_write(sensor->bus, priv->i2c_addr, reg, &data, 1);
}

/**
 * @brief LIS2DW12初始化
 */
static sensor_err_t lis2dw12_init(sensor_device_t *sensor)
{
    uint8_t data;

    SENSOR_LOG("Initializing LIS2DW12");

    /* 检查WHO_AM_I */
    if (lis2dw12_reg_read(sensor, LIS2DW12_REG_WHOAMI, &data) != SENSOR_EOK) {
        return SENSOR_EIO;
    }

    if (data != LIS2DW12_WHOAMI_VALUE) {
        SENSOR_LOG("Wrong WHO_AM_I: 0x%02X (expected 0x%02X)", data, LIS2DW12_WHOAMI_VALUE);
        return SENSOR_ERROR;
    }

    /* 软复位 */
    lis2dw12_reg_write(sensor, LIS2DW12_REG_CTRL2, 0x04);
    SENSOR_DELAY_MS(10);

    /* 配置: 低功耗模式, ±2g, 100Hz */
    data = (LIS2DW12_MODE_LOW_POWER << 5) | (LIS2DW12_RATE_100HZ << 2) | LIS2DW12_RANGE_2G;
    lis2dw12_reg_write(sensor, LIS2DW12_REG_CTRL1, data);

    ((lis2dw12_priv_t *)sensor->priv_data)->range = 2;
    ((lis2dw12_priv_t *)sensor->priv_data)->rate  = 100;
    ((lis2dw12_priv_t *)sensor->priv_data)->mode  = LIS2DW12_MODE_LOW_POWER;

    SENSOR_LOG("LIS2DW12 initialized (low-power mode, 100Hz, ±2g)");

    return SENSOR_EOK;
}

/**
 * @brief LIS2DW12反初始化
 */
static sensor_err_t lis2dw12_deinit(sensor_device_t *sensor)
{
    lis2dw12_reg_write(sensor, LIS2DW12_REG_CTRL1, LIS2DW12_RATE_POWER_DOWN << 2);
    return SENSOR_EOK;
}

/**
 * @brief 读取加速度数据
 */
static sensor_err_t lis2dw12_read(sensor_device_t *sensor, sensor_data_t *data)
{
    uint8_t buf[6];
    int16_t raw[3];

    /* 读取6字节数据 */
    for (int i = 0; i < 6; i++) {
        if (lis2dw12_reg_read(sensor, LIS2DW12_REG_OUT_X_L + i, &buf[i]) != SENSOR_EOK) {
            return SENSOR_EIO;
        }
    }

    /* 小端模式组合 */
    raw[0] = (int16_t)((buf[1] << 8) | buf[0]);
    raw[1] = (int16_t)((buf[3] << 8) | buf[2]);
    raw[2] = (int16_t)((buf[5] << 8) | buf[4]);

    /* 转换为mg */
    lis2dw12_priv_t *priv = (lis2dw12_priv_t *)sensor->priv_data;
    int32_t scale = 0;
    switch (priv->range) {
    case 2:  scale = 1; break;   /* 0.976 mg/LSB */
    case 4:  scale = 2; break;   /* 1.952 mg/LSB */
    case 8:  scale = 4; break;   /* 3.905 mg/LSB */
    case 16: scale = 8; break;  /* 7.810 mg/LSB */
    default: scale = 1;
    }

    data->type              = SENSOR_TYPE_ACCELEROMETER;
    data->unit              = SENSOR_UNIT_MILLI_G;
    data->value.val_3axis.x = (int32_t)raw[0] * scale;
    data->value.val_3axis.y = (int32_t)raw[1] * scale;
    data->value.val_3axis.z = (int32_t)raw[2] * scale;
    data->timestamp         = SENSOR_GET_TICK();
    data->accuracy          = 95;

    return SENSOR_EOK;
}

/* 操作接口 */
static const sensor_ops_t lis2dw12_ops = {
    .init   = lis2dw12_init,
    .deinit = lis2dw12_deinit,
    .read   = lis2dw12_read,
};

/**
 * @brief 创建LIS2DW12设备 (I2C)
 */
sensor_device_t *lis2dw12_create(const char *name, void *i2c_bus, uint8_t addr)
{
    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL) return NULL;

    lis2dw12_priv_t *priv = (lis2dw12_priv_t *)SENSOR_MALLOC(sizeof(lis2dw12_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(lis2dw12_priv_t));

    priv->i2c_addr = (addr != 0) ? addr : LIS2DW12_ADDR_DEFAULT;
    priv->spi_cs   = LIS2DW12_SPI_CS_NONE;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "STMicro";
    sensor->info.model      = "LIS2DW12";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit       = SENSOR_UNIT_MILLI_G;
    sensor->info.range_max  = 2000;
    sensor->info.range_min  = -2000;
    sensor->info.resolution = 14;
    sensor->info.max_odr   = 800;
    sensor->info.flags      = SENSOR_FLAG_LOW_POWER;

    sensor->ops       = &lis2dw12_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 100;

    return sensor;
}

/**
 * @brief 创建LIS2DW12设备 (SPI)
 */
sensor_device_t *lis2dw12_create_spi(const char *name, void *spi_bus, uint8_t cs)
{
    sensor_device_t *sensor = (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL) return NULL;

    lis2dw12_priv_t *priv = (lis2dw12_priv_t *)SENSOR_MALLOC(sizeof(lis2dw12_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(lis2dw12_priv_t));

    priv->spi_cs = cs;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "STMicro";
    sensor->info.model      = "LIS2DW12";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_ACCELEROMETER;
    sensor->info.unit       = SENSOR_UNIT_MILLI_G;
    sensor->info.range_max  = 2000;
    sensor->info.range_min  = -2000;
    sensor->info.resolution = 14;
    sensor->info.max_odr    = 800;
    sensor->info.flags      = SENSOR_FLAG_LOW_POWER;

    sensor->ops       = &lis2dw12_ops;
    sensor->bus       = spi_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 100;

    return sensor;
}

/**
 * @brief 设置量程
 */
int lis2dw12_set_range(sensor_device_t *dev, uint8_t range)
{
    lis2dw12_priv_t *priv = (lis2dw12_priv_t *)dev->priv_data;
    uint8_t ctrl1;

    lis2dw12_reg_read(dev, LIS2DW12_REG_CTRL1, &ctrl1);
    ctrl1 = (ctrl1 & 0xFC) | (range & 0x03);
    lis2dw12_reg_write(dev, LIS2DW12_REG_CTRL1, ctrl1);

    priv->range = (range == LIS2DW12_RANGE_2G)  ? 2 :
                  (range == LIS2DW12_RANGE_4G)  ? 4 :
                  (range == LIS2DW12_RANGE_8G)  ? 8 : 16;
    return 0;
}

/**
 * @brief 设置采样率
 */
int lis2dw12_set_rate(sensor_device_t *dev, uint8_t rate)
{
    lis2dw12_priv_t *priv = (lis2dw12_priv_t *)dev->priv_data;
    uint8_t ctrl1;

    lis2dw12_reg_read(dev, LIS2DW12_REG_CTRL1, &ctrl1);
    ctrl1 = (ctrl1 & 0xC3) | ((rate & 0x0F) << 2);
    lis2dw12_reg_write(dev, LIS2DW12_REG_CTRL1, ctrl1);

    /* 记录实际ODR */
    static const uint8_t odr_map[] = {0, 2, 12, 25, 50, 100, 200, 400, 800};
    priv->rate = (rate < 9) ? odr_map[rate] : 100;

    return 0;
}

/**
 * @brief 设置工作模式
 */
int lis2dw12_set_mode(sensor_device_t *dev, uint8_t mode)
{
    lis2dw12_priv_t *priv = (lis2dw12_priv_t *)dev->priv_data;
    uint8_t ctrl1;

    lis2dw12_reg_read(dev, LIS2DW12_REG_CTRL1, &ctrl1);
    ctrl1 = (ctrl1 & 0x1F) | ((mode & 0x03) << 5);
    lis2dw12_reg_write(dev, LIS2DW12_REG_CTRL1, ctrl1);

    priv->mode = mode;
    return 0;
}

/**
 * @brief 使能高通滤波器
 */
int lis2dw12_enable_high_pass(sensor_device_t *dev, uint8_t enable)
{
    uint8_t ctrl5;
    lis2dw12_reg_read(dev, LIS2DW12_REG_CTRL5, &ctrl5);
    ctrl5 = enable ? (ctrl5 | 0x40) : (ctrl5 & 0xBF);
    lis2dw12_reg_write(dev, LIS2DW12_REG_CTRL5, ctrl5);
    return 0;
}
