#include "sensor_ccs811.h"

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data,
                            uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg,
                             uint8_t *data, uint16_t len);
extern int hal_i2c_write(void *bus, uint8_t addr, uint8_t *data, uint16_t len);

static sensor_err_t ccs811_init(sensor_device_t *sensor)
{
    ccs811_priv_t *priv = (ccs811_priv_t *)sensor->priv_data;
    uint8_t data;

    SENSOR_LOG("Initializing CCS811");

    /* 检查HW_ID */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, CCS811_REG_HW_ID, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }

    if (data != CCS811_HW_ID) {
        SENSOR_LOG("Wrong HW_ID: 0x%02X", data);
        return SENSOR_ERROR;
    }

    /* 启动应用程序 */
    uint8_t app_start = CCS811_REG_APP_START;
    hal_i2c_write(sensor->bus, priv->i2c_addr, &app_start, 1);
    SENSOR_DELAY_MS(100);

    /* 配置测量模式: 1秒采样 */
    data = 0x10;
    if (hal_i2c_mem_write(
            sensor->bus, priv->i2c_addr, CCS811_REG_MEAS_MODE, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }

    SENSOR_LOG("CCS811 initialized");

    return SENSOR_EOK;
}

static sensor_err_t ccs811_deinit(sensor_device_t *sensor)
{
    ccs811_priv_t *priv = (ccs811_priv_t *)sensor->priv_data;
    uint8_t data        = 0x00;

    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, CCS811_REG_MEAS_MODE, &data, 1);

    return SENSOR_EOK;
}

static sensor_err_t ccs811_read_data(sensor_device_t *sensor)
{
    ccs811_priv_t *priv = (ccs811_priv_t *)sensor->priv_data;
    uint8_t buf[8];

    /* 检查数据就绪 */
    if (hal_i2c_mem_read(sensor->bus, priv->i2c_addr, CCS811_REG_STATUS, buf, 1)
        != 0) {
        return SENSOR_EIO;
    }

    if (!(buf[0] & 0x08)) {
        return SENSOR_EBUSY; /* 数据未就绪 */
    }

    /* 读取算法结果 */
    if (hal_i2c_mem_read(
            sensor->bus, priv->i2c_addr, CCS811_REG_ALG_RESULT, buf, 8)
        != 0) {
        return SENSOR_EIO;
    }

    priv->eco2 = (buf[0] << 8) | buf[1];
    priv->tvoc = (buf[2] << 8) | buf[3];

    return SENSOR_EOK;
}

static sensor_err_t ccs811_co2_read(sensor_device_t *sensor,
                                    sensor_data_t *data)
{
    ccs811_priv_t *priv = (ccs811_priv_t *)sensor->priv_data;

    if (ccs811_read_data(sensor) != SENSOR_EOK) {
        return SENSOR_EIO;
    }

    data->type             = SENSOR_TYPE_CO2;
    data->unit             = SENSOR_UNIT_PPM;
    data->value.val_uint32 = priv->eco2;
    data->timestamp        = SENSOR_GET_TICK();
    data->accuracy         = 85;

    return SENSOR_EOK;
}

static sensor_err_t ccs811_tvoc_read(sensor_device_t *sensor,
                                     sensor_data_t *data)
{
    ccs811_priv_t *priv = (ccs811_priv_t *)sensor->priv_data;

    if (ccs811_read_data(sensor) != SENSOR_EOK) {
        return SENSOR_EIO;
    }

    data->type             = SENSOR_TYPE_TVOC;
    data->unit             = SENSOR_UNIT_PPB;
    data->value.val_uint32 = priv->tvoc;
    data->timestamp        = SENSOR_GET_TICK();
    data->accuracy         = 85;

    return SENSOR_EOK;
}

static const sensor_ops_t ccs811_co2_ops = {
    .init   = ccs811_init,
    .deinit = ccs811_deinit,
    .read   = ccs811_co2_read,
};

static const sensor_ops_t ccs811_tvoc_ops = {
    .init   = ccs811_init,
    .deinit = ccs811_deinit,
    .read   = ccs811_tvoc_read,
};

sensor_device_t *ccs811_create_co2(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL)
        return NULL;

    ccs811_priv_t *priv = (ccs811_priv_t *)SENSOR_MALLOC(sizeof(ccs811_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(ccs811_priv_t));

    priv->i2c_addr = CCS811_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "ams";
    sensor->info.model      = "CCS811";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_CO2;
    sensor->info.unit       = SENSOR_UNIT_PPM;
    sensor->info.range_max  = 8192;
    sensor->info.range_min  = 400;
    sensor->info.resolution = 16;
    sensor->info.max_odr    = 1;
    sensor->info.flags      = 0;

    sensor->ops       = &ccs811_co2_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;

    return sensor;
}

sensor_device_t *ccs811_create_tvoc(const char *name, void *i2c_bus)
{
    /* 类似CO2实现 */
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL)
        return NULL;

    ccs811_priv_t *priv = (ccs811_priv_t *)SENSOR_MALLOC(sizeof(ccs811_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(ccs811_priv_t));

    priv->i2c_addr = CCS811_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "ams";
    sensor->info.model      = "CCS811";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_TVOC;
    sensor->info.unit       = SENSOR_UNIT_PPB;
    sensor->info.range_max  = 1187;
    sensor->info.range_min  = 0;
    sensor->info.resolution = 16;
    sensor->info.max_odr    = 1;
    sensor->info.flags      = 0;

    sensor->ops       = &ccs811_tvoc_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;

    return sensor;
}