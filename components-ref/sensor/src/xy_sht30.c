/**
 * @file xy_sht30.c
 * @brief SHT30 Temperature & Humidity Sensor Driver Implementation
 * @version 1.0.0
 * @date 2026-03-01
 */

#include "xy_sht30.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/**
 * @brief 计算 CRC8
 */
static uint8_t xy_sht30_calc_crc8(const uint8_t *data, uint32_t len)
{
    uint8_t crc = 0xFF;
    uint32_t i, j;
    
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

int xy_sht30_init(xy_sht30_t *dev, void *i2c_handle, uint8_t addr)
{
    int ret;
    uint8_t cmd;
    
    if (!dev || !i2c_handle) {
        return XY_SHT30_INVALID_PARAM;
    }
    
    memset(dev, 0, sizeof(*dev));
    
    /* 初始化 I2C 设备 */
    xy_i2c_device_init(&dev->i2c_dev, i2c_handle, addr, 1000);
    dev->addr = addr;
    
    /* 软件复位 */
    cmd = SHT30_CMD_SOFT_RESET >> 8;
    ret = xy_i2c_device_write(&dev->i2c_dev, &cmd, 1);
    if (ret != XY_DEVICE_OK) {
        xy_log_e("Failed to reset SHT30\n");
        return XY_SHT30_ERROR;
    }
    
    /* 等待复位完成 */
    xy_os_delay(15);
    
    dev->initialized = 1;
    xy_log_i("SHT30 initialized at 0x%02X\n", addr);
    return XY_SHT30_OK;
}

int xy_sht30_deinit(xy_sht30_t *dev)
{
    if (!dev) {
        return XY_SHT30_INVALID_PARAM;
    }
    
    dev->initialized = 0;
    return XY_SHT30_OK;
}

int xy_sht30_read(xy_sht30_t *dev)
{
    int ret;
    uint8_t cmd;
    uint8_t buf[6];
    uint16_t temp_raw, hum_raw;
    uint8_t crc;
    
    if (!dev || !dev->initialized) {
        return XY_SHT30_INVALID_PARAM;
    }
    
    /* 发送测量命令 (高重复性) */
    cmd = SHT30_CMD_MEASURE_H >> 8;
    ret = xy_i2c_device_write(&dev->i2c_dev, &cmd, 1);
    if (ret != XY_DEVICE_OK) {
        xy_log_e("Failed to send measure command\n");
        return XY_SHT30_ERROR;
    }
    
    /* 等待测量完成 (典型 15ms) */
    xy_os_delay(20);
    
    /* 读取 6 字节数据 */
    ret = xy_i2c_device_read(&dev->i2c_dev, buf, 6);
    if (ret != XY_DEVICE_OK) {
        xy_log_e("Failed to read data\n");
        return XY_SHT30_ERROR;
    }
    
    /* 验证 CRC */
    crc = xy_sht30_calc_crc8(buf, 2);
    if (crc != buf[2]) {
        xy_log_e("Temperature CRC error\n");
        return XY_SHT30_CRC_ERROR;
    }
    
    crc = xy_sht30_calc_crc8(&buf[3], 2);
    if (crc != buf[5]) {
        xy_log_e("Humidity CRC error\n");
        return XY_SHT30_CRC_ERROR;
    }
    
    /* 计算原始值 */
    temp_raw = ((uint16_t)buf[0] << 8) | buf[1];
    hum_raw = ((uint16_t)buf[3] << 8) | buf[4];
    
    /* 转换为实际值 */
    /* Temperature: -45 to 130°C */
    dev->temperature = (int16_t)((int32_t)((int32_t)temp_raw * 17500 / 65535 - 4500));
    
    /* Humidity: 0 to 100%RH */
    dev->humidity = (uint16_t)((uint32_t)hum_raw * 10000 / 65535);
    
    xy_log_d("T=%d.%02d°C, H=%d.%02d%%RH\n", 
             dev->temperature / 100, dev->temperature % 100,
             dev->humidity / 100, dev->humidity % 100);
    
    return XY_SHT30_OK;
}

int xy_sht30_read_temperature(xy_sht30_t *dev, int16_t *temperature)
{
    int ret;
    
    if (!dev || !temperature) {
        return XY_SHT30_INVALID_PARAM;
    }
    
    ret = xy_sht30_read(dev);
    if (ret == XY_SHT30_OK) {
        *temperature = dev->temperature;
    }
    
    return ret;
}

int xy_sht30_read_humidity(xy_sht30_t *dev, uint16_t *humidity)
{
    int ret;
    
    if (!dev || !humidity) {
        return XY_SHT30_INVALID_PARAM;
    }
    
    ret = xy_sht30_read(dev);
    if (ret == XY_SHT30_OK) {
        *humidity = dev->humidity;
    }
    
    return ret;
}

int xy_sht30_soft_reset(xy_sht30_t *dev)
{
    uint8_t cmd;
    int ret;
    
    if (!dev || !dev->initialized) {
        return XY_SHT30_INVALID_PARAM;
    }
    
    cmd = SHT30_CMD_SOFT_RESET >> 8;
    ret = xy_i2c_device_write(&dev->i2c_dev, &cmd, 1);
    if (ret != XY_DEVICE_OK) {
        return XY_SHT30_ERROR;
    }
    
    xy_os_delay(15);
    return XY_SHT30_OK;
}

int xy_sht30_heater_on(xy_sht30_t *dev)
{
    uint8_t cmd;
    int ret;
    
    if (!dev || !dev->initialized) {
        return XY_SHT30_INVALID_PARAM;
    }
    
    cmd = SHT30_CMD_HEATER_ON >> 8;
    ret = xy_i2c_device_write(&dev->i2c_dev, &cmd, 1);
    return ret;
}

int xy_sht30_heater_off(xy_sht30_t *dev)
{
    uint8_t cmd;
    int ret;
    
    if (!dev || !dev->initialized) {
        return XY_SHT30_INVALID_PARAM;
    }
    
    cmd = SHT30_CMD_HEATER_OFF >> 8;
    ret = xy_i2c_device_write(&dev->i2c_dev, &cmd, 1);
    return ret;
}
