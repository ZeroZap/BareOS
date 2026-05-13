/**
 * @file xy_lte_state.h
 * @brief LTE Module State Machine Definition
 * @version 1.0.0
 * @date 2025-10-30
 */

#ifndef _XY_LTE_STATE_H_
#define _XY_LTE_STATE_H_

#include "../../xy_clib/xy_typedef.h"
#include "xy_lte_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LTE module state enumeration
 */
typedef enum {
    LTE_STATE_POWER_OFF = 0,      /**< Module powered off or not initialized */
    LTE_STATE_INITIALIZING,       /**< Module initializing */
    LTE_STATE_SIM_LOCKED,         /**< SIM requires PIN/PUK */
    LTE_STATE_READY,              /**< Module ready, SIM unlocked */
    LTE_STATE_SEARCHING,          /**< Searching for network */
    LTE_STATE_REGISTERED,         /**< Registered to network */
    LTE_STATE_ONLINE,             /**< Data connection active */
    LTE_STATE_ERROR,              /**< Error state, recovery needed */
    LTE_STATE_RESETTING           /**< Module resetting */
} lte_module_state_t;

/**
 * @brief State transition event enumeration
 */
typedef enum {
    LTE_EVENT_INIT_SUCCESS,       /**< Initialization successful */
    LTE_EVENT_INIT_FAILED,        /**< Initialization failed */
    LTE_EVENT_SIM_READY,          /**< SIM card ready */
    LTE_EVENT_SIM_LOCKED,         /**< SIM card locked */
    LTE_EVENT_SIM_ERROR,          /**< SIM card error */
    LTE_EVENT_REG_STARTED,        /**< Network registration started */
    LTE_EVENT_REG_SUCCESS,        /**< Network registration successful */
    LTE_EVENT_REG_FAILED,         /**< Network registration failed */
    LTE_EVENT_NETWORK_LOST,       /**< Network connection lost */
    LTE_EVENT_DATA_CONNECTED,     /**< Data connection established */
    LTE_EVENT_DATA_DISCONNECTED,  /**< Data connection lost */
    LTE_EVENT_RESET_REQUESTED,    /**< Reset requested */
    LTE_EVENT_RESET_COMPLETE,     /**< Reset completed */
    LTE_EVENT_ERROR               /**< Error occurred */
} lte_state_event_t;

/**
 * @brief State transition structure
 */
typedef struct {
    lte_module_state_t current_state; /**< Current state */
    lte_state_event_t event;          /**< Triggering event */
    lte_module_state_t next_state;    /**< Next state */
} lte_state_transition_t;

/**
 * @brief Check if state transition is valid
 * 
 * @param current Current module state
 * @param event Event that occurred
 * @param next Pointer to store next state
 * @return LTE_OK if transition is valid, error code otherwise
 */
lte_error_t lte_state_transition_is_valid(lte_module_state_t current,
                                          lte_state_event_t event,
                                          lte_module_state_t *next);

/**
 * @brief Get state name string
 * 
 * @param state Module state
 * @return State name string (static, do not free)
 */
const char* lte_state_name(lte_module_state_t state);

/**
 * @brief Get event name string
 * 
 * @param event State event
 * @return Event name string (static, do not free)
 */
const char* lte_event_name(lte_state_event_t event);

#ifdef __cplusplus
}
#endif

#endif /* _XY_LTE_STATE_H_ */
