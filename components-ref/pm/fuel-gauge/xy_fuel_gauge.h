/**
 * @file xy_fuel_gauge.h
 * @brief Battery Fuel Gauge (State of Charge Estimation)
 * @version 1.0.0
 * @date 2026-02-28
 */

#ifndef XY_FUEL_GAUGE_H
#define XY_FUEL_GAUGE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Configuration ==================== */

#ifndef XY_FUEL_GAUGE_DEFAULT_CAPACITY_MAH
#define XY_FUEL_GAUGE_DEFAULT_CAPACITY_MAH 2000
#endif

/* ==================== Error Codes ==================== */

#define XY_FUEL_GAUGE_OK            0
#define XY_FUEL_GAUGE_ERROR         (-1)
#define XY_FUEL_GAUGE_INVALID_PARAM (-2)

/* ==================== Data Structures ==================== */

/**
 * @brief Fuel gauge configuration
 */
typedef struct {
    uint32_t design_capacity_mAh;
    uint32_t full_capacity_mAh;
    uint32_t empty_capacity_mAh;
    uint32_t nominal_voltage_mV;
    uint8_t cells;
} xy_fuel_gauge_config_t;

/**
 * @brief Battery state
 */
typedef struct {
    uint32_t voltage_mV;
    int32_t current_mA;
    int32_t temperature_celsius;
    uint8_t soc_percent;        /* State of Charge 0-100% */
    uint8_t soh_percent;        /* State of Health 0-100% */
    uint32_t remaining_mAh;
    uint32_t full_charge_mAh;
    uint32_t cycle_count;
    bool charging;
    bool full;
    bool empty;
} xy_battery_state_t;

/* ==================== Fuel Gauge Operations ==================== */

/**
 * @brief Initialize fuel gauge
 * @param config Configuration
 * @return XY_FUEL_GAUGE_OK on success
 */
int xy_fuel_gauge_init(const xy_fuel_gauge_config_t *config);

/**
 * @brief Deinitialize fuel gauge
 * @return XY_FUEL_GAUGE_OK on success
 */
int xy_fuel_gauge_deinit(void);

/**
 * @brief Update battery measurements
 * @param voltage_mV Battery voltage in mV
 * @param current_mA Battery current in mA (positive=charging, negative=discharging)
 * @param temperature_celsius Temperature in Celsius
 * @return XY_FUEL_GAUGE_OK on success
 */
int xy_fuel_gauge_update(uint32_t voltage_mV, int32_t current_mA, int32_t temperature_celsius);

/**
 * @brief Get battery state
 * @param state Pointer to battery state structure
 * @return XY_FUEL_GAUGE_OK on success
 */
int xy_fuel_gauge_get_state(xy_battery_state_t *state);

/**
 * @brief Get state of charge percentage
 * @return SOC percentage (0-100)
 */
uint8_t xy_fuel_gauge_get_soc(void);

/**
 * @brief Get state of health percentage
 * @return SOH percentage (0-100)
 */
uint8_t xy_fuel_gauge_get_soh(void);

/**
 * @brief Get remaining capacity in mAh
 * @return Remaining capacity in mAh
 */
uint32_t xy_fuel_gauge_get_remaining_mAh(void);

/**
 * @brief Get time to empty in minutes
 * @return Minutes until battery is empty (0 if charging)
 */
uint32_t xy_fuel_gauge_get_time_to_empty(void);

/**
 * @brief Get time to full in minutes
 * @return Minutes until battery is full (0 if discharging)
 */
uint32_t xy_fuel_gauge_get_time_to_full(void);

/**
 * @brief Reset fuel gauge (calibration)
 * @return XY_FUEL_GAUGE_OK on success
 */
int xy_fuel_gauge_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_FUEL_GAUGE_H */
