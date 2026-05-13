/**
 * @file xy_aht20.c
 * @brief AHT10/AHT15/AHT20/AHT21 Temperature & Humidity Sensor Driver
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#include "xy_aht20.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/**
 * @brief 检查传感器状态
 */
static int xy_aht20_check_status(xy_aht20_t *aht20)
{
    uint8_t status;
    int ret;
    uint32_t start;
    
    start = xy_os_tick_get();
    do {
        ret = xy_i2c_device_read(&aht20->i2c_dev, &status, 1);
        if (ret != XY_DEVICE_OK) {
            return ret;
        }
        
        if ((status & 0x80) == 0) {
            return XY_AHT20_OK;
        }
        
        xy_os_delay(10);
    } while ((xy_os_tick_get() - start) < 500);
    
    return XY_AHT20_BUSY;
}

int xy_aht20_init(xy_aht20_t *aht20, void *i2c_handle)
{
    int ret;
    uint8_t cmd[3];
    uint8_t status;
    
    if (!aht20 || !i2c_handle) {
        return XY_AHT20_INVALID_PARAM;
    }
    
    memset(aht20, 0, sizeof(*aht20));
    xy_i2c_device_init(&aht20->i2c_dev, i2c_handle, AHT20_ADDR, 400);
    aht20->addr = AHT20_ADDR;
    
    /* 发送初始化命令 */
    cmd[0] = AHT20_CMD_INIT;
    cmd[1] = 0x08;
    cmd[2] = 0x00;
    ret = xy_i2c_device_write(&aht20->i2c_dev, cmd, 3);
    if (ret != XY_DEVICE_OK) {
        xy_log_e("AHT20 init failed\n");
        return XY_AHT20_ERROR;
    }
    
    xy_os_delay(10);
    
    /* 检查状态 */
    ret = xy_aht20_check_status(aht20);
    if (ret != XY_AHT20_OK) {
        return ret;
    }
    
    /* 读取状态寄存器 */
    ret = xy_i2c_device_read(&aht20->i2c_dev, &status, 1);
    if (ret == XY_DEVICE_OK) {
        aht20->calibrated = (status & 0x08) ? true : false;
        xy_log_i("AHT20 found, calibrated=%d\n", aht20->calibrated);
    }
    
    aht20->initialized = true;
    return XY_AHT20_OK;
}

int xy_aht20_deinit(xy_aht20_t *aht20)
{
    if (!aht20) {
        return XY_AHT20_INVALID_PARAM;
    }
    
    aht20->initialized = false;
    return XY_AHT20_OK;
}

int xy_aht20_read(xy_aht20_t *aht20)
{
    int ret;
    uint8_t cmd[4];
    uint8_t buf[7];
    uint32_t raw_value;
    
    if (!aht20 || !aht20->initialized) {
        return XY_AHT20_INVALID_PARAM;
    }
    
    /* 检查传感器是否空闲 */
    ret = xy_aht20_check_status(aht20);
    if (ret != XY_AHT20_OK) {
        return ret;
    }
    
    /* 触发测量 */
    cmd[0] = AHT20_CMD_TRIGGER;
    cmd[1] = 0x33;
    cmd[2] = 0x00;
    ret = xy_i2c_device_write(&aht20->i2c_dev, cmd, 3);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    xy_os_delay(80);  /* 等待测量完成 */
    
    /* 检查状态 */
    ret = xy_aht20_check_status(aht20);
    if (ret != XY_AHT20_OK) {
        return ret;
    }
    
    /* 读取 7 字节数据 */
    ret = xy_i2c_device_read(&aht20->i2c_dev, buf, 7);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 解析数据 */
    /* 湿度：[19:12] = buf[1], [11:4] = buf[2], [3:0] = buf[3][7:4] */
    raw_value = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | (buf[3] >> 4);
    aht20->data.humidity = (uint16_t)((raw_value * 10000UL) >> 20);
    
    /* 温度：[19:12] = buf[3][3:0], [11:4] = buf[4], [3:0] = buf[5][7:4] */
    raw_value = ((uint32_t)(buf[3] & 0x0F) << 16) | ((uint32_t)buf[4] << 8) | buf[5];
    int32_t temp = (int32_t)((raw_value * 20000L) >> 20) - 5000;
    aht20->data.temperature = (int16_t)temp;
    
    aht20->data.timestamp = xy_os_tick_get();
    
    xy_log_d("AHT20: T=%d.%02d°C, H=%d.%02d%%RH\n",
             aht20->data.temperature / 100, 
             (aht20->data.temperature % 100 < 0 ? -aht20->data.temperature % 100 : aht20->data.temperature % 100),
             aht20->data.humidity / 100, aht20->data.humidity % 100);
    
    return XY_AHT20_OK;
}

int xy_aht20_get_temperature(xy_aht20_t *aht20, int16_t *temperature)
{
    if (!aht20 || !temperature) {
        return XY_AHT20_INVALID_PARAM;
    }
    
    int ret = xy_aht20_read(aht20);
    if (ret == XY_AHT20_OK) {
        *temperature = aht20->data.temperature;
    }
    return ret;
}

int xy_aht20_get_humidity(xy_aht20_t *aht20, uint16_t *humidity)
{
    if (!aht20 || !humidity) {
        return XY_AHT20_INVALID_PARAM;
    }
    
    int ret = xy_aht20_read(aht20);
    if (ret == XY_AHT20_OK) {
        *humidity = aht20->data.humidity;
    }
    return ret;
}

int xy_aht20_reset(xy_aht20_t *aht20)
{
    uint8_t cmd = AHT20_CMD_RESET;
    
    if (!aht20) {
        return XY_AHT20_INVALID_PARAM;
    }
    
    return xy_i2c_device_write(&aht20->i2c_dev, &cmd, 1);
}
