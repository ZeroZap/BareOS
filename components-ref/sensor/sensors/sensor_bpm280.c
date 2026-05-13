#include "sensor_bmp280.h"

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data,
                            uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg,
                             uint8_t *data, uint16_t len);

/**
 * @brief 读取校准参数
 */
static sensor_err_t bmp280_read_calibration(sensor_device_t *sensor)
{
    bmp280_priv_t *priv = (bmp280_priv_t *)sensor->priv_data;
    uint8_t calib[24];

    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, BMP280_REG_CALIB00, calib, 24)
        != 0) {
        return SENSOR_EIO;
    }

    priv->dig_T1 = (uint16_t)(calib[1] << 8 | calib[0]);
    priv->dig_T2 = (int16_t)(calib[3] << 8 | calib[2]);
    priv->dig_T3 = (int16_t)(calib[5] << 8 | calib[4]);

    priv->dig_P1 = (uint16_t)(calib[7] << 8 | calib[6]);
    priv->dig_P2 = (int16_t)(calib[9] << 8 | calib[8]);
    priv->dig_P3 = (int16_t)(calib[11] << 8 | calib[10]);
    priv->dig_P4 = (int16_t)(calib[13] << 8 | calib[12]);
    priv->dig_P5 = (int16_t)(calib[15] << 8 | calib[14]);
    priv->dig_P6 = (int16_t)(calib[17] << 8 | calib[16]);
    priv->dig_P7 = (int16_t)(calib[19] << 8 | calib[18]);
    priv->dig_P8 = (int16_t)(calib[21] << 8 | calib[20]);
    priv->dig_P9 = (int16_t)(calib[23] << 8 | calib[22]);

    return SENSOR_EOK;
}

/**
 * @brief BMP280初始化
 */
static sensor_err_t bmp280_init(sensor_device_t *sensor)
{
    bmp280_priv_t *priv = (bmp280_priv_t *)sensor->priv_data;
    uint8_t data;

    SENSOR_LOG("Initializing BMP280");

    /* 检查CHIP_ID */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, BMP280_REG_CHIP_ID, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }

    if (data != BMP280_CHIP_ID) {
        SENSOR_LOG("Wrong CHIP_ID: 0x%02X", data);
        return SENSOR_ERROR;
    }

    /* 软复位 */
    data = 0xB6;
    hal_i2c_mem_write(sensor->bus, priv->i2c_addr, BMP280_REG_RESET, &data, 1);
    SENSOR_DELAY_MS(10);

    /* 读取校准参数 */
    if (bmp280_read_calibration(sensor) != SENSOR_EOK) {
        return SENSOR_ERROR;
    }

    /* 配置: standby 0.5ms, filter off, SPI disable */
    data = 0x00;
    hal_i2c_mem_write(sensor->bus, priv->i2c_addr, BMP280_REG_CONFIG, &data, 1);

    /* 配置测量: osrs_t=1, osrs_p=1, normal mode */
    data = 0x27;
    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, BMP280_REG_CTRL_MEAS, &data, 1);

    SENSOR_LOG("BMP280 initialized successfully");

    return SENSOR_EOK;
}

/**
 * @brief BMP280反初始化
 */
static sensor_err_t bmp280_deinit(sensor_device_t *sensor)
{
    bmp280_priv_t *priv = (bmp280_priv_t *)sensor->priv_data;
    uint8_t data        = 0x00; /* sleep mode */

    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, BMP280_REG_CTRL_MEAS, &data, 1);

    return SENSOR_EOK;
}

/**
 * @brief 温度补偿计算
 */
static int32_t bmp280_compensate_temperature(bmp280_priv_t *priv, int32_t adc_T)
{
    int32_t var1, var2, T;

    var1 = ((((adc_T >> 3) - ((int32_t)priv->dig_T1 << 1)))
            * ((int32_t)priv->dig_T2))
           >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)priv->dig_T1))
              * ((adc_T >> 4) - ((int32_t)priv->dig_T1)))
             >> 12)
            * ((int32_t)priv->dig_T3))
           >> 14;

    priv->t_fine = var1 + var2;
    T            = (priv->t_fine * 5 + 128) >> 8;

    return T;
}

/**
 * @brief 气压补偿计算
 */
static uint32_t bmp280_compensate_pressure(bmp280_priv_t *priv, int32_t adc_P)
{
    int64_t var1, var2, p;

    var1 = ((int64_t)priv->t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)priv->dig_P6;
    var2 = var2 + ((var1 * (int64_t)priv->dig_P5) << 17);
    var2 = var2 + (((int64_t)priv->dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)priv->dig_P3) >> 8)
           + ((var1 * (int64_t)priv->dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)priv->dig_P1) >> 33;

    if (var1 == 0) {
        return 0;
    }

    p    = 1048576 - adc_P;
    p    = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)priv->dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)priv->dig_P8) * p) >> 19;
    p    = ((p + var1 + var2) >> 8) + (((int64_t)priv->dig_P7) << 4);

    return (uint32_t)p;
}

/**
 * @brief 读取原始数据
 */
static sensor_err_t bmp280_read_raw(sensor_device_t *sensor, int32_t *adc_T,
                                    int32_t *adc_P)
{
    bmp280_priv_t *priv = (bmp280_priv_t *)sensor->priv_data;
    uint8_t data[6];

    /* 读取温度和气压原始数据 */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, BMP280_REG_PRESS_MSB, data, 6)
        != 0) {
        return SENSOR_EIO;
    }

    *adc_P = (int32_t)((data[0] << 12) | (data[1] << 4) | (data[2] >> 4));
    *adc_T = (int32_t)((data[3] << 12) | (data[4] << 4) | (data[5] >> 4));

    return SENSOR_EOK;
}

/**
 * @brief 读取气压数据
 */
static sensor_err_t bmp280_pressure_read(sensor_device_t *sensor,
                                         sensor_data_t *data)
{
    bmp280_priv_t *priv = (bmp280_priv_t *)sensor->priv_data;
    int32_t adc_T, adc_P;

    if (bmp280_read_raw(sensor, &adc_T, &adc_P) != SENSOR_EOK) {
        return SENSOR_EIO;
    }

    /* 计算温度(用于气压补偿) */
    bmp280_compensate_temperature(priv, adc_T);

    /* 计算气压 */
    uint32_t pressure = bmp280_compensate_pressure(priv, adc_P);

    data->type             = SENSOR_TYPE_PRESSURE;
    data->unit             = SENSOR_UNIT_PASCAL;
    data->value.val_uint32 = pressure / 256; /* Pa */
    data->timestamp        = SENSOR_GET_TICK();
    data->accuracy         = 98;

    return SENSOR_EOK;
}

/**
 * @brief 读取温度数据
 */
static sensor_err_t bmp280_temperature_read(sensor_device_t *sensor,
                                            sensor_data_t *data)
{
    bmp280_priv_t *priv = (bmp280_priv_t *)sensor->priv_data;
    int32_t adc_T, adc_P;

    if (bmp280_read_raw(sensor, &adc_T, &adc_P) != SENSOR_EOK) {
        return SENSOR_EIO;
    }

    /* 计算温度 */
    int32_t temperature = bmp280_compensate_temperature(priv, adc_T);

    data->type = SENSOR_TYPE_TEMPERATURE;
    data->unit = SENSOR_UNIT_CELSIUS;
#if SENSOR_USE_FLOAT
    data->value.val_float = temperature / 100.0f;
#else
    data->value.val_int32 = temperature; /* 0.01°C */
#endif
    data->timestamp = SENSOR_GET_TICK();
    data->accuracy  = 98;

    return SENSOR_EOK;
}

/* 气压传感器操作接口 */
static const sensor_ops_t bmp280_pressure_ops = {
    .init   = bmp280_init,
    .deinit = bmp280_deinit,
    .read   = bmp280_pressure_read,
};

/* 温度传感器操作接口 */
static const sensor_ops_t bmp280_temperature_ops = {
    .init   = bmp280_init,
    .deinit = bmp280_deinit,
    .read   = bmp280_temperature_read,
};

/**
 * @brief 创建BMP280气压传感器
 */
sensor_device_t *bmp280_create_pressure(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL) {
        return NULL;
    }

    bmp280_priv_t *priv = (bmp280_priv_t *)SENSOR_MALLOC(sizeof(bmp280_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(bmp280_priv_t));

    priv->i2c_addr = BMP280_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "Bosch";
    sensor->info.model      = "BMP280";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_PRESSURE;
    sensor->info.unit       = SENSOR_UNIT_PASCAL;
    sensor->info.range_max  = 110000;
    sensor->info.range_min  = 30000;
    sensor->info.resolution = 18;
    sensor->info.max_odr    = 157;
    sensor->info.flags      = SENSOR_FLAG_HIGH_PRECISION;

    sensor->ops       = &bmp280_pressure_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 26;

    return sensor;
}

/**
 * @brief 创建BMP280温度传感器
 */
sensor_device_t *bmp280_create_temperature(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL) {
        return NULL;
    }

    bmp280_priv_t *priv = (bmp280_priv_t *)SENSOR_MALLOC(sizeof(bmp280_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(bmp280_priv_t));

    priv->i2c_addr = BMP280_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "Bosch";
    sensor->info.model      = "BMP280";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_TEMPERATURE;
    sensor->info.unit       = SENSOR_UNIT_CELSIUS;
    sensor->info.range_max  = 85;
    sensor->info.range_min  = -40;
    sensor->info.resolution = 16;
    sensor->info.max_odr    = 157;
    sensor->info.flags      = SENSOR_FLAG_HIGH_PRECISION;

    sensor->ops       = &bmp280_temperature_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 26;

    return sensor;
}