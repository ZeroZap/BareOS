/**
 * @file xy_lte_state.c
 * @brief LTE Module State Machine Implementation
 * @version 1.0.0
 * @date 2025-10-30
 */

#include "xy_lte_state.h"
#include "../../xy_clib/xy_string.h"

/**
 * @brief State transition table
 * 
 * Defines all valid state transitions based on events.
 * Format: {current_state, event, next_state}
 */
static const lte_state_transition_t g_state_transitions[] = {
    /* From POWER_OFF */
    {LTE_STATE_POWER_OFF, LTE_EVENT_INIT_SUCCESS, LTE_STATE_READY},
    {LTE_STATE_POWER_OFF, LTE_EVENT_INIT_FAILED, LTE_STATE_ERROR},
    
    /* From INITIALIZING */
    {LTE_STATE_INITIALIZING, LTE_EVENT_SIM_READY, LTE_STATE_READY},
    {LTE_STATE_INITIALIZING, LTE_EVENT_SIM_LOCKED, LTE_STATE_SIM_LOCKED},
    {LTE_STATE_INITIALIZING, LTE_EVENT_SIM_ERROR, LTE_STATE_ERROR},
    {LTE_STATE_INITIALIZING, LTE_EVENT_INIT_FAILED, LTE_STATE_ERROR},
    
    /* From SIM_LOCKED */
    {LTE_STATE_SIM_LOCKED, LTE_EVENT_SIM_READY, LTE_STATE_READY},
    {LTE_STATE_SIM_LOCKED, LTE_EVENT_SIM_ERROR, LTE_STATE_ERROR},
    {LTE_STATE_SIM_LOCKED, LTE_EVENT_RESET_REQUESTED, LTE_STATE_RESETTING},
    
    /* From READY */
    {LTE_STATE_READY, LTE_EVENT_REG_STARTED, LTE_STATE_SEARCHING},
    {LTE_STATE_READY, LTE_EVENT_SIM_LOCKED, LTE_STATE_SIM_LOCKED},
    {LTE_STATE_READY, LTE_EVENT_SIM_ERROR, LTE_STATE_ERROR},
    {LTE_STATE_READY, LTE_EVENT_RESET_REQUESTED, LTE_STATE_RESETTING},
    {LTE_STATE_READY, LTE_EVENT_ERROR, LTE_STATE_ERROR},
    
    /* From SEARCHING */
    {LTE_STATE_SEARCHING, LTE_EVENT_REG_SUCCESS, LTE_STATE_REGISTERED},
    {LTE_STATE_SEARCHING, LTE_EVENT_REG_FAILED, LTE_STATE_READY},
    {LTE_STATE_SEARCHING, LTE_EVENT_SIM_ERROR, LTE_STATE_ERROR},
    {LTE_STATE_SEARCHING, LTE_EVENT_RESET_REQUESTED, LTE_STATE_RESETTING},
    {LTE_STATE_SEARCHING, LTE_EVENT_ERROR, LTE_STATE_ERROR},
    
    /* From REGISTERED */
    {LTE_STATE_REGISTERED, LTE_EVENT_DATA_CONNECTED, LTE_STATE_ONLINE},
    {LTE_STATE_REGISTERED, LTE_EVENT_NETWORK_LOST, LTE_STATE_SEARCHING},
    {LTE_STATE_REGISTERED, LTE_EVENT_SIM_ERROR, LTE_STATE_ERROR},
    {LTE_STATE_REGISTERED, LTE_EVENT_RESET_REQUESTED, LTE_STATE_RESETTING},
    {LTE_STATE_REGISTERED, LTE_EVENT_ERROR, LTE_STATE_ERROR},
    
    /* From ONLINE */
    {LTE_STATE_ONLINE, LTE_EVENT_DATA_DISCONNECTED, LTE_STATE_REGISTERED},
    {LTE_STATE_ONLINE, LTE_EVENT_NETWORK_LOST, LTE_STATE_SEARCHING},
    {LTE_STATE_ONLINE, LTE_EVENT_SIM_ERROR, LTE_STATE_ERROR},
    {LTE_STATE_ONLINE, LTE_EVENT_RESET_REQUESTED, LTE_STATE_RESETTING},
    {LTE_STATE_ONLINE, LTE_EVENT_ERROR, LTE_STATE_ERROR},
    
    /* From ERROR */
    {LTE_STATE_ERROR, LTE_EVENT_RESET_REQUESTED, LTE_STATE_RESETTING},
    {LTE_STATE_ERROR, LTE_EVENT_INIT_SUCCESS, LTE_STATE_READY},
    
    /* From RESETTING */
    {LTE_STATE_RESETTING, LTE_EVENT_RESET_COMPLETE, LTE_STATE_INITIALIZING},
    {LTE_STATE_RESETTING, LTE_EVENT_INIT_FAILED, LTE_STATE_ERROR}
};

#define STATE_TRANSITION_COUNT (sizeof(g_state_transitions) / sizeof(lte_state_transition_t))

/**
 * @brief Check if state transition is valid
 */
lte_error_t lte_state_transition_is_valid(lte_module_state_t current,
                                          lte_state_event_t event,
                                          lte_module_state_t *next)
{
    if (next == NULL) {
        return LTE_ERROR_INVALID_PARAM;
    }
    
    /* Search for matching transition */
    for (xy_u32 i = 0; i < STATE_TRANSITION_COUNT; i++) {
        if (g_state_transitions[i].current_state == current &&
            g_state_transitions[i].event == event) {
            *next = g_state_transitions[i].next_state;
            return LTE_OK;
        }
    }
    
    /* No valid transition found */
    return LTE_ERROR_INVALID_STATE;
}

/**
 * @brief Get state name string
 */
const char* lte_state_name(lte_module_state_t state)
{
    switch (state) {
        case LTE_STATE_POWER_OFF:
            return "POWER_OFF";
        case LTE_STATE_INITIALIZING:
            return "INITIALIZING";
        case LTE_STATE_SIM_LOCKED:
            return "SIM_LOCKED";
        case LTE_STATE_READY:
            return "READY";
        case LTE_STATE_SEARCHING:
            return "SEARCHING";
        case LTE_STATE_REGISTERED:
            return "REGISTERED";
        case LTE_STATE_ONLINE:
            return "ONLINE";
        case LTE_STATE_ERROR:
            return "ERROR";
        case LTE_STATE_RESETTING:
            return "RESETTING";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Get event name string
 */
const char* lte_event_name(lte_state_event_t event)
{
    switch (event) {
        case LTE_EVENT_INIT_SUCCESS:
            return "INIT_SUCCESS";
        case LTE_EVENT_INIT_FAILED:
            return "INIT_FAILED";
        case LTE_EVENT_SIM_READY:
            return "SIM_READY";
        case LTE_EVENT_SIM_LOCKED:
            return "SIM_LOCKED";
        case LTE_EVENT_SIM_ERROR:
            return "SIM_ERROR";
        case LTE_EVENT_REG_STARTED:
            return "REG_STARTED";
        case LTE_EVENT_REG_SUCCESS:
            return "REG_SUCCESS";
        case LTE_EVENT_REG_FAILED:
            return "REG_FAILED";
        case LTE_EVENT_NETWORK_LOST:
            return "NETWORK_LOST";
        case LTE_EVENT_DATA_CONNECTED:
            return "DATA_CONNECTED";
        case LTE_EVENT_DATA_DISCONNECTED:
            return "DATA_DISCONNECTED";
        case LTE_EVENT_RESET_REQUESTED:
            return "RESET_REQUESTED";
        case LTE_EVENT_RESET_COMPLETE:
            return "RESET_COMPLETE";
        case LTE_EVENT_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}
