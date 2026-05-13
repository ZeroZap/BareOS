/**
 * @file xy_fuel_gauge_status.c
 * @brief Fuel Gauge Status Query Implementation
 * @version 1.0.0
 * @date 2026-03-05
 */

#include "xy_fuel_gauge_status.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* 状态映射表 */
static const char *const chg_state_strings[] = {
    "Idle",
    "Pre-charge",
    "Fast-charge",
    "Constant-voltage",
    "Full",
    "Error"
};

/**
 * @brief 获取充电状态
 */
xy_fg_charging_state_t xy_fuel_gauge_get_charging_state(xy_fuel_gauge_t *fg)
{
    if (!fg) {
        return XY_FG_CHG_STATE_ERROR;
    }
    
    int32_t current, soc;
    
    /* 读取电流和 SOC */
    if (xy_fuel_gauge_get(fg, XY_FG_DATA_CURRENT, &current) != 0) {
        return XY_FG_CHG_STATE_ERROR;
    }
    
    if (xy_fuel_gauge_get(fg, XY_FG_DATA_SOC, &soc) != 0) {
        return XY_FG_CHG_STATE_ERROR;
    }
    
    /* 判断充电状态 */
    if (current > 100) {  /* 充电电流 > 100mA */
        if (soc >= 100) {
            return XY_FG_CHG_STATE_FULL;
        } else if (soc < 20) {
            return XY_FG_CHG_STATE_FAST_CHARGE;
        } else {
            return XY_FG_CHG_STATE_CONSTANT_VOLT;
        }
    } else if (current < -100) {  /* 放电电流 > 100mA */
        return XY_FG_CHG_STATE_IDLE;
    } else {
        return XY_FG_CHG_STATE_IDLE;
    }
}

/**
 * @brief 获取充电模式
 */
xy_fg_charging_mode_t xy_fuel_gauge_get_charging_mode(xy_fuel_gauge_t *fg)
{
    /* 简化实现：默认返回无充电 */
    (void)fg;
    return XY_FG_CHG_MODE_NONE;
}

/**
 * @brief 获取充电电流设定值
 */
int xy_fuel_gauge_get_charge_current(xy_fuel_gauge_t *fg, uint16_t *current_ma)
{
    if (!fg || !current_ma) {
        return -1;
    }
    
    /* 简化实现：返回 0 */
    *current_ma = 0;
    return 0;
}

/**
 * @brief 获取充电电压设定值
 */
int xy_fuel_gauge_get_charge_voltage(xy_fuel_gauge_t *fg, uint16_t *voltage_mv)
{
    if (!fg || !voltage_mv) {
        return -1;
    }
    
    /* 简化实现：返回 0 */
    *voltage_mv = 0;
    return 0;
}

/**
 * @brief 获取电池健康状态
 */
int xy_fuel_gauge_get_battery_health(xy_fuel_gauge_t *fg, 
                                     xy_fg_battery_health_t *health)
{
    if (!fg || !health) {
        return -1;
    }
    
    memset(health, 0, sizeof(*health));
    
    /* 读取 SOC */
    xy_fuel_gauge_get(fg, XY_FG_DATA_SOC, (int32_t*)&health->soh_percent);
    
    /* 读取容量 */
    xy_fuel_gauge_get(fg, XY_FG_DATA_FULL_CAPACITY, (int32_t*)&health->full_charge_capacity);
    xy_fuel_gauge_get(fg, XY_FG_DATA_REMAIN_CAPACITY, (int32_t*)&health->remaining_capacity);
    
    /* 读取循环次数 */
    xy_fuel_gauge_get(fg, XY_FG_DATA_CYCLE_COUNT, (int32_t*)&health->cycle_count);
    
    /* 读取温度 */
    xy_fuel_gauge_get(fg, XY_FG_DATA_TEMPERATURE, (int32_t*)&health->temperature);
    health->temperature /= 10;  /* 转换为°C */
    
    /* 估算 SOH (简化实现) */
    health->soh_percent = 100;
    
    return 0;
}

/**
 * @brief 获取保护状态
 */
xy_fg_protection_t xy_fuel_gauge_get_protection_status(xy_fuel_gauge_t *fg)
{
    /* 简化实现：返回无保护 */
    (void)fg;
    return XY_FG_PROT_NONE;
}

/**
 * @brief 获取错误状态
 */
xy_fg_error_t xy_fuel_gauge_get_error_status(xy_fuel_gauge_t *fg)
{
    /* 简化实现：返回无错误 */
    (void)fg;
    return XY_FG_ERROR_NONE;
}

/**
 * @brief 检查是否正在充电
 */
bool xy_fuel_gauge_is_charging(xy_fuel_gauge_t *fg)
{
    xy_fg_charging_state_t state = xy_fuel_gauge_get_charging_state(fg);
    return (state == XY_FG_CHG_STATE_PRECHARGE ||
            state == XY_FG_CHG_STATE_FAST_CHARGE ||
            state == XY_FG_CHG_STATE_CONSTANT_VOLT);
}

/**
 * @brief 检查是否充满
 */
bool xy_fuel_gauge_is_full(xy_fuel_gauge_t *fg)
{
    xy_fg_charging_state_t state = xy_fuel_gauge_get_charging_state(fg);
    return (state == XY_FG_CHG_STATE_FULL);
}

/**
 * @brief 检查是否有保护激活
 */
bool xy_fuel_gauge_is_protected(xy_fuel_gauge_t *fg)
{
    xy_fg_protection_t prot = xy_fuel_gauge_get_protection_status(fg);
    return (prot != XY_FG_PROT_NONE);
}

/**
 * @brief 检查是否有错误
 */
bool xy_fuel_gauge_has_error(xy_fuel_gauge_t *fg)
{
    xy_fg_error_t error = xy_fuel_gauge_get_error_status(fg);
    return (error != XY_FG_ERROR_NONE);
}

/**
 * @brief 清除错误状态
 */
int xy_fuel_gauge_clear_error(xy_fuel_gauge_t *fg)
{
    /* 简化实现：直接返回成功 */
    (void)fg;
    return 0;
}

/**
 * @brief 获取状态字符串
 */
const char* xy_fuel_gauge_state_to_string(xy_fg_charging_state_t state)
{
    if (state >= 0 && state < sizeof(chg_state_strings)/sizeof(chg_state_strings[0])) {
        return chg_state_strings[state];
    }
    return "Unknown";
}
