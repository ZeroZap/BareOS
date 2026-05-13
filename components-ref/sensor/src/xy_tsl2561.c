/**
 * @file xy_tsl2561.c
 * @brief TSL2561 Digital Light Sensor Driver
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#include "xy_tsl2561.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/**
 * @brief 写入寄存器
 */
static int xy_tsl2561_write_reg(xy_tsl2561_t *tsl2561, uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {TSL2561_CMD_BIT | reg, value};
    return xy_i2c_device_write(&tsl2561->i2c_dev, buf, 2);
}

/**
 * @brief 读取寄存器
 */
static int xy_tsl2561_read_reg(xy_tsl2561_t *tsl2561, uint8_t reg, uint8_t *value)
{
    uint8_t cmd = TSL2561_CMD_BIT | reg;
    return xy_i2c_device_read_reg(&tsl2561->i2c_dev, cmd, value, 1);
}

/**
 * @brief 读取 16 位寄存器
 */
static int xy_tsl2561_read16(xy_tsl2561_t *tsl2561, uint8_t reg, uint16_t *value)
{
    uint8_t buf[2];
    uint8_t cmd = TSL2561_CMD_BIT | TSL2561_WORD_BIT | reg;
    int ret = xy_i2c_device_read_reg(&tsl2561->i2c_dev, cmd, buf, 2);
    if (ret == XY_DEVICE_OK) {
        *value = ((uint16_t)buf[1] << 8) | buf[0];
    }
    return ret;
}

/**
 * @brief 计算照度
 */
static float xy_tsl2561_calculate_lux(xy_tsl2561_t *tsl2561, 
                                      uint16_t broadband, uint16_t ir)
{
    float lux;
    float chScale;
    float ratio;
    long channel1, channel0;
    
    /* 根据增益设置通道缩放比例 */
    if (tsl2561->gain == XY_TSL2561_GAIN_1X) {
        chScale = 322.0f;  /* 1X 增益 */
    } else {
        chScale = 20.0f;   /* 16X 增益 */
    }
    
    /* 根据积分时间调整 */
    switch (tsl2561->integration) {
        case XY_TSL2561_INTEGRATION_13MS:
            chScale = chScale * 322.0f / 408.0f;
            break;
        case XY_TSL2561_INTEGRATION_101MS:
            chScale = chScale * 322.0f / 3220.0f;
            break;
        case XY_TSL2561_INTEGRATION_402MS:
        default:
            break;
    }
    
    /* 计算通道值 */
    channel0 = (broadband * chScale) >> 10;
    channel1 = (ir * chScale) >> 10;
    
    /* 计算比率 */
    if (channel0 != 0) {
        ratio = ((float)channel1 / (float)channel0) * 100.0f;
    } else {
        return 0.0f;
    }
    
    /* 根据比率计算照度 */
    if (ratio <= 50.0f) {
        lux = (0.0304f * channel0) - (0.062f * channel0 * (ratio * ratio / 10000.0f));
    } else if (ratio <= 80.0f) {
        lux = (0.0224f * channel0) - (0.031f * channel1);
    } else if (ratio <= 130.0f) {
        lux = (0.0157f * channel0) - (0.018f * channel1);
    } else if (ratio <= 255.0f) {
        lux = (0.00338f * channel0) - (0.0026f * channel1);
    } else {
        lux = 0.0f;
    }
    
    return lux;
}

int xy_tsl2561_init(xy_tsl2561_t *tsl2561, void *i2c_handle, uint8_t addr)
{
    int ret;
    uint8_t id;
    
    if (!tsl2561 || !i2c_handle) {
        return XY_TSL2561_INVALID_PARAM;
    }
    
    memset(tsl2561, 0, sizeof(*tsl2561));
    xy_i2c_device_init(&tsl2561->i2c_dev, i2c_handle, addr, 400);
    tsl2561->addr = addr;
    tsl2561->gain = XY_TSL2561_GAIN_1X;
    tsl2561->integration = XY_TSL2561_INTEGRATION_402MS;
    
    /* 读取 ID 寄存器验证设备 */
    ret = xy_tsl2561_read_reg(tsl2561, TSL2561_REG_ID, &id);
    if (ret != XY_DEVICE_OK || (id & 0x0A) != 0x0A) {
        xy_log_e("TSL2561 not found (ID=0x%02X)\n", id);
        return XY_TSL2561_NOT_FOUND;
    }
    
    xy_log_i("TSL2561 found at 0x%02X (ID=0x%02X)\n", addr, id);
    
    /* 初始化配置 */
    xy_tsl2561_enable(tsl2561);
    xy_tsl2561_set_gain(tsl2561, tsl2561->gain);
    xy_tsl2561_set_integration(tsl2561, tsl2561->integration);
    
    tsl2561->initialized = true;
    return XY_TSL2561_OK;
}

int xy_tsl2561_deinit(xy_tsl2561_t *tsl2561)
{
    if (!tsl2561) {
        return XY_TSL2561_INVALID_PARAM;
    }
    
    xy_tsl2561_disable(tsl2561);
    tsl2561->initialized = false;
    return XY_TSL2561_OK;
}

int xy_tsl2561_read(xy_tsl2561_t *tsl2561)
{
    int ret;
    uint16_t broadband, ir;
    
    if (!tsl2561 || !tsl2561->initialized) {
        return XY_TSL2561_INVALID_PARAM;
    }
    
    /* 确保传感器已使能 */
    xy_tsl2561_enable(tsl2561);
    
    /* 等待积分时间完成 */
    switch (tsl2561->integration) {
        case XY_TSL2561_INTEGRATION_13MS:
            xy_os_delay(20);
            break;
        case XY_TSL2561_INTEGRATION_101MS:
            xy_os_delay(120);
            break;
        case XY_TSL2561_INTEGRATION_402MS:
            xy_os_delay(420);
            break;
    }
    
    /* 读取宽带通道数据 */
    ret = xy_tsl2561_read16(tsl2561, TSL2561_REG_DATA0_L, &broadband);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    /* 读取红外通道数据 */
    ret = xy_tsl2561_read16(tsl2561, TSL2561_REG_DATA1_L, &ir);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }
    
    tsl22561->data.broadband = broadband;
    tsl2561->data.ir = ir;
    
    /* 计算照度 */
    tsl2561->data.lux = xy_tsl2561_calculate_lux(tsl2561, broadband, ir);
    tsl2561->data.timestamp = xy_os_tick_get();
    
    xy_log_d("TSL2561: broadband=%d, ir=%d, lux=%.2f\n",
             broadband, ir, tsl2561->data.lux);
    
    return XY_TSL2561_OK;
}

int xy_tsl2561_get_broadband(xy_tsl2561_t *tsl2561, uint16_t *broadband)
{
    if (!tsl2561 || !broadband) {
        return XY_TSL2561_INVALID_PARAM;
    }
    
    int ret = xy_tsl2561_read(tsl2561);
    if (ret == XY_TSL2561_OK) {
        *broadband = tsl2561->data.broadband;
    }
    return ret;
}

int xy_tsl2561_get_ir(xy_tsl2561_t *tsl2561, uint16_t *ir)
{
    if (!tsl2561 || !ir) {
        return XY_TSL2561_INVALID_PARAM;
    }
    
    int ret = xy_tsl2561_read(tsl2561);
    if (ret == XY_TSL2561_OK) {
        *ir = tsl2561->data.ir;
    }
    return ret;
}

int xy_tsl2561_get_lux(xy_tsl2561_t *tsl2561, float *lux)
{
    if (!tsl2561 || !lux) {
        return XY_TSL2561_INVALID_PARAM;
    }
    
    int ret = xy_tsl2561_read(tsl2561);
    if (ret == XY_TSL2561_OK) {
        *lux = tsl2561->data.lux;
    }
    return ret;
}

int xy_tsl2561_set_gain(xy_tsl2561_t *tsl2561, xy_tsl2561_gain_t gain)
{
    uint8_t timing_reg;
    
    if (!tsl2561 || gain > XY_TSL2561_GAIN_16X) {
        return XY_TSL2561_INVALID_PARAM;
    }
    
    tsl2561->gain = gain;
    
    /* 读取当前 TIMING 寄存器值 */
    xy_tsl2561_read_reg(tsl2561, TSL2561_REG_TIMING, &timing_reg);
    
    /* 设置增益位 */
    if (gain == XY_TSL2561_GAIN_16X) {
        timing_reg |= 0x10;
    } else {
        timing_reg &= ~0x10;
    }
    
    /* 保持积分时间设置 */
    timing_reg &= 0x03;
    timing_reg |= (tsl2561->integration << 4);
    
    return xy_tsl2561_write_reg(tsl2561, TSL2561_REG_TIMING, timing_reg);
}

int xy_tsl2561_set_integration(xy_tsl2561_t *tsl2561,
                               xy_tsl2561_integration_t integration)
{
    uint8_t timing_reg;
    
    if (!tsl2561 || integration > XY_TSL2561_INTEGRATION_402MS) {
        return XY_TSL2561_INVALID_PARAM;
    }
    
    tsl2561->integration = integration;
    
    /* 读取当前 TIMING 寄存器值 */
    xy_tsl2561_read_reg(tsl2561, TSL2561_REG_TIMING, &timing_reg);
    
    /* 设置积分时间位 */
    timing_reg &= ~0x03;
    timing_reg |= integration;
    
    /* 保持增益设置 */
    if (tsl2561->gain == XY_TSL2561_GAIN_16X) {
        timing_reg |= 0x10;
    }
    
    return xy_tsl2561_write_reg(tsl2561, TSL2561_REG_TIMING, timing_reg);
}

int xy_tsl2561_enable(xy_tsl2561_t *tsl2561)
{
    if (!tsl2561) {
        return XY_TSL2561_INVALID_PARAM;
    }
    
    return xy_tsl2561_write_reg(tsl2561, TSL2561_REG_CONTROL, 0x03);
}

int xy_tsl2561_disable(xy_tsl2561_t *tsl2561)
{
    if (!tsl2561) {
        return XY_TSL2561_INVALID_PARAM;
    }
    
    return xy_tsl2561_write_reg(tsl2561, TSL2561_REG_CONTROL, 0x00);
}
