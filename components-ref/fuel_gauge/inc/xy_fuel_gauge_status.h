/**
 * @file xy_fuel_gauge_status.h
 * @brief Fuel Gauge Status Query Interface
 * @version 1.0.0
 * @date 2026-03-05
 * 
 * 电量计状态查询功能:
 * - 充电状态
 * - 健康状态
 * - 保护状态
 * - 错误状态
 */

#ifndef XY_FUEL_GAUGE_STATUS_H
#define XY_FUEL_GAUGE_STATUS_H

#include "xy_fuel_gauge.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 充电状态 ==================== */

/**
 * @brief 充电状态
 */
typedef enum {
    XY_FG_CHG_STATE_IDLE = 0,     /* 空闲 (未充电) */
    XY_FG_CHG_STATE_PRECHARGE,    /* 预充电 */
    XY_FG_CHG_STATE_FAST_CHARGE,  /* 快充 */
    XY_FG_CHG_STATE_CONSTANT_VOLT,/* 恒压充电 */
    XY_FG_CHG_STATE_FULL,         /* 充满 */
    XY_FG_CHG_STATE_ERROR,        /* 错误 */
} xy_fg_charging_state_t;

/**
 * @brief 充电模式
 */
typedef enum {
    XY_FG_CHG_MODE_NONE = 0,      /* 未充电 */
    XY_FG_CHG_MODE_USB,           /* USB 充电 */
    XY_FG_CHG_MODE_ADAPTER,       /* 适配器充电 */
    XY_FG_CHG_MODE_WIRELESS,      /* 无线充电 */
} xy_fg_charging_mode_t;

/* ==================== 电池健康状态 ==================== */

/**
 * @brief 电池健康状态
 */
typedef struct {
    uint8_t soh_percent;          /* 健康度百分比 (0-100) */
    uint8_t cycle_count;          /* 循环次数 */
    uint16_t design_capacity;     /* 设计容量 (mAh) */
    uint16_t full_charge_capacity;/* 满充容量 (mAh) */
    uint16_t remaining_capacity;  /* 剩余容量 (mAh) */
    uint8_t temperature;          /* 温度 (°C) */
    uint8_t age_days;             /* 使用天数 */
} xy_fg_battery_health_t;

/* ==================== 保护状态 ==================== */

/**
 * @brief 保护状态标志
 */
typedef enum {
    XY_FG_PROT_NONE = 0x0000,     /* 无保护 */
    XY_FG_PROT_OVER_VOLT = 0x0001,/* 过压保护 */
    XY_FG_PROT_UNDER_VOLT = 0x0002,/* 欠压保护 */
    XY_FG_PROT_OVER_CURR = 0x0004,/* 过流保护 */
    XY_FG_PROT_SHORT_CIRC = 0x0008,/* 短路保护 */
    XY_FG_PROT_OVER_TEMP = 0x0010,/* 过温保护 */
    XY_FG_PROT_UNDER_TEMP = 0x0020,/* 低温保护 */
    XY_FG_PROT_CELL_IMBAL = 0x0040,/* 电芯不平衡 */
} xy_fg_protection_t;

/* ==================== 错误状态 ==================== */

/**
 * @brief 错误状态标志
 */
typedef enum {
    XY_FG_ERROR_NONE = 0x0000,    /* 无错误 */
    XY_FG_ERROR_COMM = 0x0001,    /* 通信错误 */
    XY_FG_ERROR_AUTH = 0x0002,    /* 认证失败 */
    XY_FG_ERROR_SENSOR = 0x0004,  /* 传感器故障 */
    XY_FG_ERROR_CALIB = 0x0008,   /* 校准错误 */
    XY_FG_ERROR_EEPROM = 0x0010,  /* EEPROM 错误 */
    XY_FG_ERROR_FUSE = 0x0020,    /* 熔丝熔断 */
} xy_fg_error_t;

/* ==================== 状态查询 API ==================== */

/**
 * @brief 获取充电状态
 * @param fg 电量计设备
 * @return 充电状态
 */
xy_fg_charging_state_t xy_fuel_gauge_get_charging_state(xy_fuel_gauge_t *fg);

/**
 * @brief 获取充电模式
 * @param fg 电量计设备
 * @return 充电模式
 */
xy_fg_charging_mode_t xy_fuel_gauge_get_charging_mode(xy_fuel_gauge_t *fg);

/**
 * @brief 获取充电电流设定值
 * @param fg 电量计设备
 * @param current_ma 充电电流 (mA)
 * @return 状态码
 */
int xy_fuel_gauge_get_charge_current(xy_fuel_gauge_t *fg, uint16_t *current_ma);

/**
 * @brief 获取充电电压设定值
 * @param fg 电量计设备
 * @param voltage_mv 充电电压 (mV)
 * @return 状态码
 */
int xy_fuel_gauge_get_charge_voltage(xy_fuel_gauge_t *fg, uint16_t *voltage_mv);

/**
 * @brief 获取电池健康状态
 * @param fg 电量计设备
 * @param health 健康状态
 * @return 状态码
 */
int xy_fuel_gauge_get_battery_health(xy_fuel_gauge_t *fg, 
                                     xy_fg_battery_health_t *health);

/**
 * @brief 获取保护状态
 * @param fg 电量计设备
 * @return 保护状态标志
 */
xy_fg_protection_t xy_fuel_gauge_get_protection_status(xy_fuel_gauge_t *fg);

/**
 * @brief 获取错误状态
 * @param fg 电量计设备
 * @return 错误状态标志
 */
xy_fg_error_t xy_fuel_gauge_get_error_status(xy_fuel_gauge_t *fg);

/**
 * @brief 检查是否正在充电
 * @param fg 电量计设备
 * @return true=充电中，false=未充电
 */
bool xy_fuel_gauge_is_charging(xy_fuel_gauge_t *fg);

/**
 * @brief 检查是否充满
 * @param fg 电量计设备
 * @return true=充满，false=未充满
 */
bool xy_fuel_gauge_is_full(xy_fuel_gauge_t *fg);

/**
 * @brief 检查是否有保护激活
 * @param fg 电量计设备
 * @return true=有保护，false=正常
 */
bool xy_fuel_gauge_is_protected(xy_fuel_gauge_t *fg);

/**
 * @brief 检查是否有错误
 * @param fg 电量计设备
 * @return true=有错误，false=正常
 */
bool xy_fuel_gauge_has_error(xy_fuel_gauge_t *fg);

/**
 * @brief 清除错误状态
 * @param fg 电量计设备
 * @return 状态码
 */
int xy_fuel_gauge_clear_error(xy_fuel_gauge_t *fg);

/**
 * @brief 获取状态字符串
 * @param state 充电状态
 * @return 状态字符串
 */
const char* xy_fuel_gauge_state_to_string(xy_fg_charging_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* XY_FUEL_GAUGE_STATUS_H */
