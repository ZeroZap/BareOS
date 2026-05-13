/**
 * @file xy_ltc2945.h
 * @brief LTC2945 Power Monitor Driver
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#ifndef XY_LTC2945_H
#define XY_LTC2945_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_dev_i2c.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief LTC2945 I2C 地址
 */
#define LTC2945_ADDR_ADDR0      0x68  /* ADDR0 = GND */
#define LTC2945_ADDR_ADDR1      0x69  /* ADDR0 = VDD */

/**
 * @brief LTC2945 寄存器地址
 */
#define LTC2945_REG_STATUS      0x00
#define LTC2945_REG_CONTROL     0x01
#define LTC2945_REG_ALERT       0x02
#define LTC2945_REG_FAULT       0x03
#define LTC2945_REG_VIN_MSB     0x08
#define LTC2945_REG_VIN_LSB     0x09
#define LTC2945_REG_VSENSE_MSB  0x0A
#define LTC2945_REG_VSENSE_LSB  0x0B
#define LTC2945_REG_VDELTA_MSB  0x0C
#define LTC2945_REG_VDELTA_LSB  0x0D
#define LTC2945_REG_POWER_MSB   0x10
#define LTC2945_REG_POWER_MID   0x11
#define LTC2945_REG_POWER_LSB   0x12
#define LTC2945_REG_CHARGE_MSB  0x14
#define LTC2945_REG_CHARGE_MID  0x15
#define LTC2945_REG_CHARGE_LSB  0x16
#define LTC2945_REG_ENERGY_MSB  0x18
#define LTC2945_REG_ENERGY_MID  0x19
#define LTC2945_REG_ENERGY_LSB  0x1A
#define LTC2945_REG_CTRL_GPIO   0x20
#define LTC2945_REG_SENSE_MSB   0x24
#define LTC2945_REG_SENSE_LSB   0x25

/**
 * @brief 错误码
 */
#define XY_LTC2945_OK           0
#define XY_LTC2945_ERROR        (-1)
#define XY_LTC2945_INVALID_PARAM (-2)
#define XY_LTC2945_NOT_FOUND    (-3)

/**
 * @brief 测量数据
 */
typedef struct {
    float voltage_v;          /* 输入电压 (V) */
    float current_a;          /* 电流 (A) */
    float power_w;            /* 功率 (W) */
    float charge_c;           /* 累计电荷 (C) */
    float energy_j;           /* 累计能量 (J) */
    float shunt_voltage_v;    /* 分流电压 (V) */
    uint32_t timestamp;       /* 时间戳 (ms) */
} xy_ltc2945_data_t;

/**
 * @brief LTC2945 配置
 */
typedef struct {
    float shunt_resistor_mohm;  /* 分流电阻 (mΩ), 典型值 10-50mΩ */
    bool auto_convert;          /* 自动转换模式 */
    uint8_t alert_gpio_config;  /* ALERT GPIO 配置 */
} xy_ltc2945_config_t;

/**
 * @brief LTC2945 句柄
 */
typedef struct {
    xy_i2c_device_t i2c_dev;    /* I2C 设备 */
    uint8_t addr;               /* I2C 地址 */
    xy_ltc2945_config_t config; /* 配置 */
    xy_ltc2945_data_t data;     /* 测量数据 */
    float power_lsb;            /* 功率 LSB (W) */
    float charge_lsb;           /* 电荷 LSB (C) */
    float energy_lsb;           /* 能量 LSB (J) */
    bool initialized;           /* 初始化标志 */
} xy_ltc2945_t;

/**
 * @brief 初始化 LTC2945
 * @param ltc2945 LTC2945 句柄
 * @param i2c_handle I2C 句柄
 * @param addr I2C 地址
 * @param config 配置
 * @return XY_LTC2945_OK 成功，其他值失败
 */
int xy_ltc2945_init(xy_ltc2945_t *ltc2945, void *i2c_handle, uint8_t addr,
                    const xy_ltc2945_config_t *config);

/**
 * @brief 反初始化
 * @param ltc2945 LTC2945 句柄
 * @return XY_LTC2945_OK 成功，其他值失败
 */
int xy_ltc2945_deinit(xy_ltc2945_t *ltc2945);

/**
 * @brief 读取测量数据
 * @param ltc2945 LTC2945 句柄
 * @return XY_LTC2945_OK 成功，其他值失败
 */
int xy_ltc2945_read(xy_ltc2945_t *ltc2945);

/**
 * @brief 获取输入电压
 * @param ltc2945 LTC2945 句柄
 * @param voltage_v 电压指针 (V)
 * @return XY_LTC2945_OK 成功，其他值失败
 */
int xy_ltc2945_get_voltage(xy_ltc2945_t *ltc2945, float *voltage_v);

/**
 * @brief 获取电流
 * @param ltc2945 LTC2945 句柄
 * @param current_a 电流指针 (A)
 * @return XY_LTC2945_OK 成功，其他值失败
 */
int xy_ltc2945_get_current(xy_ltc2945_t *ltc2945, float *current_a);

/**
 * @brief 获取功率
 * @param ltc2945 LTC2945 句柄
 * @param power_w 功率指针 (W)
 * @return XY_LTC2945_OK 成功，其他值失败
 */
int xy_ltc2945_get_power(xy_ltc2945_t *ltc2945, float *power_w);

/**
 * @brief 获取累计电荷
 * @param ltc2945 LTC2945 句柄
 * @param charge_c 电荷指针 (C)
 * @return XY_LTC2945_OK 成功，其他值失败
 */
int xy_ltc2945_get_charge(xy_ltc2945_t *ltc2945, float *charge_c);

/**
 * @brief 获取累计能量
 * @param ltc2945 LTC2945 句柄
 * @param energy_j 能量指针 (J)
 * @return XY_LTC2945_OK 成功，其他值失败
 */
int xy_ltc2945_get_energy(xy_ltc2945_t *ltc2945, float *energy_j);

/**
 * @brief 复位累计计数器
 * @param ltc2945 LTC2945 句柄
 * @return XY_LTC2945_OK 成功，其他值失败
 */
int xy_ltc2945_reset_counters(xy_ltc2945_t *ltc2945);

/**
 * @brief 使能告警
 * @param ltc2945 LTC2945 句柄
 * @param enable 是否使能
 * @return XY_LTC2945_OK 成功，其他值失败
 */
int xy_ltc2945_enable_alert(xy_ltc2945_t *ltc2945, bool enable);

#ifdef __cplusplus
}
#endif

#endif
