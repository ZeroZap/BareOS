/**
 * @file xy_fuel_gauge.c
 * @brief Battery Fuel Gauge Implementation
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "../inc/xy_pm.h"
#include <string.h>

/* Stub for tick count - returns simulated milliseconds */
static uint32_t stub_tick_get(void)
{
    static uint32_t tick = 0;
    return tick += 1000;
}

#define xy_os_tick_get stub_tick_get

/* Stub logging macros */
#define xy_log_i(fmt, ...)
#define xy_log_w(fmt, ...)
#define xy_log_d(fmt, ...)

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_INFO

/* 库仑计实现 */
typedef struct {
    xy_fuel_gauge_config_t config;
    xy_battery_state_t state;
    int32_t accumulated_charge_mAs; /* 累积电荷量 (mA·s) */
    uint32_t last_update_time;
    bool initialized;
} xy_fuel_gauge_ctrl_t;

static xy_fuel_gauge_ctrl_t s_fg;

/* 锂电池参数 */
#define NOMINAL_VOLTAGE_MV  3700
#define FULL_VOLTAGE_MV     4200
#define EMPTY_VOLTAGE_MV    3000

int xy_fuel_gauge_init(const xy_fuel_gauge_config_t *config)
{
    if (!config) return XY_FUEL_GAUGE_INVALID_PARAM;
    
    memset(&s_fg, 0, sizeof(s_fg));
    memcpy(&s_fg.config, config, sizeof(xy_fuel_gauge_config_t));
    
    /* 默认参数 */
    if (s_fg.config.design_capacity_mAh == 0) {
        s_fg.config.design_capacity_mAh = XY_FUEL_GAUGE_DEFAULT_CAPACITY_MAH;
    }
    if (s_fg.config.full_capacity_mAh == 0) {
        s_fg.config.full_capacity_mAh = s_fg.config.design_capacity_mAh;
    }
    
    /* 初始状态 */
    s_fg.state.voltage_mV = NOMINAL_VOLTAGE_MV;
    s_fg.state.current_mA = 0;
    s_fg.state.temperature_celsius = 25;
    s_fg.state.soc_percent = 50;
    s_fg.state.soh_percent = 100;
    s_fg.state.remaining_mAh = s_fg.config.design_capacity_mAh / 2;
    s_fg.state.full_charge_mAh = s_fg.config.full_capacity_mAh;
    s_fg.state.charging = false;
    
    s_fg.last_update_time = xy_os_tick_get();
    s_fg.initialized = true;
    
    xy_log_i("Fuel Gauge initialized: capacity=%dmAh, voltage=%dmV\n",
             s_fg.config.design_capacity_mAh, NOMINAL_VOLTAGE_MV);
    
    return XY_FUEL_GAUGE_OK;
}

int xy_fuel_gauge_deinit(void)
{
    memset(&s_fg, 0, sizeof(s_fg));
    xy_log_i("Fuel Gauge deinitialized\n");
    return XY_FUEL_GAUGE_OK;
}

/* 电压-SOC 曲线 (简化版) */
static uint8_t voltage_to_soc(uint32_t voltage_mV)
{
    if (voltage_mV >= FULL_VOLTAGE_MV) return 100;
    if (voltage_mV >= 4150) return 95;
    if (voltage_mV >= 4100) return 90;
    if (voltage_mV >= 4050) return 85;
    if (voltage_mV >= 4000) return 80;
    if (voltage_mV >= 3950) return 75;
    if (voltage_mV >= 3900) return 70;
    if (voltage_mV >= 3850) return 65;
    if (voltage_mV >= 3800) return 60;
    if (voltage_mV >= 3750) return 50;
    if (voltage_mV >= 3700) return 40;
    if (voltage_mV >= 3650) return 30;
    if (voltage_mV >= 3600) return 20;
    if (voltage_mV >= 3550) return 15;
    if (voltage_mV >= 3500) return 10;
    if (voltage_mV >= 3450) return 5;
    return 0;
}

int xy_fuel_gauge_update(uint32_t voltage_mV, int32_t current_mA, int32_t temperature_celsius)
{
    if (!s_fg.initialized) return XY_FUEL_GAUGE_ERROR;
    
    uint32_t now = xy_os_tick_get();
    uint32_t delta_t = (now - s_fg.last_update_time) / 1000; /* 转换为秒 */
    s_fg.last_update_time = now;
    
    /* 更新测量值 */
    s_fg.state.voltage_mV = voltage_mV;
    s_fg.state.current_mA = current_mA;
    s_fg.state.temperature_celsius = temperature_celsius;
    s_fg.state.charging = (current_mA > 0);
    
    /* 库仑计积分 */
    if (delta_t > 0 && delta_t < 60) { /* 防止异常时间间隔 */
        s_fg.accumulated_charge_mAs += current_mA * delta_t;
    }
    
    /* 计算剩余容量 */
    int32_t remaining_mAs = (s_fg.config.design_capacity_mAh * 3600) / 1000 + s_fg.accumulated_charge_mAs;
    if (remaining_mAs < 0) remaining_mAs = 0;
    if (remaining_mAs > s_fg.config.full_capacity_mAh * 3600) {
        remaining_mAs = s_fg.config.full_capacity_mAh * 3600;
    }
    
    s_fg.state.remaining_mAh = (remaining_mAs * 1000) / 3600;
    
    /* 计算 SOC (库仑计 + 电压校正) */
    uint8_t soc_coulomb = (s_fg.state.remaining_mAh * 100) / s_fg.config.design_capacity_mAh;
    uint8_t soc_voltage = voltage_to_soc(voltage_mV);
    
    /* 加权平均：库仑计 70% + 电压 30% */
    s_fg.state.soc_percent = (soc_coulomb * 70 + soc_voltage * 30) / 100;
    
    /* 边界检查 */
    if (s_fg.state.soc_percent > 100) s_fg.state.soc_percent = 100;
    if (s_fg.state.soc_percent == 100) s_fg.state.full = true;
    else s_fg.state.full = false;
    
    if (s_fg.state.soc_percent == 0) s_fg.state.empty = true;
    else s_fg.state.empty = false;
    
    xy_log_d("FG Update: V=%dmV I=%dmA SOC=%d%% Remaining=%dmAh\n",
             voltage_mV, current_mA, s_fg.state.soc_percent, s_fg.state.remaining_mAh);
    
    return XY_FUEL_GAUGE_OK;
}

int xy_fuel_gauge_get_state(xy_battery_state_t *state)
{
    if (!state) return XY_FUEL_GAUGE_INVALID_PARAM;
    
    if (!s_fg.initialized) {
        memset(state, 0, sizeof(xy_battery_state_t));
        return XY_FUEL_GAUGE_ERROR;
    }
    
    /* 更新测量值 */
    uint32_t voltage = xy_pm_get_battery_voltage_mV();
    xy_fuel_gauge_update(voltage, 0, 25);
    
    memcpy(state, &s_fg.state, sizeof(xy_battery_state_t));
    return XY_FUEL_GAUGE_OK;
}

uint8_t xy_fuel_gauge_get_soc(void)
{
    if (!s_fg.initialized) return 0;
    return s_fg.state.soc_percent;
}

uint8_t xy_fuel_gauge_get_soh(void)
{
    if (!s_fg.initialized) return 100;
    
    /* 简化 SOH 计算：基于循环次数 */
    if (s_fg.state.cycle_count < 100) return 100;
    if (s_fg.state.cycle_count < 300) return 95;
    if (s_fg.state.cycle_count < 500) return 90;
    if (s_fg.state.cycle_count < 800) return 80;
    return 70;
}

uint32_t xy_fuel_gauge_get_remaining_mAh(void)
{
    return s_fg.initialized ? s_fg.state.remaining_mAh : 0;
}

uint32_t xy_fuel_gauge_get_time_to_empty(void)
{
    if (!s_fg.initialized || s_fg.state.charging) return 0;
    
    if (s_fg.state.current_mA >= 0) return 0;
    
    int32_t discharge_current = -s_fg.state.current_mA;
    if (discharge_current == 0) discharge_current = 100; /* 假设 100mA 放电 */
    
    return (s_fg.state.remaining_mAh * 60) / discharge_current;
}

uint32_t xy_fuel_gauge_get_time_to_full(void)
{
    if (!s_fg.initialized || !s_fg.state.charging) return 0;
    
    if (s_fg.state.current_mA <= 0) return 0;
    
    uint32_t remaining_capacity = s_fg.config.full_capacity_mAh - s_fg.state.remaining_mAh;
    return (remaining_capacity * 60) / s_fg.state.current_mA;
}

int xy_fuel_gauge_reset(void)
{
    if (!s_fg.initialized) return XY_FUEL_GAUGE_ERROR;
    
    s_fg.accumulated_charge_mAs = 0;
    s_fg.state.soc_percent = 100;
    s_fg.state.remaining_mAh = s_fg.config.full_capacity_mAh;
    s_fg.state.full = true;
    s_fg.state.cycle_count++;
    
    xy_log_i("Fuel Gauge reset: SOC=100%, cycle=%d\n", s_fg.state.cycle_count);
    return XY_FUEL_GAUGE_OK;
}
