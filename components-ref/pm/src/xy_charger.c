#include "xy_charger.h"
/**
 * @file xy_charger.c
 * @brief Battery Charger Implementation
 * @version 1.0.0
 * @date 2026-03-13
 */

#include "../inc/xy_pm.h"
#include <string.h>

/* Stub logging macros */
#define xy_log_i(fmt, ...)    do { } while(0)
#define xy_log_w(fmt, ...)     do { } while(0)
#define xy_log_d(fmt, ...)     do { } while(0)

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_INFO

/* 充电状态机 */
typedef struct {
    xy_charger_config_t config;
    xy_charger_state_t state;
    bool initialized;
    bool enabled;
    uint32_t charge_start_time;
} xy_charger_ctrl_t;

static xy_charger_ctrl_t s_charger;

/* 充电阶段定义 */
#define PRE_CHARGE_VOLTAGE_MV     3000   /* 3.0V 以下涓流充电 */
#define FAST_CHARGE_VOLTAGE_MV    4200   /* 4.2V 恒流充电 */
#define CHARGE_COMPLETE_VOLTAGE_MV 4200  /* 4.2V 充满 */
#define TERMINATION_CURRENT_MV    100    /* 100mA 终止电流 */

int xy_charger_init(const xy_charger_config_t *config)
{
    if (!config) return XY_CHARGER_INVALID_PARAM;

    memset(&s_charger, 0, sizeof(s_charger));
    memcpy(&s_charger.config, config, sizeof(xy_charger_config_t));

    /* 默认参数 */
    if (s_charger.config.cell_count == 0) {
        s_charger.config.cell_count = 1;
    }
    if (s_charger.config.charge_current_mA == 0) {
        s_charger.config.charge_current_mA = 1000; /* 默认 1A */
    }
    if (s_charger.config.charge_voltage_mV == 0) {
        s_charger.config.charge_voltage_mV = 4200 * s_charger.config.cell_count;
    }

    s_charger.initialized = true;
    s_charger.enabled = false;

    xy_log_i("Charger initialized: cells=%d, current=%dmA, voltage=%dmV\n",
             s_charger.config.cell_count,
             s_charger.config.charge_current_mA,
             s_charger.config.charge_voltage_mV);

    return XY_CHARGER_OK;
}

int xy_charger_deinit(void)
{
    memset(&s_charger, 0, sizeof(s_charger));
    xy_log_i("Charger deinitialized\n");
    return XY_CHARGER_OK;
}

int xy_charger_start(void)
{
    if (!s_charger.initialized) return XY_CHARGER_NOT_CHARGING;

    s_charger.enabled = true;
    s_charger.charge_start_time = xy_os_tick_get();
    s_charger.state.status = XY_CHARGER_STATUS_PRE_CHARGE;
    s_charger.state.charging = true;

    xy_log_i("Charging started\n");
    return XY_CHARGER_OK;
}

int xy_charger_stop(void)
{
    if (!s_charger.initialized) return XY_CHARGER_NOT_CHARGING;

    s_charger.enabled = false;
    s_charger.state.status = XY_CHARGER_STATUS_IDLE;
    s_charger.state.charging = false;

    xy_log_i("Charging stopped\n");
    return XY_CHARGER_OK;
}

/* 充电状态机更新 */
static void update_charge_state(void)
{
    uint32_t voltage = xy_pm_get_battery_voltage_mV();
    uint8_t soc = xy_pm_get_battery_percent();

    s_charger.state.battery_voltage_mV = voltage;
    s_charger.state.soc_percent = soc;

    /* 状态机转换 */
    switch (s_charger.state.status) {
        case XY_CHARGER_STATUS_PRE_CHARGE:
            /* 电压低于 3V，涓流充电 */
            s_charger.state.charge_current_mA = s_charger.config.pre_charge_current_mA;
            if (voltage >= PRE_CHARGE_VOLTAGE_MV) {
                s_charger.state.status = XY_CHARGER_STATUS_FAST_CHARGE;
                xy_log_i("Charger: Pre-charge -> Fast charge\n");
            }
            break;

        case XY_CHARGER_STATUS_FAST_CHARGE:
            /* 恒流充电 */
            s_charger.state.charge_current_mA = s_charger.config.charge_current_mA;
            if (voltage >= FAST_CHARGE_VOLTAGE_MV) {
                s_charger.state.status = XY_CHARGER_STATUS_CONSTANT_VOLTAGE;
                xy_log_i("Charger: Fast charge -> Constant voltage\n");
            }
            break;

        case XY_CHARGER_STATUS_CONSTANT_VOLTAGE:
            /* 恒压充电，电流逐渐减小 */
            if (soc >= 95) {
                s_charger.state.charge_current_mA = s_charger.config.termination_current_mA;
            } else {
                s_charger.state.charge_current_mA = s_charger.config.charge_current_mA / 2;
            }

            /* 充电完成判断 */
            if (soc >= 100 || s_charger.state.charge_current_mA <= TERMINATION_CURRENT_MV) {
                s_charger.state.status = XY_CHARGER_STATUS_CHARGE_COMPLETE;
                xy_log_i("Charger: Charge complete\n");
            }
            break;

        case XY_CHARGER_STATUS_CHARGE_COMPLETE:
            s_charger.state.charge_current_mA = 0;
            break;

        default:
            break;
    }
}

int xy_charger_get_state(xy_charger_state_t *state)
{
    if (!state) return XY_CHARGER_INVALID_PARAM;

    if (!s_charger.initialized) {
        memset(state, 0, sizeof(xy_charger_state_t));
        return XY_CHARGER_NOT_CHARGING;
    }

    update_charge_state();
    memcpy(state, &s_charger.state, sizeof(xy_charger_state_t));

    return XY_CHARGER_OK;
}

int xy_charger_set_current(uint32_t current_mA)
{
    if (!s_charger.initialized) return XY_CHARGER_NOT_CHARGING;

    s_charger.config.charge_current_mA = current_mA;
    xy_log_d("Charger current set to: %dmA\n", current_mA);
    return XY_CHARGER_OK;
}

int xy_charger_enable(bool enable)
{
    if (!s_charger.initialized) return XY_CHARGER_NOT_CHARGING;

    s_charger.enabled = enable;

    if (enable) {
        xy_charger_start();
    } else {
        xy_charger_stop();
    }

    return XY_CHARGER_OK;
}

bool xy_charger_is_charging(void)
{
    return s_charger.initialized && s_charger.enabled && s_charger.state.charging;
}

/* Platform-specific tick wrapper - uses real OS tick when available */
#if defined(STM32U5) || defined(STM32F4) || defined(STM32F1) || defined(STM32L4)
#define XY_PLATFORM_STM32     1
#else
#define XY_PLATFORM_STM32     0
#endif

#if defined(MCU_CH32) || defined(CH32V103) || defined(CH32V20X)
#define XY_PLATFORM_WCH       1
#else
#define XY_PLATFORM_WCH       0
#endif

/* Use platform tick if available */
#if XY_PLATFORM_STM32
extern uint32_t HAL_GetTick(void);
#define xy_os_tick_get()  HAL_GetTick()
#elif XY_PLATFORM_WCH
static volatile uint32_t g_chg_tick = 0;
#define xy_os_tick_get()  (g_chg_tick += 1000)
#else
extern uint32_t xy_pm_tick_get(void);
#define xy_os_tick_get()  xy_pm_tick_get()
#endif

/* Charger GPIO control - real implementation in xy_pm_platform.c */
#if XY_PLATFORM_STM32
#include "xy_hal_gpio.h"
#ifndef CHARGER_EN_PORT
#define CHARGER_EN_PORT   GPIOA
#endif
#ifndef CHARGER_EN_PIN
#define CHARGER_EN_PIN    0
#endif
#endif

int xy_charger_hw_enable(int enable)
{
#if XY_PLATFORM_STM32
    xy_hal_gpio_write((void*)CHARGER_EN_PORT, CHARGER_EN_PIN, enable ? 1 : 0);
    xy_log_d("STM32 Charger HW enable: %d\n", enable);
#elif XY_PLATFORM_WCH
    xy_log_d("CH32 Charger HW enable: %d\n", enable);
#else
    (void)enable;
    xy_log_d("Charger HW enable (simulated): %d\n", enable);
#endif
    return XY_CHARGER_OK;
}

int xy_charger_hw_disable(void)
{
    return xy_charger_hw_enable(false);
}
