#include "sensor_vl53l0x.h"

extern int hal_i2c_mem_read(void *bus, uint8_t addr, uint8_t reg, uint8_t *data,
                            uint16_t len);
extern int hal_i2c_mem_write(void *bus, uint8_t addr, uint8_t reg,
                             uint8_t *data, uint16_t len);

static sensor_err_t vl53l0x_init(sensor_device_t *sensor)
{
    vl53l0x_priv_t *priv = (vl53l0x_priv_t *)sensor->priv_data;
    uint8_t data;

    SENSOR_LOG("Initializing VL53L0X");

    /* 读取Model ID */
    if (hal_i2c_mem_read(sensor->bus, priv->i2c_addr,
                         VL53L0X_REG_IDENTIFICATION_MODEL_ID, &data, 1)
        != 0) {
        return SENSOR_EIO;
    }

    if (data != 0xEE) {
        SENSOR_LOG("Wrong Model ID: 0x%02X", data);
        return SENSOR_ERROR;
    }

    /* 这里需要复杂的初始化序列 */
    /* 简化版本 - 实际需要ST提供的API */

    SENSOR_LOG("VL53L0X initialized");

    return SENSOR_EOK;
}

static sensor_err_t vl53l0x_deinit(sensor_device_t *sensor)
{
    return SENSOR_EOK;
}

static sensor_err_t vl53l0x_read(sensor_device_t *sensor, sensor_data_t *data)
{
    vl53l0x_priv_t *priv = (vl53l0x_priv_t *)sensor->priv_data;
    uint8_t buf[12];

    /* 启动测距 */
    uint8_t start = 0x01;
    hal_i2c_mem_write(
        sensor->bus, priv->i2c_addr, VL53L0X_REG_SYSRANGE_START, &start, 1);

    /* 等待测量完成 */
    SENSOR_DELAY_MS(50);

    /* 读取结果 */
    if (hal_i2c_mem_read(sensor->bus, priv->i2c_addr,
                         VL53L0X_REG_RESULT_RANGE_STATUS, buf, 12)
        != 0) {
        return SENSOR_EIO;
    }

    uint16_t distance = (buf[10] << 8) | buf[11];

    data->type             = SENSOR_TYPE_DISTANCE;
    data->unit             = SENSOR_UNIT_MILLIMETER;
    data->value.val_uint32 = distance;
    data->timestamp        = SENSOR_GET_TICK();
    data->accuracy         = 92;

    return SENSOR_EOK;
}

static const sensor_ops_t vl53l0x_ops = {
    .init   = vl53l0x_init,
    .deinit = vl53l0x_deinit,
    .read   = vl53l0x_read,
};

sensor_device_t *vl53l0x_create(const char *name, void *i2c_bus)
{
    sensor_device_t *sensor =
        (sensor_device_t *)SENSOR_MALLOC(sizeof(sensor_device_t));
    if (sensor == NULL)
        return NULL;

    vl53l0x_priv_t *priv =
        (vl53l0x_priv_t *)SENSOR_MALLOC(sizeof(vl53l0x_priv_t));
    if (priv == NULL) {
        SENSOR_FREE(sensor);
        return NULL;
    }

    memset(sensor, 0, sizeof(sensor_device_t));
    memset(priv, 0, sizeof(vl53l0x_priv_t));

    priv->i2c_addr = VL53L0X_ADDR_DEFAULT;

    strncpy(sensor->info.name, name, SENSOR_NAME_MAX_LEN - 1);
    sensor->info.vendor     = "STMicroelectronics";
    sensor->info.model      = "VL53L0X";
    sensor->info.version    = 0x0100;
    sensor->info.type       = SENSOR_TYPE_DISTANCE;
    sensor->info.unit       = SENSOR_UNIT_MILLIMETER;
    sensor->info.range_max  = 2000;
    sensor->info.range_min  = 30;
    sensor->info.resolution = 16;
    sensor->info.max_odr    = 50;
    sensor->info.flags      = SENSOR_FLAG_HIGH_PRECISION;

    sensor->ops       = &vl53l0x_ops;
    sensor->bus       = i2c_bus;
    sensor->priv_data = priv;
    sensor->status    = SENSOR_STATUS_IDLE;

    return sensor;
}