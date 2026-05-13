/**
 * @file xy_pm.h
 * @brief Power Management
 */

#ifndef XY_PM_H
#define XY_PM_H

#include <stdint.h>
#include <stdbool.h>
#include "xy_charger.h"

/* Fuel Gauge Default */
#define XY_FUEL_GAUGE_DEFAULT_CAPACITY_MAH  2000

/* Fuel Gauge Status */
typedef enum {
    XY_FUEL_GAUGE_OK = 0,
    XY_FUEL_GAUGE_ERROR,
    XY_FUEL_GAUGE_NOT_INITIALIZED,
    XY_FUEL_GAUGE_INVALID_PARAM,
    XY_FUEL_GAUGE_NOT_SUPPORTED
} xy_fuel_gauge_status_t;

/* Battery State */
typedef struct {
    uint16_t voltage_mV;
    int16_t current_mA;
    int16_t temperature_celsius;
    uint8_t soc_percent;
    uint8_t soh_percent;
    uint16_t remaining_mAh;
    uint16_t full_charge_mAh;
    bool charging;
    bool full;
    bool empty;
    uint16_t cycle_count;
} xy_battery_state_t;

typedef enum {
    XY_PM_MODE_ACTIVE = 0,
    XY_PM_MODE_SLEEP,
    XY_PM_MODE_DEEP_SLEEP,
    XY_PM_MODE_SHUTDOWN
} xy_pm_mode_t;

typedef enum {
    XY_PM_STATE_UNINITIALIZED = 0,
    XY_PM_STATE_READY,
    XY_PM_STATE_LOW_POWER,
    XY_PM_STATE_SHUTDOWN
} xy_pm_state_t;

typedef enum {
    XY_PM_OK = 0,
    XY_PM_ERROR,
    XY_PM_ERROR_INVALID_MODE,
    XY_PM_ERROR_NOT_SUPPORTED,
    XY_PM_INVALID_PARAM,
    XY_PM_NOT_INITIALIZED
} xy_pm_status_t;

/* Default configuration */
#define XY_PM_DEFAULT_BATTERY_CAPACITY_MAH  2000
#define XY_PM_DEFAULT_CHARGE_CURRENT_MAH    500

/* Platform Identifiers */
#define XY_PLATFORM_ID_UNKNOWN   0
#define XY_PLATFORM_ID_STM32     1
#define XY_PLATFORM_ID_WCH       2
#define XY_PLATFORM_ID_HC32      3
#define XY_PLATFORM_ID_PC        4

/* PM System State Enum */
typedef enum {
    XY_PM_SYSTEM_STATE_INIT = 0,
    XY_PM_SYSTEM_STATE_IDLE,
    XY_PM_SYSTEM_STATE_ACTIVE,
    XY_PM_SYSTEM_STATE_SLEEP,
    XY_PM_SYSTEM_STATE_LOW_POWER,
    XY_PM_SYSTEM_STATE_SHUTDOWN
} xy_pm_system_state_t;

/* PM System State Structure */
typedef struct {
    xy_pm_system_state_t state;
    bool enabled;
    uint32_t system_voltage_mV;
    xy_battery_state_t battery_state;
    xy_charger_state_t charger_state;
    bool power_good;
} xy_pm_system_state_info_t;

/* Fuel Gauge Configuration */
typedef struct {
    uint16_t design_capacity_mAh;
    uint16_t full_capacity_mAh;
    uint16_t nominal_voltage_mV;
    uint8_t cells;
} xy_fuel_gauge_config_t;

int xy_pm_init(void);
int xy_pm_set_mode(xy_pm_mode_t mode);
uint32_t xy_pm_get_battery_voltage(void);
uint32_t xy_pm_get_battery_voltage_mV(void);
uint8_t xy_pm_get_battery_percent(void);
xy_pm_status_t xy_pm_set_low_power_mode(bool enable);
xy_pm_status_t xy_pm_shutdown(void);
int xy_pm_get_state(xy_pm_system_state_t *state);

/* Charger API */
int xy_charger_init(const xy_charger_config_t *config);
int xy_charger_start(void);
int xy_charger_stop(void);
bool xy_charger_is_charging(void);
int xy_charger_get_status(xy_charger_state_t *state);

/* Fuel Gauge API */
int xy_fuel_gauge_init(const xy_fuel_gauge_config_t *config);
int xy_fuel_gauge_deinit(void);
int xy_fuel_gauge_update(uint32_t voltage_mV, int32_t current_mA, int32_t temperature_celsius);
uint8_t xy_fuel_gauge_get_soc(void);
uint32_t xy_fuel_gauge_get_remaining_mAh(void);
uint32_t xy_fuel_gauge_get_time_to_empty(void);
uint32_t xy_fuel_gauge_get_time_to_full(void);
int xy_fuel_gauge_reset(void);

/* PM System API */
int xy_pm_start_charging(void);
int xy_pm_stop_charging(void);
bool xy_pm_is_charging(void);
uint8_t xy_pm_get_soc(void);

/* Platform-specific functions (implemented in xy_pm_platform.c) */
uint32_t xy_pm_tick_get(void);
const char* xy_pm_get_platform_name(void);
bool xy_pm_is_platform(int platform);
int xy_charger_hw_init(void);
int xy_charger_hw_enable(int enable);
int xy_charger_hw_disable(void);

#endif /* XY_PM_H */
