/**
 * @file xy_sht40.c
 * @brief SHT40/SHT41/SHT45 Temperature & Humidity Sensor Driver
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#include "xy_sht40.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/**
 * @brief CRC8 计算
 */
static uint8_t xy_sht40_crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0xFF;
    uint16_t i, j;
    
    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

/**
 * @brief 获取测量命令
 */
static uint8_t xy_sht40_get_measure_cmd(xy_sht40_precision_t precision)
{
    switch (precision) {
        case XY_SHT40_HIGH_PRECISION:
            return SHT40_CMD_MEASURE_HPM;
        case XY_SHT40_MEDIUM_PRECISION:
            return SHT40_CMD_MEASURE_MPM;
        case XY_SHT40_LOW_PRECISION:
            return SHT40_CMD_MEASURE_LPM;
        default:
            return SHT40_CMD_MEASURE_HPM;
    }
}

/**
 * @brief 获取测量时间
 */
static uint16_t xy_sht40_get_measure_time(xy_sht40_precision_t precision)
{
    switch (precision) {
        case XY_SHT40_HIGH_PRECISION:
            return 45;
        case XY_SHT40_MEDIUM_PRECISION:
            return 25;
        case XY_SHT40_LOW_PRECISION:
            return 10;
        default:
            return 45;
    }
}

int xy_sht40_init(xy_sht40_t *sht40, void *i2c_handle)
{
    int ret;
    uint8_t cmd;
    uint8_t buf[6];
    uint8_t crc;
    
    if (!sht40 || !i2c_handle) {
        return XY_SHT40_INVALID_PARAM;
    }
    
    memset(sht40, 0, sizeof(*sht40));
    xy_i2c_device_init(&sht40->i2c_dev, i2c_handle, SHT40_ADDR, 400);
    sht40->addr = SHT40_ADDR;
    sht40->precision = XY_SHT40_HIGH_PRECISION;
    
    /* 读取序列号验证设备 */
    cmd = SHT40_CMD_READ_SERIAL;
    ret = xy_i2c_device_write(&sht40->i2c_dev, &cmd, 1);
    if (ret != XY_DEVICE_OK) {
        xy_log_e("SHT40 not found\n");
        return XY_SHT40_NOT_FOUND;
    }
    
    xy_os_delay(10);
    
    ret = xy_i2c_device_read(&sht40->i2c_dev, buf, 6);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 验证 CRC */
    crc = xy_sht40_crc8(buf, 2);
    if (crc != buf[2]) {
        return XY_SHT40_CRC_ERROR;
    }
    
    crc = xy_sht40_crc8(&buf[3], 2);
    if (crc != buf[5]) {
        return XY_SHT40_CRC_ERROR;
    }
    
    sht40->data.serial[0] = ((uint32_t)buf[0] << 8) | buf[1];
    sht40->data.serial[1] = ((uint32_t)buf[3] << 8) | buf[4];
    
    xy_log_i("SHT40 found (SN=0x%08X%08X)\n", 
             sht40->data.serial[0], sht40->data.serial[1]);
    
    sht40->initialized = true;
    return XY_SHT40_OK;
}

int xy_sht40_deinit(xy_sht40_t *sht40)
{
    if (!sht40) {
        return XY_SHT40_INVALID_PARAM;
    }
    
    sht40->initialized = false;
    return XY_SHT40_OK;
}

int xy_sht40_read(xy_sht40_t *sht40)
{
    int ret;
    uint8_t cmd;
    uint8_t buf[6];
    uint8_t crc;
    uint16_t measure_time;
    
    if (!sht40 || !sht40->initialized) {
        return XY_SHT40_INVALID_PARAM;
    }
    
    /* 获取测量命令 */
    cmd = xy_sht40_get_measure_cmd(sht40->precision);
    measure_time = xy_sht40_get_measure_time(sht40->precision);
    
    /* 触发测量 */
    ret = xy_i2c_device_write(&sht40->i2c_dev, &cmd, 1);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    xy_os_delay(measure_time);
    
    /* 读取数据 */
    ret = xy_i2c_device_read(&sht40->i2c_dev, buf, 6);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 验证 CRC */
    crc = xy_sht40_crc8(buf, 2);
    if (crc != buf[2]) {
        return XY_SHT40_CRC_ERROR;
    }
    
    crc = xy_sht40_crc8(&buf[3], 2);
    if (crc != buf[5]) {
        return XY_SHT40_CRC_ERROR;
    }
    
    /* 解析温度 */
    uint16_t temp_raw = ((uint16_t)buf[0] << 8) | buf[1];
    int32_t temp = (int32_t)((int64_t)temp_raw * 17500 >> 16) - 4500;
    sht40->data.temperature = (int16_t)temp;
    
    /* 解析湿度 */
    uint16_t hum_raw = ((uint16_t)buf[3] << 8) | buf[4];
    int32_t hum = (int64_t)hum_raw * 12500 >> 16;
    sht40->data.humidity = (uint16_t)(hum - 600);  /* 偏移修正 */
    
    sht40->data.timestamp = xy_os_tick_get();
    
    xy_log_d("SHT40: T=%d.%02d°C, H=%d.%02d%%RH\n",
             sht40->data.temperature / 100,
             (sht40->data.temperature % 100 < 0 ? -sht40->data.temperature % 100 : sht40->data.temperature % 100),
             sht40->data.humidity / 100, sht40->data.humidity % 100);
    
    return XY_SHT40_OK;
}

int xy_sht40_get_temperature(xy_sht40_t *sht40, int16_t *temperature)
{
    if (!sht40 || !temperature) {
        return XY_SHT40_INVALID_PARAM;
    }
    
    int ret = xy_sht40_read(sht40);
    if (ret == XY_SHT40_OK) {
        *temperature = sht40->data.temperature;
    }
    return ret;
}

int xy_sht40_get_humidity(xy_sht40_t *sht40, uint16_t *humidity)
{
    if (!sht40 || !humidity) {
        return XY_SHT40_INVALID_PARAM;
    }
    
    int ret = xy_sht40_read(sht40);
    if (ret == XY_SHT40_OK) {
        *humidity = sht40->data.humidity;
    }
    return ret;
}

int xy_sht40_get_serial(xy_sht40_t *sht40, uint32_t *serial)
{
    if (!sht40 || !serial) {
        return XY_SHT40_INVALID_PARAM;
    }
    
    serial[0] = sht40->data.serial[0];
    serial[1] = sht40->data.serial[1];
    return XY_SHT40_OK;
}

int xy_sht40_set_precision(xy_sht40_t *sht40, xy_sht40_precision_t precision)
{
    if (!sht40 || precision > XY_SHT40_LOW_PRECISION) {
        return XY_SHT40_INVALID_PARAM;
    }
    
    sht40->precision = precision;
    return XY_SHT40_OK;
}
