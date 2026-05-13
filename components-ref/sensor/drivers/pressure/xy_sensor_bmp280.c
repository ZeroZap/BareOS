/**
 * @file xy_sensor_bmp280.c
 * @brief BMP280 Pressure & Temperature Sensor Driver - Migrated to New Framework
 * @version 2.0.0
 * @date 2026-03-05
 */

#include "xy_sensor.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ==================== BMP280 寄存器定义 ==================== */

#define BMP280_ADDR             0x76
#define BMP280_REG_TEMP_XLSB    0xFC
#define BMP280_REG_TEMP_LSB     0xFB
#define BMP280_REG_TEMP_MSB     0xFA
#define BMP280_REG_PRESS_XLSB   0xF9
#define BMP280_REG_PRESS_LSB    0xF8
#define BMP280_REG_PRESS_MSB    0xF7
#define BMP280_REG_CONFIG       0xF5
#define BMP280_REG_CTRL_MEAS    0xF4
#define BMP280_REG_STATUS       0xF3
#define BMP280_REG_RESET        0xE0
#define BMP280_REG_ID           0xD0
#define BMP280_ID               0x58

#define BMP280_SOFT_RESET       0xB6

/* 校准数据 */
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;
} bmp280_calib_data_t;

/* ==================== 私有数据结构 ==================== */

typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    bmp280_calib_data_t calib;
    xy_sensor_value_t temp_data;
    xy_sensor_value_t pressure_data;
    int32_t t_fine;
} bmp280_private_data_t;

/* ==================== 驱动实现 ==================== */

/**
 * @brief 读取校准数据
 */
static int bmp280_read_calib(bmp280_private_data_t *priv)
{
    uint8_t buf[26];
    if (xy_sensor_i2c_read(&priv->bus, 0x88, buf, 26) != 0) {
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
    
    return 0;
}

/**
 * @brief BMP280 初始化
 */
static int bmp280_init(xy_sensor_device_t *dev)
{
    bmp280_private_data_t *priv = (bmp280_private_data_t *)dev->data;
    
    /* 检查设备 ID */
    uint8_t id;
    if (xy_sensor_i2c_read_reg(&priv->bus, BMP280_REG_ID, &id) != 0) {
        xy_log_e("BMP280: Failed to read ID\n");
        return -1;
    }
    
    if (id != BMP280_ID) {
        xy_log_e("BMP280: Wrong ID (0x%02X)\n", id);
        return -1;
    }
    
    /* 软件复位 */
    xy_sensor_i2c_write_reg(&priv->bus, BMP280_REG_RESET, BMP280_SOFT_RESET);
    xy_os_delay(10);
    
    /* 读取校准数据 */
    if (bmp280_read_calib(priv) != 0) {
        xy_log_e("BMP280: Failed to read calibration\n");
        return -1;
    }
    
    /* 配置：正常模式，16x 过采样，500ms 间隔 */
    xy_sensor_i2c_write_reg(&priv->bus, BMP280_REG_CONFIG, 0x00);
    xy_sensor_i2c_write_reg(&priv->bus, BMP280_REG_CTRL_MEAS, 0x37);
    
    priv->initialized = true;
    xy_log_i("BMP280 initialized\n");
    return 0;
}

/**
 * @brief 补偿温度
 */
static float bmp280_compensate_temp(bmp280_private_data_t *priv, int32_t adc_temp)
{
    int32_t var1, var2;
    
    var1 = ((((adc_temp >> 3) - ((int32_t)priv->calib.dig_T1 << 1))) * 
            ((int32_t)priv->calib.dig_T2)) >> 11;
    var2 = (((((adc_temp >> 4) - ((int32_t)priv->calib.dig_T1)) * 
              ((adc_temp >> 4) - ((int32_t)priv->calib.dig_T1))) >> 12) * 
            ((int32_t)priv->calib.dig_T3)) >> 14;
    
    priv->t_fine = var1 + var2;
    
    float temperature = (priv->t_fine * 5 + 128) >> 8;
    return temperature / 100.0f;
}

/**
 * @brief 补偿压力
 */
static uint32_t bmp280_compensate_pressure(bmp280_private_data_t *priv, int32_t adc_press)
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

/**
 * @brief BMP280 采样获取
 */
static int bmp280_sample_fetch(xy_sensor_device_t *dev, xy_sensor_channel_t channel)
{
    bmp280_private_data_t *priv = (bmp280_private_data_t *)dev->data;
    
    if (!priv->initialized) {
        return -1;
    }
    
    /* 读取原始数据 */
    uint8_t buf[6];
    if (xy_sensor_i2c_read(&priv->bus, 0xF7, buf, 6) != 0) {
        return -1;
    }
    
    int32_t adc_temp = ((int32_t)buf[3] << 12) | ((int32_t)buf[4] << 4) | (buf[5] >> 4);
    int32_t adc_press = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) | (buf[2] >> 4);
    
    /* 补偿 */
    float temperature = bmp280_compensate_temp(priv, adc_temp);
    uint32_t pressure = bmp280_compensate_pressure(priv, adc_press);
    
    /* 存储到缓存 */
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->temp_data, temperature);
    XY_SENSOR_VALUE_FROM_FLOAT(&priv->pressure_data, pressure / 256.0f);
    
    xy_log_d("BMP280: T=%.2f°C, P=%.2f Pa\n", temperature, pressure / 256.0f);
    return 0;
}

/**
 * @brief BMP280 通道数据获取
 */
static int bmp280_channel_get(xy_sensor_device_t *dev, 
                              xy_sensor_channel_t channel, 
                              xy_sensor_value_t *val)
{
    bmp280_private_data_t *priv = (bmp280_private_data_t *)dev->data;
    
    if (!val) {
        return -1;
    }
    
    switch (channel) {
        case XY_SENSOR_CHAN_AMBIENT_TEMP:
            *val = priv->temp_data;
            break;
            
        case XY_SENSOR_CHAN_PRESSURE:
            *val = priv->pressure_data;
            break;
            
        default:
            return -1;
    }
    
    return 0;
}

/**
 * @brief BMP280 驱动 API
 */
static const xy_sensor_driver_api_t bmp280_driver_api = {
    .init = bmp280_init,
    .sample_fetch = bmp280_sample_fetch,
    .channel_get = bmp280_channel_get,
    .attr_set = NULL,
    .attr_get = NULL,
    .trigger_set = NULL,
};

/* ==================== 设备注册 ==================== */

static bmp280_private_data_t bmp280_priv_data;
static xy_sensor_device_t bmp280_device = {
    .name = "BMP280",
    .type = XY_SENSOR_TYPE_PRESSURE,
    .api = &bmp280_driver_api,
    .data = &bmp280_priv_data,
    .bus = {0},
    .initialized = false,
};

/**
 * @brief 注册 BMP280 传感器
 */
int xy_sensor_bmp280_register(void *i2c_handle, uint8_t addr)
{
    xy_sensor_bus_config_i2c(&bmp280_priv_data.bus, i2c_handle, addr);
    return xy_sensor_device_register(&bmp280_device);
}
