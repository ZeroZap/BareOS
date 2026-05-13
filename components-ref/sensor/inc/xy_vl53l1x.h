/**
 * @file xy_vl53l1x.h
 * @brief VL53L1X Time-of-Flight (ToF) Distance Sensor Driver
 * @version 1.0.0
 * @date 2026-03-14
 * 
 * VL53L1X 是 ST 出品的高性能 ToF 测距传感器
 * 特点：
 * - 测距范围：0-4 米
 * - 精度：±3% (理想条件)
 * - I2C 接口 (最高 400kHz)
 * - 区域测量模式支持
 * - 多传感器同步能力
 * 
 * @note 工作电压 2.6V-3.5V
 */

#ifndef XY_VL53L1X_H
#define XY_VL53L1X_H

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
 * I2C 设备结构体
 */
typedef struct {
    void *handle;         /*!< I2C 设备句柄 */
    uint8_t address;      /*!< VL53L1X I2C 地址 (默认 0x29) */
} xy_i2c_dev_t;

/**
 * @brief I2C 写寄存器 (需要用户实现)
 * @param dev I2C 设备句柄
 * @param reg_addr 寄存器地址 (16-bit)
 * @param data 数据指针
 * @param len 数据长度
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_i2c_write_reg16(xy_i2c_dev_t *dev, uint16_t reg_addr, const uint8_t *data, uint16_t len);

/**
 * @brief I2C 读寄存器 (需要用户实现)
 * @param dev I2C 设备句柄
 * @param reg_addr 寄存器地址 (16-bit)
 * @param data 数据指针
 * @param len 数据长度
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_i2c_read_reg16(xy_i2c_dev_t *dev, uint16_t reg_addr, uint8_t *data, uint16_t len);

/**
 * @brief 延时函数 (需要用户实现)
 * @param ms 延时毫秒数
 */
void xy_delay_ms(uint32_t ms);

/*============================================================================
 * VL53L1X 寄存器定义 (关键寄存器)
 *===========================================================================*/

#define VL53L1X_I2C_ADDR          0x29  /*!< 默认 I2C 地址 */

/* 系统寄存器 */
#define VL53L1X_SOFTWARE_RESET    0x0000
#define VL53L1X_SYSTEM_GROUP_INDEX 0x0006

/* 测量配置寄存器 */
#define VL53L1X_SYSTEM_INTERRUPT_CONFIG_GPIO 0x000A
#define VL53L1X_SYSTEM_INTERRUPT_CLEAR       0x000B
#define VL53L1X_SYSTEM_START                 0x000C

/* 测量结果寄存器 */
#define VL53L1X_RESULT_INTERRUPT_STATUS      0x0013
#define VL53L1X_RESULT_RANGE_STATUS          0x001D
#define VL53L1X_RESULT_SPAD_NB               0x001E
#define VL53L1X_RESULT_SIGNAL_RATE           0x001F
#define VL53L1X_RESULT_FINAL_CROSSTALK       0x0021
#define VL53L1X_RESULT_PEAK_SIGNAL_RATE      0x0023
#define VL53L1X_RESULT_DISTANCE              0x0025  /* 距离结果 (mm) */

/* 定时配置 */
#define VL53L1X_SYSTEM_INTERMEASUREMENT_PERIOD 0x000E
#define VL53L1X_SYSTEM_THRESH_RATE_HIGH      0x0010
#define VL53L1X_SYSTEM_THRESH_RATE_LOW       0x0012

/* 测距配置 */
#define VL53L1X_DSS_CONFIG_TARGET_TOTAL_RATE 0x000E
#define VL53L1X_DSS_CONFIG_ROI_MODE          0x0018
#define VL53L1X_DSS_CONFIG_MANUAL_EFFECTIVE_SPADS 0x001A

/* ROI (Region of Interest) 配置 */
#define VL53L1X_ROI_CONFIG_USER_ROI_CENTRE_SPAD 0x0016
#define VL53L1X_ROI_CONFIG_USER_ROI_REQUESTED_GLOBAL_XY_SIZE 0x0017

/* 时序配置 */
#define VL53L1X_TIMING_CONFIG_VCSEL_PERIOD_A 0x000C
#define VL53L1X_TIMING_CONFIG_VCSEL_PERIOD_B 0x000E
#define VL53L1X_TIMING_CONFIG_REPEAT_MODE    0x0010

/* 固件配置 */
#define VL53L1X_FIRMWARE_CONFIG              0x0012

/* 校准数据 */
#define VL53L1X_CAL_CONFIG_VCSEL_START       0x0002
#define VL53L1X_CAL_CONFIG_REPEAT_RATE       0x0004

/* 动态配置 */
#define VL53L1X_DYNAMIC_CONFIG_TARGET_TOTAL_RATE_MIN_MCPS 0x0006
#define VL53L1X_DYNAMIC_CONFIG_VCSEL_PERIOD_A_OFFSET 0x0008
#define VL53L1X_DYNAMIC_CONFIG_VCSEL_PERIOD_B_OFFSET 0x000A

/* 系统阈值 */
#define VL53L1X_SYSTEM_THRESHOLD_RATE_HIGH  0x000C
#define VL53L1X_SYSTEM_THRESHOLD_RATE_LOW   0x000E

/* GPIO 配置 */
#define VL53L1X_GPIO_CONFIG_MODE            0x0010
#define VL53L1X_GPIO_CONFIG_POLARITY        0x0011

/* 调试寄存器 */
#define VL53L1X_DEBUG_CONFIG                0x0014

/* 校准状态 */
#define VL53L1X_CAL_CONFIG_SPAD_MAP         0x0006

/* 默认时序配置值 */
#define VL53L1X_DEFAULT_TIMEOUT_A           0x07
#define VL53L1X_DEFAULT_TIMEOUT_B           0x05
#define VL53L1X_DEFAULT_VCSEL_PERIOD_A      0x0B
#define VL53L1X_DEFAULT_VCSEL_PERIOD_B      0x09

/*============================================================================
 * 数据类型定义
 *===========================================================================*/

/**
 * 测距模式
 */
typedef enum {
    XY_VL53L1X_MODE_SINGLE = 0,    /*!< 单次测量 */
    XY_VL53L1X_MODE_CONTINUOUS,    /*!< 连续测量 */
    XY_VL53L1X_MODE_TIMED          /*!< 定时测量 */
} xy_vl53l1x_mode_t;

/**
 * 测距范围配置
 */
typedef enum {
    XY_VL53L1X_RANGE_SHORT = 0,    /*!< 短距离：0-1.3m (最佳精度) */
    XY_VL53L1X_RANGE_MEDIUM,       /*!< 中距离：0-3m */
    XY_VL53L1X_RANGE_LONG          /*!< 长距离：0-4m (降低精度) */
} xy_vl53l1x_range_t;

/**
 * 测量定时配置 (测量时间预算)
 */
typedef enum {
    XY_VL53L1X_TIMING_15MS = 15,    /*!< 15ms - 最快，噪声大 */
    XY_VL53L1X_TIMING_20MS = 20,    /*!< 20ms */
    XY_VL53L1X_TIMING_33MS = 33,    /*!< 33ms - 默认推荐 */
    XY_VL53L1X_TIMING_50MS = 50,    /*!< 50ms */
    XY_VL53L1X_TIMING_100MS = 100,  /*!< 100ms - 高精度 */
    XY_VL53L1X_TIMING_200MS = 200,  /*!< 200ms - 最高精度 */
    XY_VL53L1X_TIMING_500MS = 500   /*!< 500ms - 实验室级 */
} xy_vl53l1x_timing_t;

/**
 * 中断阈值模式
 */
typedef enum {
    XY_VL53L1X_INT_DISABLED = 0,   /*!< 禁用中断 */
    XY_VL53L1X_INT_LEVEL_LOW,      /*!< 低于低阈值触发 */
    XY_VL53L1X_INT_LEVEL_HIGH,     /*!< 高于高阈值触发 */
    XY_VL53L1X_INT_OUT_OF_WINDOW,  /*!< 超出窗口触发 */
    XY_VL53L1X_INT_IN_WINDOW       /*!< 在窗口内触发 */
} xy_vl53l1x_int_mode_t;

/**
 * 测量状态
 */
typedef enum {
    XY_VL53L1X_STATUS_VALID = 0,       /*!< 测量有效 */
    XY_VL53L1X_STATUS_SIGNAL_FAIL,     /*!< 信号强度不足 */
    XY_VL53L1X_STATUS_RANGE_FAIL,      /*!< 超出量程 */
    XY_VL53L1X_STATUS_REF_OVERFLOW,    /*!< 参考溢出 */
    XY_VL53L1X_STATUS_TIMEOUT,         /*!< 测量超时 */
    XY_VL53L1X_STATUS_NO_TARGET        /*!< 未检测到目标 */
} xy_vl53l1x_status_t;

/**
 * 测量结果结构体
 */
typedef struct {
    uint16_t distance;             /*!< 距离 (mm) */
    uint16_t signal_rate;          /*!< 信号率 (kcps) */
    uint16_t ambient_rate;         /*!< 环境光率 (kcps) */
    uint8_t spad_count;            /*!< 使用的 SPAD 数量 */
    uint8_t status;                /*!< 测量状态 @ref xy_vl53l1x_status_t */
    uint32_t timestamp;            /*!< 时间戳 (ms) */
} xy_vl53l1x_result_t;

/**
 * ROI (Region of Interest) 配置
 */
typedef struct {
    uint8_t centre_spad;           /*!< 中心 SPAD (0-255) */
    uint8_t width;                 /*!< ROI 宽度 (1-16) */
    uint8_t height;                /*!< ROI 高度 (1-16) */
} xy_vl53l1x_roi_t;

/**
 * 阈值配置 (用于中断)
 */
typedef struct {
    uint16_t low;                  /*!< 低阈值 (mm) */
    uint16_t high;                 /*!< 高阈值 (mm) */
} xy_vl53l1x_threshold_t;

/**
 * 设备配置结构体
 */
typedef struct {
    xy_vl53l1x_mode_t mode;        /*!< 测量模式 */
    xy_vl53l1x_range_t range;      /*!< 测距范围 */
    xy_vl53l1x_timing_t timing;    /*!< 测量定时 */
    xy_vl53l1x_int_mode_t int_mode;/*!< 中断模式 */
    xy_vl53l1x_roi_t roi;          /*!< ROI 配置 */
    xy_vl53l1x_threshold_t threshold; /*!< 阈值配置 */
    uint8_t i2c_address;           /*!< I2C 地址 */
    bool enable_interleaved;       /*!< 启用交错测量模式 */
} xy_vl53l1x_config_t;

/**
 * VL53L1X 设备结构体
 */
typedef struct {
    xy_i2c_dev_t *i2c;             /*!< I2C 设备 */
    xy_vl53l1x_config_t config;    /*!< 配置 */
    bool is_initialized;           /*!< 是否已初始化 */
    
    /* 设备信息 */
    uint8_t model_id;              /*!< 模型 ID */
    uint8_t module_type;           /*!< 模块类型 */
    uint16_t revision_id;          /*!< 版本号 */
    
    /* 校准数据 */
    uint16_t offset;               /*!< 距离偏移 (mm) */
    float xtalk;                   /*!< 串扰补偿 (kcps) */
    
    /* 状态 */
    xy_vl53l1x_result_t last_result; /*!< 上次测量结果 */
    uint32_t measurement_count;    /*!< 测量次数 */
} xy_vl53l1x_dev_t;

/*============================================================================
 * API 函数声明
 *===========================================================================*/

/**
 * @brief 初始化 VL53L1X 设备
 * @param dev VL53L1X 设备结构体
 * @param i2c I2C 设备指针
 * @param config 配置结构体 (可为 NULL 使用默认配置)
 * @return XY_OK 成功，其他失败
 * 
 * @note 初始化流程:
 * 1. 软件复位
 * 2. 读取设备信息
 * 3. 加载默认配置
 * 4. 配置测距参数
 */
xy_ret_t xy_vl53l1x_init(xy_vl53l1x_dev_t *dev, xy_i2c_dev_t *i2c, xy_vl53l1x_config_t *config);

/**
 * @brief 去初始化 VL53L1X 设备
 * @param dev VL53L1X 设备结构体
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_vl53l1x_deinit(xy_vl53l1x_dev_t *dev);

/**
 * @brief 读取设备信息
 * @param dev VL53L1X 设备结构体
 * @param model_id 模型 ID 指针 (可为 NULL)
 * @param module_type 模块类型指针 (可为 NULL)
 * @param revision_id 版本号指针 (可为 NULL)
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_vl53l1x_read_device_info(xy_vl53l1x_dev_t *dev, uint8_t *model_id, uint8_t *module_type, uint16_t *revision_id);

/**
 * @brief 软件复位 VL53L1X
 * @param dev VL53L1X 设备结构体
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_vl53l1x_soft_reset(xy_vl53l1x_dev_t *dev);

/**
 * @brief 开始单次测量
 * @param dev VL53L1X 设备结构体
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_vl53l1x_start_single(xy_vl53l1x_dev_t *dev);

/**
 * @brief 开始连续测量
 * @param dev VL53L1X 设备结构体
 * @param period_ms 测量间隔 (ms, 0=最快)
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_vl53l1x_start_continuous(xy_vl53l1x_dev_t *dev, uint32_t period_ms);

/**
 * @brief 停止测量
 * @param dev VL53L1X 设备结构体
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_vl53l1x_stop(xy_vl53l1x_dev_t *dev);

/**
 * @brief 检查测量是否完成
 * @param dev VL53L1X 设备结构体
 * @param ready 就绪状态指针
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_vl53l1x_check_data_ready(xy_vl53l1x_dev_t *dev, bool *ready);

/**
 * @brief 读取测量结果
 * @param dev VL53L1X 设备结构体
 * @param result 结果指针
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_vl53l1x_read_result(xy_vl53l1x_dev_t *dev, xy_vl53l1x_result_t *result);

/**
 * @brief 执行单次测量并读取结果 (阻塞)
 * @param dev VL53L1X 设备结构体
 * @param result 结果指针
 * @param timeout_ms 超时时间 (ms)
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_vl53l1x_measure(xy_vl53l1x_dev_t *dev, xy_vl53l1x_result_t *result, uint32_t timeout_ms);

/**
 * @brief 设置测距范围
 * @param dev VL53L1X 设备结构体
 * @param range 测距范围
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_vl53l1x_set_range(xy_vl53l1x_dev_t *dev, xy_vl53l1x_range_t range);

/**
 * @brief 设置测量定时 (时间预算)
 * @param dev VL53L1X 设备结构体
 * @param timing 定时配置
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_vl53l1x_set_timing(xy_vl53l1x_dev_t *dev, xy_vl53l1x_timing_t timing);

/**
 * @brief 设置 ROI (Region of Interest)
 * @param dev VL53L1X 设备结构体
 * @param roi ROI 配置
 * @return XY_OK 成功，其他失败
 * 
 * @note ROI 用于限制测量区域，可减少串扰和提高精度
 */
xy_ret_t xy_vl53l1x_set_roi(xy_vl53l1x_dev_t *dev, xy_vl53l1x_roi_t *roi);

/**
 * @brief 设置距离偏移校准
 * @param dev VL53L1X 设备结构体
 * @param offset 偏移量 (mm, 可为负数)
 */
void xy_vl53l1x_set_offset(xy_vl53l1x_dev_t *dev, int16_t offset);

/**
 * @brief 设置串扰补偿
 * @param dev VL53L1X 设备结构体
 * @param xtalk 串扰值 (kcps)
 */
void xy_vl53l1x_set_xtalk(xy_vl53l1x_dev_t *dev, float xtalk);

/**
 * @brief 执行偏移校准
 * @param dev VL53L1X 设备结构体
 * @param target_distance 目标距离 (mm)
 * @param samples 采样次数
 * @return XY_OK 成功，其他失败
 * 
 * @note 需要在已知距离放置反射率 17% 的目标物
 */
xy_ret_t xy_vl53l1x_calibrate_offset(xy_vl53l1x_dev_t *dev, uint16_t target_distance, uint8_t samples);

/**
 * @brief 执行串扰校准
 * @param dev VL53L1X 设备结构体
 * @param target_distance 目标距离 (mm)
 * @param target_reflectance 目标反射率 (%)
 * @param samples 采样次数
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_vl53l1x_calibrate_xtalk(xy_vl53l1x_dev_t *dev, uint16_t target_distance, uint8_t target_reflectance, uint8_t samples);

/**
 * @brief 配置中断阈值
 * @param dev VL53L1X 设备结构体
 * @param mode 中断模式
 * @param low 低阈值 (mm)
 * @param high 高阈值 (mm)
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_vl53l1x_configure_interrupt(xy_vl53l1x_dev_t *dev, xy_vl53l1x_int_mode_t mode, uint16_t low, uint16_t high);

/**
 * @brief 清除中断
 * @param dev VL53L1X 设备结构体
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_vl53l1x_clear_interrupt(xy_vl53l1x_dev_t *dev);

/**
 * @brief 更改 I2C 地址
 * @param dev VL53L1X 设备结构体
 * @param new_address 新 I2C 地址
 * @return XY_OK 成功，其他失败
 * 
 * @note 用于多传感器应用，避免地址冲突
 */
xy_ret_t xy_vl53l1x_change_i2c_address(xy_vl53l1x_dev_t *dev, uint8_t new_address);

/**
 * @brief 获取设备状态
 * @param dev VL53L1X 设备结构体
 * @return true 已初始化，false 未初始化
 */
bool xy_vl53l1x_is_ready(xy_vl53l1x_dev_t *dev);

/**
 * @brief 获取最后测量结果
 * @param dev VL53L1X 设备结构体
 * @return 测量结果指针
 */
xy_vl53l1x_result_t *xy_vl53l1x_get_last_result(xy_vl53l1x_dev_t *dev);

/*============================================================================
 * 工具函数
 *===========================================================================*/

/**
 * @brief 检查测量状态是否有效
 * @param status 测量状态
 * @return true 有效，false 无效
 */
static inline bool xy_vl53l1x_is_status_valid(uint8_t status)
{
    return (status == XY_VL53L1X_STATUS_VALID);
}

/**
 * @brief 将距离转换为厘米
 * @param distance_mm 距离 (mm)
 * @return 距离 (cm)
 */
static inline float xy_vl53l1x_mm_to_cm(uint16_t distance_mm)
{
    return (float)distance_mm / 10.0f;
}

/**
 * @brief 将距离转换为米
 * @param distance_mm 距离 (mm)
 * @return 距离 (m)
 */
static inline float xy_vl53l1x_mm_to_m(uint16_t distance_mm)
{
    return (float)distance_mm / 1000.0f;
}

#ifdef __cplusplus
}
#endif

#endif /* XY_VL53L1X_H */
