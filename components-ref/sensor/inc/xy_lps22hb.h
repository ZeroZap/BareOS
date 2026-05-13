/**
 * @file xy_lps22hb.h
 * @brief LPS22HB Waterproof Barometric Pressure Sensor Driver
 * @version 1.0.0
 * @date 2026-03-14
 * 
 * LPS22HB 是 ST 出品的防水气压传感器
 * 特点：
 * - 压力范围：260-1260 hPa
 * - 精度：±0.25 hPa (±2m 海拔精度)
 * - 防水设计 (IP67)
 * - I2C/SPI接口
 * - 低功耗 (1.8μA @1Hz)
 * 
 * @note 工作电压 1.7V-3.6V
 */

#ifndef XY_LPS22HB_H
#define XY_LPS22HB_H

#include <stdint.h>
#include <stdbool.h>
#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 * 硬件抽象层接口
 *===========================================================================*/

/**
 * I2C/SPI设备结构体
 */
typedef struct {
    void *handle;
    uint8_t address;
    bool is_spi;
} xy_interface_dev_t;

/**
 * @brief 写寄存器 (需要用户实现)
 */
xy_ret_t xy_write_reg(xy_interface_dev_t *dev, uint8_t reg_addr, const uint8_t *data, uint16_t len);

/**
 * @brief 读寄存器 (需要用户实现)
 */
xy_ret_t xy_read_reg(xy_interface_dev_t *dev, uint8_t reg_addr, uint8_t *data, uint16_t len);

/**
 * @brief 延时函数 (需要用户实现)
 */
void xy_delay_ms(uint32_t ms);

/*============================================================================
 * LPS22HB 寄存器定义
 *===========================================================================*/

#define LPS22HB_I2C_ADDR        0x5D  /* 默认 I2C 地址 */

/* 功能寄存器 */
#define LPS22HB_INTERRUPT_CFG   0x0B
#define LPS22HB_THS_P_L         0x0C
#define LPS22HB_THS_P_H         0x0D
#define LPS22HB_IF_CTRL         0x0E
#define LPS22HB_WHO_AM_I        0x0F
#define LPS22HB_CTRL_REG1       0x10
#define LPS22HB_CTRL_REG2       0x11
#define LPS22HB_CTRL_REG3       0x12
#define LPS22HB_FIFO_CTRL       0x14
#define LPS22HB_FIFO_WTM        0x15
#define LPS22HB_REF_P_L         0x16
#define LPS22HB_REF_P_H         0x17
#define LPS22HB_RPDS_L          0x18
#define LPS22HB_RPDS_H          0x19
#define LPS22HB_INT_SOURCE      0x24
#define LPS22HB_FIFO_STATUS1    0x25
#define LPS22HB_FIFO_STATUS2    0x26
#define LPS22HB_STATUS          0x27
#define LPS22HB_PRESS_OUT_XL    0x28
#define LPS22HB_PRESS_OUT_L     0x29
#define LPS22HB_PRESS_OUT_H     0x2A
#define LPS22HB_TEMP_OUT_L      0x2B
#define LPS22HB_TEMP_OUT_H      0x2C
#define LPS22HB_FIFO_DATA_OUT_PRESS_XL  0x78
#define LPS22HB_FIFO_DATA_OUT_PRESS_L   0x79
#define LPS22HB_FIFO_DATA_OUT_PRESS_H   0x7A
#define LPS22HB_FIFO_DATA_OUT_TEMP_L    0x7B
#define LPS22HB_FIFO_DATA_OUT_TEMP_H    0x7C

/* WHO_AM_I 值 */
#define LPS22HB_WHO_AM_I_VALUE  0xB1

/* CTRL_REG1 位定义 */
#define LPS22HB_ODR_MASK        0xF0
#define LPS22HB_LPFP_CFG_MASK   0x0C
#define LPS22HB_EN_LPFP         0x08
#define LPS22HB_SIM             0x01

/* CTRL_REG2 位定义 */
#define LPS22HB_BOOT            0x80
#define LPS22HB_FIFO_EN         0x40
#define LPS22HB_WTM_EN          0x20
#define LPS22HB_SWRESET         0x04
#define LPS22HB_AUTOZERO        0x02
#define LPS22HB_AUTO_RIFP       0x01

/* CTRL_REG3 位定义 */
#define LPS22HB_INT_DRDY        0x01

/* STATUS 位定义 */
#define LPS22HB_P_DA            0x01  /* 压力数据就绪 */
#define LPS22HB_T_DA            0x02  /* 温度数据就绪 */

/*============================================================================
 * 数据类型定义
 *===========================================================================*/

/**
 * 输出数据速率
 */
typedef enum {
    XY_LPS22HB_ODR_ONE_SHOT = 0x00,  /*!< 单次测量 */
    XY_LPS22HB_ODR_1HZ      = 0x10,  /*!< 1 Hz */
    XY_LPS22HB_ODR_10HZ     = 0x20,  /*!< 10 Hz */
    XY_LPS22HB_ODR_25HZ     = 0x30,  /*!< 25 Hz */
    XY_LPS22HB_ODR_50HZ     = 0x40,  /*!< 50 Hz */
    XY_LPS22HB_ODR_75HZ     = 0x50,  /*!< 75 Hz */
    XY_LPS22HB_ODR_100HZ    = 0x60,  /*!< 100 Hz */
    XY_LPS22HB_ODR_200HZ    = 0x70   /*!< 200 Hz */
} xy_lps22hb_odr_t;

/**
 * 低通滤波器配置
 */
typedef enum {
    XY_LPS22HB_LPF_ODR_9   = 0x00,  /*!< ODR/9 */
    XY_LPS22HB_LPF_ODR_20  = 0x04,  /*!< ODR/20 */
    XY_LPS22HB_LPF_ODR_32  = 0x08,  /*!< ODR/32 */
    XY_LPS22HB_LPF_ODR_5   = 0x0C   /*!< ODR/5 */
} xy_lps22hb_lpf_t;

/**
 * 测量结果结构体
 */
typedef struct {
    float pressure;            /*!< 压力 (hPa) */
    float temperature;         /*!< 温度 (°C) */
    float altitude;            /*!< 海拔高度 (m) */
    uint32_t timestamp;        /*!< 时间戳 (ms) */
    uint8_t status;            /*!< 状态寄存器值 */
} xy_lps22hb_data_t;

/**
 * 阈值配置 (用于中断)
 */
typedef struct {
    uint16_t low;              /*!< 低阈值 (hPa * 100) */
    uint16_t high;             /*!< 高阈值 (hPa * 100) */
} xy_lps22hb_threshold_t;

/**
 * FIFO 配置
 */
typedef enum {
    XY_LPS22HB_FIFO_BYPASS = 0x00,
    XY_LPS22HB_FIFO_MODE   = 0x20,
    XY_LPS22HB_FIFO_STREAM = 0x40,
    XY_LPS22HB_FIFO_TRIGGER = 0x60
} xy_lps22hb_fifo_mode_t;

/**
 * 设备配置结构体
 */
typedef struct {
    xy_lps22hb_odr_t odr;      /*!< 输出数据速率 */
    xy_lps22hb_lpf_t lpf;      /*!< 低通滤波器 */
    bool enable_lpf;           /*!< 使能低通滤波 */
    bool enable_fifo;          /*!< 使能 FIFO */
    xy_lps22hb_fifo_mode_t fifo_mode; /*!< FIFO 模式 */
    uint8_t fifo_wtm;          /*!< FIFO 水位标记 */
    bool enable_interrupt;     /*!< 使能中断 */
    xy_lps22hb_threshold_t threshold; /*!< 阈值配置 */
} xy_lps22hb_config_t;

/**
 * LPS22HB 设备结构体
 */
typedef struct {
    xy_interface_dev_t *interface;  /*!< 接口设备 */
    xy_lps22hb_config_t config;     /*!< 配置 */
    bool is_initialized;            /*!< 是否已初始化 */
    
    /* 设备信息 */
    uint8_t who_am_i;               /*!< WHO_AM_I 值 */
    
    /* 校准参数 */
    float pressure_offset;          /*!< 压力偏移 (hPa) */
    float temperature_offset;       /*!< 温度偏移 (°C) */
    float sea_level_pressure;       /*!< 海平面气压 (hPa), 用于海拔计算 */
    
    /* 最后测量结果 */
    xy_lps22hb_data_t last_data;    /*!< 最后测量数据 */
    uint32_t measurement_count;     /*!< 测量次数 */
} xy_lps22hb_dev_t;

/*============================================================================
 * API 函数声明
 *===========================================================================*/

/**
 * @brief 初始化 LPS22HB 设备
 * @param dev LPS22HB 设备结构体
 * @param interface 接口设备指针
 * @param config 配置结构体 (可为 NULL 使用默认配置)
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_lps22hb_init(xy_lps22hb_dev_t *dev, xy_interface_dev_t *interface, xy_lps22hb_config_t *config);

/**
 * @brief 去初始化 LPS22HB 设备
 * @param dev LPS22HB 设备结构体
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_lps22hb_deinit(xy_lps22hb_dev_t *dev);

/**
 * @brief 读取 WHO_AM_I
 * @param dev LPS22HB 设备结构体
 * @param who_am_i WHO_AM_I 值指针
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_lps22hb_read_who_am_i(xy_lps22hb_dev_t *dev, uint8_t *who_am_i);

/**
 * @brief 软件复位 LPS22HB
 * @param dev LPS22HB 设备结构体
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_lps22hb_soft_reset(xy_lps22hb_dev_t *dev);

/**
 * @brief 启动单次测量
 * @param dev LPS22HB 设备结构体
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_lps22hb_start_single(xy_lps22hb_dev_t *dev);

/**
 * @brief 启动连续测量
 * @param dev LPS22HB 设备结构体
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_lps22hb_start_continuous(xy_lps22hb_dev_t *dev);

/**
 * @brief 停止测量
 * @param dev LPS22HB 设备结构体
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_lps22hb_stop(xy_lps22hb_dev_t *dev);

/**
 * @brief 检查数据是否就绪
 * @param dev LPS22HB 设备结构体
 * @param ready 就绪状态指针
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_lps22hb_check_data_ready(xy_lps22hb_dev_t *dev, bool *ready);

/**
 * @brief 读取压力和温度数据
 * @param dev LPS22HB 设备结构体
 * @param data 数据指针
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_lps22hb_read_data(xy_lps22hb_dev_t *dev, xy_lps22hb_data_t *data);

/**
 * @brief 执行单次测量并读取结果 (阻塞)
 * @param dev LPS22HB 设备结构体
 * @param data 数据指针
 * @param timeout_ms 超时时间 (ms)
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_lps22hb_measure(xy_lps22hb_dev_t *dev, xy_lps22hb_data_t *data, uint32_t timeout_ms);

/**
 * @brief 设置输出数据速率
 * @param dev LPS22HB 设备结构体
 * @param odr 数据速率
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_lps22hb_set_odr(xy_lps22hb_dev_t *dev, xy_lps22hb_odr_t odr);

/**
 * @brief 配置低通滤波器
 * @param dev LPS22HB 设备结构体
 * @param enable 使能
 * @param lpf 滤波器配置
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_lps22hb_configure_lpf(xy_lps22hb_dev_t *dev, bool enable, xy_lps22hb_lpf_t lpf);

/**
 * @brief 设置压力偏移校准
 * @param dev LPS22HB 设备结构体
 * @param offset 偏移量 (hPa)
 */
void xy_lps22hb_set_pressure_offset(xy_lps22hb_dev_t *dev, float offset);

/**
 * @brief 设置温度偏移校准
 * @param dev LPS22HB 设备结构体
 * @param offset 偏移量 (°C)
 */
void xy_lps22hb_set_temperature_offset(xy_lps22hb_dev_t *dev, float offset);

/**
 * @brief 设置海平面气压 (用于海拔计算)
 * @param dev LPS22HB 设备结构体
 * @param pressure 海平面气压 (hPa)
 */
void xy_lps22hb_set_sea_level_pressure(xy_lps22hb_dev_t *dev, float pressure);

/**
 * @brief 执行自动归零校准
 * @param dev LPS22HB 设备结构体
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_lps22hb_auto_zero(xy_lps22hb_dev_t *dev);

/**
 * @brief 配置 FIFO
 * @param dev LPS22HB 设备结构体
 * @param mode FIFO 模式
 * @param wtm 水位标记
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_lps22hb_configure_fifo(xy_lps22hb_dev_t *dev, xy_lps22hb_fifo_mode_t mode, uint8_t wtm);

/**
 * @brief 配置中断阈值
 * @param dev LPS22HB 设备结构体
 * @param low 低阈值 (hPa * 100)
 * @param high 高阈值 (hPa * 100)
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_lps22hb_configure_threshold(xy_lps22hb_dev_t *dev, uint16_t low, uint16_t high);

/**
 * @brief 清除中断
 * @param dev LPS22HB 设备结构体
 * @return XY_OK 成功，其他失败
 */
xy_ret_t xy_lps22hb_clear_interrupt(xy_lps22hb_dev_t *dev);

/**
 * @brief 获取设备状态
 * @param dev LPS22HB 设备结构体
 * @return true 已初始化，false 未初始化
 */
bool xy_lps22hb_is_ready(xy_lps22hb_dev_t *dev);

/**
 * @brief 获取最后测量结果
 * @param dev LPS22HB 设备结构体
 * @return 数据指针
 */
xy_lps22hb_data_t *xy_lps22hb_get_last_data(xy_lps22hb_dev_t *dev);

/*============================================================================
 * 工具函数
 *===========================================================================*/

/**
 * @brief 将压力转换为海拔高度
 * @param pressure 压力 (hPa)
 * @param sea_level_pressure 海平面气压 (hPa)
 * @return 海拔高度 (m)
 * 
 * @note 使用国际大气公式
 */
float xy_lps22hb_pressure_to_altitude(float pressure, float sea_level_pressure);

/**
 * @brief 将海拔高度转换为压力
 * @param altitude 海拔高度 (m)
 * @param sea_level_pressure 海平面气压 (hPa)
 * @return 压力 (hPa)
 */
float xy_lps22hb_altitude_to_pressure(float altitude, float sea_level_pressure);

#ifdef __cplusplus
}
#endif

#endif /* XY_LPS22HB_H */
