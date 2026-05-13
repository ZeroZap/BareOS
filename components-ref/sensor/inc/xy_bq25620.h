/**
 * @file xy_bq25620.h
 * @brief BQ25620 Li-Ion Battery Charger Driver
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#ifndef XY_BQ25620_H
#define XY_BQ25620_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_dev_i2c.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief BQ25620 I2C 地址
 */
#define BQ25620_ADDR            0x6B

/**
 * @brief BQ25620 寄存器地址
 */
#define BQ25620_REG_CHG_STAT    0x00  /* 充电状态寄存器 */
#define BQ25620_REG_FAULT       0x01  /* 故障寄存器 */
#define BQ25620_REG_CHG_CTRL0   0x02  /* 充电控制 0 */
#define BQ25620_REG_CHG_CTRL1   0x03  /* 充电控制 1 */
#define BQ25620_REG_CHG_CTRL2   0x04  /* 充电控制 2 */
#define BQ25620_REG_CHG_CTRL3   0x05  /* 充电控制 3 */
#define BQ25620_REG_CHG_CTRL4   0x06  /* 充电控制 4 */
#define BQ25620_REG_CHG_CTRL5   0x07  /* 充电控制 5 */
#define BQ25620_REG_CHG_CTRL6   0x08  /* 充电控制 6 */
#define BQ25620_REG_CHG_CTRL7   0x09  /* 充电控制 7 */
#define BQ25620_REG_CHG_CTRL8   0x0A  /* 充电控制 8 */
#define BQ25620_REG_PART_ID     0x0B  /* 型号 ID */
#define BQ25620_REG_DEV_ID      0x0C  /* 设备 ID */

/**
 * @brief BQ25620 ID 值
 */
#define BQ25620_PART_ID_VALUE   0x16
#define BQ25620_DEV_ID_VALUE    0x02

/**
 * @brief 充电状态
 */
typedef enum {
    XY_BQ_CHG_STATE_IDLE = 0,       /* 空闲 */
    XY_BQ_CHG_STATE_PRECHARGE,      /* 预充电 */
    XY_BQ_CHG_STATE_FAST_CHARGE,    /* 快充 */
    XY_BQ_CHG_STATE_CHARGE_DONE,    /* 充满 */
} xy_bq_chg_state_t;

/**
 * @brief 故障类型
 */
typedef enum {
    XY_BQ_FAULT_NONE = 0,           /* 无故障 */
    XY_BQ_FAULT_INPUT_OVP,          /* 输入过压 */
    XY_BQ_FAULT_THERMAL,            /* 过热 */
    XY_BQ_FAULT_CHG_TIMEOUT,        /* 充电超时 */
    XY_BQ_FAULT_BAT_OVP,            /* 电池过压 */
} xy_bq_fault_t;

/**
 * @brief 错误码
 */
#define XY_BQ_OK                0
#define XY_BQ_ERROR             (-1)
#define XY_BQ_INVALID_PARAM     (-2)
#define XY_BQ_NOT_FOUND         (-3)
#define XY_BQ_FAULT           (-4)

/**
 * @brief 充电配置
 */
typedef struct {
    uint16_t vbat_reg_mv;         /* 电池调节电压 (mV), 默认 4200mV */
    uint16_t ichg_ma;             /* 充电电流 (mA), 默认 1000mA */
    uint16_t iprecharge_ma;       /* 预充电电流 (mA), 默认 100mA */
    uint16_t iterm_ma;            /* 终止电流 (mA), 默认 100mA */
    uint16_t vindpm_mv;           /* VINDPM 电压 (mV), 默认 4500mV */
    uint16_t ivlim_ma;            /* 输入电流限制 (mA), 默认 1500mA */
    uint8_t  recharge_mv;         /* 再充电阈值 (mV), 默认 100mV */
    bool     enable_auto_recharge;/* 使能自动再充电 */
} xy_bq25620_config_t;

/**
 * @brief 充电状态数据
 */
typedef struct {
    xy_bq_chg_state_t chg_state;  /* 充电状态 */
    xy_bq_fault_t fault;          /* 故障类型 */
    bool vbus_present;            /* VBUS 存在 */
    bool chg_enabled;             /* 充电使能 */
    bool dpm_active;              /* DPM 激活 */
    bool thermal_reg_active;      /* 热调节激活 */
    uint16_t vbat_mv;             /* 电池电压 (mV) */
    uint16_t sys_mv;              /* 系统电压 (mV) */
    uint16_t ichg_ma;             /* 充电电流 (mA) */
    int16_t temperature;          /* 温度 (0.1°C) */
    uint32_t timestamp;           /* 时间戳 (ms) */
} xy_bq25620_data_t;

/**
 * @brief BQ25620 句柄
 */
typedef struct {
    xy_i2c_device_t i2c_dev;      /* I2C 设备 */
    xy_bq25620_config_t config;   /* 配置 */
    xy_bq25620_data_t data;       /* 状态数据 */
    bool initialized;             /* 初始化标志 */
} xy_bq25620_t;

/**
 * @brief 初始化 BQ25620
 * @param bq BQ25620 句柄
 * @param i2c_handle I2C 句柄
 * @param config 充电配置
 * @return XY_BQ_OK 成功，其他值失败
 */
int xy_bq25620_init(xy_bq25620_t *bq, void *i2c_handle, const xy_bq25620_config_t *config);

/**
 * @brief 反初始化
 * @param bq BQ25620 句柄
 * @return XY_BQ_OK 成功，其他值失败
 */
int xy_bq25620_deinit(xy_bq25620_t *bq);

/**
 * @brief 读取充电状态
 * @param bq BQ25620 句柄
 * @return XY_BQ_OK 成功，其他值失败
 */
int xy_bq25620_read(xy_bq25620_t *bq);

/**
 * @brief 获取充电状态
 * @param bq BQ25620 句柄
 * @param state 充电状态指针
 * @return XY_BQ_OK 成功，其他值失败
 */
int xy_bq25620_get_chg_state(xy_bq25620_t *bq, xy_bq_chg_state_t *state);

/**
 * @brief 获取故障信息
 * @param bq BQ25620 句柄
 * @param fault 故障指针
 * @return XY_BQ_OK 成功，其他值失败
 */
int xy_bq25620_get_fault(xy_bq25620_t *bq, xy_bq_fault_t *fault);

/**
 * @brief 检查 VBUS 是否存在
 * @param bq BQ25620 句柄
 * @return true: VBUS 存在，false: VBUS 不存在
 */
bool xy_bq25620_is_vbus_present(xy_bq25620_t *bq);

/**
 * @brief 使能/禁用充电
 * @param bq BQ25620 句柄
 * @param enable true: 使能，false: 禁用
 * @return XY_BQ_OK 成功，其他值失败
 */
int xy_bq25620_enable_charge(xy_bq25620_t *bq, bool enable);

/**
 * @brief 设置充电电流
 * @param bq BQ25620 句柄
 * @param current_ma 充电电流 (mA)
 * @return XY_BQ_OK 成功，其他值失败
 */
int xy_bq25620_set_charge_current(xy_bq25620_t *bq, uint16_t current_ma);

/**
 * @brief 设置电池电压
 * @param bq BQ25620 句柄
 * @param voltage_mv 电池电压 (mV)
 * @return XY_BQ_OK 成功，其他值失败
 */
int xy_bq25620_set_battery_voltage(xy_bq25620_t *bq, uint16_t voltage_mv);

/**
 * @brief 启动充电
 * @param bq BQ25620 句柄
 * @return XY_BQ_OK 成功，其他值失败
 */
int xy_bq25620_start_charging(xy_bq25620_t *bq);

/**
 * @brief 停止充电
 * @param bq BQ25620 句柄
 * @return XY_BQ_OK 成功，其他值失败
 */
int xy_bq25620_stop_charging(xy_bq25620_t *bq);

/**
 * @brief 复位故障
 * @param bq BQ25620 句柄
 * @return XY_BQ_OK 成功，其他值失败
 */
int xy_bq25620_reset_fault(xy_bq25620_t *bq);

#ifdef __cplusplus
}
#endif

#endif
