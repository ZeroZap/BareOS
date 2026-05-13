/**
 * @file xy_lte_types.h
 * @brief LTE Module Core Data Types and Structures
 * @version 1.0.0
 * @date 2025-10-30
 * 
 * This file defines core data structures for LTE module communication
 * based on 3GPP TS 27.007 standards.
 */

#ifndef _XY_LTE_TYPES_H_
#define _XY_LTE_TYPES_H_

#include "../../xy_clib/xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Network Registration Status ==================== */

/**
 * @brief Network registration status enumeration
 */
typedef enum {
    LTE_REG_NOT_REGISTERED = 0,   /**< Not registered, not searching */
    LTE_REG_REGISTERED_HOME = 1,  /**< Registered, home network */
    LTE_REG_SEARCHING = 2,        /**< Not registered, searching */
    LTE_REG_DENIED = 3,           /**< Registration denied */
    LTE_REG_UNKNOWN = 4,          /**< Unknown status */
    LTE_REG_REGISTERED_ROAMING = 5 /**< Registered, roaming */
} lte_reg_status_t;

/**
 * @brief Radio Access Technology (RAT) enumeration
 */
typedef enum {
    LTE_ACT_GSM = 0,        /**< GSM */
    LTE_ACT_GSM_COMPACT = 1, /**< GSM Compact (not used) */
    LTE_ACT_UTRAN = 2,      /**< UTRAN (WCDMA) */
    LTE_ACT_GSM_EGPRS = 3,  /**< GSM with EGPRS */
    LTE_ACT_UTRAN_HSDPA = 4, /**< UTRAN with HSDPA */
    LTE_ACT_UTRAN_HSUPA = 5, /**< UTRAN with HSUPA */
    LTE_ACT_UTRAN_HSDPA_HSUPA = 6, /**< UTRAN with HSDPA and HSUPA */
    LTE_ACT_E_UTRAN = 7,    /**< E-UTRAN (LTE) */
    LTE_ACT_EC_GSM_IoT = 8, /**< EC-GSM-IoT */
    LTE_ACT_E_UTRAN_NB_S1 = 9, /**< E-UTRAN (NB-IoT) */
    LTE_ACT_E_UTRAN_CAT_M1 = 10 /**< E-UTRAN (Cat-M1/eMTC) */
} lte_access_tech_t;

/**
 * @brief Network registration status structure
 */
typedef struct {
    lte_reg_status_t status;    /**< Registration status */
    lte_access_tech_t access_tech; /**< Access technology */
    xy_u16 lac;                 /**< Location Area Code (GSM/WCDMA) */
    xy_u16 tac;                 /**< Tracking Area Code (LTE) */
    xy_u32 ci;                  /**< Cell ID */
    xy_u8 reject_cause;         /**< Registration rejection cause (3GPP TS 24.008) */
} lte_network_reg_t;

/* ==================== Signal Quality Information ==================== */

/**
 * @brief Signal quality information structure
 */
typedef struct {
    xy_i16 rssi;      /**< Received Signal Strength Indicator (dBm) */
    xy_i16 rsrp;      /**< Reference Signal Received Power (dBm, LTE) */
    xy_i16 rsrq;      /**< Reference Signal Received Quality (dB, LTE) */
    xy_i16 sinr;      /**< Signal to Interference plus Noise Ratio (dB, LTE) */
    xy_u8 ber;        /**< Bit Error Rate (encoded value 0-7) */
    xy_u8 bars;       /**< Signal strength bars (0-5) */
    xy_u8 rssi_raw;   /**< Raw RSSI value from AT+CSQ (0-31, 99=unknown) */
} lte_signal_quality_t;

/* ==================== Operator Information ==================== */

/**
 * @brief Operator status enumeration
 */
typedef enum {
    LTE_OPER_UNKNOWN = 0,    /**< Unknown */
    LTE_OPER_AVAILABLE = 1,  /**< Available */
    LTE_OPER_CURRENT = 2,    /**< Current */
    LTE_OPER_FORBIDDEN = 3   /**< Forbidden */
} lte_operator_status_t;

/**
 * @brief Operator information structure
 */
typedef struct {
    char operator_numeric[8];  /**< MCC+MNC (e.g., "46000") */
    char operator_short[17];   /**< Short alphanumeric name */
    char operator_long[33];    /**< Long alphanumeric name */
    lte_access_tech_t act;     /**< Access technology */
    lte_operator_status_t status; /**< Operator status */
} lte_operator_info_t;

/* ==================== SIM Card Status ==================== */

/**
 * @brief SIM card status enumeration
 */
typedef enum {
    LTE_SIM_READY = 0,          /**< SIM card ready */
    LTE_SIM_NOT_INSERTED = 1,   /**< SIM card not inserted */
    LTE_SIM_PIN_REQUIRED = 2,   /**< PIN entry required */
    LTE_SIM_PUK_REQUIRED = 3,   /**< PUK entry required */
    LTE_SIM_PIN2_REQUIRED = 4,  /**< PIN2 entry required */
    LTE_SIM_PUK2_REQUIRED = 5,  /**< PUK2 entry required */
    LTE_SIM_NETWORK_LOCKED = 6, /**< Network personalization locked */
    LTE_SIM_ERROR = 7           /**< General SIM failure */
} lte_sim_status_t;

/**
 * @brief SIM card information structure
 */
typedef struct {
    lte_sim_status_t status;  /**< SIM status */
    char imsi[16];            /**< International Mobile Subscriber Identity */
    char iccid[21];           /**< Integrated Circuit Card Identifier */
    xy_u8 pin_retry_count;    /**< Remaining PIN retry attempts */
    xy_u8 puk_retry_count;    /**< Remaining PUK retry attempts */
} lte_sim_info_t;

/* ==================== Module Capabilities ==================== */

/**
 * @brief Module capabilities structure
 */
typedef struct {
    xy_bool supports_cat_m1;   /**< Supports LTE Cat-M1 (eMTC) */
    xy_bool supports_cat_nb1;  /**< Supports LTE Cat-NB1 (NB-IoT) */
    xy_bool supports_lte;      /**< Supports standard LTE */
    xy_bool supports_gnss;     /**< Integrated GNSS capability */
    xy_bool supports_voice;    /**< Voice call support */
    xy_bool supports_sms;      /**< SMS capability */
    xy_u8 max_pdp_contexts;    /**< Maximum simultaneous PDP contexts */
    xy_bool supports_ipv6;     /**< IPv6 support */
} lte_module_caps_t;

/* ==================== Device Information ==================== */

/**
 * @brief Device information structure
 */
typedef struct {
    char imei[16];             /**< International Mobile Equipment Identity */
    char manufacturer[32];     /**< Module manufacturer */
    char model[32];            /**< Module model */
    char firmware_version[64]; /**< Firmware version string */
} lte_device_info_t;

#ifdef __cplusplus
}
#endif

#endif /* _XY_LTE_TYPES_H_ */
