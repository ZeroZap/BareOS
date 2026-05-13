/**
 * @file xy_bmi270.c
 * @brief Bosch BMI270 6-Axis IMU Sensor Driver Implementation
 * @version 1.0.0
 * @date 2026-03-18
 * 
 * @note BMI270 是专为可穿戴应用优化的 6 轴 IMU
 * 
 * 主要特性:
 * - 3 轴加速度计 (±2g/±4g/±8g/±16g)
 * - 3 轴陀螺仪 (±125/±250/±500/±1000/±2000°/s)
 * - 超低功耗 (38μA @ 100Hz 加速度计)
 * - 内置智能运动触发
 * - SPI/I2C 接口
 * 
 * 使用示例:
 * @code
 * xy_bmi270_t bmi270;
 * 
 * // I2C 初始化
 * xy_bmi270_init(&bmi270, i2c_handle, 0x68, false);
 * 
 * // SPI 初始化
 * xy_bmi270_init(&bmi270, spi_handle, 0, true);
 * 
 * // 配置量程
 * bmi270_range_t range = {
 *     .acc_range = BMI270_ACC_RANGE_4G,
 *     .gyr_range = BMI270_GYR_RANGE_500,
 *     .acc_odr = 0x0A,  // 100Hz
 *     .gyr_odr = 0x0A   // 100Hz
 * };
 * xy_bmi270_set_range(&bmi270, &range);
 * 
 * // 读取数据
 * bmi270_data_t data;
 * xy_bmi270_read_data(&bmi270, &data);
 * @endcode
 */

#include "xy_bmi270.h"
#include "xy_i2c.h"
#include "xy_spi.h"
#include "xy_log.h"
#include <string.h>

#define XY_LOG_TAG "BMI270"

/* ==================== 常量定义 ==================== */

/** 重力加速度 (m/s²) */
#define GRAVITY_MSS2               (9.80665f)

/** 角度转弧度 */
#define DEG_TO_RAD                 (0.017453292519943295f)

/** 传感器时间分辨率 (秒) */
#define BMI270_SENSOR_TIME_RES     (39.0625e-6f)

/* ==================== 内部辅助函数 ==================== */

/**
 * @brief 总线读取
 */
static int bus_read(xy_bmi270_t *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (dev->is_spi) {
        /* SPI 模式：读操作需要设置读标志 */
        reg |= 0x80;
        return xy_spi_transfer((xy_spi_t*)dev->bus_handle, dev->bus_addr, &reg, 1, buf, len, 100);
    } else {
        /* I2C 模式 */
        return xy_i2c_master_transmit((xy_i2c_t*)dev->bus_handle, dev->bus_addr, &reg, 1, buf, len, 100);
    }
}

/**
 * @brief 总线写入
 */
static int bus_write(xy_bmi270_t *dev, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    if (dev->is_spi) {
        /* SPI 模式：写操作清除写标志 */
        reg &= ~0x80;
        /* 需要构造发送缓冲区：寄存器地址 + 数据 */
        uint8_t tx_buf[64];
        if (len + 1 > sizeof(tx_buf)) {
            return XY_DEVICE_ENOMEM;
        }
        tx_buf[0] = reg;
        memcpy(tx_buf + 1, buf, len);
        return xy_spi_transfer((xy_spi_t*)dev->bus_handle, dev->bus_addr, tx_buf, len + 1, NULL, 0, 100);
    } else {
        /* I2C 模式 */
        return xy_i2c_master_transmit((xy_i2c_t*)dev->bus_handle, dev->bus_addr, &reg, 1, buf, len, 100);
    }
}

/**
 * @brief 延迟函数
 */
static void delay_ms(uint32_t ms)
{
    /* 平台相关延迟实现 */
    volatile uint32_t i;
    for (uint32_t j = 0; j < ms; j++) {
        for (i = 0; i < 10000; i++);
    }
}

/* ==================== 公共 API 实现 ==================== */

int xy_bmi270_init(xy_bmi270_t *dev, void *bus_handle, uint8_t bus_addr, bool is_spi)
{
    if (!dev || !bus_handle) {
        XY_LOG_ERROR("Invalid parameters");
        return XY_DEVICE_EINVAL;
    }

    memset(dev, 0, sizeof(xy_bmi270_t));
    dev->bus_handle = bus_handle;
    dev->bus_addr = bus_addr;
    dev->is_spi = is_spi;
    dev->initialized = false;

    XY_LOG_INFO("Initializing BMI270 on %s...", is_spi ? "SPI" : "I2C");

    /* 软复位 */
    int ret = xy_bmi270_reset(dev);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Reset failed: %d", ret);
        return ret;
    }

    /* 等待复位完成 */
    delay_ms(10);

    /* 读取芯片 ID */
    uint8_t chip_id = 0;
    ret = xy_bmi270_get_chip_id(dev, &chip_id);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Failed to read chip ID: %d", ret);
        return ret;
    }

    if (chip_id != BMI270_CHIPID_VAL) {
        XY_LOG_ERROR("Invalid chip ID: 0x%02X (expected 0x27)", chip_id);
        return XY_DEVICE_ENODEV;
    }

    XY_LOG_INFO("BMI270 found, chip ID: 0x%02X", chip_id);

    /* 默认配置：±4g, ±500°/s, 100Hz */
    bmi270_range_t default_range = {
        .acc_range = BMI270_ACC_RANGE_4G,
        .gyr_range = BMI270_GYR_RANGE_500,
        .acc_odr = 0x0A,  /* 100Hz */
        .gyr_odr = 0x0A   /* 100Hz */
    };

    ret = xy_bmi270_set_range(dev, &default_range);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Failed to set default range: %d", ret);
        return ret;
    }

    /* 使能加速度计和陀螺仪 */
    ret = xy_bmi270_enable_acc(dev, true);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Failed to enable accelerometer: %d", ret);
        return ret;
    }

    ret = xy_bmi270_enable_gyr(dev, true);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Failed to enable gyroscope: %d", ret);
        return ret;
    }

    /* 等待传感器启动 */
    delay_ms(50);

    dev->initialized = true;
    XY_LOG_INFO("BMI270 initialized successfully");

    return XY_DEVICE_OK;
}

int xy_bmi270_deinit(xy_bmi270_t *dev)
{
    if (!dev) {
        return XY_DEVICE_EINVAL;
    }

    if (dev->initialized) {
        /* 禁用传感器 */
        xy_bmi270_enable_acc(dev, false);
        xy_bmi270_enable_gyr(dev, false);
        
        /* 进入睡眠模式 */
        xy_bmi270_sleep(dev);
        
        dev->initialized = false;
        XY_LOG_INFO("BMI270 deinitialized");
    }

    return XY_DEVICE_OK;
}

int xy_bmi270_read_regs(xy_bmi270_t *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (!dev || !buf || !len || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    return bus_read(dev, reg, buf, len);
}

int xy_bmi270_write_regs(xy_bmi270_t *dev, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    if (!dev || !buf || !len || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    return bus_write(dev, reg, buf, len);
}

int xy_bmi270_get_chip_id(xy_bmi270_t *dev, uint8_t *chip_id)
{
    if (!dev || !chip_id) {
        return XY_DEVICE_EINVAL;
    }

    return xy_bmi270_read_regs(dev, BMI270_REG_CHIPID, chip_id, 1);
}

int xy_bmi270_set_range(xy_bmi270_t *dev, const bmi270_range_t *range)
{
    if (!dev || !range || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    int ret;

    /* 配置加速度计 */
    uint8_t acc_conf = (range->acc_odr << 4) | BMI270_ACC_PERF_MODE;
    ret = xy_bmi270_write_regs(dev, BMI270_REG_ACC_CONF, &acc_conf, 1);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Failed to configure accelerometer");
        return ret;
    }

    /* 配置加速度计量程 */
    ret = xy_bmi270_write_regs(dev, BMI270_REG_ACC_RANGE, &range->acc_range, 1);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Failed to set accelerometer range");
        return ret;
    }

    /* 配置陀螺仪 */
    uint8_t gyr_conf = (range->gyr_odr << 4);
    ret = xy_bmi270_write_regs(dev, BMI270_REG_GYR_CONF, &gyr_conf, 1);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Failed to configure gyroscope");
        return ret;
    }

    /* 配置陀螺仪量程 */
    ret = xy_bmi270_write_regs(dev, BMI270_REG_GYR_RANGE, &range->gyr_range, 1);
    if (ret != XY_DEVICE_OK) {
        XY_LOG_ERROR("Failed to set gyroscope range");
        return ret;
    }

    /* 保存配置 */
    dev->range = *range;

    /* 计算比例因子 */
    switch (range->acc_range) {
        case BMI270_ACC_RANGE_2G:
            dev->acc_scale = 2.0f * GRAVITY_MSS2 / 32768.0f;
            break;
        case BMI270_ACC_RANGE_4G:
            dev->acc_scale = 4.0f * GRAVITY_MSS2 / 32768.0f;
            break;
        case BMI270_ACC_RANGE_8G:
            dev->acc_scale = 8.0f * GRAVITY_MSS2 / 32768.0f;
            break;
        case BMI270_ACC_RANGE_16G:
            dev->acc_scale = 16.0f * GRAVITY_MSS2 / 32768.0f;
            break;
        default:
            dev->acc_scale = 4.0f * GRAVITY_MSS2 / 32768.0f;
            break;
    }

    switch (range->gyr_range) {
        case BMI270_GYR_RANGE_2000:
            dev->gyr_scale = 2000.0f * DEG_TO_RAD / 32768.0f;
            break;
        case BMI270_GYR_RANGE_1000:
            dev->gyr_scale = 1000.0f * DEG_TO_RAD / 32768.0f;
            break;
        case BMI270_GYR_RANGE_500:
            dev->gyr_scale = 500.0f * DEG_TO_RAD / 32768.0f;
            break;
        case BMI270_GYR_RANGE_250:
            dev->gyr_scale = 250.0f * DEG_TO_RAD / 32768.0f;
            break;
        case BMI270_GYR_RANGE_125:
            dev->gyr_scale = 125.0f * DEG_TO_RAD / 32768.0f;
            break;
        default:
            dev->gyr_scale = 500.0f * DEG_TO_RAD / 32768.0f;
            break;
    }

    XY_LOG_INFO("Range configured: ACC ±%dg, GYR ±%d°/s", 
                (range->acc_range == 0) ? 2 : (range->acc_range == 1) ? 4 : 
                (range->acc_range == 2) ? 8 : 16,
                (range->gyr_range == 0) ? 2000 : (range->gyr_range == 1) ? 1000 :
                (range->gyr_range == 2) ? 500 : (range->gyr_range == 3) ? 250 : 125);

    return XY_DEVICE_OK;
}

int xy_bmi270_enable_acc(xy_bmi270_t *dev, bool enable)
{
    if (!dev || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    uint8_t conf = 0;
    int ret = xy_bmi270_read_regs(dev, BMI270_REG_ACC_CONF, &conf, 1);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    if (enable) {
        conf |= BMI270_ACC_EN;
    } else {
        conf &= ~BMI270_ACC_EN;
    }

    return xy_bmi270_write_regs(dev, BMI270_REG_ACC_CONF, &conf, 1);
}

int xy_bmi270_enable_gyr(xy_bmi270_t *dev, bool enable)
{
    if (!dev || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    uint8_t conf = 0;
    int ret = xy_bmi270_read_regs(dev, BMI270_REG_GYR_CONF, &conf, 1);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    if (enable) {
        conf |= BMI270_GYR_EN;
    } else {
        conf &= ~BMI270_GYR_EN;
    }

    return xy_bmi270_write_regs(dev, BMI270_REG_GYR_CONF, &conf, 1);
}

int xy_bmi270_read_raw(xy_bmi270_t *dev, bmi270_raw_data_t *raw_data)
{
    if (!dev || !raw_data || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    uint8_t buf[15];
    
    /* 等待数据就绪 */
    uint8_t status = 0;
    int ret = xy_bmi270_get_status(dev, &status);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    if (!(status & BMI270_DRDY_ACC) || !(status & BMI270_DRDY_GYR)) {
        return XY_DEVICE_EAGAIN;
    }

    /* 读取加速度和陀螺仪数据 (0x04 - 0x0F, 共 12 字节) */
    ret = xy_bmi270_read_regs(dev, BMI270_REG_ACC_X_LSB, buf, 12);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    /* 解析加速度数据 */
    raw_data->acc_x = (int16_t)((buf[1] << 8) | buf[0]);
    raw_data->acc_y = (int16_t)((buf[3] << 8) | buf[2]);
    raw_data->acc_z = (int16_t)((buf[5] << 8) | buf[4]);

    /* 解析陀螺仪数据 */
    raw_data->gyr_x = (int16_t)((buf[7] << 8) | buf[6]);
    raw_data->gyr_y = (int16_t)((buf[9] << 8) | buf[8]);
    raw_data->gyr_z = (int16_t)((buf[11] << 8) | buf[10]);

    /* 读取传感器时间 */
    ret = xy_bmi270_read_regs(dev, BMI270_REG_SENSORTIME_0, buf, 3);
    if (ret == XY_DEVICE_OK) {
        raw_data->sensor_time = (uint32_t)((buf[2] << 16) | (buf[1] << 8) | buf[0]);
    } else {
        raw_data->sensor_time = 0;
    }

    return XY_DEVICE_OK;
}

int xy_bmi270_read_data(xy_bmi270_t *dev, bmi270_data_t *data)
{
    if (!dev || !data || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    bmi270_raw_data_t raw;
    int ret = xy_bmi270_read_raw(dev, &raw);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    /* 转换为物理单位 */
    data->acc_x = raw.acc_x * dev->acc_scale;
    data->acc_y = raw.acc_y * dev->acc_scale;
    data->acc_z = raw.acc_z * dev->acc_scale;

    data->gyr_x = raw.gyr_x * dev->gyr_scale;
    data->gyr_y = raw.gyr_y * dev->gyr_scale;
    data->gyr_z = raw.gyr_z * dev->gyr_scale;

    /* 温度 (BMI270 无内置温度传感器，返回 0) */
    data->temperature = 0.0f;

    return XY_DEVICE_OK;
}

int xy_bmi270_get_status(xy_bmi270_t *dev, uint8_t *status)
{
    if (!dev || !status || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    return xy_bmi270_read_regs(dev, BMI270_REG_STATUS, status, 1);
}

int xy_bmi270_reset(xy_bmi270_t *dev)
{
    if (!dev || !dev->initialized) {
        /* 复位前需要先初始化通信 */
        dev->initialized = true;
    }

    /* 写入 0xB6 到 0xB6 寄存器触发软复位 */
    uint8_t cmd = 0xB6;
    int ret = xy_bmi270_write_regs(dev, 0xB6, &cmd, 1);
    
    dev->initialized = false;
    return ret;
}

int xy_bmi270_sleep(xy_bmi270_t *dev)
{
    if (!dev || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    /* 禁用加速度计和陀螺仪进入睡眠 */
    xy_bmi270_enable_acc(dev, false);
    xy_bmi270_enable_gyr(dev, false);

    XY_LOG_INFO("BMI270 entered sleep mode");
    return XY_DEVICE_OK;
}

int xy_bmi270_wakeup(xy_bmi270_t *dev)
{
    if (!dev || !dev->initialized) {
        return XY_DEVICE_EINVAL;
    }

    /* 使能加速度计和陀螺仪 */
    int ret = xy_bmi270_enable_acc(dev, true);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    ret = xy_bmi270_enable_gyr(dev, true);
    if (ret != XY_DEVICE_OK) {
        return ret;
    }

    XY_LOG_INFO("BMI270 woken up");
    return XY_DEVICE_OK;
}
