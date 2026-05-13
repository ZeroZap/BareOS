/**
 * @file xy_max17043.h
 * @brief MAX17043/MAX17044 Fuel Gauge Driver
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#ifndef XY_MAX17043_H
#define XY_MAX17043_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_dev_i2c.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief MAX17043 I2C 地址
 */
#define MAX17043_ADDR           0x36

/**
 * @brief MAX17043 寄存器地址
 */
#define MAX17043_REG_VCELL      0x02  /* 电池电压 */
#define MAX17043_REG_SOC        0x04  /* 电量百分比 */
#define MAX17043_REG_MODE       0x06  /* 模式寄存器 */
#define MAX17043_REG_VER        0x08  /* 版本寄存器 */
#define MAX17043_REG_HIBRT      0x0A  /* 休眠唤醒 */
#define MAX17043_REG_CONFIG     0x0C  /* 配置寄存器 */
#define MAX17043_REG_VALRT      0x14  /* 电压告警 */
#define MAX17043_REG_CRATE      0x16  /* 充放电率 */
#define MAX17043_REG_VRESET     0x18  /* 复位电压 */
#define MAX17043_REG_STATUS     0x1A  /* 状态寄存器 */
#define MAX17043_REG_UNLOCK     0x3E  /* 解锁寄存器 */
#define MAX17043_REG_COMMAND    0xFF  /* 命令寄存器 */

/**
 * @brief 错误码
 */
#define XY_MAX17043_OK          0
#define XY_MAX17043_ERROR       (-1)
#define XY_MAX17043_INVALID_PARAM (-2)
#define XY_MAX17043_NOT_FOUND   (-3)

/**
 * @brief 电量计数据
 */
typedef struct {
    float voltage_mv;         /* 电池电压 (mV) */
    float percentage;         /* 电量百分比 (0-100%) */
    float crate;              /* 充放电率 (C 率，正=充电，负=放电) */
    uint8_t version;          /* 版本号 */
    bool low_battery;         /* 低电量告警 */
    bool reset_triggered;     /* 复位触发 */
    uint32_t timestamp;       /* 时间戳 (ms) */
} xy_max17043_data_t;

/**
 * @brief MAX17043 配置
 */
typedef struct {
    float capacity_mah;         /* 电池容量 (mAh) */
    float alert_voltage_mv;     /* 低电量告警电压 (mV) */
    bool enable_hibernate;      /* 使能休眠模式 */
} xy_max17043_config_t;

/**
 * @brief MAX17043 句柄
 */
typedef struct {
    xy_i2c_device_t i2c_dev;    /* I2C 设备 */
    xy_max17043_config_t config;/* 配置 */
    xy_max17043_data_t data;    /* 最新数据 */
    bool initialized;           /* 初始化标志 */
} xy_max17043_t;

/**
 * @brief 初始化 MAX17043
 * @param max17043 MAX17043 句柄
 * @param i2c_handle I2C 句柄
 * @param config 配置
 * @return XY_MAX17043_OK 成功，其他值失败
 */
int xy_max17043_init(xy_max17043_t *max17043, void *i2c_handle, 
                     const xy_max17043_config_t *config);

/**
 * @brief 反初始化
 * @param max17043 MAX17043 句柄
 * @return XY_MAX17043_OK 成功，其他值失败
 */
int xy_max17043_deinit(xy_max17043_t *max17043);

/**
 * @brief 读取电量数据
 * @param max17043 MAX17043 句柄
 * @return XY_MAX17043_OK 成功，其他值失败
 */
int xy_max17043_read(xy_max17043_t *max17043);

/**
 * @brief 读取电压
 * @param max17043 MAX17043 句柄
 * @param voltage_mv 电压指针 (mV)
 * @return XY_MAX17043_OK 成功，其他值失败
 */
int xy_max17043_get_voltage(xy_max17043_t *max17043, float *voltage_mv);

/**
 * @brief 读取电量百分比
 * @param max17043 MAX17043 句柄
 * @param percentage 百分比指针 (0-100%)
 * @return XY_MAX17043_OK 成功，其他值失败
 */
int xy_max17043_get_percentage(xy_max17043_t *max17043, float *percentage);

/**
 * @brief 读取充放电率
 * @param max17043 MAX17043 句柄
 * @param crate 充放电率指针 (C 率)
 * @return XY_MAX17043_OK 成功，其他值失败
 */
int xy_max17043_get_crate(xy_max17043_t *max17043, float *crate);

/**
 * @brief 设置电池容量
 * @param max17043 MAX17043 句柄
 * @param capacity_mah 电池容量 (mAh)
 * @return XY_MAX17043_OK 成功，其他值失败
 */
int xy_max17043_set_capacity(xy_max17043_t *max17043, float capacity_mah);

/**
 * @brief 使能休眠模式
 * @param max17043 MAX17043 句柄
 * @param enable 是否使能
 * @return XY_MAX17043_OK 成功，其他值失败
 */
int xy_max17043_enable_hibernate(xy_max17043_t *max17043, bool enable);

/**
 * @brief 复位电量计
 * @param max17043 MAX17043 句柄
 * @return XY_MAX17043_OK 成功，其他值失败
 */
int xy_max17043_reset(xy_max17043_t *max17043);

#ifdef __cplusplus
}
#endif

#endif
