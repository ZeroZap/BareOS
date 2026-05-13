#include <stdbool.h>
/**
 * @file xy_coulomb.h
 * @brief Coulomb Counter Fuel Gauge Driver (INA226/INA219)
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#ifndef XY_COULOMB_H
#define XY_COULOMB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_dev_i2c.h"
#include <stdint.h>

/**
 * @brief INA226 I2C 地址
 */
#define INA226_ADDR_GND         0x40  /* A0 -> GND */
#define INA226_ADDR_VREF        0x41  /* A0 -> VREF */
#define INA226_ADDR_SDA         0x42  /* A0 -> SDA */
#define INA226_ADDR_SCL         0x43  /* A0 -> SCL */

/**
 * @brief INA226 寄存器地址
 */
#define INA226_REG_CONFIG       0x00
#define INA226_REG_SHUNT_VOLT   0x01
#define INA226_REG_BUS_VOLT     0x02
#define INA226_REG_POWER        0x03
#define INA226_REG_CURRENT      0x04
#define INA226_REG_CALIB        0x05
#define INA226_REG_MASK_EN      0x06
#define INA226_REG_ALERT_LIMIT  0x07
#define INA226_REG_MFG_ID       0xFE
#define INA226_REG_DIE_ID       0xFF

/**
 * @brief INA226 ID 值
 */
#define INA226_MFG_ID_VALUE     0x5449  /* Texas Instruments */
#define INA226_DIE_ID_VALUE     0x2260

/**
 * @brief 错误码
 */
#define XY_COULOMB_OK           0
#define XY_COULOMB_ERROR        (-1)
#define XY_COULOMB_INVALID_PARAM (-2)
#define XY_COULOMB_NOT_FOUND    (-3)

/**
 * @brief 电量计数据
 */
typedef struct {
    float voltage_mv;         /* 电池电压 (mV) */
    float current_ma;         /* 电流 (mA, 正=充电，负=放电) */
    float power_mw;           /* 功率 (mW) */
    float charge_mah;         /* 累计容量 (mAh) */
    float percentage;         /* 电量百分比 (0-100%) */
    int16_t temperature;      /* 温度 (0.1°C) */
    uint32_t timestamp;       /* 时间戳 (ms) */
} xy_coulomb_data_t;

/**
 * @brief 电量计配置
 */
typedef struct {
    float shunt_resistor_mohm;  /* 分流电阻 (mΩ), 典型值 10-50mΩ */
    float capacity_mah;         /* 电池容量 (mAh) */
    uint8_t avg_samples;        /* 平均采样数 (1/4/16/64/128) */
    uint16_t alert_current_ma;  /* 电流告警阈值 (mA) */
} xy_coulomb_config_t;

/**
 * @brief 电量计句柄
 */
typedef struct {
    xy_i2c_device_t i2c_dev;    /* I2C 设备 */
    uint8_t addr;               /* I2C 地址 */
    xy_coulomb_config_t config; /* 配置 */
    xy_coulomb_data_t data;     /* 最新数据 */
    float current_lsb;          /* 电流 LSB (A) */
    uint16_t calib_value;       /* 校准值 */
    bool initialized;           /* 初始化标志 */
} xy_coulomb_t;

/**
 * @brief 初始化电量计
 * @param coulomb 电量计句柄
 * @param i2c_handle I2C 句柄
 * @param addr I2C 地址
 * @param config 配置
 * @return XY_COULOMB_OK 成功，其他值失败
 */
int xy_coulomb_init(xy_coulomb_t *coulomb, void *i2c_handle, uint8_t addr, 
                    const xy_coulomb_config_t *config);

/**
 * @brief 反初始化
 * @param coulomb 电量计句柄
 * @return XY_COULOMB_OK 成功，其他值失败
 */
int xy_coulomb_deinit(xy_coulomb_t *coulomb);

/**
 * @brief 读取电量数据
 * @param coulomb 电量计句柄
 * @return XY_COULOMB_OK 成功，其他值失败
 */
int xy_coulomb_read(xy_coulomb_t *coulomb);

/**
 * @brief 读取电压
 * @param coulomb 电量计句柄
 * @param voltage_mv 电压指针 (mV)
 * @return XY_COULOMB_OK 成功，其他值失败
 */
int xy_coulomb_get_voltage(xy_coulomb_t *coulomb, float *voltage_mv);

/**
 * @brief 读取电流
 * @param coulomb 电量计句柄
 * @param current_ma 电流指针 (mA, 正=充电，负=放电)
 * @return XY_COULOMB_OK 成功，其他值失败
 */
int xy_coulomb_get_current(xy_coulomb_t *coulomb, float *current_ma);

/**
 * @brief 读取功率
 * @param coulomb 电量计句柄
 * @param power_mw 功率指针 (mW)
 * @return XY_COULOMB_OK 成功，其他值失败
 */
int xy_coulomb_get_power(xy_coulomb_t *coulomb, float *power_mw);

/**
 * @brief 读取累计容量
 * @param coulomb 电量计句柄
 * @param charge_mah 容量指针 (mAh)
 * @return XY_COULOMB_OK 成功，其他值失败
 */
int xy_coulomb_get_charge(xy_coulomb_t *coulomb, float *charge_mah);

/**
 * @brief 读取电量百分比
 * @param coulomb 电量计句柄
 * @param percentage 百分比指针 (0-100%)
 * @return XY_COULOMB_OK 成功，其他值失败
 */
int xy_coulomb_get_percentage(xy_coulomb_t *coulomb, float *percentage);

/**
 * @brief 重置累计容量
 * @param coulomb 电量计句柄
 * @return XY_COULOMB_OK 成功，其他值失败
 */
int xy_coulomb_reset_charge(xy_coulomb_t *coulomb);

/**
 * @brief 设置电池容量
 * @param coulomb 电量计句柄
 * @param capacity_mah 电池容量 (mAh)
 * @return XY_COULOMB_OK 成功，其他值失败
 */
int xy_coulomb_set_capacity(xy_coulomb_t *coulomb, float capacity_mah);

/**
 * @brief 使能告警
 * @param coulomb 电量计句柄
 * @param enable 是否使能
 * @return XY_COULOMB_OK 成功，其他值失败
 */
int xy_coulomb_enable_alert(xy_coulomb_t *coulomb, bool enable);

#ifdef __cplusplus
}
#endif

#endif
