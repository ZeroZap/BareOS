/**
 * @file xy_sgp40.c
 * @brief SGP40 VOC Gas Sensor Driver Implementation
 * @version 1.0.0
 * @date 2026-03-14
 * 
 * SGP40 是 Sensirion 出品的 VOC 气体传感器
 * 内置温湿度补偿，长期稳定性好，适用于室内空气质量监测
 */

#include "xy_sgp40.h"
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

/* 默认配置 */
#define SGP40_DEFAULT_ENABLE_COMP   true
#define SGP40_DEFAULT_TEMPERATURE   25.0f
#define SGP40_DEFAULT_HUMIDITY      50.0f
#define SGP40_DEFAULT_I2C_ADDR      0x59

/* 预热时间 (10 秒) */
#define SGP40_WARMUP_MS             10000

/* 数据就绪轮询间隔 */
#define SGP40_POLL_INTERVAL_MS      10

/*============================================================================
 * 内部辅助函数
 *===========================================================================*/

/**
 * @brief CRC8 计算实现
 */
uint8_t xy_sgp40_crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = SGP40_CRC8_INIT_VALUE;
    
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ SGP40_CRC8_POLYNOMIAL;
            } else {
                crc = crc << 1;
            }
        }
    }
    
    return crc;
}

/**
 * @brief 验证 CRC
 */
static bool sgp40_verify_crc(const uint8_t *data, uint16_t len, uint8_t expected_crc)
{
    return xy_sgp40_crc8(data, len) == expected_crc;
}

/**
 * @brief 写入命令
 */
static xy_ret_t sgp40_write_command(xy_sgp40_dev_t *dev, uint16_t command)
{
    if (dev == XY_NULL || dev->i2c == XY_NULL) {
        return XY_ERROR;
    }
    return xy_i2c_write_command(dev->i2c, command);
}

/**
 * @brief 写入数据
 */
static xy_ret_t sgp40_write_data(xy_sgp40_dev_t *dev, const uint8_t *data, uint16_t len)
{
    if (dev == XY_NULL || dev->i2c == XY_NULL || data == XY_NULL) {
        return XY_ERROR;
    }
    return xy_i2c_write_data(dev->i2c, data, len);
}

/**
 * @brief 读取数据 (带 CRC 验证)
 */
static xy_ret_t sgp40_read_data(xy_sgp40_dev_t *dev, uint8_t *data, uint16_t len)
{
    if (dev == XY_NULL || dev->i2c == XY_NULL || data == XY_NULL) {
        return XY_ERROR;
    }
    
    /* SGP40 每 2 字节数据后跟 1 字节 CRC */
    uint16_t expected_len = (len / 2) * 3;  /* 2 字节数据 + 1 字节 CRC */
    uint8_t *buffer = data;
    
    xy_ret_t ret = xy_i2c_read_data(dev->i2c, buffer, expected_len);
    if (ret != XY_OK) return ret;
    
    /* 验证 CRC 并解包数据 */
    uint16_t data_idx = 0;
    uint16_t buffer_idx = 0;
    
    while (data_idx < len && buffer_idx < expected_len) {
        uint8_t byte1 = buffer[buffer_idx++];
        uint8_t byte2 = buffer[buffer_idx++];
        uint8_t crc = buffer[buffer_idx++];
        
        /* 验证 CRC */
        if (!sgp40_verify_crc(&byte1, 2, crc)) {
            return XY_ERROR;
        }
        
        data[data_idx++] = byte1;
        data[data_idx++] = byte2;
    }
    
    return XY_OK;
}

/**
 * @brief 读取 16 位数据 (带 CRC)
 */
static xy_ret_t sgp40_read_u16(xy_sgp40_dev_t *dev, uint16_t *value)
{
    uint8_t buffer[3];  /* 2 字节数据 + 1 字节 CRC */
    
    xy_ret_t ret = xy_i2c_read_data(dev->i2c, buffer, 3);
    if (ret != XY_OK) return ret;
    
    /* 验证 CRC */
    if (!sgp40_verify_crc(buffer, 2, buffer[2])) {
        return XY_ERROR;
    }
    
    *value = ((uint16_t)buffer[0] << 8) | buffer[1];
    
    return XY_OK;
}

/**
 * @brief 等待测量完成
 */
static xy_ret_t sgp40_wait_measurement(xy_sgp40_dev_t *dev, uint32_t timeout_ms)
{
    uint32_t elapsed = 0;
    
    /* SGP40 测量时间固定约 30ms */
    while (elapsed < timeout_ms) {
        xy_delay_ms(SGP40_POLL_INTERVAL_MS);
        elapsed += SGP40_POLL_INTERVAL_MS;
        
        if (elapsed >= SGP40_MEASURE_TIME_MS) {
            return XY_OK;
        }
    }
    
    return XY_TIMEOUT;
}

/*============================================================================
 * 公开 API 实现
 *===========================================================================*/

xy_ret_t xy_sgp40_init(xy_sgp40_dev_t *dev, xy_i2c_dev_t *i2c, xy_sgp40_config_t *config)
{
    if (dev == XY_NULL || i2c == XY_NULL) {
        return XY_ERROR;
    }
    
    memset(dev, 0, sizeof(xy_sgp40_dev_t));
    dev->i2c = i2c;
    
    /* 设置默认配置 */
    dev->config.enable_compensation = SGP40_DEFAULT_ENABLE_COMP;
    dev->config.default_temperature = SGP40_DEFAULT_TEMPERATURE;
    dev->config.default_humidity = SGP40_DEFAULT_HUMIDITY;
    dev->config.i2c_address = SGP40_DEFAULT_I2C_ADDR;
    
    if (config != XY_NULL) {
        dev->config = *config;
    }
    
    /* 等待传感器上电稳定 */
    xy_delay_ms(20);
    
    /* 读取特征集 */
    xy_ret_t ret = xy_sgp40_read_feature_set(dev, &dev->feature_set);
    if (ret != XY_OK) {
        return ret;
    }
    
    /* 读取序列号 */
    ret = xy_sgp40_read_serial_id(dev, dev->serial_id);
    if (ret != XY_OK) {
        return ret;
    }
    
    /* 执行自测试 */
    bool self_test_passed = false;
    ret = xy_sgp40_self_test(dev, &self_test_passed);
    if (ret != XY_OK || !self_test_passed) {
        return XY_ERROR;
    }
    
    /* 开始预热 */
    dev->is_warmed_up = false;
    dev->uptime_ms = 0;
    
    /* 预热期间可以开始测量，但数据可能不准确 */
    dev->is_initialized = true;
    
    return XY_OK;
}

xy_ret_t xy_sgp40_deinit(xy_sgp40_dev_t *dev)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    /* 停止测量 */
    xy_sgp40_stop(dev);
    
    dev->is_initialized = false;
    
    return XY_OK;
}

xy_ret_t xy_sgp40_read_feature_set(xy_sgp40_dev_t *dev, uint16_t *feature_set)
{
    if (dev == XY_NULL || feature_set == XY_NULL) {
        return XY_ERROR;
    }
    
    /* 发送获取特征集命令 */
    xy_ret_t ret = sgp40_write_command(dev, SGP40_CMD_FEATURE_SET);
    if (ret != XY_OK) return ret;
    
    /* 等待响应 */
    xy_delay_ms(5);
    
    /* 读取特征集 */
    return sgp40_read_u16(dev, feature_set);
}

xy_ret_t xy_sgp40_read_serial_id(xy_sgp40_dev_t *dev, uint32_t serial_id[3])
{
    if (dev == XY_NULL || serial_id == XY_NULL) {
        return XY_ERROR;
    }
    
    /* 发送读取序列号命令 */
    xy_ret_t ret = sgp40_write_command(dev, SGP40_CMD_SERIAL_ID);
    if (ret != XY_OK) return ret;
    
    /* 等待响应 */
    xy_delay_ms(5);
    
    /* 读取 6 字节序列号 (3 个 uint16_t) */
    uint8_t buffer[9];  /* 6 字节数据 + 3 字节 CRC */
    ret = xy_i2c_read_data(dev->i2c, buffer, 9);
    if (ret != XY_OK) return ret;
    
    /* 验证并解析序列号 */
    for (uint8_t i = 0; i < 3; i++) {
        uint8_t idx = i * 3;
        
        if (!sgp40_verify_crc(&buffer[idx], 2, buffer[idx + 2])) {
            return XY_ERROR;
        }
        
        serial_id[i] = ((uint32_t)buffer[idx] << 8) | buffer[idx + 1];
    }
    
    return XY_OK;
}

xy_ret_t xy_sgp40_self_test(xy_sgp40_dev_t *dev, bool *passed)
{
    if (dev == XY_NULL || passed == XY_NULL) {
        return XY_ERROR;
    }
    
    /* 发送自测试命令 */
    xy_ret_t ret = sgp40_write_command(dev, SGP40_CMD_SELF_TEST);
    if (ret != XY_OK) return ret;
    
    /* 等待自测试完成 (约 220ms) */
    xy_delay_ms(250);
    
    /* 读取结果 */
    uint16_t result;
    ret = sgp40_read_u16(dev, &result);
    if (ret != XY_OK) {
        *passed = false;
        return ret;
    }
    
    /* 0xD400 表示自测试通过 */
    *passed = (result == 0xD400);
    
    return XY_OK;
}

xy_ret_t xy_sgp40_start_measurement(xy_sgp40_dev_t *dev)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    /* 发送测量 VOC 命令 */
    return sgp40_write_command(dev, SGP40_CMD_MEASURE_VOC);
}

xy_ret_t xy_sgp40_read_voc(xy_sgp40_dev_t *dev, xy_sgp40_data_t *data)
{
    if (dev == XY_NULL || data == XY_NULL) {
        return XY_ERROR;
    }
    
    /* 读取 VOC 指数 */
    uint16_t voc_index;
    xy_ret_t ret = sgp40_read_u16(dev, &voc_index);
    if (ret != XY_OK) return ret;
    
    /* 填充数据结构 */
    data->voc_index = voc_index;
    data->raw_signal = 0;  /* 简化实现 */
    data->temperature = dev->config.default_temperature;
    data->humidity = dev->config.default_humidity;
    data->timestamp = 0;   /* 简化实现 */
    data->status = 0;
    
    /* 更新最后测量结果 */
    dev->last_data = *data;
    dev->measurement_count++;
    
    return XY_OK;
}

xy_ret_t xy_sgp40_measure_voc(xy_sgp40_dev_t *dev, xy_sgp40_data_t *data, uint32_t timeout_ms)
{
    if (dev == XY_NULL || data == XY_NULL) {
        return XY_ERROR;
    }
    
    /* 启动测量 */
    xy_ret_t ret = xy_sgp40_start_measurement(dev);
    if (ret != XY_OK) return ret;
    
    /* 等待测量完成 */
    ret = sgp40_wait_measurement(dev, timeout_ms);
    if (ret != XY_OK) return ret;
    
    /* 读取结果 */
    return xy_sgp40_read_voc(dev, data);
}

xy_ret_t xy_sgp40_start_continuous(xy_sgp40_dev_t *dev)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    /* SGP40 连续测量通过周期性调用 measure_voc 实现 */
    /* 建议调用间隔 1 秒 */
    return XY_OK;
}

xy_ret_t xy_sgp40_stop(xy_sgp40_dev_t *dev)
{
    if (dev == XY_NULL) {
        return XY_ERROR;
    }
    
    /* SGP40 没有显式停止命令 */
    /* 只需停止周期性调用 measure_voc */
    return XY_OK;
}

xy_ret_t xy_sgp40_set_compensation(xy_sgp40_dev_t *dev, float temperature, float humidity)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    /* 保存补偿参数 */
    dev->config.default_temperature = temperature;
    dev->config.default_humidity = humidity;
    
    /* SGP40 的温湿度补偿在内部自动处理 */
    /* 实际应用中需要外接温湿度传感器 */
    
    return XY_OK;
}

xy_ret_t xy_sgp40_enable_burn_in(xy_sgp40_dev_t *dev)
{
    if (dev == XY_NULL || !dev->is_initialized) {
        return XY_ERROR;
    }
    
    /* 发送老化模式命令 */
    xy_ret_t ret = sgp40_write_command(dev, SGP40_CMD_BURN_IN);
    if (ret != XY_OK) return ret;
    
    dev->is_burned_in = true;
    
    return XY_OK;
}

bool xy_sgp40_is_warmed_up(xy_sgp40_dev_t *dev)
{
    if (dev == XY_NULL) return false;
    
    /* 检查上电时间是否超过预热时间 */
    return dev->uptime_ms >= SGP40_WARMUP_MS;
}

xy_sgp40_voc_level_t xy_sgp40_get_voc_level(uint16_t voc_index)
{
    if (voc_index < 100) {
        return XY_SGP40_VOC_EXCELLENT;
    } else if (voc_index < 200) {
        return XY_SGP40_VOC_GOOD;
    } else if (voc_index < 300) {
        return XY_SGP40_VOC_MODERATE;
    } else if (voc_index < 400) {
        return XY_SGP40_VOC_POOR;
    } else {
        return XY_SGP40_VOC_BAD;
    }
}

void xy_sgp40_set_offset(xy_sgp40_dev_t *dev, int16_t offset)
{
    if (dev == XY_NULL) return;
    dev->offset = offset;
}

bool xy_sgp40_is_ready(xy_sgp40_dev_t *dev)
{
    if (dev == XY_NULL) return false;
    return dev->is_initialized;
}

xy_sgp40_data_t *xy_sgp40_get_last_data(xy_sgp40_dev_t *dev)
{
    if (dev == XY_NULL) return XY_NULL;
    return &dev->last_data;
}

uint32_t xy_sgp40_get_uptime(xy_sgp40_dev_t *dev)
{
    if (dev == XY_NULL) return 0;
    return dev->uptime_ms;
}
