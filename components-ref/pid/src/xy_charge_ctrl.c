/**
 * @file xy_charge_ctrl.c
 * @brief Smart Charging Control System
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "xy_log.h"
#include <stdint.h>
#include <stdbool.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_INFO

/* 充电状态机 */
typedef enum {
    CHARGE_STATE_IDLE = 0,
    CHARGE_STATE_PRE_CHARGE,      /* 涓流充电 */
    CHARGE_STATE_FAST_CHARGE,     /* 恒流充电 */
    CHARGE_STATE_CONSTANT_VOLTAGE,/* 恒压充电 */
    CHARGE_STATE_TOP_OFF,         /* 补电充电 */
    CHARGE_STATE_COMPLETE,        /* 充电完成 */
    CHARGE_STATE_ERROR,           /* 错误状态 */
} charge_state_t;

/* 充电控制块 */
typedef struct {
    bool initialized;
    bool enabled;
    charge_state_t state;
    
    /* 电池参数 */
    uint32_t battery_voltage_mV;
    int32_t battery_current_mA;
    uint8_t battery_soc;
    int32_t battery_temp_x100;
    
    /* 充电参数 */
    uint32_t charge_voltage_limit;  /* 充电电压限制 */
    uint32_t charge_current_limit;  /* 充电电流限制 */
    uint32_t pre_charge_current;    /* 涓流充电电流 */
    uint32_t fast_charge_current;   /* 快充当前 */
    uint32_t termination_current;   /* 终止电流 */
    
    /* 充电统计 */
    uint32_t charge_start_time;
    uint32_t charge_elapsed_ms;
    uint32_t charged_capacity_mAh;
    
    /* 保护参数 */
    uint32_t over_voltage_threshold;
    uint32_t over_current_threshold;
    int32_t over_temp_threshold;
    int32_t under_temp_threshold;
    
    /* 故障标志 */
    bool over_voltage;
    bool over_current;
    bool over_temp;
    bool under_temp;
} charge_ctrl_t;

static charge_ctrl_t s_charge;

/* 默认参数 */
#define PRE_CHARGE_VOLTAGE_MV    3000   /* 3.0V 以下涓流 */
#define FAST_CHARGE_VOLTAGE_MV   4200   /* 4.2V 恒压点 */
#define TERMINATION_CURRENT_MA   100    /* 100mA 终止 */

#define OVER_VOLTAGE_MV          4350   /* 过压保护 */
#define OVER_CURRENT_MA          2000   /* 过流保护 */
#define OVER_TEMP_C_X100         6000   /* 60°C 过温 */
#define UNDER_TEMP_C_X100        0      /* 0°C 低温 */

/* ==================== 状态机转换 ==================== */

static void update_charge_state(void)
{
    charge_state_t new_state = s_charge.state;
    
    switch (s_charge.state) {
        case CHARGE_STATE_IDLE:
            if (s_charge.enabled && s_charge.battery_voltage_mV > 0) {
                new_state = CHARGE_STATE_PRE_CHARGE;
                s_charge.charge_start_time = xy_os_tick_get();
            }
            break;
        
        case CHARGE_STATE_PRE_CHARGE:
            /* 涓流充电：V < 3.0V */
            if (s_charge.battery_voltage_mV >= PRE_CHARGE_VOLTAGE_MV) {
                new_state = CHARGE_STATE_FAST_CHARGE;
                xy_log_i("Charge: Pre-charge -> Fast charge (V=%dmV)\n",
                        s_charge.battery_voltage_mV);
            }
            break;
        
        case CHARGE_STATE_FAST_CHARGE:
            /* 恒流充电：V < 4.2V */
            if (s_charge.battery_voltage_mV >= FAST_CHARGE_VOLTAGE_MV) {
                new_state = CHARGE_STATE_CONSTANT_VOLTAGE;
                xy_log_i("Charge: Fast charge -> Constant voltage (V=%dmV)\n",
                        s_charge.battery_voltage_mV);
            }
            break;
        
        case CHARGE_STATE_CONSTANT_VOLTAGE:
            /* 恒压充电：电流逐渐减小 */
            if (s_charge.battery_current_mA <= TERMINATION_CURRENT_MA ||
                s_charge.battery_soc >= 95) {
                new_state = CHARGE_STATE_TOP_OFF;
                xy_log_i("Charge: CV -> Top-off (I=%dmA, SOC=%d%%)\n",
                        s_charge.battery_current_mA, s_charge.battery_soc);
            }
            break;
        
        case CHARGE_STATE_TOP_OFF:
            /* 补电充电：SOC < 100% */
            if (s_charge.battery_soc >= 100) {
                new_state = CHARGE_STATE_COMPLETE;
                xy_log_i("Charge: Complete (SOC=100%%)\n");
            }
            break;
        
        case CHARGE_STATE_COMPLETE:
            /* 充电完成：自动停止 */
            if (!s_charge.enabled || s_charge.battery_soc < 95) {
                new_state = CHARGE_STATE_IDLE;
            }
            break;
        
        case CHARGE_STATE_ERROR:
            /* 错误状态：清除故障后恢复 */
            if (!s_charge.over_voltage && !s_charge.over_current &&
                !s_charge.over_temp && !s_charge.under_temp) {
                new_state = CHARGE_STATE_IDLE;
                xy_log_i("Charge: Error cleared\n");
            }
            break;
        
        default:
            break;
    }
    
    s_charge.state = new_state;
}

/* ==================== 保护检测 ==================== */

static void check_protections(void)
{
    /* 过压保护 */
    s_charge.over_voltage = (s_charge.battery_voltage_mV > s_charge.over_voltage_threshold);
    
    /* 过流保护 */
    s_charge.over_current = (s_charge.battery_current_mA > s_charge.over_current_threshold);
    
    /* 过温保护 */
    s_charge.over_temp = (s_charge.battery_temp_x100 > s_charge.over_temp_threshold);
    
    /* 低温保护 */
    s_charge.under_temp = (s_charge.battery_temp_x100 < s_charge.under_temp_threshold);
    
    /* 任何故障触发错误状态 */
    if (s_charge.over_voltage || s_charge.over_current ||
        s_charge.over_temp || s_charge.under_temp) {
        if (s_charge.state != CHARGE_STATE_ERROR) {
            xy_log_e("Charge: Protection triggered! OV=%d OC=%d OT=%d UT=%d\n",
                    s_charge.over_voltage, s_charge.over_current,
                    s_charge.over_temp, s_charge.under_temp);
            s_charge.state = CHARGE_STATE_ERROR;
        }
    }
}

/* ==================== 充电电流计算 ==================== */

static uint32_t calculate_charge_current(void)
{
    if (!s_charge.enabled || s_charge.state == CHARGE_STATE_ERROR) {
        return 0;
    }
    
    switch (s_charge.state) {
        case CHARGE_STATE_PRE_CHARGE:
            return s_charge.pre_charge_current;
        
        case CHARGE_STATE_FAST_CHARGE:
            return s_charge.fast_charge_current;
        
        case CHARGE_STATE_CONSTANT_VOLTAGE:
            /* 恒压阶段：电流逐渐减小 */
            if (s_charge.battery_soc >= 90) {
                return s_charge.fast_charge_current / 4;
            } else if (s_charge.battery_soc >= 80) {
                return s_charge.fast_charge_current / 2;
            }
            return s_charge.fast_charge_current;
        
        case CHARGE_STATE_TOP_OFF:
            return s_charge.termination_current;
        
        default:
            return 0;
    }
}

/* ==================== 公开 API ==================== */

int xy_charge_ctrl_init(void)
{
    memset(&s_charge, 0, sizeof(s_charge));
    
    /* 默认充电参数 */
    s_charge.charge_voltage_limit = 4200;
    s_charge.charge_current_limit = 1000;
    s_charge.pre_charge_current = 100;
    s_charge.fast_charge_current = 1000;
    s_charge.termination_current = 100;
    
    /* 保护参数 */
    s_charge.over_voltage_threshold = OVER_VOLTAGE_MV;
    s_charge.over_current_threshold = OVER_CURRENT_MA;
    s_charge.over_temp_threshold = OVER_TEMP_C_X100;
    s_charge.under_temp_threshold = UNDER_TEMP_C_X100;
    
    s_charge.initialized = true;
    s_charge.enabled = false;
    s_charge.state = CHARGE_STATE_IDLE;
    
    xy_log_i("Charge Ctrl initialized: V_limit=%dmV, I_limit=%dmA\n",
             s_charge.charge_voltage_limit, s_charge.charge_current_limit);
    
    return 0;
}

int xy_charge_ctrl_update(uint32_t voltage_mV, int32_t current_mA,
                          uint8_t soc, int32_t temp_x100)
{
    if (!s_charge.initialized) return -1;
    
    /* 更新测量值 */
    s_charge.battery_voltage_mV = voltage_mV;
    s_charge.battery_current_mA = current_mA;
    s_charge.battery_soc = soc;
    s_charge.battery_temp_x100 = temp_x100;
    
    /* 更新充电时间 */
    if (s_charge.state != CHARGE_STATE_IDLE && 
        s_charge.state != CHARGE_STATE_COMPLETE) {
        s_charge.charge_elapsed_ms = xy_os_tick_get() - s_charge.charge_start_time;
    }
    
    /* 检查保护 */
    check_protections();
    
    /* 更新状态机 */
    if (s_charge.state != CHARGE_STATE_ERROR) {
        update_charge_state();
    }
    
    /* 计算充电电流 */
    uint32_t target_current = calculate_charge_current();
    
    xy_log_d("Charge Update: V=%dmV I=%dmA SOC=%d%% T=%d.%d°C state=%d I_target=%dmA\n",
             voltage_mV, current_mA, soc,
             temp_x100/100, temp_x100%100,
             s_charge.state, target_current);
    
    return (int)target_current;
}

int xy_charge_ctrl_start(void)
{
    if (!s_charge.initialized) return -1;
    
    s_charge.enabled = true;
    s_charge.state = CHARGE_STATE_PRE_CHARGE;
    s_charge.charge_start_time = xy_os_tick_get();
    
    xy_log_i("Charge started\n");
    return 0;
}

int xy_charge_ctrl_stop(void)
{
    if (!s_charge.initialized) return -1;
    
    s_charge.enabled = false;
    s_charge.state = CHARGE_STATE_IDLE;
    
    xy_log_i("Charge stopped\n");
    return 0;
}

uint8_t xy_charge_ctrl_get_state(void)
{
    return s_charge.initialized ? (uint8_t)s_charge.state : 0;
}

bool xy_charge_ctrl_is_charging(void)
{
    return s_charge.initialized && s_charge.enabled &&
           s_charge.state != CHARGE_STATE_IDLE &&
           s_charge.state != CHARGE_STATE_COMPLETE &&
           s_charge.state != CHARGE_STATE_ERROR;
}

bool xy_charge_ctrl_is_complete(void)
{
    return s_charge.initialized && s_charge.state == CHARGE_STATE_COMPLETE;
}

uint32_t xy_charge_ctrl_get_elapsed_time(void)
{
    return s_charge.initialized ? s_charge.charge_elapsed_ms / 1000 : 0;
}

int xy_charge_ctrl_set_current_limit(uint32_t current_mA)
{
    if (!s_charge.initialized) return -1;
    
    s_charge.charge_current_limit = current_mA;
    s_charge.fast_charge_current = current_mA;
    
    xy_log_d("Charge current limit set: %dmA\n", current_mA);
    return 0;
}

int xy_charge_ctrl_set_voltage_limit(uint32_t voltage_mV)
{
    if (!s_charge.initialized) return -1;
    
    s_charge.charge_voltage_limit = voltage_mV;
    
    xy_log_d("Charge voltage limit set: %dmV\n", voltage_mV);
    return 0;
}
