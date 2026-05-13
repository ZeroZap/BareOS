#include "sensor_aht20.h"

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data,
                            uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg,
                             uint8_t *data, uint16_t len);
extern int hal_i2c_write(void *bus, uint8_t addr, uint8_t *data, uint16_t len);
extern int hal_i2c_read(void *bus, uint8_t addr, uint8_t *data, uint16_t len);

/**
 * @brief AHT20初始化
 */
static sensor_err_t aht20_init(sensor_device_t *sensor)
{
    aht20_priv_t *priv = (aht20_priv_t *)sensor->priv_data;
    uint8_t cmd[3];

    SENSOR_LOG("Initializing AHT20");

    /* 软复位 */
    cmd[0] = AHT20_CMD_SOFT_RESET;
    hal_i2c_write(sensor->bus, priv->i2c_addr, cmd, 1);
    SENSOR_DELAY_MS(20);

    /* 初始化命令 */
    cmd[0] = AHT20_CMD_INIT;
    cmd[1] = 0x08;
    cmd[2] = 0x00;
    if (hal_i2c_write(sensor->bus, priv->i2c_addr, cmd, 3) != 0) {
        return SENSOR_EIO;
    }

    SENSOR_DELAY_MS(10);

    /* 读取状态 */
    uint8_t status;
    if (hal_i2c_read(sensor->bus, priv->i2c_addr, &status, 1) != 0) {
        return SENSOR_EIO;
    }

    if ((status & 0x08) == 0) {
        SENSOR_LOG("AHT20 calibration failed");
        return SENSOR_ERROR;
    }

    priv->initialized = true;

    SENSOR_LOG("AHT20 initialized successfully");

    return SENSOR_EOK;
}

/**
 * @brief AHT20反初始化
 */
static sensor_err_t aht20_deinit(sensor_device_t *sensor)
{
    aht20_priv_t *priv = (aht20_priv_t *)sensor->priv_data;
    priv->initialized  = false;

    return SENSOR_EOK;
}

/**
 * @brief 触发测量并读取数据
 */
static sensor_err_t aht20_trigger_measurement(sensor_device_t *sensor,
                                              uint8_t *data)
{
    aht20_priv_t *priv = (aht20_priv_t *)sensor->priv_data;
    uint8_t cmd[3];

    /* 触发测量 */
    cmd[0] = AHT20_CMD_TRIGGER;
    cmd[1] = 0x33;
    cmd[2] = 0x00;

    if (hal_i2c_write(sensor->bus, priv->i2c_addr, cmd, 3) != 0) {
        return SENSOR_EIO;
    }

    /* 等待测量完成 (最大80ms) */
    SENSOR_DELAY_MS(80);

    /* 读取数据 (7字节) */
    if (hal_i2c_read(sensor->bus, priv->i2c_addr, data, 7) != 0) {
        return SENSOR_EIO;
    }

    /* 检查忙状态 */
    if (data[0] & 0x80) {
        return SENSOR_EBUSY;
    }

    return SENSOR_EOK;
}

/**
 * @brief 读取温度数据
 */
static sensor_err_t aht20_temperature_read(sensor_device_t *sensor,
                                           sensor_data_t *data)
{
    uint8_t raw_data[7];

    if (aht20_trigger_measurement(sensor, raw_data) != SENSOR_EOK) {
        return SENSOR_EIO;
    }

    /* 计算温度 (20位数据) */
    uint32_t temp_raw = ((uint32_t)(raw_data[3] & 0x0F) << 16)
                        | ((uint32_t)raw_data[4] << 8) | raw_data[5];

#if SENSOR_USE_FLOAT
    float temperature = ((float)temp_raw / 1048576.0f) * 200.0f - 50.0f;

    data->type            = SENSOR_TYPE_TEMPERATURE;
    data->unit            = SENSOR_UNIT_CELSIUS;
    data->value.val_float = temperature;
#else
    /* 使用整数运算 (单位: 0.01°C) */
    int32_t temperature = ((int64_t)temp_raw * 20000 / 1048576) - 5000;

    data->type            = SENSOR_TYPE_TEMPERATURE;
    data->unit            = SENSOR_UNIT_CELSIUS;
    data->value.val_int32 = temperature;
#endif

    data->timestamp = SENSOR_GET_TICK();
    data->accuracy  = 98;

    return SENSOR_EOK;
}

/**
 * @brief 读取湿度数据
 */
static sensor_err_t aht20_humidity_read(sensor_device_t *sensor,
                                        sensor_data_t *data)
{
    uint8_t raw_data[7];

    if (aht20_trigger_measurement(sensor, raw_data) != SENSOR_EOK) {
        return SENSOR_EIO;
    }

    /* 计算湿度 (20位数据) */
    uint32_t humi_raw = ((uint32_t)raw_data[1] << 12)
                        | ((uint32_t)raw_data[2] << 4)
                        | ((uint32_t)raw_data[3] >> 4);

#if SENSOR_USE_FLOAT
    float humidity = ((float)humi_raw / 1048576.0f) * 100.0f;

    data->type            = SENSOR_TYPE_HUMIDITY;
    data->unit            = SENSOR_UNIT_PERCENT;
    data->value.val_float = humidity;
#else
    /* 使用整数运算 (单位: 0.01%) */
    int32_t humidity = ((int64_t)humi_raw * 10000 / 1048576);

    data->type            = SENSOR_TYPE_HUMIDITY;
    data->unit            = SENSOR_UNIT_PERCENT;
    data->value.val_int32 = humidity;
#endif

    data->timestamp = SENSOR_GET_TICK();
    data->accuracy  = 98;

    return SENSOR_EOK;
}

/* 温度传感器操作接口 */
static const sensor_ops_t aht20_temperature_ops = {
    .init   = aht20_init,
    .deinit = aht20_deinit,
    .read   = aht20_temperature_read,
};

/* 湿度传感器操作接口 */
static const sensor_ops_t aht20_humidity_ops = {
    .init   = aht20_init,
    .deinit = aht20_deinit,
    .read   = aht20_humidity_read,
};

/**
 * @brief 创建AHT20温度传感器
 */
sensor_device_t *aht20_create_temperature(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL) {
        return NULL;
    }

    aht20_priv_t *priv = (aht20_priv_t *)SENSOR_MALLOC(sizeof(aht20_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(aht20_priv_t));

    priv->i2c_addr = AHT20_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "ASAIR";
    sensor->info.model      = "AHT20";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_TEMPERATURE;
    sensor->info.unit       = SENSOR_UNIT_CELSIUS;
    sensor->info.range_max  = 85;
    sensor->info.range_min  = -40;
    sensor->info.resolution = 16;
    sensor->info.max_odr    = 10;
    sensor->info.flags      = 0;

    sensor->ops       = &aht20_temperature_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 1;

    return sensor;
}

/**
 * @brief 创建AHT20湿度传感器
 */
sensor_device_t *aht20_create_humidity(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL) {
        return NULL;
    }

    aht20_priv_t *priv = (aht20_priv_t *)SENSOR_MALLOC(sizeof(aht20_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(aht20_priv_t));

    priv->i2c_addr = AHT20_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "ASAIR";
    sensor->info.model      = "AHT20";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_HUMIDITY;
    sensor->info.unit       = SENSOR_UNIT_PERCENT;
    sensor->info.range_max  = 100;
    sensor->info.range_min  = 0;
    sensor->info.resolution = 16;
    sensor->info.max_odr    = 10;
    sensor->info.flags      = 0;

    sensor->ops       = &aht20_humidity_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;
    sensor->odr       = 1;

    return sensor;
}