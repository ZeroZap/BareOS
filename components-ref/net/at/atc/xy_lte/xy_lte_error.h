/**
 * @file xy_lte_error.h
 * @brief LTE Module Error Codes and Error Handling
 * @version 1.0.0
 * @date 2025-10-30
 * 
 * This file defines error codes for LTE module operations based on
 * 3GPP TS 27.007 CME/CMS error codes.
 */

#ifndef _XY_LTE_ERROR_H_
#define _XY_LTE_ERROR_H_

#include "../../xy_clib/xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== LTE Error Codes ==================== */

/**
 * @brief LTE operation result codes
 */
typedef enum {
    /* Success codes */
    LTE_OK = 0,                          /**< Operation completed successfully */
    
    /* General errors (-1 to -9) */
    LTE_ERROR_TIMEOUT = -1,              /**< Operation timeout */
    LTE_ERROR_CME = -2,                  /**< Equipment error (+CME ERROR) */
    LTE_ERROR_CMS = -3,                  /**< Message service error (+CMS ERROR) */
    LTE_ERROR_INVALID_PARAM = -4,        /**< Invalid parameter */
    LTE_ERROR_NO_MEMORY = -5,            /**< Out of memory */
    LTE_ERROR_NOT_INITIALIZED = -6,      /**< Module not initialized */
    LTE_ERROR_BUSY = -7,                 /**< Operation in progress */
    LTE_ERROR_NOT_SUPPORTED = -8,        /**< Operation not supported */
    LTE_ERROR_UNKNOWN = -9,              /**< Unknown error */
    
    /* SIM errors (-10 to -19) */
    LTE_ERROR_NO_SIM = -10,              /**< SIM not inserted */
    LTE_ERROR_SIM_PIN = -11,             /**< SIM PIN required */
    LTE_ERROR_SIM_PUK = -12,             /**< SIM PUK required */
    LTE_ERROR_SIM_FAILURE = -13,         /**< SIM failure */
    LTE_ERROR_SIM_BUSY = -14,            /**< SIM busy */
    LTE_ERROR_SIM_WRONG_PIN = -15,       /**< Wrong PIN */
    LTE_ERROR_SIM_WRONG_PUK = -16,       /**< Wrong PUK */
    LTE_ERROR_SIM_PIN2 = -17,            /**< SIM PIN2 required */
    LTE_ERROR_SIM_PUK2 = -18,            /**< SIM PUK2 required */
    
    /* Network errors (-20 to -29) */
    LTE_ERROR_NO_NETWORK = -20,          /**< No network service */
    LTE_ERROR_NETWORK_DENIED = -21,      /**< Network registration denied */
    LTE_ERROR_NETWORK_TIMEOUT = -22,     /**< Network registration timeout */
    LTE_ERROR_NETWORK_NOT_ALLOWED = -23, /**< Network not allowed */
    LTE_ERROR_OPERATOR_NOT_FOUND = -24,  /**< Operator not found */
    LTE_ERROR_ROAMING_NOT_ALLOWED = -25, /**< Roaming not allowed */
    
    /* AT command errors (-30 to -39) */
    LTE_ERROR_AT_SYNTAX = -30,           /**< AT command syntax error */
    LTE_ERROR_AT_NOT_SUPPORTED = -31,    /**< AT command not supported */
    LTE_ERROR_AT_RESPONSE = -32,         /**< Invalid AT response */
    LTE_ERROR_AT_BUFFER_FULL = -33,      /**< AT buffer full */
    
    /* State errors (-40 to -49) */
    LTE_ERROR_INVALID_STATE = -40,       /**< Invalid module state */
    LTE_ERROR_NOT_REGISTERED = -41,      /**< Not registered to network */
    LTE_ERROR_ALREADY_REGISTERED = -42,  /**< Already registered */
    
    /* Hardware errors (-50 to -59) */
    LTE_ERROR_UART = -50,                /**< UART communication error */
    LTE_ERROR_POWER = -51,               /**< Power control error */
    LTE_ERROR_MODULE_RESET = -52         /**< Module reset detected */
} lte_error_t;

/**
 * @brief 3GPP TS 27.007 CME ERROR codes
 * 
 * These are standard error codes returned by cellular modules
 * in response to AT commands.
 */
typedef enum {
    CME_ERROR_PHONE_FAILURE = 0,               /**< Phone failure */
    CME_ERROR_NO_CONNECTION = 1,               /**< No connection to phone */
    CME_ERROR_PHONE_ADAPTOR_LINK = 2,          /**< Phone adaptor link reserved */
    CME_ERROR_OPERATION_NOT_ALLOWED = 3,       /**< Operation not allowed */
    CME_ERROR_OPERATION_NOT_SUPPORTED = 4,     /**< Operation not supported */
    CME_ERROR_PH_SIM_PIN_REQUIRED = 5,         /**< PH-SIM PIN required */
    CME_ERROR_PH_FSIM_PIN_REQUIRED = 6,        /**< PH-FSIM PIN required */
    CME_ERROR_PH_FSIM_PUK_REQUIRED = 7,        /**< PH-FSIM PUK required */
    CME_ERROR_SIM_NOT_INSERTED = 10,           /**< SIM not inserted */
    CME_ERROR_SIM_PIN_REQUIRED = 11,           /**< SIM PIN required */
    CME_ERROR_SIM_PUK_REQUIRED = 12,           /**< SIM PUK required */
    CME_ERROR_SIM_FAILURE = 13,                /**< SIM failure */
    CME_ERROR_SIM_BUSY = 14,                   /**< SIM busy */
    CME_ERROR_SIM_WRONG = 15,                  /**< SIM wrong */
    CME_ERROR_INCORRECT_PASSWORD = 16,         /**< Incorrect password */
    CME_ERROR_SIM_PIN2_REQUIRED = 17,          /**< SIM PIN2 required */
    CME_ERROR_SIM_PUK2_REQUIRED = 18,          /**< SIM PUK2 required */
    CME_ERROR_MEMORY_FULL = 20,                /**< Memory full */
    CME_ERROR_INVALID_INDEX = 21,              /**< Invalid index */
    CME_ERROR_NOT_FOUND = 22,                  /**< Not found */
    CME_ERROR_MEMORY_FAILURE = 23,             /**< Memory failure */
    CME_ERROR_TEXT_STRING_TOO_LONG = 24,       /**< Text string too long */
    CME_ERROR_INVALID_CHARACTERS = 25,         /**< Invalid characters in text string */
    CME_ERROR_DIAL_STRING_TOO_LONG = 26,       /**< Dial string too long */
    CME_ERROR_INVALID_DIAL_CHAR = 27,          /**< Invalid characters in dial string */
    CME_ERROR_NO_NETWORK_SERVICE = 30,         /**< No network service */
    CME_ERROR_NETWORK_TIMEOUT = 31,            /**< Network timeout */
    CME_ERROR_NETWORK_NOT_ALLOWED = 32,        /**< Network not allowed - emergency calls only */
    CME_ERROR_NETWORK_PERSONALIZATION_PIN = 40, /**< Network personalization PIN required */
    CME_ERROR_NETWORK_PERSONALIZATION_PUK = 41, /**< Network personalization PUK required */
    CME_ERROR_NETWORK_SUBSET_PERSONALIZATION_PIN = 42, /**< Network subset personalization PIN required */
    CME_ERROR_NETWORK_SUBSET_PERSONALIZATION_PUK = 43, /**< Network subset personalization PUK required */
    CME_ERROR_SERVICE_PROVIDER_PERSONALIZATION_PIN = 44, /**< Service provider personalization PIN required */
    CME_ERROR_SERVICE_PROVIDER_PERSONALIZATION_PUK = 45, /**< Service provider personalization PUK required */
    CME_ERROR_CORPORATE_PERSONALIZATION_PIN = 46, /**< Corporate personalization PIN required */
    CME_ERROR_CORPORATE_PERSONALIZATION_PUK = 47, /**< Corporate personalization PUK required */
    CME_ERROR_UNKNOWN = 100                    /**< Unknown error */
} cme_error_code_t;

/**
 * @brief Convert CME error code to LTE error code
 * 
 * @param cme_code CME error code from AT response
 * @return Corresponding LTE error code
 */
lte_error_t lte_error_from_cme(cme_error_code_t cme_code);

/**
 * @brief Get error description string
 * 
 * @param error LTE error code
 * @return Error description string (static, do not free)
 */
const char* lte_error_string(lte_error_t error);

/**
 * @brief Check if error is recoverable
 * 
 * @param error LTE error code
 * @return true if error is recoverable, false otherwise
 */
xy_bool lte_error_is_recoverable(lte_error_t error);

#ifdef __cplusplus
}
#endif

#endif /* _XY_LTE_ERROR_H_ */
