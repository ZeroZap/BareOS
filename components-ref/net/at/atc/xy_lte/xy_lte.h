/**
 * @file xy_lte.h
 * @brief XinYi LTE Module Communication API
 * @version 1.0.0
 * @date 2025-10-30
 * 
 * This file provides a comprehensive API for LTE module communication
 * based on 3GPP TS 27.007 AT command standards. Supports Cat-M1, Cat-NB1,
 * and standard LTE modules from multiple vendors (SIMCOM, Quectel, U-blox).
 * 
 * ## Features
 * - Network registration and management
 * - Signal quality monitoring
 * - SIM card management
 * - Operator selection
 * - Device information retrieval
 * - Event-driven URC handling
 * - Multi-vendor support
 * 
 * ## Usage Example
 * @code
 * // Initialize module
 * lte_config_t config = {
 *     .uart_port = 1,
 *     .baudrate = 115200,
 *     .apn = "internet",
 *     .auto_register = true
 * };
 * 
 * lte_handle_t handle = lte_module_init(&config);
 * 
 * // Register network callback
 * lte_network_register_callback(handle, on_network_status, NULL);
 * 
 * // Wait for registration
 * if (lte_network_wait_registered(handle, 180000) == LTE_OK) {
 *     lte_signal_quality_t signal;
 *     lte_signal_get_quality(handle, &signal);
 *     printf("RSSI: %d dBm, Bars: %d\n", signal.rssi, signal.bars);
 * }
 * @endcode
 */

#ifndef _XY_LTE_H_
#define _XY_LTE_H_

#include "xy_lte_types.h"
#include "xy_lte_error.h"
#include "../../xy_clib/xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Module Configuration ==================== */

/**
 * @brief LTE module handle (opaque pointer)
 */
typedef void* lte_handle_t;

/**
 * @brief Network status callback function type
 * 
 * @param status Current network registration status
 * @param user_data User-provided context data
 */
typedef void (*lte_network_callback_t)(const lte_network_reg_t *status, void *user_data);

/**
 * @brief Signal quality callback function type
 * 
 * @param quality Current signal quality
 * @param user_data User-provided context data
 */
typedef void (*lte_signal_callback_t)(const lte_signal_quality_t *quality, void *user_data);

/**
 * @brief SIM status callback function type
 * 
 * @param status Current SIM status
 * @param user_data User-provided context data
 */
typedef void (*lte_sim_callback_t)(lte_sim_status_t status, void *user_data);

/**
 * @brief Preferred RAT (Radio Access Technology) selection
 */
typedef enum {
    LTE_RAT_AUTO = 0,        /**< Automatic selection */
    LTE_RAT_GSM_ONLY = 1,    /**< GSM only */
    LTE_RAT_WCDMA_ONLY = 2,  /**< WCDMA only */
    LTE_RAT_LTE_ONLY = 3,    /**< LTE only */
    LTE_RAT_CAT_M1_ONLY = 4, /**< Cat-M1 only */
    LTE_RAT_CAT_NB1_ONLY = 5 /**< Cat-NB1 (NB-IoT) only */
} lte_rat_preference_t;

/**
 * @brief LTE module configuration structure
 */
typedef struct {
    xy_u8 uart_port;                /**< UART port number (1-based) */
    xy_u32 baudrate;                /**< UART baud rate (default: 115200) */
    char apn[64];                   /**< Access Point Name (optional) */
    char pin_code[9];               /**< SIM PIN code (optional, 4-8 digits) */
    xy_bool auto_register;          /**< Enable automatic network registration */
    lte_rat_preference_t preferred_rat; /**< Preferred radio access technology */
    xy_u32 network_search_timeout;  /**< Network search timeout (ms, default: 180000) */
    xy_u32 response_timeout;        /**< AT command response timeout (ms, default: 5000) */
    xy_u8 max_retry;                /**< Maximum retry count for operations (default: 3) */
} lte_config_t;

/* ==================== Module Initialization and Control ==================== */

/**
 * @brief Initialize LTE module
 * 
 * This function initializes the LTE module, performs basic AT command checks,
 * verifies SIM card status, and optionally starts automatic network registration.
 * 
 * @param config Module configuration parameters
 * @return Module handle on success, NULL on error
 * 
 * @note The handle must be destroyed with lte_module_deinit() when finished
 * @see lte_module_deinit
 */
lte_handle_t lte_module_init(const lte_config_t *config);

/**
 * @brief Deinitialize and cleanup LTE module
 * 
 * @param handle Module handle
 * @return LTE_OK on success, error code on failure
 */
lte_error_t lte_module_deinit(lte_handle_t handle);

/**
 * @brief Reset LTE module
 * 
 * Performs a soft reset of the LTE module and reinitializes communication.
 * 
 * @param handle Module handle
 * @return LTE_OK on success, error code on failure
 */
lte_error_t lte_module_reset(lte_handle_t handle);

/**
 * @brief Get module capabilities
 * 
 * @param handle Module handle
 * @param caps Pointer to capabilities structure to fill
 * @return LTE_OK on success, error code on failure
 */
lte_error_t lte_module_get_capabilities(lte_handle_t handle, lte_module_caps_t *caps);

/**
 * @brief Get device information
 * 
 * Retrieves IMEI, manufacturer, model, and firmware version.
 * 
 * @param handle Module handle
 * @param info Pointer to device information structure to fill
 * @return LTE_OK on success, error code on failure
 */
lte_error_t lte_module_get_device_info(lte_handle_t handle, lte_device_info_t *info);

/* ==================== Network Management API ==================== */

/**
 * @brief Initiate network registration
 * 
 * Starts the network registration process. Use lte_network_register_callback()
 * to receive status updates, or lte_network_wait_registered() to block until complete.
 * 
 * @param handle Module handle
 * @return LTE_OK on success, error code on failure
 */
lte_error_t lte_network_register(lte_handle_t handle);

/**
 * @brief Deregister from network
 * 
 * Detaches from the current network.
 * 
 * @param handle Module handle
 * @return LTE_OK on success, error code on failure
 */
lte_error_t lte_network_deregister(lte_handle_t handle);

/**
 * @brief Get current network registration status
 * 
 * @param handle Module handle
 * @param status Pointer to status structure to fill
 * @return LTE_OK on success, error code on failure
 */
lte_error_t lte_network_get_status(lte_handle_t handle, lte_network_reg_t *status);

/**
 * @brief Wait for network registration (blocking)
 * 
 * Blocks until network registration is complete or timeout occurs.
 * 
 * @param handle Module handle
 * @param timeout_ms Timeout in milliseconds (0 = use configured timeout)
 * @return LTE_OK when registered, LTE_ERROR_TIMEOUT or other error code
 */
lte_error_t lte_network_wait_registered(lte_handle_t handle, xy_u32 timeout_ms);

/**
 * @brief Set preferred RAT (Radio Access Technology)
 * 
 * @param handle Module handle
 * @param rat Preferred RAT
 * @return LTE_OK on success, error code on failure
 * 
 * @note Module may need to be reset for changes to take effect
 */
lte_error_t lte_network_set_rat(lte_handle_t handle, lte_rat_preference_t rat);

/**
 * @brief Register network status change callback
 * 
 * @param handle Module handle
 * @param callback Callback function (NULL to unregister)
 * @param user_data User data passed to callback
 * @return LTE_OK on success, error code on failure
 */
lte_error_t lte_network_register_callback(lte_handle_t handle, 
                                          lte_network_callback_t callback,
                                          void *user_data);

/* ==================== Signal Quality API ==================== */

/**
 * @brief Get current signal quality
 * 
 * Queries the module for current signal quality metrics (RSSI, RSRP, RSRQ, SINR).
 * 
 * @param handle Module handle
 * @param quality Pointer to signal quality structure to fill
 * @return LTE_OK on success, error code on failure
 */
lte_error_t lte_signal_get_quality(lte_handle_t handle, lte_signal_quality_t *quality);

/**
 * @brief Start periodic signal quality monitoring
 * 
 * Starts a periodic timer that queries signal quality and invokes the callback.
 * 
 * @param handle Module handle
 * @param callback Callback function for signal updates
 * @param interval_ms Monitoring interval in milliseconds (minimum: 1000)
 * @param user_data User data passed to callback
 * @return LTE_OK on success, error code on failure
 */
lte_error_t lte_signal_start_monitor(lte_handle_t handle,
                                     lte_signal_callback_t callback,
                                     xy_u32 interval_ms,
                                     void *user_data);

/**
 * @brief Stop signal quality monitoring
 * 
 * @param handle Module handle
 * @return LTE_OK on success, error code on failure
 */
lte_error_t lte_signal_stop_monitor(lte_handle_t handle);

/**
 * @brief Calculate signal strength bars (0-5)
 * 
 * Converts RSSI/RSRP to a 0-5 bar indicator suitable for UI display.
 * 
 * @param quality Signal quality structure
 * @return Signal bars (0-5)
 */
xy_u8 lte_signal_get_bars(const lte_signal_quality_t *quality);

/* ==================== SIM Management API ==================== */

/**
 * @brief Get SIM card status
 * 
 * @param handle Module handle
 * @param info Pointer to SIM information structure to fill
 * @return LTE_OK on success, error code on failure
 */
lte_error_t lte_sim_get_status(lte_handle_t handle, lte_sim_info_t *info);

/**
 * @brief Enter SIM PIN code
 * 
 * @param handle Module handle
 * @param pin_code PIN code (4-8 digits)
 * @return LTE_OK on success, error code on failure (e.g., LTE_ERROR_SIM_WRONG_PIN)
 * 
 * @warning Three consecutive wrong PIN attempts will lock the SIM (PUK required)
 */
lte_error_t lte_sim_enter_pin(lte_handle_t handle, const char *pin_code);

/**
 * @brief Change SIM PIN code
 * 
 * @param handle Module handle
 * @param old_pin Current PIN code
 * @param new_pin New PIN code (4-8 digits)
 * @return LTE_OK on success, error code on failure
 */
lte_error_t lte_sim_change_pin(lte_handle_t handle, const char *old_pin, const char *new_pin);

/**
 * @brief Enable SIM PIN check
 * 
 * @param handle Module handle
 * @param pin_code PIN code for verification
 * @return LTE_OK on success, error code on failure
 */
lte_error_t lte_sim_enable_pin(lte_handle_t handle, const char *pin_code);

/**
 * @brief Disable SIM PIN check
 * 
 * @param handle Module handle
 * @param pin_code PIN code for verification
 * @return LTE_OK on success, error code on failure
 */
lte_error_t lte_sim_disable_pin(lte_handle_t handle, const char *pin_code);

/**
 * @brief Unlock SIM with PUK code
 * 
 * @param handle Module handle
 * @param puk_code PUK code (8 digits)
 * @param new_pin New PIN code to set (4-8 digits)
 * @return LTE_OK on success, error code on failure
 * 
 * @warning Ten consecutive wrong PUK attempts will permanently block the SIM
 */
lte_error_t lte_sim_unlock_puk(lte_handle_t handle, const char *puk_code, const char *new_pin);

/**
 * @brief Get IMSI (International Mobile Subscriber Identity)
 * 
 * @param handle Module handle
 * @param imsi Buffer for IMSI string (minimum 16 bytes)
 * @param size Buffer size
 * @return LTE_OK on success, error code on failure
 */
lte_error_t lte_sim_get_imsi(lte_handle_t handle, char *imsi, xy_size_t size);

/**
 * @brief Get ICCID (Integrated Circuit Card Identifier)
 * 
 * @param handle Module handle
 * @param iccid Buffer for ICCID string (minimum 21 bytes)
 * @param size Buffer size
 * @return LTE_OK on success, error code on failure
 */
lte_error_t lte_sim_get_iccid(lte_handle_t handle, char *iccid, xy_size_t size);

/**
 * @brief Register SIM status change callback
 * 
 * @param handle Module handle
 * @param callback Callback function (NULL to unregister)
 * @param user_data User data passed to callback
 * @return LTE_OK on success, error code on failure
 */
lte_error_t lte_sim_register_callback(lte_handle_t handle,
                                      lte_sim_callback_t callback,
                                      void *user_data);

/* ==================== Operator Selection API ==================== */

/**
 * @brief Scan for available network operators
 * 
 * This is a blocking operation that can take up to 180 seconds.
 * 
 * @param handle Module handle
 * @param operators Array to store operator information
 * @param max_count Maximum number of operators to retrieve
 * @param found_count Pointer to store actual number of operators found
 * @return LTE_OK on success, error code on failure
 * 
 * @note This operation may take 30-180 seconds depending on network conditions
 */
lte_error_t lte_operator_scan(lte_handle_t handle,
                              lte_operator_info_t *operators,
                              xy_u8 max_count,
                              xy_u8 *found_count);

/**
 * @brief Get current operator information
 * 
 * @param handle Module handle
 * @param operator Pointer to operator info structure to fill
 * @return LTE_OK on success, error code on failure
 */
lte_error_t lte_operator_get_current(lte_handle_t handle, lte_operator_info_t *operator);

/**
 * @brief Enable automatic operator selection
 * 
 * Allows the module to automatically select the best available operator.
 * 
 * @param handle Module handle
 * @return LTE_OK on success, error code on failure
 */
lte_error_t lte_operator_select_auto(lte_handle_t handle);

/**
 * @brief Manually select a network operator
 * 
 * @param handle Module handle
 * @param operator_numeric MCC+MNC code (e.g., "46000" for China Mobile)
 * @param act Preferred access technology
 * @return LTE_OK on success, error code on failure
 * 
 * @note Network registration may take several seconds after this call
 */
lte_error_t lte_operator_select_manual(lte_handle_t handle,
                                       const char *operator_numeric,
                                       lte_access_tech_t act);

/* ==================== Module Processing ==================== */

/**
 * @brief Process LTE module events
 * 
 * This function must be called periodically (e.g., in main loop or dedicated thread)
 * to process AT responses and URCs. Recommended call frequency: 10-100ms.
 * 
 * @param handle Module handle
 * 
 * @note In RTOS environments, this can run in a dedicated thread
 */
void lte_module_process(lte_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* _XY_LTE_H_ */
