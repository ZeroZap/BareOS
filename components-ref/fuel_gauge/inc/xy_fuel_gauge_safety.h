/**
 * @file xy_fuel_gauge_safety.h
 * @brief Fuel Gauge Safety Status Interface
 * @version 1.0.0
 * @date 2026-03-05
 * 
 * 电量计 Safety 安全功能:
 * - 安全状态查询
 * - 警告状态查询
 * - 故障状态查询
 * - 安全事件记录
 */

#ifndef XY_FUEL_GAUGE_SAFETY_H
#define XY_FUEL_GAUGE_SAFETY_H

#include "xy_fuel_gauge.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 安全状态标志 ==================== */

/**
 * @brief 安全状态标志 (SAS - Safety Status)
 */
typedef enum {
    XY_FG_SAFETY_OK = 0x0000,           /* 安全状态正常 */
    XY_FG_SAFETY_CELL_OVP = 0x0001,     /* 单体过压 */
    XY_FG_SAFETY_CELL_UVP = 0x0002,     /* 单体欠压 */
    XY_FG_SAFETY_PACK_OVP = 0x0004,     /* 电池组过压 */
    XY_FG_SAFETY_PACK_UVP = 0x0008,     /* 电池组欠压 */
    XY_FG_SAFETY_CHG_OCP =  0x0010,     /* 充电过流 */
    XY_FG_SAFETY_DISCHG_OCP = 0x0020,   /* 放电过流 */
    XY_FG_SAFETY_CHG_OCD = 0x0040,      /* 充电短路 */
    XY_FG_SAFETY_DISCHG_OCD = 0x0080,   /* 放电短路 */
    XY_FG_SAFETY_CELL_OTP = 0x0100,     /* 单体过温 */
    XY_FG_SAFETY_CELL_UTP = 0x0200,     /* 单体低温 */
    XY_FG_SAFETY_PACK_OTC = 0x0400,     /* 充电过温 */
    XY_FG_SAFETY_PACK_OTD = 0x0800,     /* 放电过温 */
    XY_FG_SAFETY_PACK_UTP = 0x1000,     /* 电池组低温 */
    XY_FG_SAFETY_AFE_FAULT = 0x2000,    /* AFE 故障 */
    XY_FG_SAFETY_FUSE_FAULT = 0x4000,   /* 熔丝故障 */
} xy_fg_safety_status_t;

/**
 * @brief 警告状态标志 (SAS - Warning Status)
 */
typedef enum {
    XY_FG_WARNING_NONE = 0x0000,        /* 无警告 */
    XY_FG_WARNING_CELL_HIGH = 0x0001,   /* 单体电压高 */
    XY_FG_WARNING_CELL_LOW = 0x0002,    /* 单体电压低 */
    XY_FG_WARNING_PACK_HIGH = 0x0004,   /* 电池组电压高 */
    XY_FG_WARNING_PACK_LOW = 0x0008,    /* 电池组电压低 */
    XY_FG_WARNING_CHG_HIGH = 0x0010,    /* 充电电流高 */
    XY_FG_WARNING_DISCHG_HIGH = 0x0020, /* 放电电流高 */
    XY_FG_WARNING_TEMP_HIGH = 0x0040,   /* 温度高 */
    XY_FG_WARNING_TEMP_LOW = 0x0080,    /* 温度低 */
    XY_FG_WARNING_SOC_LOW = 0x0100,     /* SOC 低 */
    XY_FG_WARNING_SOC_HIGH = 0x0200,    /* SOC 高 */
    XY_FG_WARNING_SOH_LOW = 0x0400,     /* SOH 低 */
    XY_FG_WARNING_IMBAL_HIGH = 0x0800,  /* 电芯不平衡 */
} xy_fg_warning_status_t;

/**
 * @brief 故障状态标志 (FAS - Fault Status)
 */
typedef enum {
    XY_FG_FAULT_NONE = 0x0000,          /* 无故障 */
    XY_FG_FAULT_AFE_COMM = 0x0001,      /* AFE 通信故障 */
    XY_FG_FAULT_AFE_CRC = 0x0002,       /* AFE CRC 错误 */
    XY_FG_FAULT_TEMP_SENSOR = 0x0004,   /* 温度传感器故障 */
    XY_FG_FAULT_CELL_SENSOR = 0x0008,   /* 单体传感器故障 */
    XY_FG_FAULT_CURR_SENSOR = 0x0010,   /* 电流传感器故障 */
    XY_FG_FAULT_EEPROM = 0x0020,        /* EEPROM 故障 */
    XY_FG_FAULT_FUSE_OPEN = 0x0040,     /* 熔丝断开 */
    XY_FG_FAULT_FUSE_SHORT = 0x0080,    /* 熔丝短路 */
    XY_FG_FAULT_PRE_FET = 0x0100,       /* 预充电 FET 故障 */
    XY_FG_FAULT_CHG_FET = 0x0200,       /* 充电 FET 故障 */
    XY_FG_FAULT_DISCHG_FET = 0x0400,    /* 放电 FET 故障 */
    XY_FG_FAULT_BAL_FET = 0x0800,       /* 平衡 FET 故障 */
} xy_fg_fault_status_t;

/* ==================== 安全阈值 ==================== */

/**
 * @brief 安全阈值配置
 */
typedef struct {
    /* 电压阈值 (mV) */
    uint16_t cell_ovp_threshold;        /* 单体过压阈值 */
    uint16_t cell_uvp_threshold;        /* 单体欠压阈值 */
    uint16_t pack_ovp_threshold;        /* 电池组过压阈值 */
    uint16_t pack_uvp_threshold;        /* 电池组欠压阈值 */
    
    /* 电流阈值 (mA) */
    uint16_t chg_ocp_threshold;         /* 充电过流阈值 */
    uint16_t dischg_ocp_threshold;      /* 放电过流阈值 */
    uint16_t chg_ocd_threshold;         /* 充电短路阈值 */
    uint16_t dischg_ocd_threshold;      /* 放电短路阈值 */
    
    /* 温度阈值 (0.1°C) */
    int16_t cell_otp_threshold;         /* 单体过温阈值 */
    int16_t cell_utp_threshold;         /* 单体低温阈值 */
    int16_t pack_otc_threshold;         /* 充电过温阈值 */
    int16_t pack_otd_threshold;         /* 放电过温阈值 */
    int16_t pack_utp_threshold;         /* 电池组低温阈值 */
} xy_fg_safety_thresholds_t;

/* ==================== 安全事件记录 ==================== */

/**
 * @brief 安全事件类型
 */
typedef enum {
    XY_FG_SAFETY_EVENT_NONE = 0,
    XY_FG_SAFETY_EVENT_OVP,
    XY_FG_SAFETY_EVENT_UVP,
    XY_FG_SAFETY_EVENT_OCP,
    XY_FG_SAFETY_EVENT_OCD,
    XY_FG_SAFETY_EVENT_OTP,
    XY_FG_SAFETY_EVENT_UTP,
    XY_FG_SAFETY_EVENT_FAULT,
} xy_fg_safety_event_type_t;

/**
 * @brief 安全事件记录
 */
typedef struct {
    xy_fg_safety_event_type_t type;     /* 事件类型 */
    uint32_t timestamp;                 /* 时间戳 */
    uint16_t voltage_mv;                /* 事件时电压 */
    int16_t current_ma;                 /* 事件时电流 */
    int16_t temperature;                /* 事件时温度 */
    uint8_t soc;                        /* 事件时 SOC */
} xy_fg_safety_event_t;

/* ==================== Safety API ==================== */

/**
 * @brief 获取安全状态
 * @param fg 电量计设备
 * @return 安全状态标志
 */
xy_fg_safety_status_t xy_fuel_gauge_get_safety_status(xy_fuel_gauge_t *fg);

/**
 * @brief 获取警告状态
 * @param fg 电量计设备
 * @return 警告状态标志
 */
xy_fg_warning_status_t xy_fuel_gauge_get_warning_status(xy_fuel_gauge_t *fg);

/**
 * @brief 获取故障状态
 * @param fg 电量计设备
 * @return 故障状态标志
 */
xy_fg_fault_status_t xy_fuel_gauge_get_fault_status(xy_fuel_gauge_t *fg);

/**
 * @brief 配置安全阈值
 * @param fg 电量计设备
 * @param thresholds 阈值配置
 * @return 状态码
 */
int xy_fuel_gauge_config_safety_thresholds(xy_fuel_gauge_t *fg,
                                           const xy_fg_safety_thresholds_t *thresholds);

/**
 * @brief 获取安全阈值
 * @param fg 电量计设备
 * @param thresholds 阈值配置
 * @return 状态码
 */
int xy_fuel_gauge_get_safety_thresholds(xy_fuel_gauge_t *fg,
                                        xy_fg_safety_thresholds_t *thresholds);

/**
 * @brief 获取最新安全事件
 * @param fg 电量计设备
 * @param event 事件记录
 * @return 状态码
 */
int xy_fuel_gauge_get_safety_event(xy_fuel_gauge_t *fg,
                                   xy_fg_safety_event_t *event);

/**
 * @brief 获取安全事件历史
 * @param fg 电量计设备
 * @param events 事件数组
 * @param max_events 最大事件数
 * @return 实际事件数
 */
int xy_fuel_gauge_get_safety_event_history(xy_fuel_gauge_t *fg,
                                           xy_fg_safety_event_t *events,
                                           uint8_t max_events);

/**
 * @brief 清除安全事件历史
 * @param fg 电量计设备
 * @return 状态码
 */
int xy_fuel_gauge_clear_safety_events(xy_fuel_gauge_t *fg);

/**
 * @brief 检查是否安全
 * @param fg 电量计设备
 * @return true=安全，false=不安全
 */
bool xy_fuel_gauge_is_safe(xy_fuel_gauge_t *fg);

/**
 * @brief 检查是否有警告
 * @param fg 电量计设备
 * @return true=有警告，false=无警告
 */
bool xy_fuel_gauge_has_warning(xy_fuel_gauge_t *fg);

/**
 * @brief 检查是否有故障
 * @param fg 电量计设备
 * @return true=有故障，false=无故障
 */
bool xy_fuel_gauge_has_fault(xy_fuel_gauge_t *fg);

/**
 * @brief 获取安全状态字符串
 * @param status 安全状态
 * @return 状态字符串
 */
const char* xy_fuel_gauge_safety_status_to_string(xy_fg_safety_status_t status);

/**
 * @brief 获取警告状态字符串
 * @param status 警告状态
 * @return 状态字符串
 */
const char* xy_fuel_gauge_warning_status_to_string(xy_fg_warning_status_t status);

/**
 * @brief 获取故障状态字符串
 * @param status 故障状态
 * @return 状态字符串
 */
const char* xy_fuel_gauge_fault_status_to_string(xy_fg_fault_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* XY_FUEL_GAUGE_SAFETY_H */
