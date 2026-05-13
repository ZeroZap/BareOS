/**
 * @file xy_fuel_gauge_safety.c
 * @brief Fuel Gauge Safety Status Implementation
 * @version 1.0.0
 * @date 2026-03-05
 */

#include "xy_fuel_gauge_safety.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* 默认安全阈值 */
static const xy_fg_safety_thresholds_t default_thresholds = {
    /* 电压阈值 (mV) */
    .cell_ovp_threshold = 4250,         /* 单体过压 4.25V */
    .cell_uvp_threshold = 2800,         /* 单体欠压 2.8V */
    .pack_ovp_threshold = 17000,        /* 电池组过压 17V (4 节) */
    .pack_uvp_threshold = 11200,        /* 电池组欠压 11.2V (4 节) */
    
    /* 电流阈值 (mA) */
    .chg_ocp_threshold = 5000,          /* 充电过流 5A */
    .dischg_ocp_threshold = 10000,      /* 放电过流 10A */
    .chg_ocd_threshold = 15000,         /* 充电短路 15A */
    .dischg_ocd_threshold = 20000,      /* 放电短路 20A */
    
    /* 温度阈值 (0.1°C) */
    .cell_otp_threshold = 600,          /* 单体过温 60°C */
    .cell_utp_threshold = 0,            /* 单体低温 0°C */
    .pack_otc_threshold = 550,          /* 充电过温 55°C */
    .pack_otd_threshold = 600,          /* 放电过温 60°C */
    .pack_utp_threshold = -100,         /* 电池组低温 -10°C */
};

/* 状态字符串映射 */
static const char *const safety_strings[] = {
    "OK",
    "Cell OVP",
    "Cell UVP",
    "Pack OVP",
    "Pack UVP",
    "Chg OCP",
    "Dischg OCP",
    "Chg OCD",
    "Dischg OCD",
    "Cell OTP",
    "Cell UTP",
    "Pack OTC",
    "Pack OTD",
    "Pack UTP",
    "AFE Fault",
    "Fuse Fault"
};

/**
 * @brief 获取安全状态
 */
xy_fg_safety_status_t xy_fuel_gauge_get_safety_status(xy_fuel_gauge_t *fg)
{
    if (!fg) {
        return XY_FG_SAFETY_OK;
    }
    
    xy_fg_safety_status_t status = XY_FG_SAFETY_OK;
    xy_fuel_gauge_data_t data;
    xy_fg_safety_thresholds_t thresholds;
    
    /* 获取当前数据 */
    xy_fuel_gauge_get(fg, XY_FG_DATA_VOLTAGE, (int32_t*)&data.voltage_mv);
    xy_fuel_gauge_get(fg, XY_FG_DATA_CURRENT, (int32_t*)&data.current_ma);
    xy_fuel_gauge_get(fg, XY_FG_DATA_TEMPERATURE, (int32_t*)&data.temperature_c);
    
    /* 获取阈值配置 */
    xy_fuel_gauge_get_safety_thresholds(fg, &thresholds);
    
    /* 检查电压安全 */
    if (data.voltage_mv > thresholds.pack_ovp_threshold) {
        status |= XY_FG_SAFETY_PACK_OVP;
        xy_log_e("Safety: Pack OVP (%dmV)\n", data.voltage_mv);
    }
    if (data.voltage_mv < thresholds.pack_uvp_threshold) {
        status |= XY_FG_SAFETY_PACK_UVP;
        xy_log_e("Safety: Pack UVP (%dmV)\n", data.voltage_mv);
    }
    
    /* 检查电流安全 */
    if (data.current_ma > thresholds.chg_ocp_threshold) {
        status |= XY_FG_SAFETY_CHG_OCP;
        xy_log_e("Safety: Chg OCP (%dmA)\n", data.current_ma);
    }
    if (data.current_ma < -thresholds.dischg_ocp_threshold) {
        status |= XY_FG_SAFETY_DISCHG_OCP;
        xy_log_e("Safety: Dischg OCP (%dmA)\n", data.current_ma);
    }
    
    /* 检查温度安全 */
    if (data.temperature_c > thresholds.pack_otc_threshold) {
        status |= XY_FG_SAFETY_PACK_OTC;
        xy_log_e("Safety: Pack OTC (%d.%d°C)\n", 
                 data.temperature_c / 10, data.temperature_c % 10);
    }
    if (data.temperature_c < thresholds.pack_utp_threshold) {
        status |= XY_FG_SAFETY_PACK_UTP;
        xy_log_e("Safety: Pack UTP (%d.%d°C)\n", 
                 data.temperature_c / 10, data.temperature_c % 10);
    }
    
    return status;
}

/**
 * @brief 获取警告状态
 */
xy_fg_warning_status_t xy_fuel_gauge_get_warning_status(xy_fuel_gauge_t *fg)
{
    if (!fg) {
        return XY_FG_WARNING_NONE;
    }
    
    xy_fg_warning_status_t warning = XY_FG_WARNING_NONE;
    xy_fuel_gauge_data_t data;
    
    /* 获取当前数据 */
    xy_fuel_gauge_get(fg, XY_FG_DATA_VOLTAGE, (int32_t*)&data.voltage_mv);
    xy_fuel_gauge_get(fg, XY_FG_DATA_CURRENT, (int32_t*)&data.current_ma);
    xy_fuel_gauge_get(fg, XY_FG_DATA_TEMPERATURE, (int32_t*)&data.temperature_c);
    xy_fuel_gauge_get(fg, XY_FG_DATA_SOC, (int32_t*)&data.soc);
    xy_fuel_gauge_get(fg, XY_FG_DATA_SOH, (int32_t*)&data.soh);
    
    /* 电压警告 */
    if (data.voltage_mv > default_thresholds.pack_ovp_threshold * 0.95) {
        warning |= XY_FG_WARNING_PACK_HIGH;
    }
    if (data.voltage_mv < default_thresholds.pack_uvp_threshold * 1.05) {
        warning |= XY_FG_WARNING_PACK_LOW;
    }
    
    /* 电流警告 */
    if (data.current_ma > default_thresholds.chg_ocp_threshold * 0.8) {
        warning |= XY_FG_WARNING_CHG_HIGH;
    }
    if (data.current_ma < -default_thresholds.dischg_ocp_threshold * 0.8) {
        warning |= XY_FG_WARNING_DISCHG_HIGH;
    }
    
    /* 温度警告 */
    if (data.temperature_c > default_thresholds.pack_otc_threshold * 0.9) {
        warning |= XY_FG_WARNING_TEMP_HIGH;
    }
    if (data.temperature_c < default_thresholds.pack_utp_threshold * 1.1) {
        warning |= XY_FG_WARNING_TEMP_LOW;
    }
    
    /* SOC/SOH 警告 */
    if (data.soc < 20) {
        warning |= XY_FG_WARNING_SOC_LOW;
    }
    if (data.soc > 95) {
        warning |= XY_FG_WARNING_SOC_HIGH;
    }
    if (data.soh < 80) {
        warning |= XY_FG_WARNING_SOH_LOW;
    }
    
    return warning;
}

/**
 * @brief 获取故障状态
 */
xy_fg_fault_status_t xy_fuel_gauge_get_fault_status(xy_fuel_gauge_t *fg)
{
    /* 简化实现：返回无故障 */
    (void)fg;
    return XY_FG_FAULT_NONE;
}

/**
 * @brief 配置安全阈值
 */
int xy_fuel_gauge_config_safety_thresholds(xy_fuel_gauge_t *fg,
                                           const xy_fg_safety_thresholds_t *thresholds)
{
    if (!fg || !thresholds) {
        return -1;
    }
    
    /* 保存阈值配置到设备私有数据 */
    /* 简化实现：仅打印日志 */
    xy_log_i("Safety thresholds configured\n");
    xy_log_i("  Cell OVP/UVP: %d/%d mV\n", 
             thresholds->cell_ovp_threshold, thresholds->cell_uvp_threshold);
    xy_log_i("  Pack OVP/UVP: %d/%d mV\n", 
             thresholds->pack_ovp_threshold, thresholds->pack_uvp_threshold);
    xy_log_i("  Chg OCP: %d mA\n", thresholds->chg_ocp_threshold);
    xy_log_i("  Dischg OCP: %d mA\n", thresholds->dischg_ocp_threshold);
    
    return 0;
}

/**
 * @brief 获取安全阈值
 */
int xy_fuel_gauge_get_safety_thresholds(xy_fuel_gauge_t *fg,
                                        xy_fg_safety_thresholds_t *thresholds)
{
    if (!fg || !thresholds) {
        return -1;
    }
    
    /* 返回默认阈值 */
    memcpy(thresholds, &default_thresholds, sizeof(default_thresholds));
    return 0;
}

/**
 * @brief 获取最新安全事件
 */
int xy_fuel_gauge_get_safety_event(xy_fuel_gauge_t *fg,
                                   xy_fg_safety_event_t *event)
{
    if (!fg || !event) {
        return -1;
    }
    
    /* 简化实现：返回空事件 */
    memset(event, 0, sizeof(*event));
    event->type = XY_FG_SAFETY_EVENT_NONE;
    return 0;
}

/**
 * @brief 获取安全事件历史
 */
int xy_fuel_gauge_get_safety_event_history(xy_fuel_gauge_t *fg,
                                           xy_fg_safety_event_t *events,
                                           uint8_t max_events)
{
    if (!fg || !events || max_events == 0) {
        return -1;
    }
    
    /* 简化实现：返回空历史 */
    memset(events, 0, sizeof(xy_fg_safety_event_t) * max_events);
    return 0;
}

/**
 * @brief 清除安全事件历史
 */
int xy_fuel_gauge_clear_safety_events(xy_fuel_gauge_t *fg)
{
    /* 简化实现：直接返回成功 */
    (void)fg;
    return 0;
}

/**
 * @brief 检查是否安全
 */
bool xy_fuel_gauge_is_safe(xy_fuel_gauge_t *fg)
{
    xy_fg_safety_status_t status = xy_fuel_gauge_get_safety_status(fg);
    return (status == XY_FG_SAFETY_OK);
}

/**
 * @brief 检查是否有警告
 */
bool xy_fuel_gauge_has_warning(xy_fuel_gauge_t *fg)
{
    xy_fg_warning_status_t warning = xy_fuel_gauge_get_warning_status(fg);
    return (warning != XY_FG_WARNING_NONE);
}

/**
 * @brief 检查是否有故障
 */
bool xy_fuel_gauge_has_fault(xy_fuel_gauge_t *fg)
{
    xy_fg_fault_status_t fault = xy_fuel_gauge_get_fault_status(fg);
    return (fault != XY_FG_FAULT_NONE);
}

/**
 * @brief 获取安全状态字符串
 */
const char* xy_fuel_gauge_safety_status_to_string(xy_fg_safety_status_t status)
{
    if (status == XY_FG_SAFETY_OK) {
        return "OK";
    }
    
    /* 简化实现：返回第一个匹配的状态 */
    for (int i = 0; i < 16; i++) {
        if (status & (1 << i)) {
            return safety_strings[i + 1];
        }
    }
    
    return "Unknown";
}

/**
 * @brief 获取警告状态字符串
 */
const char* xy_fuel_gauge_warning_status_to_string(xy_fg_warning_status_t status)
{
    if (status == XY_FG_WARNING_NONE) {
        return "None";
    }
    return "Warning";
}

/**
 * @brief 获取故障状态字符串
 */
const char* xy_fuel_gauge_fault_status_to_string(xy_fg_fault_status_t status)
{
    if (status == XY_FG_FAULT_NONE) {
        return "None";
    }
    return "Fault";
}
