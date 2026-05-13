/**
 * @file xy_sensor_bme280.c
 * @brief BME280 Environmental Sensor - Migrated
 * @version 2.0.0
 * @date 2026-03-05
 */

#include "xy_sensor.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* BME280 寄存器定义 */
#define BME280_ADDR             0x76
#define BME280_REG_CHIP_ID      0xD0
#define BME280_REG_RESET        0xE0
#define BME280_REG_CTRL_HUM     0xF2
#define BME280_REG_CTRL_MEAS    0xF4
#define BME280_REG_CONFIG       0xF5
#define BME280_REG_PRESS_MSB    0xF7
#define BME280_REG_CALIB00      0x88
#define BME280_CHIP_ID          0x60

/* 校准参数 */
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
    uint8_t  dig_H1;
    int16_t  dig_H2, dig_H3, dig_H4, dig_H5, dig_H6;
} bme280_calib_t;

/* 私有数据 */
typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    bme280_calib_t calib;
    int32_t t_fine;
    xy_sensor_value_t temp_data;
    xy_sensor_value_t pressure_data;
    xy_sensor_value_t humidity_data;
} bme280_private_data_t;

/* 读取校准数据 */
static int bme280_read_calib(bme280_private_data_t *priv)
{
    uint8_t buf[26];
    
    /* 读取温度/压力校准 */
    if (xy_sensor_i2c_read(&priv->bus, BME280_REG_CALIB00, buf, 26) != 0) {
        return -1;
    }
    
    priv->calib.dig_T1 = (buf[1] << 8) | buf[0];
    priv->calib.dig_T2 = (buf[3] << 8) | buf[2];
    priv->calib.dig_T3 = (buf[5] << 8) | buf[4];
    priv->calib.dig_P1 = (buf[7] << 8) | buf[6];
    priv->calib.dig_P2 = (buf[9] << 8) | buf[8];
    priv->calib.dig_P3 = (buf[11] << 8) | buf[10];
    priv->calib.dig_P4 = (buf[13] << 8) | buf[12];
    priv->calib.dig_P5 = (buf[15] << 8) | buf[14];
    priv->calib.dig_P6 = (buf[17] << 8) | buf[16];
    priv->calib.dig_P7 = (buf[19] << 8) | buf[18];
    priv->calib.dig_P8 = (buf[21] << 8) | buf[20];
    priv->calib.dig_P9 = (buf[23] << 8) | buf[22];
    
    /* 读取湿度校准 */
    uint8_t h4, h5, h6;
    xy_sensor_i2c_read_reg(&priv->bus, 0xA1, &priv->calib.dig_H1);
    xy_sensor_i2c_read_reg(&priv->bus, 0xE1, &h4);
    xy_sensor_i2c_read_reg(&priv->bus, 0xE3, &h5);
    xy_sensor_i2c_read_reg(&priv->bus, 0xE4, &h4);
    xy_sensor_i2c_read_reg(&priv->bus, 0xE5, &h5);
    xy_sensor_i2c_read_reg(&priv->bus, 0xE7, &h6);
    
    priv->calib.dig_H2 = (h5 << 4) | (h4 & 0x0F);
    priv->calib.dig_H3 = h5;
    priv->calib.dig_H4 = ((h4 & 0xF0) << 4) | (h5 & 0x0F);
    priv->calib.dig_H5 = h6;
    priv->calib.dig_H6 = h6;
    
    return 0;
}

/* 初始化 */
static int bme280_init(xy_sensor_device_t *dev)
{
    bme280_private_data_t *priv = (bme280_private_data_t *)dev->data;
    
    /* 检查设备 ID */
    uint8_t id;
    if (xy_sensor_i2c_read_reg(&priv->bus, BME280_REG_CHIP_ID, &id) != 0) {
        return -1;
    }
    
    if (id != BME280_CHIP_ID) {
        xy_log_e("BME280: Wrong ID (0x%02X)\n", id);
        return -1;
    }
    
    /* 软件复位 */
    xy_sensor_i2c_write_reg(&priv->bus, BME280_REG_RESET, 0xB6);
    xy_os_delay(10);
    
    /* 读取校准数据 */
    if (bme280_read_calib(priv) != 0) {
        return -1;
    }
    
    /* 配置：湿度 x1, 温度 x1, 压力 x1, 正常模式 */
    xy_sensor_i2c_write_reg(&priv->bus, BME280_REG_CTRL_HUM, 0x01);
    xy_sensor_i2c_write_reg(&priv->bus, BME280_REG_CTRL_MEAS, 0x27);
    xy_sensor_i2c_write_reg(&priv->bus, BME280_REG_CONFIG, 0x00);
    
    priv->initialized = true;
    xy_log_i("BME280 initialized\n");
    return 0;
}

/* 补偿计算 */
static float bme280_compensate_temp(bme280_private_data_t *priv, int32_t adc_temp)
{
    int32_t var1, var2;
    var1 = ((((adc_temp >> 3) - ((int32_t)priv->calib.dig_T1 << 1))) * 
            ((int32_t)priv->calib.dig_T2)) >> 11;
    var2 = (((((adc_temp >> 4) - ((int32_t)priv->calib.dig_T1)) * 
              ((adc_temp >> 4) - ((int32_t)priv->calib.dig_T1))) >> 12) * 
            ((int32_t)priv->calib.dig_T3)) >> 14;
    priv->t_fine = var1 + var2;
    return (priv->t_fine * 5 + 128) >> 8;
}

static uint32_t bme280_compensate_pressure(bme280_private_data_t *priv, int32_t adc_press)
{
    int64_t var1, var2, p;
    var1 = ((int64_t)priv->t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)priv->calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)priv->calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)priv->calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)priv->calib.dig_P3) >> 8) + 
           ((var1 * (int64_t)priv->calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)priv->calib.dig_P1) >> 33;
    if (var1 == 0) return 0;
    p = 1048576 - adc_press;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)priv->calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)priv->calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)priv->calib.dig_P7) << 4);
    return (uint32_t)(p >> 8);
}

static uint32_t bme280_compensate_humidity(bme280_private_data_t *priv, int32_t adc_hum)
{
    int32_t var1 = priv->t_fine - ((int32_t)76800);
    var1 = ((((adc_hum << 14) - (((int32_t)priv->calib.dig_H4) << 20) - 
              (((int32_t)priv->calib.dig_H5) * var1)) + ((int32_t)16384)) >> 15) * 
           (((((((var1 * ((int32_t)priv->calib.dig_H3)) >> 16) * 
                (((var1 * ((int32_t)priv->calib.dig_H2)) >> 15) + 
                 ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * 
              ((int32_t)priv->calib.dig_H6) + ((int32_t)8192)) >> 14);
    var1 = var1 - (((((var1 >> 15) * (var1 >> 15)) >> 7) * 
                    ((int32_t)priv->calib.dig_H1)) >> 4);
    var1 = (var1 < 0) ? 0 : var1;
    var1 = (var1 > 419430400) ? 419430400 : var1;
    return (uint32_t)(var1 >> 12);
}

/* 采样获取 */
static int bme280_sample_fetch(xy_sensor_device_t *dev, xy_sensor_channel_t channel)
{
    bme280_private_data_t *priv = (bme280_private_data_t *)dev->data;
    
    /* 读取数据 */
    uint8_t buf[8];
    if (xy_sensor_i2c_read(&priv->bus, BME280_REG_PRESS_MSB, buf, 8) != 0) {
        return -1;
    }
    
    int32_t adc_press = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) | (buf[2] >> 4);
    int32_t adc_temp = ((int32_t)buf[3] << 12) | ((int32_t)buf[4] << 4) | (buf[5] >> 4);
    int32_t adc_hum = ((int32_t)buf[6] << 8) | (int32_t)buf[7];
    
    /* 补偿 */
    float temperature = bme280_compensate_temp(priv, adc_temp) / 100.0f;
    uint32_t pressure = bme280_compensate_pressure(priv, adc_press) / 256;
    uint32_t humidity = bme280_compensate_humidity(priv, adc_hum) / 1024;
    
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->temp_data, temperature);
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->pressure_data, pressure / 1000.0f);  /* kPa */
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->humidity_data, humidity / 1000.0f);  /* % */
    
    return 0;
}

/* 通道数据获取 */
static int bme280_channel_get(xy_sensor_device_t *dev, 
                              xy_sensor_channel_t channel, 
                              xy_sensor_value_t *val)
{
    bme280_private_data_t *priv = (bme280_private_data_t *)dev->data;
    if (!val) return -1;
    
    switch (channel) {
        case XY_SENSOR_CHAN_AMBIENT_TEMP: *val = priv->temp_data; break;
        case XY_SENSOR_CHAN_PRESSURE: *val = priv->pressure_data; break;
        case XY_SENSOR_CHAN_HUMIDITY: *val = priv->humidity_data; break;
        default: return -1;
    }
    return 0;
}

/* 驱动 API */
static const xy_sensor_driver_api_t bme280_driver_api = {
    .init = bme280_init,
    .sample_fetch = bme280_sample_fetch,
    .channel_get = bme280_channel_get,
};

static bme280_private_data_t bme280_priv;
static xy_sensor_device_t bme280_device = {
    .name = "BME280",
    .type = XY_SENSOR_TYPE_COMBO,
    .api = &bme280_driver_api,
    .data = &bme280_priv,
};

int xy_sensor_bme280_register(void *i2c_handle, uint8_t addr)
{
    xy_sensor_bus_config_i2c(&bme280_priv.bus, i2c_handle, addr);
    return xy_sensor_device_register(&bme280_device);
}
