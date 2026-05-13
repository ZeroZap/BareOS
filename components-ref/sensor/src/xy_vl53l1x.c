/**
 * @file xy_vl53l1x.c
 * @brief VL53L1X Time-of-Flight (ToF) Distance Sensor Driver Implementation
 * @version 1.0.0
 * @date 2026-03-14
 * 
 * VL53L1X 是 ST 出品的高性能 ToF 测距传感器
 * 测距范围 0-4 米，精度±3%，适用于机器人避障、液位检测等应用
 */

#include "xy_vl53l1x.h"
#include <string.h>

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

/* 默认配置值 */
#define VL53L1X_DEFAULT_RANGE           XY_VL53L1X_RANGE_MEDIUM
#define VL53L1X_DEFAULT_TIMING          XY_VL53L1X_TIMING_33MS
#define VL53L1X_DEFAULT_MODE            XY_VL53L1X_MODE_SINGLE
#define VL53L1X_DEFAULT_I2C_ADDR        0x29

/* 测量超时时间 */
#define VL53L1X_SINGLE_TIMEOUT_MS       100
#define VL53L1X_POLL_INTERVAL_MS        5

/* 校准默认值 */
#define VL53L1X_DEFAULT_OFFSET          0
#define VL53L1X_DEFAULT_XTALK           0.0f

/* 校准采样次数 */
#define VL53L1X_CALIBRATION_MIN_SAMPLES 10
#define VL53L1X_CALIBRATION_MAX_SAMPLES 100

/*============================================================================
 * 内部辅助函数
 *===========================================================================*/

/**
 * @brief 写入 16 位寄存器地址
 */
static xy_ret_t vl53l1x_write_reg(xy_vl53l1x_dev_t *dev, uint16_t reg_addr, const uint8_t *data, uint16_t len)
{
    if (dev == XY_NULL || dev->i2c == XY_NULL || data == XY_NULL) {
        return XY_ERROR;
    }
    return xy_i2c_write_reg16(dev->i2c, reg_addr, data, len);
}

/**
 * @brief 读取 16 位寄存器地址
 */
static xy_ret_t vl53l1x_read_reg(xy_vl53l1x_dev_t *dev, uint16_t reg_addr, uint8_t *data, uint16_t len)
{
    if (dev == XY_NULL || dev->i2c == XY_NULL || data == XY_NULL) {
        return XY_ERROR;
    }
    return xy_i2c_read_reg16(dev->i2c, reg_addr, data, len);
}

/**
 * @brief 写入 8 位寄存器
 */
static xy_ret_t vl53l1x_write_reg8(xy_vl53l1x_dev_t *dev, uint16_t reg_addr, uint8_t value)
{
    return vl53l1x_write_reg(dev, reg_addr, &value, 1);
}

/**
 * @brief 读取 8 位寄存器
 */
static xy_ret_t vl53l1x_read_reg8(xy_vl53l1x_dev_t *dev, uint16_t reg_addr, uint8_t *value)
{
    return vl53l1x_read_reg(dev, reg_addr, value, 1);
}

/**
 * @brief 写入 16 位寄存器
 */
static xy_ret_t vl53l1x_write_reg16(xy_vl53l1x_dev_t *dev, uint16_t reg_addr, uint16_t value)
{
    uint8_t buffer[2];
    buffer[0] = (value >> 8) & 0xFF;
    buffer[1] = value & 0xFF;
    return vl53l1x_write_reg(dev, reg_addr, buffer, 2);
}

/**
 * @brief 读取 16 位寄存器
 */
static xy_ret_t vl53l1x_read_reg16(xy_vl53l1x_dev_t *dev, uint16_t reg_addr, uint16_t *value)
{
    uint8_t buffer[2];
    xy_ret_t ret = vl53l1x_read_reg(dev, reg_addr, buffer, 2);
    if (ret != XY_OK) return ret;
    *value = ((uint16_t)buffer[0] << 8) | buffer[1];
    return XY_OK;
}

/**
 * @brief 更新寄存器位
 */
static xy_ret_t vl53l1x_update_bits(xy_vl53l1x_dev_t *dev, uint16_t reg_addr, uint8_t mask, uint8_t value)
{
    uint8_t reg_value;
    xy_ret_t ret = vl53l1x_read_reg8(dev, reg_addr, &reg_value);
    if (ret != XY_OK) return ret;
    
    reg_value = (reg_value & ~mask) | (value & mask);
    return vl53l1x_write_reg8(dev, reg_addr, reg_value);
}

/**
 * @brief 等待寄存器值满足条件
 */
static xy_ret_t vl53l1x_wait_reg_value(xy_vl53l1x_dev_t *dev, uint16_t reg_addr, uint8_t expected, uint32_t timeout_ms)
{
    uint32_t start = 0;  /* 实际应用中应使用系统时间 */
    uint8_t value;
    
    while ((start - start) < timeout_ms) {  /* 简化实现 */
        xy_ret_t ret = vl53l1x_read_reg8(dev, reg_addr, &value);
        if (ret != XY_OK) return ret;
        
        if (value == expected) {
            return XY_OK;
        }
        
        xy_delay_ms(VL53L1X_POLL_INTERVAL_MS);
    }
    
    return XY_TIMEOUT;
}

/**
 * @brief 获取测距范围对应的时序配置
 */
static void vl53l1x_get_range_timing(xy_vl53l1x_range_t range, uint8_t *vcsel_a, uint8_t *vcsel_b, uint8_t *timeout_a, uint8_t *timeout_b)
{
    switch (range) {
        case XY_VL53L1X_RANGE_SHORT:
            *vcsel_a = 0x07;
            *vcsel_b = 0x05;
            *timeout_a = 0x07;
            *timeout_b = 0x05;
            break;
        case XY_VL53L1X_RANGE_MEDIUM:
            *vcsel_a = 0x0B;
            *vcsel_b = 0x09;
            *timeout_a = 0x09;
            *timeout_b = 0x07;
            break;
        case XY_VL53L1X_RANGE_LONG:
            *vcsel_a = 0x0F;
            *vcsel_b = 0x0D;
            *timeout_a = 0x0B;
            *timeout_b = 0x09;
            break;
        default:
            *vcsel_a = 0x0B;
            *vcsel_b = 0x09;
            *timeout_a = 0x09;
            *timeout_b = 0x07;
    }
}

/**
 * @brief 应用测距配置
 */
static xy_ret_t vl53l1x_apply_range_config(xy_vl53l1x_dev_t *dev)
{
    uint8_t vcsel_a, vcsel_b, timeout_a, timeout_b;
    vl53l1x_get_range_timing(dev->config.range, &vcsel_a, &vcsel_b, &timeout_a, &timeout_b);
    
    /* 配置 VCSEL 周期 */
    xy_ret_t ret = vl53l1x_write_reg8(dev, 0x0060, vcsel_a);  /* PHASECAL_CONFIG_VCSEL_START */
    if (ret != XY_OK) return ret;
    
    ret = vl53l1x_write_reg8(dev, 0x0070, vcsel_b);
    if (ret != XY_OK) return ret;
    
    /* 配置超时 */
    ret = vl53l1x_write_reg8(dev, 0x0064, timeout_a);
    if (ret != XY_OK) return ret;
    
    ret = vl53l1x_write_reg8(dev, 0x0074, timeout_b);
    if (ret != XY_OK) return ret;
    
    return XY_OK;
}

/*============================================================================
 * 公开 API 实现
 *===========================================================================*/

xy_ret_t xy_vl53l1x_init(xy_vl53l1x_dev_t *dev, xy_i2c_dev_t *i2c, xy_vl53l1x_config_t *config)
{
    if (dev == XY_NULL || i2c == XY_NULL) {
        return XY_ERROR;
    }
    
    memset(dev, 0, sizeof(xy_vl53l1x_dev_t));
    dev->i2c = i2c;
    
    /* 设置默认配置 */
    dev->config.mode = VL53L1X_DEFAULT_MODE;
    dev->config.range = VL53L1X_DEFAULT_RANGE;
    dev->config.timing = VL53L1X_DEFAULT_TIMING;
    dev->config.int_mode = XY_VL53L1X_INT_DISABLED;
    dev->config.i2c_address = VL53L1X_DEFAULT_I2C_ADDR;
    dev->config.roi.centre_spad = 199;  /* 默认中心 SPAD */
    dev->config.roi.width = 16;
    dev->config.roi.height = 16;
    dev->config.threshold.low = 0;
    dev->config.threshold.high = 4000;
    
    if (config != XY_NULL) {
        dev->config = *config;
    }
    
    /* 等待传感器上电稳定 */
    xy_delay_ms(50);
    
    /* 读取设备信息验证连接 */
    xy_ret_t ret = xy_vl53l1x_read_device_info(dev, &dev->model_id, &dev->module_type, &dev->revision_id);
    if (ret != XY_OK) {
        return ret;
    }
    
    /* 验证模型 ID (VL53L1X 应为 0xEA) */
    if (dev->model_id != 0xEA) {
        return XY_ERROR;
    }
    
    /* 软件复位 */
    ret = xy_vl53l1x_soft_reset(dev);
    if (ret != XY_OK) {
        return ret;
    }
    
    xy_delay_ms(10);
    
    /* 应用测距配置 */
    ret = vl53l1x_apply_range_config(dev);
    if (ret != XY_OK) return ret;
    
    /* 配置测量定时 */
    ret = xy_vl53l1x_set_timing(dev, dev->config.timing);
    if (ret != XY_OK) return ret;
    
    /* 配置 ROI */
    ret = xy_vl53l1x_set_roi(dev, &dev->config.roi);
    if (ret != XY_OK) return ret;
    
    /* 应用校准参数 */
    dev->offset = VL53L1X_DEFAULT_OFFSET;
    dev->xtalk = VL53L1X_DEFAULT_XTALK;
    
    dev->is_initialized = true;
    dev->measurement_count = 0;
    
    return XY_OK;
}

xy_ret_t xy_vl53l1x_deinit(xy_vl53l1x_dev_t *dev)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    /* 停止测量 */
    xy_vl53l1x_stop(dev);
    
    dev->is_initialized = false;
    
    return XY_OK;
}

xy_ret_t xy_vl53l1x_read_device_info(xy_vl53l1x_dev_t *dev, uint8_t *model_id, uint8_t *module_type, uint16_t *revision_id)
{
    if (dev == XY_NULL) {
        return XY_ERROR;
    }
    
    xy_ret_t ret = XY_OK;
    
    if (model_id != XY_NULL) {
        ret = vl53l1x_read_reg8(dev, 0x010F, model_id);
        if (ret != XY_OK) return ret;
    }
    
    if (module_type != XY_NULL) {
        ret = vl53l1x_read_reg8(dev, 0x0110, module_type);
        if (ret != XY_OK) return ret;
    }
    
    if (revision_id != XY_NULL) {
        uint8_t buffer[2];
        ret = vl53l1x_read_reg(dev, 0x0112, buffer, 2);
        if (ret != XY_OK) return ret;
        *revision_id = ((uint16_t)buffer[0] << 8) | buffer[1];
    }
    
    return XY_OK;
}

xy_ret_t xy_vl53l1x_soft_reset(xy_vl53l1x_dev_t *dev)
{
    if (dev == XY_NULL) {
        return XY_ERROR;
    }
    
    /* 写入软件复位寄存器 */
    xy_ret_t ret = vl53l1x_write_reg8(dev, VL53L1X_SOFTWARE_RESET, 0x00);
    if (ret != XY_OK) return ret;
    
    /* 等待复位完成 */
    xy_delay_ms(10);
    
    /* 等待传感器准备好 */
    ret = vl53l1x_wait_reg_value(dev, 0x0000, 0x00, 100);
    
    return ret;
}

xy_ret_t xy_vl53l1x_start_single(xy_vl53l1x_dev_t *dev)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    /* 停止之前的测量 */
    xy_vl53l1x_stop(dev);
    
    /* 配置为单次模式 */
    xy_ret_t ret = vl53l1x_write_reg8(dev, VL53L1X_SYSTEM_START, 0x10);
    
    return ret;
}

xy_ret_t xy_vl53l1x_start_continuous(xy_vl53l1x_dev_t *dev, uint32_t period_ms)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    /* 停止之前的测量 */
    xy_vl53l1x_stop(dev);
    
    /* 配置测量间隔 */
    if (period_ms > 0) {
        uint32_t intermeasurement = period_ms * 1000;  /* 转换为微秒 */
        uint8_t buffer[4];
        buffer[0] = (intermeasurement >> 24) & 0xFF;
        buffer[1] = (intermeasurement >> 16) & 0xFF;
        buffer[2] = (intermeasurement >> 8) & 0xFF;
        buffer[3] = intermeasurement & 0xFF;
        
        xy_ret_t ret = vl53l1x_write_reg(dev, VL53L1X_SYSTEM_INTERMEASUREMENT_PERIOD, buffer, 4);
        if (ret != XY_OK) return ret;
    }
    
    /* 启动连续测量 */
    return vl53l1x_write_reg8(dev, VL53L1X_SYSTEM_START, 0x02);
}

xy_ret_t xy_vl53l1x_stop(xy_vl53l1x_dev_t *dev)
{
    if (dev == XY_NULL) {
        return XY_ERROR;
    }
    
    /* 写入停止命令 */
    return vl53l1x_write_reg8(dev, VL53L1X_SYSTEM_START, 0x00);
}

xy_ret_t xy_vl53l1x_check_data_ready(xy_vl53l1x_dev_t *dev, bool *ready)
{
    if (dev == XY_NULL || ready == XY_NULL) {
        return XY_ERROR;
    }
    
    uint8_t interrupt_status;
    xy_ret_t ret = vl53l1x_read_reg8(dev, VL53L1X_RESULT_INTERRUPT_STATUS, &interrupt_status);
    if (ret != XY_OK) {
        *ready = false;
        return ret;
    }
    
    /* 检查数据就绪位 */
    *ready = ((interrupt_status & 0x07) == 0x04);
    
    return XY_OK;
}

xy_ret_t xy_vl53l1x_read_result(xy_vl53l1x_dev_t *dev, xy_vl53l1x_result_t *result)
{
    if (dev == XY_NULL || result == XY_NULL) {
        return XY_ERROR;
    }
    
    uint8_t buffer[10];
    xy_ret_t ret = vl53l1x_read_reg(dev, VL53L1X_RESULT_RANGE_STATUS, buffer, 10);
    if (ret != XY_OK) return ret;
    
    /* 解析结果 */
    result->status = buffer[0] & 0x1F;
    result->spad_count = buffer[1];
    
    /* 信号率 (16-bit) */
    result->signal_rate = ((uint16_t)buffer[2] << 8) | buffer[3];
    
    /* 距离 (16-bit, mm) */
    uint16_t raw_distance = ((uint16_t)buffer[8] << 8) | buffer[9];
    
    /* 应用偏移校准 */
    if (raw_distance > 0 && dev->offset != 0) {
        if (raw_distance > (uint16_t)dev->offset) {
            result->distance = raw_distance - dev->offset;
        } else {
            result->distance = 0;
        }
    } else {
        result->distance = raw_distance;
    }
    
    result->ambient_rate = 0;  /* 简化实现 */
    result->timestamp = 0;     /* 简化实现 */
    
    /* 更新测量计数 */
    if (result->status == XY_VL53L1X_STATUS_VALID) {
        dev->measurement_count++;
        dev->last_result = *result;
    }
    
    return XY_OK;
}

xy_ret_t xy_vl53l1x_measure(xy_vl53l1x_dev_t *dev, xy_vl53l1x_result_t *result, uint32_t timeout_ms)
{
    if (dev == XY_NULL || result == XY_NULL) {
        return XY_ERROR;
    }
    
    /* 启动单次测量 */
    xy_ret_t ret = xy_vl53l1x_start_single(dev);
    if (ret != XY_OK) return ret;
    
    /* 等待测量完成 */
    uint32_t elapsed = 0;
    bool ready = false;
    
    while (elapsed < timeout_ms) {
        xy_delay_ms(VL53L1X_POLL_INTERVAL_MS);
        elapsed += VL53L1X_POLL_INTERVAL_MS;
        
        ret = xy_vl53l1x_check_data_ready(dev, &ready);
        if (ret != XY_OK) return ret;
        
        if (ready) {
            break;
        }
    }
    
    if (!ready) {
        xy_vl53l1x_stop(dev);
        return XY_TIMEOUT;
    }
    
    /* 读取结果 */
    ret = xy_vl53l1x_read_result(dev, result);
    if (ret != XY_OK) return ret;
    
    /* 清除中断 */
    xy_vl53l1x_clear_interrupt(dev);
    
    return XY_OK;
}

xy_ret_t xy_vl53l1x_set_range(xy_vl53l1x_dev_t *dev, xy_vl53l1x_range_t range)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    dev->config.range = range;
    return vl53l1x_apply_range_config(dev);
}

xy_ret_t xy_vl53l1x_set_timing(xy_vl53l1x_dev_t *dev, xy_vl53l1x_timing_t timing)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    dev->config.timing = timing;
    
    /* 配置测量时间预算 */
    uint16_t timing_budget = (uint16_t)timing * 1000;  /* 转换为微秒 */
    return vl53l1x_write_reg16(dev, 0x0008, timing_budget);
}

xy_ret_t xy_vl53l1x_set_roi(xy_vl53l1x_dev_t *dev, xy_vl53l1x_roi_t *roi)
{
    if (dev == XY_NULL || !dev->is_initialized || roi == XY_NULL) {
        return XY_ERROR;
    }
    
    /* 配置 ROI 中心 SPAD */
    xy_ret_t ret = vl53l1x_write_reg8(dev, 0x0016, roi->centre_spad);
    if (ret != XY_OK) return ret;
    
    /* 配置 ROI 尺寸 */
    uint8_t roi_size = ((roi->width & 0x0F) << 4) | (roi->height & 0x0F);
    return vl53l1x_write_reg8(dev, 0x0017, roi_size);
}

void xy_vl53l1x_set_offset(xy_vl53l1x_dev_t *dev, int16_t offset)
{
    if (dev == XY_NULL) return;
    dev->offset = offset;
}

void xy_vl53l1x_set_xtalk(xy_vl53l1x_dev_t *dev, float xtalk)
{
    if (dev == XY_NULL) return;
    dev->xtalk = xtalk;
}

xy_ret_t xy_vl53l1x_calibrate_offset(xy_vl53l1x_dev_t *dev, uint16_t target_distance, uint8_t samples)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    if (samples < VL53L1X_CALIBRATION_MIN_SAMPLES) {
        samples = VL53L1X_CALIBRATION_MIN_SAMPLES;
    }
    if (samples > VL53L1X_CALIBRATION_MAX_SAMPLES) {
        samples = VL53L1X_CALIBRATION_MAX_SAMPLES;
    }
    
    int32_t sum = 0;
    uint8_t valid_count = 0;
    
    /* 多次采样 */
    for (uint8_t i = 0; i < samples; i++) {
        xy_vl53l1x_result_t result;
        xy_ret_t ret = xy_vl53l1x_measure(dev, &result, VL53L1X_SINGLE_TIMEOUT_MS);
        
        if (ret == XY_OK && result.status == XY_VL53L1X_STATUS_VALID) {
            sum += result.distance;
            valid_count++;
        }
        
        xy_delay_ms(10);
    }
    
    if (valid_count == 0) {
        return XY_ERROR;
    }
    
    /* 计算平均偏移 */
    int32_t avg_distance = sum / valid_count;
    dev->offset = (uint16_t)avg_distance - target_distance;
    
    return XY_OK;
}

xy_ret_t xy_vl53l1x_calibrate_xtalk(xy_vl53l1x_dev_t *dev, uint16_t target_distance, uint8_t target_reflectance, uint8_t samples)
{
    /* 简化实现 - 完整实现需要更复杂的计算 */
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    /* 串扰校准需要覆盖黑色目标物，这里仅做框架实现 */
    /* 实际应用需要参考 ST 官方校准算法 */
    
    return XY_OK;
}

xy_ret_t xy_vl53l1x_configure_interrupt(xy_vl53l1x_dev_t *dev, xy_vl53l1x_int_mode_t mode, uint16_t low, uint16_t high)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    dev->config.int_mode = mode;
    dev->config.threshold.low = low;
    dev->config.threshold.high = high;
    
    /* 配置中断模式 */
    xy_ret_t ret = vl53l1x_write_reg8(dev, VL53L1X_SYSTEM_INTERRUPT_CONFIG_GPIO, mode);
    if (ret != XY_OK) return ret;
    
    /* 配置阈值 */
    ret = vl53l1x_write_reg16(dev, VL53L1X_SYSTEM_THRESH_RATE_LOW, low);
    if (ret != XY_OK) return ret;
    
    return vl53l1x_write_reg16(dev, VL53L1X_SYSTEM_THRESH_RATE_HIGH, high);
}

xy_ret_t xy_vl53l1x_clear_interrupt(xy_vl53l1x_dev_t *dev)
{
    if (dev == XY_NULL) {
        return XY_ERROR;
    }
    
    return vl53l1x_write_reg8(dev, VL53L1X_SYSTEM_INTERRUPT_CLEAR, 0x01);
}

xy_ret_t xy_vl53l1x_change_i2c_address(xy_vl53l1x_dev_t *dev, uint8_t new_address)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    /* VL53L1X 新地址需要左移 1 位 */
    uint8_t addr = (new_address << 1) & 0xFE;
    
    xy_ret_t ret = vl53l1x_write_reg8(dev, 0x0001, addr);
    if (ret != XY_OK) return ret;
    
    /* 更新设备地址 */
    dev->i2c->address = new_address;
    dev->config.i2c_address = new_address;
    
    xy_delay_ms(10);
    
    return XY_OK;
}

bool xy_vl53l1x_is_ready(xy_vl53l1x_dev_t *dev)
{
    if (dev == XY_NULL) return false;
    return dev->is_initialized;
}

xy_vl53l1x_result_t *xy_vl53l1x_get_last_result(xy_vl53l1x_dev_t *dev)
{
    if (dev == XY_NULL) return XY_NULL;
    return &dev->last_result;
}
