/**
 * @file xy_error.h
 * @brief XinYi Error Code Definitions
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_ERROR_H
#define XY_ERROR_H

#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Standard Error Codes ==================== */

/**
 * @brief Standard return values
 */
typedef enum {
    XY_OK                    =  0,  /**< Success */
    XY_ERROR                 = -1,  /**< General error */
    XY_ERROR_INVALID_PARAM   = -2,  /**< Invalid parameter */
    XY_ERROR_NOT_SUPPORTED   = -3,  /**< Not supported */
    XY_ERROR_TIMEOUT         = -4,  /**< Timeout */
    XY_ERROR_BUSY            = -5,  /**< Resource busy */
    XY_ERROR_NO_MEMORY       = -6,  /**< Out of memory */
    XY_ERROR_IO              = -7,  /**< I/O error */
    XY_ERROR_NOT_INIT        = -8,  /**< Not initialized */
    XY_ERROR_ALREADY_INIT    = -9,  /**< Already initialized */
    XY_ERROR_NO_RESOURCE     = -10, /**< No resource */
    XY_ERROR_FAIL            = -11, /**< Operation failed */
    XY_ERROR_NO_DATA         = -12, /**< No data */
    XY_ERROR_OVERFLOW        = -13, /**< Buffer overflow */
    XY_ERROR_UNDERFLOW       = -14, /**< Buffer underflow */
    XY_ERROR_CRC             = -15, /**< CRC error */
    XY_ERROR_CHECKSUM        = -16, /**< Checksum error */
    XY_ERROR_AUTH            = -17, /**< Authentication error */
    XY_ERROR_ACCESS_DENIED   = -18, /**< Access denied */
    XY_ERROR_NOT_FOUND       = -19, /**< Not found */
    XY_ERROR_INVALID_STATE   = -20, /**< Invalid state */
    XY_ERROR_INVALID_SIZE    = -21, /**< Invalid size */
    XY_ERROR_INVALID_ADDR    = -22, /**< Invalid address */
    XY_ERROR_NOT_READY       = -23, /**< Not ready */
    XY_ERROR_OUT_OF_RANGE    = -24, /**< Out of range */
    XY_ERROR_ALREADY_EXISTS  = -25, /**< Already exists */
    XY_ERROR_NOT_AVAILABLE   = -26, /**< Not available */
    XY_ERROR_NOT_IMPLEMENTED = -27, /**< Not implemented */
    XY_ERROR_INVALID_FORMAT  = -28, /**< Invalid format */
    XY_ERROR_INVALID_VERSION = -29, /**< Invalid version */
    XY_ERROR_SECURITY        = -30, /**< Security error */
    XY_ERROR_CALIBRATION     = -31, /**< Calibration error */
    XY_ERROR_ECC             = -32, /**< ECC error */
    XY_ERROR_WATCHDOG        = -33, /**< Watchdog error */
    XY_ERROR_STACK_OVERFLOW  = -34, /**< Stack overflow */
    XY_ERROR_UNALIGNED       = -35, /**< Unaligned access */
    XY_ERROR_ALIGNMENT       = -35, /**< Alignment error */
    XY_ERROR_BUS             = -36, /**< Bus error */
    XY_ERROR_PARITY          = -37, /**< Parity error */
    XY_ERROR_FRAMING         = -38, /**< Framing error */
    XY_ERROR_NOISE           = -39, /**< Noise error */
    XY_ERROR_COLLISION       = -40, /**< Collision error */
    XY_ERROR_ABORTED         = -41, /**< Operation aborted */
    XY_ERROR_SUSPENDED       = -42, /**< Operation suspended */
    XY_ERROR_RESUMED         = -43, /**< Operation resumed */
    XY_ERROR_INTERRUPTED     = -44, /**< Operation interrupted */
    XY_ERROR_LOCKED          = -45, /**< Locked */
    XY_ERROR_UNLOCKED        = -46, /**< Unlocked */
    XY_ERROR_EXPIRED         = -47, /**< Expired */
    XY_ERROR_DUPLICATE       = -48, /**< Duplicate */
    XY_ERROR_CORRUPT         = -49, /**< Corrupt data */
    XY_ERROR_INCOMPLETE      = -50, /**< Incomplete operation */
} xy_error_t;

/* ==================== Module-Specific Error Ranges ==================== */

/**
 * @brief HAL driver error codes (range: -100 to -199)
 */
typedef enum {
    XY_HAL_ERROR_BASE = -100,
    XY_HAL_ERROR_INVALID_PARAM = -101,   /**< Invalid parameter */
    XY_HAL_ERROR_INVALID_HANDLE = -102,  /**< Invalid handle */
    XY_HAL_ERROR_INVALID_MODE = -103,    /**< Invalid mode */
    XY_HAL_ERROR_INVALID_CONFIG = -104,  /**< Invalid configuration */
    XY_HAL_ERROR_PIN_CONFLICT = -105,    /**< Pin conflict */
    XY_HAL_ERROR_CLOCK_ERROR = -106,     /**< Clock error */
    XY_HAL_ERROR_PERIPH_ERROR = -107,    /**< Peripheral error */
    XY_HAL_ERROR_DMA_ERROR = -108,       /**< DMA error */
    XY_HAL_ERROR_IRQ_ERROR = -109,       /**< IRQ error */
    XY_HAL_ERROR_GPIO_ERROR = -110,      /**< GPIO error */
    XY_HAL_ERROR_UART_ERROR = -111,      /**< UART error */
    XY_HAL_ERROR_SPI_ERROR = -112,       /**< SPI error */
    XY_HAL_ERROR_I2C_ERROR = -113,       /**< I2C error */
    XY_HAL_ERROR_TIMER_ERROR = -114,     /**< Timer error */
    XY_HAL_ERROR_ADC_ERROR = -115,       /**< ADC error */
    XY_HAL_ERROR_DAC_ERROR = -116,       /**< DAC error */
    XY_HAL_ERROR_RTC_ERROR = -117,       /**< RTC error */
    XY_HAL_ERROR_WDG_ERROR = -118,       /**< Watchdog error */
    XY_HAL_ERROR_TIMEOUT = -119,         /**< Timeout */
    XY_HAL_ERROR_BUSY = -120,            /**< Busy */
    XY_HAL_ERROR_NO_MEMORY = -121,       /**< No memory */
    XY_HAL_ERROR_IO = -122,              /**< I/O error */
    XY_HAL_ERROR_FAIL = -123,            /**< Generic failure */
    XY_HAL_ERROR_FLASH_ERROR = -124,     /**< Flash error */
    XY_HAL_ERROR_CRC_ERROR = -125,       /**< CRC error */
    XY_HAL_ERROR_RNG_ERROR = -126,       /**< RNG error */
    XY_HAL_ERROR_I2S_ERROR = -127,       /**< I2S error */
    XY_HAL_ERROR_CAN_ERROR = -128,       /**< CAN error */
    XY_HAL_ERROR_SDMMC_ERROR = -129,     /**< SDMMC error */
    XY_HAL_ERROR_ETH_ERROR = -130,       /**< Ethernet error */
    XY_HAL_ERROR_USB_ERROR = -131,       /**< USB error */
    XY_HAL_ERROR_LCD_ERROR = -132,       /**< LCD error */
    XY_HAL_ERROR_TOUCH_ERROR = -133,     /**< Touch error */
    XY_HAL_ERROR_SENSOR_ERROR = -134,    /**< Sensor error */
    XY_HAL_ERROR_END = -199,
} xy_hal_error_codes_t;

/**
 * @brief File system error codes (range: -200 to -299)
 */
typedef enum {
    XY_FS_ERROR_BASE = -200,
    XY_FS_ERROR_INVALID_PATH = -201,     /**< Invalid path */
    XY_FS_ERROR_FILE_NOT_FOUND = -202,   /**< File not found */
    XY_FS_ERROR_DIR_NOT_FOUND = -203,    /**< Directory not found */
    XY_FS_ERROR_ACCESS_DENIED = -204,    /**< Access denied */
    XY_FS_ERROR_DISK_FULL = -205,        /**< Disk full */
    XY_FS_ERROR_EOF = -206,              /**< End of file */
    XY_FS_ERROR_SEEK = -207,             /**< Seek error */
    XY_FS_ERROR_READ_ONLY = -208,        /**< Read-only */
    XY_FS_ERROR_WRITE_PROTECTED = -209,  /**< Write protected */
    XY_FS_ERROR_INVALID_SECTOR = -210,   /**< Invalid sector */
    XY_FS_ERROR_NO_MEDIA = -211,         /**< No media */
    XY_FS_ERROR_MEDIA_CHANGED = -212,    /**< Media changed */
    XY_FS_ERROR_DIR_NOT_EMPTY = -213,    /**< Directory not empty */
    XY_FS_ERROR_NAME_TOO_LONG = -214,    /**< Name too long */
    XY_FS_ERROR_INVALID_NAME = -215,     /**< Invalid name */
    XY_FS_ERROR_TOO_MANY_OPEN_FILES = -216, /**< Too many open files */
    XY_FS_ERROR_FILE_OPEN = -217,        /**< File open error */
    XY_FS_ERROR_INVALID_CLUSTER = -218,  /**< Invalid cluster */
    XY_FS_ERROR_FAT_ERROR = -219,        /**< FAT error */
    XY_FS_ERROR_DIRTY_SHUTDOWN = -220,   /**< Dirty shutdown */
    XY_FS_ERROR_END = -299,
} xy_fs_error_codes_t;

/**
 * @brief Network error codes (range: -300 to -399)
 */
typedef enum {
    XY_NET_ERROR_BASE = -300,
    XY_NET_ERROR_SOCKET = -301,          /**< Socket error */
    XY_NET_ERROR_CONNECTION = -302,      /**< Connection error */
    XY_NET_ERROR_DISCONNECTED = -303,    /**< Disconnected */
    XY_NET_ERROR_TIMEOUT_CONNECT = -304, /**< Connection timeout */
    XY_NET_ERROR_TIMEOUT_SEND = -305,    /**< Send timeout */
    XY_NET_ERROR_TIMEOUT_RECV = -306,    /**< Receive timeout */
    XY_NET_ERROR_NO_ROUTE = -307,        /**< No route */
    XY_NET_ERROR_HOST_UNREACHABLE = -308,/**< Host unreachable */
    XY_NET_ERROR_PORT_UNREACHABLE = -309,/**< Port unreachable */
    XY_NET_ERROR_ADDR_IN_USE = -310,     /**< Address in use */
    XY_NET_ERROR_ADDR_NOT_AVAIL = -311,  /**< Address not available */
    XY_NET_ERROR_NET_UNREACHABLE = -312, /**< Network unreachable */
    XY_NET_ERROR_CONN_ABORTED = -313,    /**< Connection aborted */
    XY_NET_ERROR_CONN_RESET = -314,      /**< Connection reset */
    XY_NET_ERROR_CONN_REFUSED = -315,    /**< Connection refused */
    XY_NET_ERROR_NO_BUFFER = -316,       /**< No buffer */
    XY_NET_ERROR_INVALID_PROTO = -317,   /**< Invalid protocol */
    XY_NET_ERROR_INVALID_ADDR = -318,    /**< Invalid address */
    XY_NET_ERROR_DNS_ERROR = -319,       /**< DNS error */
    XY_NET_ERROR_DHCP_ERROR = -320,      /**< DHCP error */
    XY_NET_ERROR_SSL_ERROR = -321,       /**< SSL error */
    XY_NET_ERROR_END = -399,
} xy_net_error_codes_t;

/**
 * @brief Crypto error codes (range: -400 to -499)
 */
typedef enum {
    XY_CRYPTO_ERROR_BASE = -400,
    XY_CRYPTO_ERROR_INVALID_KEY = -401,  /**< Invalid key */
    XY_CRYPTO_ERROR_INVALID_IV = -402,   /**< Invalid IV */
    XY_CRYPTO_ERROR_INVALID_TAG = -403,  /**< Invalid tag */
    XY_CRYPTO_ERROR_INVALID_MODE = -404, /**< Invalid mode */
    XY_CRYPTO_ERROR_INVALID_SIZE = -405, /**< Invalid size */
    XY_CRYPTO_ERROR_INVALID_DATA = -406, /**< Invalid data */
    XY_CRYPTO_ERROR_AUTH_FAIL = -407,    /**< Authentication failed */
    XY_CRYPTO_ERROR_DECRYPT_FAIL = -408, /**< Decryption failed */
    XY_CRYPTO_ERROR_ENCRYPT_FAIL = -409, /**< Encryption failed */
    XY_CRYPTO_ERROR_HASH_MISMATCH = -410,/**< Hash mismatch */
    XY_CRYPTO_ERROR_KEY_GEN_FAIL = -411, /**< Key generation failed */
    XY_CRYPTO_ERROR_SIGN_FAIL = -412,    /**< Signing failed */
    XY_CRYPTO_ERROR_VERIFY_FAIL = -413,  /**< Verification failed */
    XY_CRYPTO_ERROR_RNG_FAIL = -414,     /**< RNG failed */
    XY_CRYPTO_ERROR_NOT_SUPPORTED = -415,/**< Algorithm not supported */
    XY_CRYPTO_ERROR_INVALID_PADDING = -416,/**< Invalid padding */
    XY_CRYPTO_ERROR_INVALID_NONCE = -417,/**< Invalid nonce */
    XY_CRYPTO_ERROR_INVALID_SALT = -418, /**< Invalid salt */
    XY_CRYPTO_ERROR_INVALID_DIGEST = -419,/**< Invalid digest */
    XY_CRYPTO_ERROR_KEY_EXCHANGE_FAIL = -420,/**< Key exchange failed */
    XY_CRYPTO_ERROR_END = -499,
} xy_crypto_error_codes_t;

/* ==================== Convenience Macros ==================== */

/**
 * @brief Check if error code indicates success
 * @param err Error code
 * @return 1 if success, 0 otherwise
 */
#define XY_IS_SUCCESS(err) ((err) >= 0)

/**
 * @brief Check if error code indicates failure
 * @param err Error code
 * @return 1 if failure, 0 otherwise
 */
#define XY_IS_ERROR(err) ((err) < 0)

/**
 * @brief Convert error code to positive/negative indicator
 * @param err Error code
 * @return 1 if error, 0 if success
 */
#define XY_FAILED(err) XY_IS_ERROR(err)

/**
 * @brief Convert error code to success indicator
 * @param err Error code
 * @return 1 if success, 0 if error
 */
#define XY_SUCCEEDED(err) XY_IS_SUCCESS(err)

/* ==================== Common Error Handling Macros ==================== */

/**
 * @brief Early return if error
 * @param expr Expression that returns error code
 */
#define XY_RETURN_ON_ERROR(expr) \
    do { \
        xy_error_t _err = (expr); \
        if (XY_FAILED(_err)) { \
            return _err; \
        } \
    } while(0)

/**
 * @brief Early return if error with specific return value
 * @param expr Expression that returns error code
 * @param ret_val Return value if error
 */
#define XY_RETURN_VAL_ON_ERROR(expr, ret_val) \
    do { \
        xy_error_t _err = (expr); \
        if (XY_FAILED(_err)) { \
            return (ret_val); \
        } \
    } while(0)

/**
 * @brief Early break if error
 * @param expr Expression that returns error code
 */
#define XY_BREAK_ON_ERROR(expr) \
    do { \
        xy_error_t _err = (expr); \
        if (XY_FAILED(_err)) { \
            break; \
        } \
    } while(0)

/**
 * @brief Early continue if error
 * @param expr Expression that returns error code
 */
#define XY_CONTINUE_ON_ERROR(expr) \
    do { \
        xy_error_t _err = (expr); \
        if (XY_FAILED(_err)) { \
            continue; \
        } \
    } while(0)

/**
 * @brief Go to label if error
 * @param expr Expression that returns error code
 * @param label Label to jump to
 */
#define XY_GOTO_ON_ERROR(expr, label) \
    do { \
        xy_error_t _err = (expr); \
        if (XY_FAILED(_err)) { \
            goto label; \
        } \
    } while(0)

/* ==================== Error Code Conversion ==================== */

/**
 * @brief Convert standard error to HAL error
 * @param std_err Standard error code
 * @return HAL error code
 */
static inline xy_error_t xy_std_to_hal_error(xy_error_t std_err)
{
    switch (std_err) {
    case XY_OK: return XY_OK;
    case XY_ERROR_INVALID_PARAM: return XY_HAL_ERROR_INVALID_PARAM;
    case XY_ERROR_TIMEOUT: return XY_HAL_ERROR_TIMEOUT;
    case XY_ERROR_BUSY: return XY_HAL_ERROR_BUSY;
    case XY_ERROR_NO_MEMORY: return XY_HAL_ERROR_NO_MEMORY;
    case XY_ERROR_IO: return XY_HAL_ERROR_IO;
    default: return XY_HAL_ERROR_FAIL;
    }
}

/**
 * @brief Convert HAL error to standard error
 * @param hal_err HAL error code
 * @return Standard error code
 */
static inline xy_error_t xy_hal_to_std_error(xy_error_t hal_err)
{
    switch (hal_err) {
    case XY_OK: return XY_OK;
    case XY_HAL_ERROR_INVALID_PARAM: return XY_ERROR_INVALID_PARAM;
    case XY_HAL_ERROR_TIMEOUT: return XY_ERROR_TIMEOUT;
    case XY_HAL_ERROR_BUSY: return XY_ERROR_BUSY;
    case XY_HAL_ERROR_NO_MEMORY: return XY_ERROR_NO_MEMORY;
    case XY_HAL_ERROR_IO: return XY_ERROR_IO;
    default: return XY_ERROR_FAIL;
    }
}

/* ==================== Error String Conversion ==================== */

/**
 * @brief Get error string representation
 * @param err Error code
 * @return Error string
 */
static inline const char *xy_error_to_string(xy_error_t err)
{
    switch (err) {
    case XY_OK:                    return "XY_OK";
    case XY_ERROR:                 return "XY_ERROR";
    case XY_ERROR_INVALID_PARAM:   return "XY_ERROR_INVALID_PARAM";
    case XY_ERROR_NOT_SUPPORTED:   return "XY_ERROR_NOT_SUPPORTED";
    case XY_ERROR_TIMEOUT:         return "XY_ERROR_TIMEOUT";
    case XY_ERROR_BUSY:            return "XY_ERROR_BUSY";
    case XY_ERROR_NO_MEMORY:       return "XY_ERROR_NO_MEMORY";
    case XY_ERROR_IO:              return "XY_ERROR_IO";
    case XY_ERROR_NOT_INIT:        return "XY_ERROR_NOT_INIT";
    case XY_ERROR_ALREADY_INIT:    return "XY_ERROR_ALREADY_INIT";
    case XY_ERROR_NO_RESOURCE:     return "XY_ERROR_NO_RESOURCE";
    case XY_ERROR_FAIL:            return "XY_ERROR_FAIL";
    case XY_ERROR_NO_DATA:         return "XY_ERROR_NO_DATA";
    case XY_ERROR_OVERFLOW:        return "XY_ERROR_OVERFLOW";
    case XY_ERROR_UNDERFLOW:       return "XY_ERROR_UNDERFLOW";
    case XY_ERROR_CRC:             return "XY_ERROR_CRC";
    case XY_ERROR_CHECKSUM:        return "XY_ERROR_CHECKSUM";
    case XY_ERROR_AUTH:            return "XY_ERROR_AUTH";
    case XY_ERROR_ACCESS_DENIED:   return "XY_ERROR_ACCESS_DENIED";
    case XY_ERROR_NOT_FOUND:       return "XY_ERROR_NOT_FOUND";
    case XY_ERROR_INVALID_STATE:   return "XY_ERROR_INVALID_STATE";
    case XY_ERROR_INVALID_SIZE:    return "XY_ERROR_INVALID_SIZE";
    case XY_ERROR_INVALID_ADDR:    return "XY_ERROR_INVALID_ADDR";
    case XY_ERROR_NOT_READY:       return "XY_ERROR_NOT_READY";
    case XY_ERROR_OUT_OF_RANGE:    return "XY_ERROR_OUT_OF_RANGE";
    case XY_ERROR_ALREADY_EXISTS:  return "XY_ERROR_ALREADY_EXISTS";
    case XY_ERROR_NOT_AVAILABLE:   return "XY_ERROR_NOT_AVAILABLE";
    case XY_ERROR_NOT_IMPLEMENTED: return "XY_ERROR_NOT_IMPLEMENTED";
    case XY_ERROR_INVALID_FORMAT:  return "XY_ERROR_INVALID_FORMAT";
    case XY_ERROR_INVALID_VERSION: return "XY_ERROR_INVALID_VERSION";
    case XY_ERROR_SECURITY:        return "XY_ERROR_SECURITY";
    case XY_ERROR_CALIBRATION:     return "XY_ERROR_CALIBRATION";
    case XY_ERROR_ECC:             return "XY_ERROR_ECC";
    case XY_ERROR_WATCHDOG:        return "XY_ERROR_WATCHDOG";
    case XY_ERROR_STACK_OVERFLOW:  return "XY_ERROR_STACK_OVERFLOW";
    case XY_ERROR_UNALIGNED:       return "XY_ERROR_UNALIGNED";
    case XY_ERROR_BUS:             return "XY_ERROR_BUS";
    case XY_ERROR_PARITY:          return "XY_ERROR_PARITY";
    case XY_ERROR_FRAMING:         return "XY_ERROR_FRAMING";
    case XY_ERROR_NOISE:           return "XY_ERROR_NOISE";
    case XY_ERROR_COLLISION:       return "XY_ERROR_COLLISION";
    case XY_ERROR_ABORTED:         return "XY_ERROR_ABORTED";
    case XY_ERROR_SUSPENDED:       return "XY_ERROR_SUSPENDED";
    case XY_ERROR_RESUMED:         return "XY_ERROR_RESUMED";
    case XY_ERROR_INTERRUPTED:     return "XY_ERROR_INTERRUPTED";
    case XY_ERROR_LOCKED:          return "XY_ERROR_LOCKED";
    case XY_ERROR_UNLOCKED:        return "XY_ERROR_UNLOCKED";
    case XY_ERROR_EXPIRED:         return "XY_ERROR_EXPIRED";
    case XY_ERROR_DUPLICATE:       return "XY_ERROR_DUPLICATE";
    case XY_ERROR_CORRUPT:         return "XY_ERROR_CORRUPT";
    case XY_ERROR_INCOMPLETE:      return "XY_ERROR_INCOMPLETE";
    default:                       return "XY_ERROR_UNKNOWN";
    }
}

/**
 * @brief Get error description
 * @param err Error code
 * @return Error description
 */
static inline const char *xy_error_get_description(xy_error_t err)
{
    switch (err) {
    case XY_OK:                    return "Operation completed successfully";
    case XY_ERROR:                 return "General error";
    case XY_ERROR_INVALID_PARAM:   return "Invalid parameter provided";
    case XY_ERROR_NOT_SUPPORTED:   return "Feature not supported";
    case XY_ERROR_TIMEOUT:         return "Operation timed out";
    case XY_ERROR_BUSY:            return "Resource is busy";
    case XY_ERROR_NO_MEMORY:       return "Out of memory";
    case XY_ERROR_IO:              return "Input/output error";
    case XY_ERROR_NOT_INIT:        return "Module not initialized";
    case XY_ERROR_ALREADY_INIT:    return "Module already initialized";
    case XY_ERROR_NO_RESOURCE:     return "Resource unavailable";
    case XY_ERROR_FAIL:            return "Operation failed";
    case XY_ERROR_NO_DATA:         return "No data available";
    case XY_ERROR_OVERFLOW:        return "Buffer overflow";
    case XY_ERROR_UNDERFLOW:       return "Buffer underflow";
    case XY_ERROR_CRC:             return "CRC error";
    case XY_ERROR_CHECKSUM:        return "Checksum error";
    case XY_ERROR_AUTH:            return "Authentication error";
    case XY_ERROR_ACCESS_DENIED:   return "Access denied";
    case XY_ERROR_NOT_FOUND:       return "Resource not found";
    case XY_ERROR_INVALID_STATE:   return "Invalid state";
    case XY_ERROR_INVALID_SIZE:    return "Invalid size";
    case XY_ERROR_INVALID_ADDR:    return "Invalid address";
    case XY_ERROR_NOT_READY:       return "Not ready";
    case XY_ERROR_OUT_OF_RANGE:    return "Value out of range";
    case XY_ERROR_ALREADY_EXISTS:  return "Resource already exists";
    case XY_ERROR_NOT_AVAILABLE:   return "Resource not available";
    case XY_ERROR_NOT_IMPLEMENTED: return "Feature not implemented";
    case XY_ERROR_INVALID_FORMAT:  return "Invalid format";
    case XY_ERROR_INVALID_VERSION: return "Invalid version";
    case XY_ERROR_SECURITY:        return "Security violation";
    case XY_ERROR_CALIBRATION:     return "Calibration error";
    case XY_ERROR_ECC:             return "ECC error";
    case XY_ERROR_WATCHDOG:        return "Watchdog error";
    case XY_ERROR_STACK_OVERFLOW:  return "Stack overflow";
    case XY_ERROR_UNALIGNED:       return "Unaligned access";
    case XY_ERROR_BUS:             return "Bus error";
    case XY_ERROR_PARITY:          return "Parity error";
    case XY_ERROR_FRAMING:         return "Framing error";
    case XY_ERROR_NOISE:           return "Noise error";
    case XY_ERROR_COLLISION:       return "Collision error";
    case XY_ERROR_ABORTED:         return "Operation aborted";
    case XY_ERROR_SUSPENDED:       return "Operation suspended";
    case XY_ERROR_RESUMED:         return "Operation resumed";
    case XY_ERROR_INTERRUPTED:     return "Operation interrupted";
    case XY_ERROR_LOCKED:          return "Resource locked";
    case XY_ERROR_UNLOCKED:        return "Resource unlocked";
    case XY_ERROR_EXPIRED:         return "Expired";
    case XY_ERROR_DUPLICATE:       return "Duplicate resource";
    case XY_ERROR_CORRUPT:         return "Corrupt data";
    case XY_ERROR_INCOMPLETE:      return "Incomplete operation";
    default:                       return "Unknown error";
    }
}

/* ==================== Helper Functions ==================== */

/**
 * @brief Check if error is in range
 * @param err Error code
 * @param base Base error code
 * @param end End error code
 * @return 1 if in range, 0 otherwise
 */
static inline int xy_error_in_range(xy_error_t err, xy_error_t base, xy_error_t end)
{
    return (err >= end) && (err <= base);
}

/**
 * @brief Get error module from error code
 * @param err Error code
 * @return Module identifier
 */
static inline int xy_error_get_module(xy_error_t err)
{
    if (xy_error_in_range(err, XY_HAL_ERROR_BASE, XY_HAL_ERROR_END)) {
        return 1; // HAL module
    } else if (xy_error_in_range(err, XY_FS_ERROR_BASE, XY_FS_ERROR_END)) {
        return 2; // File system module
    } else if (xy_error_in_range(err, XY_NET_ERROR_BASE, XY_NET_ERROR_END)) {
        return 3; // Network module
    } else if (xy_error_in_range(err, XY_CRYPTO_ERROR_BASE, XY_CRYPTO_ERROR_END)) {
        return 4; // Crypto module
    } else {
        return 0; // Standard module
    }
}

#ifdef __cplusplus
}
#endif

#endif /* XY_ERROR_H */
