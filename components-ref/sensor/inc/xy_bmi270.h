/**
 * @file xy_bmi270.h
 * @brief Bosch BMI270 6-Axis IMU Sensor Driver
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
 * - 工作温度：-40°C ~ +85°C
 */

#ifndef XY_BMI270_H
#define XY_BMI270_H

#include "xy_sensor.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== BMI270 Register Definitions ==================== */

/**
 * @brief BMI270 寄存器地址
 */
typedef enum {
    BMI270_REG_CHIPID        = 0x00,  /**< 芯片 ID (0x27) */
    BMI270_REG_ERR_REG       = 0x02,  /**< 错误寄存器 */
    BMI270_REG_STATUS        = 0x03,  /**< 状态寄存器 */
    BMI270_REG_ACC_X_LSB     = 0x04,  /**< 加速度 X LSB */
    BMI270_REG_ACC_X_MSB     = 0x05,  /**< 加速度 X MSB */
    BMI270_REG_ACC_Y_LSB     = 0x06,  /**< 加速度 Y LSB */
    BMI270_REG_ACC_Y_MSB     = 0x07,  /**< 加速度 Y MSB */
    BMI270_REG_ACC_Z_LSB     = 0x08,  /**< 加速度 Z LSB */
    BMI270_REG_ACC_Z_MSB     = 0x09,  /**< 加速度 Z MSB */
    BMI270_REG_GYR_X_LSB     = 0x0A,  /**< 陀螺仪 X LSB */
    BMI270_REG_GYR_X_MSB     = 0x0B,  /**< 陀螺仪 X MSB */
    BMI270_REG_GYR_Y_LSB     = 0x0C,  /**< 陀螺仪 Y LSB */
    BMI270_REG_GYR_Y_MSB     = 0x0D,  /**< 陀螺仪 Y MSB */
    BMI270_REG_GYR_Z_LSB     = 0x0E,  /**< 陀螺仪 Z LSB */
    BMI270_REG_GYR_Z_MSB     = 0x0F,  /**< 陀螺仪 Z MSB */
    BMI270_REG_SENSORTIME_0  = 0x10,  /**< 传感器时间 LSB */
    BMI270_REG_SENSORTIME_1  = 0x11,  /**< 传感器时间 MID */
    BMI270_REG_SENSORTIME_2  = 0x12,  /**< 传感器时间 MSB */
    BMI270_REG_EVENT_MAP     = 0x13,  /**< 事件映射 */
    BMI270_REG_STATUS_2      = 0x14,  /**< 状态寄存器 2 */
    BMI270_REG_INT_STATUS_0  = 0x15,  /**< 中断状态 0 */
    BMI270_REG_INT_STATUS_1  = 0x16,  /**< 中断状态 1 */
    BMI270_REG_FIFO_LENGTH_0 = 0x17,  /**< FIFO 长度 LSB */
    BMI270_REG_FIFO_LENGTH_1 = 0x18,  /**< FIFO 长度 MSB */
    BMI270_REG_FIFO_DATA     = 0x19,  /**< FIFO 数据 */
    BMI270_REG_FEAT_EN_0     = 0x1A,  /**< 功能使能 0 */
    BMI270_REG_FEAT_EN_1     = 0x1B,  /**< 功能使能 1 */
    BMI270_REG_ACC_CONF      = 0x20,  /**< 加速度配置 */
    BMI270_REG_ACC_RANGE     = 0x21,  /**< 加速度量程 */
    BMI270_REG_GYR_CONF      = 0x22,  /**< 陀螺仪配置 */
    BMI270_REG_GYR_RANGE     = 0x23,  /**< 陀螺仪量程 */
    BMI270_REG_AUX_CONF      = 0x24,  /**< 辅助传感器配置 */
    BMI270_REG_FIFO_DOWNS    = 0x25,  /**< FIFO 下采样 */
    BMI270_REG_FIFO_WTM_0    = 0x26,  /**< FIFO 水位 LSB */
    BMI270_REG_FIFO_WTM_1    = 0x27,  /**< FIFO 水位 MSB */
    BMI270_REG_FIFO_CONFIG_0 = 0x28,  /**< FIFO 配置 0 */
    BMI270_REG_FIFO_CONFIG_1 = 0x29,  /**< FIFO 配置 1 */
    BMI270_REG_INT1_IO_CTRL  = 0x2A,  /**< INT1 IO 控制 */
    BMI270_REG_INT2_IO_CTRL  = 0x2B,  /**< INT2 IO 控制 */
    BMI270_REG_INT1_MAP      = 0x2C,  /**< INT1 映射 */
    BMI270_REG_INT2_MAP      = 0x2D,  /**< INT2 映射 */
    BMI270_REG_INIT_CTRL     = 0x59,  /**< 初始化控制 */
    BMI270_REG_INIT_STAT     = 0x5E,  /**< 初始化状态 */
} bmi270_reg_t;

/* ==================== Register Bit Definitions ==================== */

/* CHIPID (0x00) */
#define BMI270_CHIPID_VAL          (0x27)  /**< BMI270 芯片 ID */

/* ERR_REG (0x02) */
#define BMI270_ERR_FATAL_MASK      (0x07 << 5)  /**< 致命错误掩码 */
#define BMI270_ERR_CODE_MASK       (0x1F << 0)  /**< 错误代码掩码 */

/* STATUS (0x03) */
#define BMI270_DRDY_ACC            (0x01 << 7)  /**< 加速度数据就绪 */
#define BMI270_DRDY_GYR            (0x01 << 6)  /**< 陀螺仪数据就绪 */
#define BMI270_DRDY_AUX            (0x01 << 5)  /**< 辅助数据就绪 */
#define BMI270_CMD_RDY             (0x01 << 4)  /**< 命令就绪 */

/* ACC_CONF (0x20) */
#define BMI270_ACC_ODR_MASK        (0x0F << 4)  /**< ODR 掩码 */
#define BMI270_ACC_BWP_MASK        (0x03 << 2)  /**< 带宽掩码 */
#define BMI270_ACC_PERF_MODE       (0x01 << 1)  /**< 性能模式 */
#define BMI270_ACC_EN              (0x01 << 0)  /**< 加速度使能 */

/* ACC_RANGE (0x21) */
#define BMI270_ACC_RANGE_2G        (0x00)  /**< ±2g */
#define BMI270_ACC_RANGE_4G        (0x01)  /**< ±4g */
#define BMI270_ACC_RANGE_8G        (0x02)  /**< ±8g */
#define BMI270_ACC_RANGE_16G       (0x03)  /**< ±16g */

/* GYR_CONF (0x22) */
#define BMI270_GYR_ODR_MASK        (0x0F << 4)  /**< ODR 掩码 */
#define BMI270_GYR_BWP_MASK        (0x03 << 2)  /**< 带宽掩码 */
#define BMI270_GYR_EN              (0x01 << 0)  /**< 陀螺仪使能 */

/* GYR_RANGE (0x23) */
#define BMI270_GYR_RANGE_2000      (0x00)  /**< ±2000°/s */
#define BMI270_GYR_RANGE_1000      (0x01)  /**< ±1000°/s */
#define BMI270_GYR_RANGE_500       (0x02)  /**< ±500°/s */
#define BMI270_GYR_RANGE_250       (0x03)  /**< ±250°/s */
#define BMI270_GYR_RANGE_125       (0x04)  /**< ±125°/s */

/* FIFO_CONFIG_1 (0x29) */
#define BMI270_FIFO_ACC_EN         (0x01 << 6)  /**< FIFO 加速度使能 */
#define BMI270_FIFO_GYR_EN         (0x01 << 5)  /**< FIFO 陀螺仪使能 */
#define BMI270_FIFO_TIME_EN        (0x01 << 4)  /**< FIFO 时间使能 */

/* ==================== Data Structures ==================== */

/**
 * @brief BMI270 量程配置
 */
typedef struct {
    uint8_t acc_range;    /**< 加速度量程：0=2g, 1=4g, 2=8g, 3=16g */
    uint8_t gyr_range;    /**< 陀螺仪量程：0=2000, 1=1000, 2=500, 3=250, 4=125 */
    uint8_t acc_odr;      /**< 加速度 ODR */
    uint8_t gyr_odr;      /**< 陀螺仪 ODR */
} bmi270_range_t;

/**
 * @brief BMI270 原始数据
 */
typedef struct {
    int16_t acc_x;        /**< 加速度 X (LSB) */
    int16_t acc_y;        /**< 加速度 Y (LSB) */
    int16_t acc_z;        /**< 加速度 Z (LSB) */
    int16_t gyr_x;        /**< 陀螺仪 X (LSB) */
    int16_t gyr_y;        /**< 陀螺仪 Y (LSB) */
    int16_t gyr_z;        /**< 陀螺仪 Z (LSB) */
    uint32_t sensor_time; /**< 传感器时间 */
} bmi270_raw_data_t;

/**
 * @brief BMI270 物理数据
 */
typedef struct {
    float acc_x;          /**< 加速度 X (m/s²) */
    float acc_y;          /**< 加速度 Y (m/s²) */
    float acc_z;          /**< 加速度 Z (m/s²) */
    float gyr_x;          /**< 陀螺仪 X (rad/s) */
    float gyr_y;          /**< 陀螺仪 Y (rad/s) */
    float gyr_z;          /**< 陀螺仪 Z (rad/s) */
    float temperature;    /**< 温度 (°C) */
} bmi270_data_t;

/**
 * @brief BMI270 设备结构
 */
typedef struct {
    xy_sensor_t base;         /**< 传感器基类 */
    void *bus_handle;         /**< 总线句柄 (I2C/SPI) */
    uint8_t bus_addr;         /**< 总线地址 (I2C: 0x68, SPI: CS) */
    bool is_spi;              /**< SPI 模式标志 */
    bmi270_range_t range;     /**< 量程配置 */
    float acc_scale;          /**< 加速度比例因子 */
    float gyr_scale;          /**< 陀螺仪比例因子 */
    bool initialized;         /**< 初始化标志 */
} xy_bmi270_t;

/* ==================== BMI270 API ==================== */

/**
 * @brief 初始化 BMI270
 * @param dev BMI270 设备句柄
 * @param bus_handle 总线句柄 (I2C 或 SPI)
 * @param bus_addr 总线地址 (I2C: 0x68)
 * @param is_spi SPI 模式标志
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bmi270_init(xy_bmi270_t *dev, void *bus_handle, uint8_t bus_addr, bool is_spi);

/**
 * @brief 反初始化 BMI270
 * @param dev BMI270 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bmi270_deinit(xy_bmi270_t *dev);

/**
 * @brief 读取 BMI270 寄存器
 * @param dev BMI270 设备句柄
 * @param reg 寄存器地址
 * @param buf 数据缓冲区
 * @param len 数据长度
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bmi270_read_regs(xy_bmi270_t *dev, uint8_t reg, uint8_t *buf, uint16_t len);

/**
 * @brief 写入 BMI270 寄存器
 * @param dev BMI270 设备句柄
 * @param reg 寄存器地址
 * @param buf 数据缓冲区
 * @param len 数据长度
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bmi270_write_regs(xy_bmi270_t *dev, uint8_t reg, const uint8_t *buf, uint16_t len);

/**
 * @brief 读取芯片 ID
 * @param dev BMI270 设备句柄
 * @param chip_id 芯片 ID 输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bmi270_get_chip_id(xy_bmi270_t *dev, uint8_t *chip_id);

/**
 * @brief 配置 BMI270 量程
 * @param dev BMI270 设备句柄
 * @param range 量程配置
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bmi270_set_range(xy_bmi270_t *dev, const bmi270_range_t *range);

/**
 * @brief 使能/禁用加速度计
 * @param dev BMI270 设备句柄
 * @param enable 使能标志
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bmi270_enable_acc(xy_bmi270_t *dev, bool enable);

/**
 * @brief 使能/禁用陀螺仪
 * @param dev BMI270 设备句柄
 * @param enable 使能标志
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bmi270_enable_gyr(xy_bmi270_t *dev, bool enable);

/**
 * @brief 读取 BMI270 原始数据
 * @param dev BMI270 设备句柄
 * @param raw_data 原始数据输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bmi270_read_raw(xy_bmi270_t *dev, bmi270_raw_data_t *raw_data);

/**
 * @brief 读取 BMI270 物理数据
 * @param dev BMI270 设备句柄
 * @param data 物理数据输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bmi270_read_data(xy_bmi270_t *dev, bmi270_data_t *data);

/**
 * @brief 读取 BMI270 状态
 * @param dev BMI270 设备句柄
 * @param status 状态输出
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bmi270_get_status(xy_bmi270_t *dev, uint8_t *status);

/**
 * @brief 复位 BMI270
 * @param dev BMI270 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bmi270_reset(xy_bmi270_t *dev);

/**
 * @brief 进入睡眠模式
 * @param dev BMI270 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bmi270_sleep(xy_bmi270_t *dev);

/**
 * @brief 退出睡眠模式
 * @param dev BMI270 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_bmi270_wakeup(xy_bmi270_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* XY_BMI270_H */
