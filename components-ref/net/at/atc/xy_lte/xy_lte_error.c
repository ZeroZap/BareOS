/**
 * @file xy_lte_error.c
 * @brief LTE Module Error Code Implementation
 * @version 1.0.0
 * @date 2025-10-30
 */

#include "xy_lte_error.h"

/**
 * @brief Convert CME error code to LTE error code
 */
lte_error_t lte_error_from_cme(cme_error_code_t cme_code)
{
    switch (cme_code) {
        /* SIM-related errors */
        case CME_ERROR_SIM_NOT_INSERTED:
            return LTE_ERROR_NO_SIM;
        case CME_ERROR_SIM_PIN_REQUIRED:
        case CME_ERROR_PH_SIM_PIN_REQUIRED:
            return LTE_ERROR_SIM_PIN;
        case CME_ERROR_SIM_PUK_REQUIRED:
        case CME_ERROR_PH_FSIM_PUK_REQUIRED:
            return LTE_ERROR_SIM_PUK;
        case CME_ERROR_SIM_FAILURE:
        case CME_ERROR_SIM_WRONG:
            return LTE_ERROR_SIM_FAILURE;
        case CME_ERROR_SIM_BUSY:
            return LTE_ERROR_SIM_BUSY;
        case CME_ERROR_INCORRECT_PASSWORD:
            return LTE_ERROR_SIM_WRONG_PIN;
        case CME_ERROR_SIM_PIN2_REQUIRED:
            return LTE_ERROR_SIM_PIN2;
        case CME_ERROR_SIM_PUK2_REQUIRED:
            return LTE_ERROR_SIM_PUK2;
            
        /* Network-related errors */
        case CME_ERROR_NO_NETWORK_SERVICE:
            return LTE_ERROR_NO_NETWORK;
        case CME_ERROR_NETWORK_TIMEOUT:
            return LTE_ERROR_NETWORK_TIMEOUT;
        case CME_ERROR_NETWORK_NOT_ALLOWED:
            return LTE_ERROR_NETWORK_NOT_ALLOWED;
            
        /* Operation errors */
        case CME_ERROR_OPERATION_NOT_ALLOWED:
            return LTE_ERROR_INVALID_STATE;
        case CME_ERROR_OPERATION_NOT_SUPPORTED:
            return LTE_ERROR_NOT_SUPPORTED;
            
        /* Memory errors */
        case CME_ERROR_MEMORY_FULL:
        case CME_ERROR_MEMORY_FAILURE:
            return LTE_ERROR_NO_MEMORY;
            
        /* Default */
        case CME_ERROR_UNKNOWN:
        default:
            return LTE_ERROR_CME;
    }
}

/**
 * @brief Get error description string
 */
const char* lte_error_string(lte_error_t error)
{
    switch (error) {
        case LTE_OK:
            return "Success";
            
        /* General errors */
        case LTE_ERROR_TIMEOUT:
            return "Operation timeout";
        case LTE_ERROR_CME:
            return "Equipment error";
        case LTE_ERROR_CMS:
            return "Message service error";
        case LTE_ERROR_INVALID_PARAM:
            return "Invalid parameter";
        case LTE_ERROR_NO_MEMORY:
            return "Out of memory";
        case LTE_ERROR_NOT_INITIALIZED:
            return "Module not initialized";
        case LTE_ERROR_BUSY:
            return "Operation in progress";
        case LTE_ERROR_NOT_SUPPORTED:
            return "Operation not supported";
        case LTE_ERROR_UNKNOWN:
            return "Unknown error";
            
        /* SIM errors */
        case LTE_ERROR_NO_SIM:
            return "SIM not inserted";
        case LTE_ERROR_SIM_PIN:
            return "SIM PIN required";
        case LTE_ERROR_SIM_PUK:
            return "SIM PUK required";
        case LTE_ERROR_SIM_FAILURE:
            return "SIM failure";
        case LTE_ERROR_SIM_BUSY:
            return "SIM busy";
        case LTE_ERROR_SIM_WRONG_PIN:
            return "Wrong PIN";
        case LTE_ERROR_SIM_WRONG_PUK:
            return "Wrong PUK";
        case LTE_ERROR_SIM_PIN2:
            return "SIM PIN2 required";
        case LTE_ERROR_SIM_PUK2:
            return "SIM PUK2 required";
            
        /* Network errors */
        case LTE_ERROR_NO_NETWORK:
            return "No network service";
        case LTE_ERROR_NETWORK_DENIED:
            return "Network registration denied";
        case LTE_ERROR_NETWORK_TIMEOUT:
            return "Network registration timeout";
        case LTE_ERROR_NETWORK_NOT_ALLOWED:
            return "Network not allowed";
        case LTE_ERROR_OPERATOR_NOT_FOUND:
            return "Operator not found";
        case LTE_ERROR_ROAMING_NOT_ALLOWED:
            return "Roaming not allowed";
            
        /* AT command errors */
        case LTE_ERROR_AT_SYNTAX:
            return "AT command syntax error";
        case LTE_ERROR_AT_NOT_SUPPORTED:
            return "AT command not supported";
        case LTE_ERROR_AT_RESPONSE:
            return "Invalid AT response";
        case LTE_ERROR_AT_BUFFER_FULL:
            return "AT buffer full";
            
        /* State errors */
        case LTE_ERROR_INVALID_STATE:
            return "Invalid module state";
        case LTE_ERROR_NOT_REGISTERED:
            return "Not registered to network";
        case LTE_ERROR_ALREADY_REGISTERED:
            return "Already registered";
            
        /* Hardware errors */
        case LTE_ERROR_UART:
            return "UART communication error";
        case LTE_ERROR_POWER:
            return "Power control error";
        case LTE_ERROR_MODULE_RESET:
            return "Module reset detected";
            
        default:
            return "Unknown error code";
    }
}

/**
 * @brief Check if error is recoverable
 */
xy_bool lte_error_is_recoverable(lte_error_t error)
{
    switch (error) {
        /* Recoverable errors */
        case LTE_ERROR_TIMEOUT:
        case LTE_ERROR_BUSY:
        case LTE_ERROR_SIM_BUSY:
        case LTE_ERROR_NO_NETWORK:
        case LTE_ERROR_NETWORK_TIMEOUT:
        case LTE_ERROR_AT_BUFFER_FULL:
            return true;
            
        /* Non-recoverable errors */
        case LTE_ERROR_INVALID_PARAM:
        case LTE_ERROR_NOT_SUPPORTED:
        case LTE_ERROR_NO_SIM:
        case LTE_ERROR_SIM_FAILURE:
        case LTE_ERROR_SIM_PUK:
        case LTE_ERROR_SIM_PUK2:
        case LTE_ERROR_NETWORK_DENIED:
        case LTE_ERROR_NETWORK_NOT_ALLOWED:
        case LTE_ERROR_AT_SYNTAX:
        case LTE_ERROR_AT_NOT_SUPPORTED:
            return false;
            
        /* Default: assume recoverable with user intervention */
        default:
            return true;
    }
}
