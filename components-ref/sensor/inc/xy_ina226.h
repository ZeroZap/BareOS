/**
 * @file xy_ina226.h
 * @brief INA226/INA229 Power Monitor Driver
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#ifndef XY_INA226_H
#define XY_INA226_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_dev_i2c.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief INA226 I2C 地址
 */
#define INA226_ADDR_GND         0x40
#define INA226_ADDR_VREF        0x41
#define INA226_ADDR_SDA         0x42
#define INA226_ADDR_SCL         0x43

/**
 * @brief INA229 I2C 地址
 */
#define INA229_ADDR_GND         0x40
#define INA229_ADDR_VREF        0x41
#define INA229_ADDR_SDA         0x42
#define INA229_ADDR_SCL         0x43

/**
 * @brief 寄存器地址
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
 * @brief ID 值
 */
#define INA226_MFG_ID_VALUE     0x5449  /* TI */
#define INA226_DIE_ID_VALUE     0x2260
#define INA229_DIE_ID_VALUE     0x2290

/**
 * @brief 错误码
 */
#define XY_INA_OK               0
#define XY_INA_ERROR            (-1)
#define XY_INA_INVALID_PARAM    (-2)
#define XY_INA_NOT_FOUND        (-3)

/**
 * @brief 设备类型
 */
typedef enum {
    XY_INA_DEVICE_INA226 = 0,
    XY_INA_DEVICE_INA229,
} xy_ina_device_t;

/**
 * @brief 平均采样数
 */
typedef enum {
    XY_INA_AVG_1 = 0,
    XY_INA_AVG_4 = 1,
    XY_INA_AVG_16 = 2,
    XY_INA_AVG_64 = 3,
    XY_INA_AVG_128 = 4,
    XY_INA_AVG_256 = 5,
    XY_INA_AVG_512 = 6,
    XY_INA_AVG_1024 = 7,
} xy_ina_avg_t;

/**
 * @brief 配置结构
 */
typedef struct {
    float shunt_resistor_mohm;  /* 分流电阻 (mΩ) */
    xy_ina_avg_t avg_samples;   /* 平均采样数 */
    uint16_t alert_current_ma;  /* 告警电流阈值 (mA) */
} xy_ina_config_t;

/**
 * @brief 测量数据
 */
typedef struct {
    float voltage_mv;           /* 母线电压 (mV) */
    float current_ma;           /* 电流 (mA, 正=充电，负=放电) */
    float power_mw;             /* 功率 (mW) */
    float shunt_voltage_uv;     /* 分流电压 (uV) */
    uint32_t timestamp;         /* 时间戳 (ms) */
} xy_ina_data_t;

/**
 * @brief 设备句柄
 */
typedef struct {
    xy_i2c_device_t i2c_dev;    /* I2C 设备 */
    uint8_t addr;               /* I2C 地址 */
    xy_ina_device_t device;     /* 设备类型 */
    xy_ina_config_t config;     /* 配置 */
    xy_ina_data_t data;         /* 测量数据 */
    float current_lsb;          /* 电流 LSB (A) */
    uint16_t calib_value;       /* 校准值 */
    bool initialized;           /* 初始化标志 */
} xy_ina_t;

/**
 * @brief 初始化 INA226/INA229
 * @param ina 设备句柄
 * @param i2c_handle I2C 句柄
 * @param addr I2C 地址
 * @param config 配置
 * @return XY_INA_OK 成功，其他值失败
 */
int xy_ina_init(xy_ina_t *ina, void *i2c_handle, uint8_t addr, const xy_ina_config_t *config);

/**
 * @brief 反初始化
 * @param ina 设备句柄
 * @return XY_INA_OK 成功，其他值失败
 */
int xy_ina_deinit(xy_ina_t *ina);

/**
 * @brief 读取测量数据
 * @param ina 设备句柄
 * @return XY_INA_OK 成功，其他值失败
 */
int xy_ina_read(xy_ina_t *ina);

/**
 * @brief 获取母线电压
 * @param ina 设备句柄
 * @param voltage_mv 电压指针 (mV)
 * @return XY_INA_OK 成功，其他值失败
 */
int xy_ina_get_voltage(xy_ina_t *ina, float *voltage_mv);

/**
 * @brief 获取电流
 * @param ina 设备句柄
 * @param current_ma 电流指针 (mA)
 * @return XY_INA_OK 成功，其他值失败
 */
int xy_ina_get_current(xy_ina_t *ina, float *current_ma);

/**
 * @brief 获取功率
 * @param ina 设备句柄
 * @param power_mw 功率指针 (mW)
 * @return XY_INA_OK 成功，其他值失败
 */
int xy_ina_get_power(xy_ina_t *ina, float *power_mw);

/**
 * @brief 获取分流电压
 * @param ina 设备句柄
 * @param voltage_uv 分流电压指针 (uV)
 * @return XY_INA_OK 成功，其他值失败
 */
int xy_ina_get_shunt_voltage(xy_ina_t *ina, float *voltage_uv);

/**
 * @brief 使能告警
 * @param ina 设备句柄
 * @param enable 是否使能
 * @return XY_INA_OK 成功，其他值失败
 */
int xy_ina_enable_alert(xy_ina_t *ina, bool enable);

#ifdef __cplusplus
}
#endif

#endif
