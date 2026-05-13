/**
 * @file xy_bh1750.c
 * @brief BH1750 Ambient Light Sensor Driver
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#include "xy_bh1750.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/**
 * @brief 获取测量命令
 */
static uint8_t xy_bh1750_get_measure_cmd(xy_bh1750_t *bh1750)
{
    uint8_t cmd;
    
    switch (bh1750->resolution) {
        case XY_BH1750_HIGH_RES:
            cmd = bh1750->mode == XY_BH1750_CONTINUOUS ? 
                  BH1750_CMD_CONT_H : BH1750_CMD_ONCE_H;
            break;
        case XY_BH1750_HIGH_RES2:
            cmd = bh1750->mode == XY_BH1750_CONTINUOUS ? 
                  BH1750_CMD_CONT_H2 : BH1750_CMD_ONCE_H2;
            break;
        case XY_BH1750_LOW_RES:
            cmd = bh1750->mode == XY_BH1750_CONTINUOUS ? 
                  BH1750_CMD_CONT_L : BH1750_CMD_ONCE_L;
            break;
        default:
            cmd = BH1750_CMD_CONT_H;
            break;
    }
    
    return cmd;
}

/**
 * @brief 获取测量时间
 */
static uint16_t xy_bh1750_get_measure_time(xy_bh1750_res_t resolution)
{
    switch (resolution) {
        case XY_BH1750_HIGH_RES:
        case XY_BH1750_HIGH_RES2:
            return 180;
        case XY_BH1750_LOW_RES:
            return 16;
        default:
            return 180;
    }
}

int xy_bh1750_init(xy_bh1750_t *bh1750, void *i2c_handle, uint8_t addr)
{
    int ret;
    uint8_t cmd;
    
    if (!bh1750 || !i2c_handle) {
        return XY_BH1750_INVALID_PARAM;
    }
    
    memset(bh1750, 0, sizeof(*bh1750));
    xy_i2c_device_init(&bh1750->i2c_dev, i2c_handle, addr, 400);
    bh1750->addr = addr;
    bh1750->resolution = XY_BH1750_HIGH_RES;
    bh1750->mode = XY_BH1750_ONE_TIME;
    
    /* 开机 */
    cmd = BH1750_CMD_POWER_ON;
    ret = xy_i2c_device_write(&bh1750->i2c_dev, &cmd, 1);
    if (ret != XY_DEVICE_OK) {
        xy_log_e("BH1750 not found\n");
        return XY_BH1750_NOT_FOUND;
    }
    
    xy_os_delay(10);
    
    /* 软件复位 */
    cmd = BH1750_CMD_RESET;
    ret = xy_i2c_device_write(&bh1750->i2c_dev, &cmd, 1);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    xy_os_delay(10);
    
    bh1750->initialized = true;
    xy_log_i("BH1750 initialized at 0x%02X\n", addr);
    
    return XY_BH1750_OK;
}

int xy_bh1750_deinit(xy_bh1750_t *bh1750)
{
    if (!bh1750) {
        return XY_BH1750_INVALID_PARAM;
    }
    
    xy_bh1750_power_down(bh1750);
    bh1750->initialized = false;
    return XY_BH1750_OK;
}

int xy_bh1750_read(xy_bh1750_t *bh1750)
{
    int ret;
    uint8_t cmd;
    uint8_t buf[2];
    uint16_t raw_value;
    uint16_t measure_time;
    
    if (!bh1750 || !bh1750->initialized) {
        return XY_BH1750_INVALID_PARAM;
    }
    
    /* 开机 */
    cmd = BH1750_CMD_POWER_ON;
    ret = xy_i2c_device_write(&bh1750->i2c_dev, &cmd, 1);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    xy_os_delay(10);
    
    /* 发送测量命令 */
    cmd = xy_bh1750_get_measure_cmd(bh1750);
    ret = xy_i2c_device_write(&bh1750->i2c_dev, &cmd, 1);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 等待测量完成 */
    measure_time = xy_bh1750_get_measure_time(bh1750->resolution);
    xy_os_delay(measure_time);
    
    /* 读取数据 */
    ret = xy_i2c_device_read(&bh1750->i2c_dev, buf, 2);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 解析数据 */
    raw_value = ((uint16_t)buf[0] << 8) | buf[1];
    
    /* 转换为照度 (lux) */
    /* 高分辨率模式：1 lux = 2 LSB */
    /* 低分辨率模式：1 lux = 8 LSB */
    if (bh1750->resolution == XY_BH1750_LOW_RES) {
        bh1750->data.illuminance = (float)raw_value / 8.0f;
    } else if (bh1750->resolution == XY_BH1750_HIGH_RES2) {
        bh1750->data.illuminance = (float)raw_value / 2.0f;
    } else {
        bh1750->data.illuminance = (float)raw_value;
    }
    
    bh1750->data.timestamp = xy_os_tick_get();
    
    xy_log_d("BH1750: %.1f lux\n", bh1750->data.illuminance);
    
    return XY_BH1750_OK;
}

int xy_bh1750_get_illuminance(xy_bh1750_t *bh1750, float *illuminance)
{
    if (!bh1750 || !illuminance) {
        return XY_BH1750_INVALID_PARAM;
    }
    
    int ret = xy_bh1750_read(bh1750);
    if (ret == XY_BH1750_OK) {
        *illuminance = bh1750->data.illuminance;
    }
    return ret;
}

int xy_bh1750_set_resolution(xy_bh1750_t *bh1750, xy_bh1750_res_t resolution)
{
    if (!bh1750 || resolution > XY_BH1750_LOW_RES) {
        return XY_BH1750_INVALID_PARAM;
    }
    
    bh1750->resolution = resolution;
    return XY_BH1750_OK;
}

int xy_bh1750_set_mode(xy_bh1750_t *bh1750, xy_bh1750_mode_t mode)
{
    if (!bh1750 || mode > XY_BH1750_ONE_TIME) {
        return XY_BH1750_INVALID_PARAM;
    }
    
    bh1750->mode = mode;
    return XY_BH1750_OK;
}

int xy_bh1750_power_down(xy_bh1750_t *bh1750)
{
    uint8_t cmd = BH1750_CMD_POWER_DOWN;
    
    if (!bh1750) {
        return XY_BH1750_INVALID_PARAM;
    }
    
    return xy_i2c_device_write(&bh1750->i2c_dev, &cmd, 1);
}

int xy_bh1750_power_on(xy_bh1750_t *bh1750)
{
    uint8_t cmd = BH1750_CMD_POWER_ON;
    
    if (!bh1750) {
        return XY_BH1750_INVALID_PARAM;
    }
    
    return xy_i2c_device_write(&bh1750->i2c_dev, &cmd, 1);
}

int xy_bh1750_reset(xy_bh1750_t *bh1750)
{
    uint8_t cmd = BH1750_CMD_RESET;
    
    if (!bh1750) {
        return XY_BH1750_INVALID_PARAM;
    }
    
    return xy_i2c_device_write(&bh1750->i2c_dev, &cmd, 1);
}
