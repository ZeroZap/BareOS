/**
 * @file xy_hdc1080.c
 * @brief HDC1080 Temperature & Humidity Sensor Driver
 * @version 1.0.0
 * @date 2026-03-01 自主任务
 */

#include "xy_hdc1080.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

int xy_hdc1080_init(xy_hdc1080_t *dev, void *i2c_handle, uint8_t addr)
{
    int ret;
    uint16_t config;
    
    if (!dev || !i2c_handle) {
        return XY_HDC1080_INVALID_PARAM;
    }
    
    memset(dev, 0, sizeof(*dev));
    xy_i2c_device_init(&dev->i2c_dev, i2c_handle, addr, 1000);
    dev->addr = addr;
    
    /* 软件复位 */
    config = HDC1080_CONFIG_RST;
    uint8_t buf[2] = {(config >> 8) & 0xFF, config & 0xFF};
    ret = xy_i2c_device_write_reg(&dev->i2c_dev, HDC1080_REG_CONFIG, buf, 2);
    if (ret != XY_DEVICE_OK) {
        return XY_HDC1080_ERROR;
    }
    xy_os_delay(15);
    
    /* 配置：温度 14 位 + 湿度 14 位，顺序测量 */
    config = HDC1080_CONFIG_MODE;
    buf[0] = (config >> 8) & 0xFF;
    buf[1] = config & 0xFF;
    ret = xy_i2c_device_write_reg(&dev->i2c_dev, HDC1080_REG_CONFIG, buf, 2);
    if (ret != XY_DEVICE_OK) {
        return XY_HDC1080_ERROR;
    }
    
    dev->initialized = 1;
    xy_log_i("HDC1080 initialized at 0x%02X\n", addr);
    return XY_HDC1080_OK;
}

int xy_hdc1080_deinit(xy_hdc1080_t *dev)
{
    if (!dev) return XY_HDC1080_INVALID_PARAM;
    dev->initialized = 0;
    return XY_HDC1080_OK;
}

int xy_hdc1080_read(xy_hdc1080_t *dev)
{
    uint8_t buf[4];
    int ret;
    
    if (!dev || !dev->initialized) {
        return XY_HDC1080_INVALID_PARAM;
    }
    
    /* 触发温湿度测量 */
    buf[0] = HDC1080_REG_TEMP;
    ret = xy_i2c_device_write(&dev->i2c_dev, buf, 1);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 等待测量完成 (典型 6.5ms) */
    xy_os_delay(10);
    
    /* 读取数据 (4 字节：温度 2 字节 + 湿度 2 字节) */
    ret = xy_i2c_device_read(&dev->i2c_dev, buf, 4);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 转换数据 */
    uint16_t temp_raw = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t humi_raw = ((uint16_t)buf[2] << 8) | buf[3];
    
    /* 温度：-40 to 125°C */
    dev->temperature = (int16_t)((int32_t)((int32_t)temp_raw * 16500 / 65535 - 4000));
    
    /* 湿度：0 to 100%RH */
    dev->humidity = (uint16_t)((uint32_t)humi_raw * 10000 / 65535);
    
    xy_log_d("HDC1080: T=%d.%02d°C, H=%d.%02d%%RH\n",
             dev->temperature / 100, dev->temperature % 100,
             dev->humidity / 100, dev->humidity % 100);
    
    return XY_HDC1080_OK;
}

int xy_hdc1080_read_temperature(xy_hdc1080_t *dev, int16_t *temp)
{
    int ret;
    if (!dev || !temp) return XY_HDC1080_INVALID_PARAM;
    ret = xy_hdc1080_read(dev);
    if (ret == XY_HDC1080_OK) {
        *temp = dev->temperature;
    }
    return ret;
}

int xy_hdc1080_read_humidity(xy_hdc1080_t *dev, uint16_t *humi)
{
    int ret;
    if (!dev || !humi) return XY_HDC1080_INVALID_PARAM;
    ret = xy_hdc1080_read(dev);
    if (ret == XY_HDC1080_OK) {
        *humi = dev->humidity;
    }
    return ret;
}

int xy_hdc1080_heater_on(xy_hdc1080_t *dev)
{
    uint16_t config = HDC1080_CONFIG_HEATER;
    uint8_t buf[2] = {(config >> 8) & 0xFF, config & 0xFF};
    return xy_i2c_device_write_reg(&dev->i2c_dev, HDC1080_REG_CONFIG, buf, 2);
}

int xy_hdc1080_heater_off(xy_hdc1080_t *dev)
{
    uint8_t buf[2] = {0, 0};
    return xy_i2c_device_write_reg(&dev->i2c_dev, HDC1080_REG_CONFIG, buf, 2);
}
