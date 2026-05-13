/**
 * @file xy_charger.h
 * @brief Battery Charger Management
 * @version 1.0.0
 * @date 2026-02-28
 */

#ifndef XY_CHARGER_H
#define XY_CHARGER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Configuration ==================== */

#ifndef XY_CHARGER_MAX_CELLS
#define XY_CHARGER_MAX_CELLS 4
#endif

/* ==================== Error Codes ==================== */

#define XY_CHARGER_OK               0
#define XY_CHARGER_ERROR            (-1)
#define XY_CHARGER_INVALID_PARAM    (-2)
#define XY_CHARGER_OVER_VOLTAGE     (-3)
#define XY_CHARGER_OVER_CURRENT     (-4)
#define XY_CHARGER_OVER_TEMP        (-5)
#define XY_CHARGER_NOT_CHARGING     (-6)

/* ==================== Data Structures ==================== */

/**
 * @brief Charger status
 */
typedef enum {
    XY_CHARGER_STATUS_IDLE = 0,
    XY_CHARGER_STATUS_PRE_CHARGE,
    XY_CHARGER_STATUS_FAST_CHARGE,
    XY_CHARGER_STATUS_CONSTANT_VOLTAGE,
    XY_CHARGER_STATUS_CHARGE_COMPLETE,
    XY_CHARGER_STATUS_ERROR,
} xy_charger_status_t;

/**
 * @brief Charger configuration
 */
typedef struct {
    uint8_t cell_count;
    uint32_t charge_current_mA;
    uint32_t charge_voltage_mV;
    uint32_t pre_charge_current_mA;
    uint32_t termination_current_mA;
    int32_t temp_min_celsius;
    int32_t temp_max_celsius;
} xy_charger_config_t;

/**
 * @brief Charger state
 */
typedef struct {
    xy_charger_status_t status;
    uint32_t battery_voltage_mV;
    uint32_t charge_current_mA;
    int32_t temperature_celsius;
    uint32_t charge_time_seconds;
    uint8_t soc_percent;
    bool charging;
    bool error;
} xy_charger_state_t;

/* ==================== Charger Operations ==================== */

/**
 * @brief Initialize charger
 * @param config Charger configuration
 * @return XY_CHARGER_OK on success
 */
int xy_charger_init(const xy_charger_config_t *config);

/**
 * @brief Deinitialize charger
 * @return XY_CHARGER_OK on success
 */
int xy_charger_deinit(void);

/**
 * @brief Start charging
 * @return XY_CHARGER_OK on success
 */
int xy_charger_start(void);

/**
 * @brief Stop charging
 * @return XY_CHARGER_OK on success
 */
int xy_charger_stop(void);

/**
 * @brief Get charger state
 * @param state Pointer to state structure
 * @return XY_CHARGER_OK on success
 */
int xy_charger_get_state(xy_charger_state_t *state);

/**
 * @brief Set charge current
 * @param current_mA Charge current in mA
 * @return XY_CHARGER_OK on success
 */
int xy_charger_set_current(uint32_t current_mA);

/**
 * @brief Enable/disable charging
 * @param enable true to enable, false to disable
 * @return XY_CHARGER_OK on success
 */
int xy_charger_enable(bool enable);

/**
 * @brief Check if charger is active
 * @return true if charging
 */
bool xy_charger_is_charging(void);

/**
 * @brief Hardware enable/disable charger (platform-specific)
 * @param enable 1 to enable, 0 to disable
 * @return XY_CHARGER_OK on success
 */
int xy_charger_hw_enable(int enable);

/**
 * @brief Hardware disable charger
 * @return XY_CHARGER_OK on success
 */
int xy_charger_hw_disable(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_CHARGER_H */
