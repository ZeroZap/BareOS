#include "xy_ret.h"
/**
 * @file xy_lps22hb.c
 * @brief LPS22HB Waterproof Barometric Pressure Sensor Driver Implementation
 * @version 1.0.0
 * @date 2026-03-14
 * 
 * LPS22HB 是 ST 出品的防水气压传感器
 * 精度±0.25 hPa (±2m 海拔精度), 适用于户外设备、无人机定高等应用
 */

#include "xy_lps22hb.h"
#include <string.h>
#include <math.h>

#ifndef XY_NULL
#define XY_NULL NULL
#endif

#ifndef XY_OK
#define XY_OK 0
#endif

#ifndef XY_ERROR
#define XY_ERROR -1
#endif

#ifndef XY_TIMEOUT
#define XY_TIMEOUT -2
#endif

/*============================================================================
 * 内部常量定义
 *===========================================================================*/

/* 默认配置 */
#define LPS22HB_DEFAULT_ODR           XY_LPS22HB_ODR_10HZ
#define LPS22HB_DEFAULT_LPFP          XY_LPS22HB_LPF_ODR_20
#define LPS22HB_DEFAULT_ENABLE_LPF    true
#define LPS22HB_DEFAULT_ENABLE_FIFO   false
#define LPS22HB_DEFAULT_I2C_ADDR      0x5D

/* 默认海平面气压 (标准大气压) */
#define LPS22HB_DEFAULT_SEA_LEVEL_PRESSURE  1013.25f

/* 测量超时 */
#define LPS22HB_MEASURE_TIMEOUT_MS    100
#define LPS22HB_POLL_INTERVAL_MS      5

/* 启动时间 */
#define LPS22HB_BOOT_TIME_MS          10

/*============================================================================
 * 内部辅助函数
 *===========================================================================*/

/**
 * @brief 写入寄存器
 */
static xy_ret_t lps22hb_write_reg(xy_lps22hb_dev_t *dev, uint8_t reg_addr, const uint8_t *data, uint16_t len)
{
    if (dev == XY_NULL || dev->interface == XY_NULL || data == XY_NULL) {
        return XY_ERROR;
    }
    return xy_write_reg(dev->interface, reg_addr, data, len);
}

/**
 * @brief 读取寄存器
 */
static xy_ret_t lps22hb_read_reg(xy_lps22hb_dev_t *dev, uint8_t reg_addr, uint8_t *data, uint16_t len)
{
    if (dev == XY_NULL || dev->interface == XY_NULL || data == XY_NULL) {
        return XY_ERROR;
    }
    return xy_read_reg(dev->interface, reg_addr, data, len);
}

/**
 * @brief 写入 8 位寄存器
 */
static xy_ret_t lps22hb_write_reg8(xy_lps22hb_dev_t *dev, uint8_t reg_addr, uint8_t value)
{
    return lps22hb_write_reg(dev, reg_addr, &value, 1);
}

/**
 * @brief 读取 8 位寄存器
 */
static xy_ret_t lps22hb_read_reg8(xy_lps22hb_dev_t *dev, uint8_t reg_addr, uint8_t *value)
{
    return lps22hb_read_reg(dev, reg_addr, value, 1);
}

/**
 * @brief 更新寄存器位
 */
static xy_ret_t lps22hb_update_bits(xy_lps22hb_dev_t *dev, uint8_t reg_addr, uint8_t mask, uint8_t value)
{
    uint8_t reg_value;
    xy_ret_t ret = lps22hb_read_reg8(dev, reg_addr, &reg_value);
    if (ret != XY_OK) return ret;
    
    reg_value = (reg_value & ~mask) | (value & mask);
    return lps22hb_write_reg8(dev, reg_addr, reg_value);
}

/**
 * @brief 等待数据就绪
 */
static xy_ret_t lps22hb_wait_data(xy_lps22hb_dev_t *dev, uint8_t mask, uint32_t timeout_ms)
{
    uint32_t elapsed = 0;
    uint8_t status;
    
    while (elapsed < timeout_ms) {
        xy_ret_t ret = lps22hb_read_reg8(dev, LPS22HB_STATUS, &status);
        if (ret != XY_OK) return ret;
        
        if (status & mask) {
            return XY_OK;
        }
        
        xy_delay_ms(LPS22HB_POLL_INTERVAL_MS);
        elapsed += LPS22HB_POLL_INTERVAL_MS;
    }
    
    return XY_TIMEOUT;
}

/*============================================================================
 * 公开 API 实现
 *===========================================================================*/

xy_ret_t xy_lps22hb_init(xy_lps22hb_dev_t *dev, xy_interface_dev_t *interface, xy_lps22hb_config_t *config)
{
    if (dev == XY_NULL || interface == XY_NULL) {
        return XY_ERROR;
    }
    
    memset(dev, 0, sizeof(xy_lps22hb_dev_t));
    dev->interface = interface;
    
    /* 设置默认配置 */
    dev->config.odr = LPS22HB_DEFAULT_ODR;
    dev->config.lpf = LPS22HB_DEFAULT_LPFP;
    dev->config.enable_lpf = LPS22HB_DEFAULT_ENABLE_LPF;
    dev->config.enable_fifo = LPS22HB_DEFAULT_ENABLE_FIFO;
    dev->config.fifo_mode = XY_LPS22HB_FIFO_BYPASS;
    dev->config.fifo_wtm = 0;
    dev->config.enable_interrupt = false;
    dev->config.threshold.low = 0;
    dev->config.threshold.high = 126000;  /* 1260 hPa * 100 */
    
    if (config != XY_NULL) {
        dev->config = *config;
    }
    
    /* 等待传感器上电稳定 */
    xy_delay_ms(20);
    
    /* 读取 WHO_AM_I 验证连接 */
    xy_ret_t ret = xy_lps22hb_read_who_am_i(dev, &dev->who_am_i);
    if (ret != XY_OK) {
        return ret;
    }
    
    /* 验证 WHO_AM_I (LPS22HB 应为 0xB1) */
    if (dev->who_am_i != LPS22HB_WHO_AM_I_VALUE) {
        return XY_ERROR;
    }
    
    /* 软件复位 */
    ret = xy_lps22hb_soft_reset(dev);
    if (ret != XY_OK) {
        return ret;
    }
    
    xy_delay_ms(LPS22HB_BOOT_TIME_MS);
    
    /* 等待复位完成 */
    uint8_t ctrl2;
    uint32_t timeout = 100;
    while (timeout > 0) {
        ret = lps22hb_read_reg8(dev, LPS22HB_CTRL_REG2, &ctrl2);
        if (ret != XY_OK) return ret;
        
        if (!(ctrl2 & LPS22HB_BOOT)) {
            break;
        }
        
        xy_delay_ms(1);
        timeout--;
    }
    
    if (timeout == 0) {
        return XY_TIMEOUT;
    }
    
    /* 配置 CTRL_REG1 */
    uint8_t ctrl1 = dev->config.odr;
    if (dev->config.enable_lpf) {
        ctrl1 |= LPS22HB_EN_LPFP;
        ctrl1 |= dev->config.lpf;
    }
    
    ret = lps22hb_write_reg8(dev, LPS22HB_CTRL_REG1, ctrl1);
    if (ret != XY_OK) return ret;
    
    /* 配置 CTRL_REG2 */
    uint8_t ctrl2 = 0;
    if (dev->config.enable_fifo) {
        ctrl2 |= LPS22HB_FIFO_EN;
        ctrl2 |= dev->config.fifo_mode;
    }
    
    ret = lps22hb_write_reg8(dev, LPS22HB_CTRL_REG2, ctrl2);
    if (ret != XY_OK) return ret;
    
    /* 配置 CTRL_REG3 (中断) */
    if (dev->config.enable_interrupt) {
        ret = lps22hb_write_reg8(dev, LPS22HB_CTRL_REG3, LPS22HB_INT_DRDY);
        if (ret != XY_OK) return ret;
        
        /* 配置阈值 */
        ret = xy_lps22hb_configure_threshold(dev, dev->config.threshold.low, dev->config.threshold.high);
        if (ret != XY_OK) return ret;
    } else {
        ret = lps22hb_write_reg8(dev, LPS22HB_CTRL_REG3, 0x00);
        if (ret != XY_OK) return ret;
    }
    
    /* 设置默认校准参数 */
    dev->pressure_offset = 0.0f;
    dev->temperature_offset = 0.0f;
    dev->sea_level_pressure = LPS22HB_DEFAULT_SEA_LEVEL_PRESSURE;
    
    dev->is_initialized = true;
    dev->measurement_count = 0;
    
    return XY_OK;
}

xy_ret_t xy_lps22hb_deinit(xy_lps22hb_dev_t *dev)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    /* 停止测量 */
    xy_lps22hb_stop(dev);
    
    dev->is_initialized = false;
    
    return XY_OK;
}

xy_ret_t xy_lps22hb_read_who_am_i(xy_lps22hb_dev_t *dev, uint8_t *who_am_i)
{
    if (dev == XY_NULL || who_am_i == XY_NULL) {
        return XY_ERROR;
    }
    
    return lps22hb_read_reg8(dev, LPS22HB_WHO_AM_I, who_am_i);
}

xy_ret_t xy_lps22hb_soft_reset(xy_lps22hb_dev_t *dev)
{
    if (dev == XY_NULL) {
        return XY_ERROR;
    }
    
    /* 写入 SWRESET 位 */
    xy_ret_t ret = lps22hb_update_bits(dev, LPS22HB_CTRL_REG2, LPS22HB_SWRESET, LPS22HB_SWRESET);
    if (ret != XY_OK) return ret;
    
    /* 等待复位完成 */
    xy_delay_ms(10);
    
    return XY_OK;
}

xy_ret_t xy_lps22hb_start_single(xy_lps22hb_dev_t *dev)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    /* 停止当前测量 */
    xy_lps22hb_stop(dev);
    
    /* 配置为单次测量 (ODR = 0x00) */
    return lps22hb_update_bits(dev, LPS22HB_CTRL_REG1, LPS22HB_ODR_MASK, XY_LPS22HB_ODR_ONE_SHOT);
}

xy_ret_t xy_lps22hb_start_continuous(xy_lps22hb_dev_t *dev)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    /* 配置为连续测量 */
    return lps22hb_update_bits(dev, LPS22HB_CTRL_REG1, LPS22HB_ODR_MASK, dev->config.odr);
}

xy_ret_t xy_lps22hb_stop(xy_lps22hb_dev_t *dev)
{
    if (dev == XY_NULL) {
        return XY_ERROR;
    }
    
    /* 停止测量 (ODR = 0x00) */
    return lps22hb_update_bits(dev, LPS22HB_CTRL_REG1, LPS22HB_ODR_MASK, XY_LPS22HB_ODR_ONE_SHOT);
}

xy_ret_t xy_lps22hb_check_data_ready(xy_lps22hb_dev_t *dev, bool *ready)
{
    if (dev == XY_NULL || ready == XY_NULL) {
        return XY_ERROR;
    }
    
    uint8_t status;
    xy_ret_t ret = lps22hb_read_reg8(dev, LPS22HB_STATUS, &status);
    if (ret != XY_OK) {
        *ready = false;
        return ret;
    }
    
    /* 检查压力和温度数据就绪位 */
    *ready = (status & LPS22HB_P_DA) && (status & LPS22HB_T_DA);
    
    return XY_OK;
}

xy_ret_t xy_lps22hb_read_data(xy_lps22hb_dev_t *dev, xy_lps22hb_data_t *data)
{
    if (dev == XY_NULL || data == XY_NULL) {
        return XY_ERROR;
    }
    
    uint8_t buffer[5];
    xy_ret_t ret = lps22hb_read_reg(dev, LPS22HB_PRESS_OUT_XL, buffer, 5);
    if (ret != XY_OK) return ret;
    
    /* 解析压力数据 (24-bit) */
    uint32_t pressure_raw = ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[1] << 8) | buffer[0];
    
    /* 解析温度数据 (16-bit) */
    int16_t temp_raw = (int16_t)(((uint16_t)buffer[4] << 8) | buffer[3]);
    
    /* 转换为物理单位 */
    float pressure = (float)pressure_raw / 4096.0f;  /* hPa */
    float temperature = (float)temp_raw / 100.0f;    /* °C */
    
    /* 应用偏移校准 */
    pressure += dev->pressure_offset;
    temperature += dev->temperature_offset;
    
    /* 计算海拔高度 */
    float altitude = xy_lps22hb_pressure_to_altitude(pressure, dev->sea_level_pressure);
    
    /* 填充结果结构体 */
    data->pressure = pressure;
    data->temperature = temperature;
    data->altitude = altitude;
    data->timestamp = 0;  /* 简化实现 */
    data->status = 0;     /* 简化实现 */
    
    /* 更新最后测量结果 */
    dev->last_data = *data;
    dev->measurement_count++;
    
    return XY_OK;
}

xy_ret_t xy_lps22hb_measure(xy_lps22hb_dev_t *dev, xy_lps22hb_data_t *data, uint32_t timeout_ms)
{
    if (dev == XY_NULL || data == XY_NULL) {
        return XY_ERROR;
    }
    
    /* 启动单次测量 */
    xy_ret_t ret = xy_lps22hb_start_single(dev);
    if (ret != XY_OK) return ret;
    
    /* 等待数据就绪 */
    ret = lps22hb_wait_data(dev, LPS22HB_P_DA | LPS22HB_T_DA, timeout_ms);
    if (ret != XY_OK) {
        xy_lps22hb_stop(dev);
        return ret;
    }
    
    /* 读取数据 */
    ret = xy_lps22hb_read_data(dev, data);
    if (ret != XY_OK) return ret;
    
    return XY_OK;
}

xy_ret_t xy_lps22hb_set_odr(xy_lps22hb_dev_t *dev, xy_lps22hb_odr_t odr)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    dev->config.odr = odr;
    
    /* 保持低通滤波配置 */
    uint8_t ctrl1;
    xy_ret_t ret = lps22hb_read_reg8(dev, LPS22HB_CTRL_REG1, &ctrl1);
    if (ret != XY_OK) return ret;
    
    ctrl1 = (ctrl1 & ~LPS22HB_ODR_MASK) | odr;
    
    return lps22hb_write_reg8(dev, LPS22HB_CTRL_REG1, ctrl1);
}

xy_ret_t xy_lps22hb_configure_lpf(xy_lps22hb_dev_t *dev, bool enable, xy_lps22hb_lpf_t lpf)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    dev->config.enable_lpf = enable;
    dev->config.lpf = lpf;
    
    uint8_t ctrl1;
    xy_ret_t ret = lps22hb_read_reg8(dev, LPS22HB_CTRL_REG1, &ctrl1);
    if (ret != XY_OK) return ret;
    
    /* 清除 LPF 配置位 */
    ctrl1 &= ~(LPS22HB_EN_LPFP | LPS22HB_LPFP_CFG_MASK);
    
    if (enable) {
        ctrl1 |= LPS22HB_EN_LPFP;
        ctrl1 |= lpf;
    }
    
    return lps22hb_write_reg8(dev, LPS22HB_CTRL_REG1, ctrl1);
}

void xy_lps22hb_set_pressure_offset(xy_lps22hb_dev_t *dev, float offset)
{
    if (dev == XY_NULL) return;
    dev->pressure_offset = offset;
}

void xy_lps22hb_set_temperature_offset(xy_lps22hb_dev_t *dev, float offset)
{
    if (dev == XY_NULL) return;
    dev->temperature_offset = offset;
}

void xy_lps22hb_set_sea_level_pressure(xy_lps22hb_dev_t *dev, float pressure)
{
    if (dev == XY_NULL) return;
    dev->sea_level_pressure = pressure;
}

xy_ret_t xy_lps22hb_auto_zero(xy_lps22hb_dev_t *dev)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    /* 启动自动归零 */
    xy_ret_t ret = lps22hb_update_bits(dev, LPS22HB_CTRL_REG2, LPS22HB_AUTOZERO, LPS22HB_AUTOZERO);
    if (ret != XY_OK) return ret;
    
    /* 等待完成 */
    xy_delay_ms(10);
    
    uint8_t ctrl2;
    uint32_t timeout = 100;
    while (timeout > 0) {
        ret = lps22hb_read_reg8(dev, LPS22HB_CTRL_REG2, &ctrl2);
        if (ret != XY_OK) return ret;
        
        if (!(ctrl2 & LPS22HB_AUTOZERO)) {
            break;
        }
        
        xy_delay_ms(1);
        timeout--;
    }
    
    return (timeout > 0) ? XY_OK : XY_TIMEOUT;
}

xy_ret_t xy_lps22hb_configure_fifo(xy_lps22hb_dev_t *dev, xy_lps22hb_fifo_mode_t mode, uint8_t wtm)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    dev->config.fifo_mode = mode;
    dev->config.fifo_wtm = wtm;
    
    /* 配置 FIFO_CTRL */
    uint8_t fifo_ctrl = mode | (wtm & 0x1F);
    xy_ret_t ret = lps22hb_write_reg8(dev, LPS22HB_FIFO_CTRL, fifo_ctrl);
    if (ret != XY_OK) return ret;
    
    /* 使能/禁用 FIFO */
    if (mode != XY_LPS22HB_FIFO_BYPASS) {
        dev->config.enable_fifo = true;
        return lps22hb_update_bits(dev, LPS22HB_CTRL_REG2, LPS22HB_FIFO_EN, LPS22HB_FIFO_EN);
    } else {
        dev->config.enable_fifo = false;
        return lps22hb_update_bits(dev, LPS22HB_CTRL_REG2, LPS22HB_FIFO_EN, 0x00);
    }
}

xy_ret_t xy_lps22hb_configure_threshold(xy_lps22hb_dev_t *dev, uint16_t low, uint16_t high)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    dev->config.threshold.low = low;
    dev->config.threshold.high = high;
    
    uint8_t buffer[2];
    
    /* 配置低阈值 */
    buffer[0] = low & 0xFF;
    buffer[1] = (low >> 8) & 0xFF;
    xy_ret_t ret = lps22hb_write_reg(dev, LPS22HB_THS_P_L, buffer, 2);
    if (ret != XY_OK) return ret;
    
    /* 配置高阈值 */
    buffer[0] = high & 0xFF;
    buffer[1] = (high >> 8) & 0xFF;
    return lps22hb_write_reg(dev, LPS22HB_THS_P_H, buffer, 2);
}

xy_ret_t xy_lps22hb_clear_interrupt(xy_lps22hb_dev_t *dev)
{
    if (dev == XY_NULL) {
        return XY_ERROR;
    }
    
    /* 读取 INT_SOURCE 清除中断 */
    uint8_t int_source;
    return lps22hb_read_reg8(dev, LPS22HB_INT_SOURCE, &int_source);
}

bool xy_lps22hb_is_ready(xy_lps22hb_dev_t *dev)
{
    if (dev == XY_NULL) return false;
    return dev->is_initialized;
}

xy_lps22hb_data_t *xy_lps22hb_get_last_data(xy_lps22hb_dev_t *dev)
{
    if (dev == XY_NULL) return XY_NULL;
    return &dev->last_data;
}

/*============================================================================
 * 工具函数实现
 *===========================================================================*/

float xy_lps22hb_pressure_to_altitude(float pressure, float sea_level_pressure)
{
    /* 使用国际大气公式 (Hypsometric formula) */
    /* h = 44330 * (1 - (P / P0)^(1/5.255)) */
    
    if (pressure <= 0 || sea_level_pressure <= 0) {
        return 0.0f;
    }
    
    float ratio = pressure / sea_level_pressure;
    float exponent = 1.0f / 5.255f;
    
    return 44330.0f * (1.0f - powf(ratio, exponent));
}

float xy_lps22hb_altitude_to_pressure(float altitude, float sea_level_pressure)
{
    /* 反向计算：P = P0 * (1 - h/44330)^5.255 */
    
    if (sea_level_pressure <= 0) {
        return 0.0f;
    }
    
    float ratio = 1.0f - (altitude / 44330.0f);
    float exponent = 5.255f;
    
    return sea_level_pressure * powf(ratio, exponent);
}
