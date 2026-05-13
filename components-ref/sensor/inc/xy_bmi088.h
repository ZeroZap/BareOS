/**
 * @file xy_bmi088.h
 * @brief BMI088 6-Axis IMU Sensor Driver (Accelerometer + Gyroscope)
 * @version 1.0.0
 * @date 2026-03-14
 * 
 * BMI088 是 Bosch 出品的高性能 6 轴 IMU，包含独立的加速度计和陀螺仪
 * 特点：
 * - 抗振动，适合无人机和机器人应用
 * - 加速度计量程：±3g/±6g/±12g/±24g
 * - 陀螺仪量程：±125/±250/±500/±1000/±2000°/s
 * - SPI 接口 (加速度计和陀螺仪独立 CS)
 * 
 * @note 需要两个片选信号：ACC_CS 和 GYRO_CS
 */

#ifndef XY_BMI088_H
#define XY_BMI088_H

#include <stdint.h>
#include <stdbool.h>
#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * 硬件抽象层接口 (需要用户实现)
 *===========================================================================*/

/**
 * SPI 设备结构体
 */
typedef struct {
    void *handle;         /*!< SPI 设备句柄 */
    uint8_t acc_cs_pin;   /*!< 加速度计片选引脚 */
    uint8_t gyro_cs_pin;  /*!< 陀螺仪片选引脚 */
} xy_spi_dev_t;

/**
 * @brief SPI 写寄存器 (需要用户实现)
 * @param dev SPI 设备句柄
 * @param cs_pin 片选引脚 (0=ACC, 1=GYRO)
 * @param reg_addr 寄存器地址
 * @param data 数据指针
 * @param len 数据长度
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_spi_write_reg(xy_spi_dev_t *dev, uint8_t cs_pin, uint8_t reg_addr, const uint8_t *data, uint16_t len);

/**
 * @brief SPI 读寄存器 (需要用户实现)
 * @param dev SPI 设备句柄
 * @param cs_pin 片选引脚 (0=ACC, 1=GYRO)
 * @param reg_addr 寄存器地址
 * @param data 数据指针
 * @param len 数据长度
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_spi_read_reg(xy_spi_dev_t *dev, uint8_t cs_pin, uint8_t reg_addr, uint8_t *data, uint16_t len);

/**
 * @brief 延时函数 (需要用户实现)
 * @param ms 延时毫秒数
 */
void xy_delay_ms(uint32_t ms);

/*============================================================================
 * BMI088 寄存器定义
 *===========================================================================*/

/* 加速度计寄存器 */
#define BMI088_ACC_CHIP_ID          0x00
#define BMI088_ACC_ERROR_ADDR       0x02
#define BMI088_ACC_STATUS           0x03
#define BMI088_ACC_INT_STAT_1       0x1A

#define BMI088_ACC_RANGE_ADDR       0x41
#define BMI088_ACC_BW_ADDR          0x42
#define BMI088_ACC_PWR_CONF_ADDR    0x40
#define BMI088_ACC_PWR_CTRL_ADDR    0x7D
#define BMI088_ACC_SOFTRESET_ADDR   0x7E

#define BMI088_ACC_TEMP_LSB_ADDR    0x22
#define BMI088_ACC_TEMP_MSB_ADDR    0x23

#define BMI088_ACC_X_LSB_ADDR       0x12
#define BMI088_ACC_X_MSB_ADDR       0x13
#define BMI088_ACC_Y_LSB_ADDR       0x14
#define BMI088_ACC_Y_MSB_ADDR       0x15
#define BMI088_ACC_Z_LSB_ADDR       0x16
#define BMI088_ACC_Z_MSB_ADDR       0x17

/* 陀螺仪寄存器 */
#define BMI088_GYRO_CHIP_ID         0x00
#define BMI088_GYRO_RANGE_ADDR      0x0F
#define BMI088_GYRO_BANDWIDTH_ADDR  0x10
#define BMI088_GYRO_LPM1_ADDR       0x11
#define BMI088_GYRO_SOFTRESET_ADDR  0x14

#define BMI088_GYRO_X_LSB_ADDR      0x04
#define BMI088_GYRO_X_MSB_ADDR      0x05
#define BMI088_GYRO_Y_LSB_ADDR      0x06
#define BMI088_GYRO_Y_MSB_ADDR      0x07
#define BMI088_GYRO_Z_LSB_ADDR      0x08
#define BMI088_GYRO_Z_MSB_ADDR      0x09

/* 芯片 ID */
#define BMI088_ACC_CHIP_ID_VALUE    0x1F
#define BMI088_GYRO_CHIP_ID_VALUE   0x0F

/*============================================================================
 * 数据类型定义
 *===========================================================================*/

/**
 * 加速度计量程
 */
typedef enum {
    XY_BMI088_ACC_RANGE_3G  = 0,  /*!< ±3g */
    XY_BMI088_ACC_RANGE_6G  = 1,  /*!< ±6g */
    XY_BMI088_ACC_RANGE_12G = 2,  /*!< ±12g */
    XY_BMI088_ACC_RANGE_24G = 3   /*!< ±24g */
} xy_bmi088_acc_range_t;

/**
 * 陀螺仪量程
 */
typedef enum {
    XY_BMI088_GYRO_RANGE_125   = 0,  /*!< ±125°/s */
    XY_BMI088_GYRO_RANGE_250   = 1,  /*!< ±250°/s */
    XY_BMI088_GYRO_RANGE_500   = 2,  /*!< ±500°/s */
    XY_BMI088_GYRO_RANGE_1000  = 3,  /*!< ±1000°/s */
    XY_BMI088_GYRO_RANGE_2000  = 4   /*!< ±2000°/s */
} xy_bmi088_gyro_range_t;

/**
 * 输出数据速率 (加速度计)
 */
typedef enum {
    XY_BMI088_ACC_ODR_12_5HZ  = 5,
    XY_BMI088_ACC_ODR_25HZ    = 6,
    XY_BMI088_ACC_ODR_50HZ    = 7,
    XY_BMI088_ACC_ODR_100HZ   = 8,
    XY_BMI088_ACC_ODR_200HZ   = 9,
    XY_BMI088_ACC_ODR_400HZ   = 10,
    XY_BMI088_ACC_ODR_800HZ   = 11,
    XY_BMI088_ACC_ODR_1600HZ  = 12
} xy_bmi088_acc_odr_t;

/**
 * 输出数据速率 (陀螺仪)
 */
typedef enum {
    XY_BMI088_GYRO_ODR_2000HZ = 0,
    XY_BMI088_GYRO_ODR_1000HZ = 1,
    XY_BMI088_GYRO_ODR_400HZ  = 2,
    XY_BMI088_GYRO_ODR_200HZ  = 3,
    XY_BMI088_GYRO_ODR_100HZ  = 4,
    XY_BMI088_GYRO_ODR_200HZ_LP = 5
} xy_bmi088_gyro_odr_t;

/**
 * 带宽配置
 */
typedef enum {
    XY_BMI088_BW_OSR4 = 0,    /*!< 平均 4 个样本 */
    XY_BMI088_BW_OSR2 = 1,    /*!< 平均 2 个样本 */
    XY_BMI088_BW_NORMAL = 2   /*!< 正常模式 */
} xy_bmi088_bw_t;

/**
 * IMU 原始数据
 */
typedef struct {
    int16_t acc_x;      /*!< 加速度计 X 轴原始数据 */
    int16_t acc_y;      /*!< 加速度计 Y 轴原始数据 */
    int16_t acc_z;      /*!< 加速度计 Z 轴原始数据 */
    int16_t gyro_x;     /*!< 陀螺仪 X 轴原始数据 */
    int16_t gyro_y;     /*!< 陀螺仪 Y 轴原始数据 */
    int16_t gyro_z;     /*!< 陀螺仪 Z 轴原始数据 */
    int16_t temperature;/*!< 温度原始数据 */
} xy_bmi088_raw_data_t;

/**
 * IMU 物理数据 (SI 单位)
 */
typedef struct {
    float acc_x;        /*!< 加速度计 X 轴 (m/s²) */
    float acc_y;        /*!< 加速度计 Y 轴 (m/s²) */
    float acc_z;        /*!< 加速度计 Z 轴 (m/s²) */
    float gyro_x;       /*!< 陀螺仪 X 轴 (rad/s) */
    float gyro_y;       /*!< 陀螺仪 Y 轴 (rad/s) */
    float gyro_z;       /*!< 陀螺仪 Z 轴 (rad/s) */
    float temperature;  /*!< 温度 (°C) */
    uint32_t timestamp; /*!< 时间戳 (ms) */
} xy_bmi088_data_t;

/**
 * BMI088 配置结构体
 */
typedef struct {
    xy_bmi088_acc_range_t acc_range;    /*!< 加速度计量程 */
    xy_bmi088_gyro_range_t gyro_range;  /*!< 陀螺仪量程 */
    xy_bmi088_acc_odr_t acc_odr;        /*!< 加速度计 ODR */
    xy_bmi088_gyro_odr_t gyro_odr;      /*!< 陀螺仪 ODR */
    xy_bmi088_bw_t bw;                  /*!< 带宽配置 */
    bool enable_interrupt;              /*!< 是否启用中断 */
} xy_bmi088_config_t;

/**
 * BMI088 设备结构体
 */
typedef struct {
    xy_spi_dev_t *spi;                  /*!< SPI 设备 */
    xy_bmi088_config_t config;          /*!< 配置 */
    bool is_initialized;                /*!< 是否已初始化 */
    
    /* 灵敏度 (用于原始数据转换) */
    float acc_sensitivity;              /*!< 加速度计灵敏度 (LSB/g) */
    float gyro_sensitivity;             /*!< 陀螺仪灵敏度 (LSB/°/s) */
    
    /* 校准参数 */
    float acc_offset[3];                /*!< 加速度计零偏 */
    float gyro_offset[3];               /*!< 陀螺仪零偏 */
} xy_bmi088_dev_t;

/*============================================================================
 * API 函数声明
 *===========================================================================*/

/**
 * @brief 初始化 BMI088 设备
 * @param dev BMI088 设备结构体
 * @param spi SPI 设备指针
 * @param config 配置结构体
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_bmi088_init(xy_bmi088_dev_t *dev, xy_spi_dev_t *spi, xy_bmi088_config_t *config);

/**
 * @brief 去初始化 BMI088 设备
 * @param dev BMI088 设备结构体
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_bmi088_deinit(xy_bmi088_dev_t *dev);

/**
 * @brief 读取 BMI088 芯片 ID
 * @param dev BMI088 设备结构体
 * @param acc_id 加速度计芯片 ID 指针 (可为 NULL)
 * @param gyro_id 陀螺仪芯片 ID 指针 (可为 NULL)
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_bmi088_read_chip_id(xy_bmi088_dev_t *dev, uint8_t *acc_id, uint8_t *gyro_id);

/**
 * @brief 读取 BMI088 原始数据
 * @param dev BMI088 设备结构体
 * @param raw_data 原始数据指针
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_bmi088_read_raw_data(xy_bmi088_dev_t *dev, xy_bmi088_raw_data_t *raw_data);

/**
 * @brief 读取 BMI088 物理数据 (SI 单位)
 * @param dev BMI088 设备结构体
 * @param data 物理数据指针
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_bmi088_read_data(xy_bmi088_dev_t *dev, xy_bmi088_data_t *data);

/**
 * @brief 软件复位 BMI088
 * @param dev BMI088 设备结构体
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_bmi088_soft_reset(xy_bmi088_dev_t *dev);

/**
 * @brief 设置加速度计量程
 * @param dev BMI088 设备结构体
 * @param range 量程
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_bmi088_set_acc_range(xy_bmi088_dev_t *dev, xy_bmi088_acc_range_t range);

/**
 * @brief 设置陀螺仪量程
 * @param dev BMI088 设备结构体
 * @param range 量程
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_bmi088_set_gyro_range(xy_bmi088_dev_t *dev, xy_bmi088_gyro_range_t range);

/**
 * @brief 校准 BMI088 (静止状态下调用)
 * @param dev BMI088 设备结构体
 * @param samples 采样次数 (建议 100-1000)
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_bmi088_calibrate(xy_bmi088_dev_t *dev, uint16_t samples);

/**
 * @brief 设置校准参数
 * @param dev BMI088 设备结构体
 * @param acc_offset 加速度计零偏 [x, y, z]
 * @param gyro_offset 陀螺仪零偏 [x, y, z]
 */
void xy_bmi088_set_calibration(xy_bmi088_dev_t *dev, float acc_offset[3], float gyro_offset[3]);

/**
 * @brief 获取设备状态
 * @param dev BMI088 设备结构体
 * @return true 已初始化，false 未初始化
 */
bool xy_bmi088_is_ready(xy_bmi088_dev_t *dev);

/*============================================================================
 * 工具函数
 *===========================================================================*/

/**
 * @brief 将原始加速度数据转换为 m/s²
 * @param raw 原始数据
 * @param sensitivity 灵敏度 (LSB/g)
 * @return 加速度 (m/s²)
 */
static inline float xy_bmi088_acc_raw_to_ms2(int16_t raw, float sensitivity)
{
    return (float)raw / sensitivity * 9.80665f;
}

/**
 * @brief 将原始角速度数据转换为 rad/s
 * @param raw 原始数据
 * @param sensitivity 灵敏度 (LSB/°/s)
 * @return 角速度 (rad/s)
 */
static inline float xy_bmi088_gyro_raw_to_rads(int16_t raw, float sensitivity)
{
    return (float)raw / sensitivity * 0.01745329252f;  /* π/180 */
}

#ifdef __cplusplus
}
#endif

#endif /* XY_BMI088_H */
