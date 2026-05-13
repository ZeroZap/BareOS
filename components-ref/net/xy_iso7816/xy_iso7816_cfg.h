/**
 * @file xy_iso7816_cfg.h
 * @brief ISO7816 Configuration Header
 * @version 1.0
 * @date 2025-11-01
 *
 * This file contains compile-time configuration options for the ISO7816 module.
 */

#ifndef XY_ISO7816_CFG_H
#define XY_ISO7816_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Protocol Configuration */
/* ========================================================================== */

/**
 * @brief Enable T=0 protocol support (ISO7816-3)
 *
 * T=0 is the most common protocol for SIM cards.
 * Disable only if you don't need T=0 support.
 */
#ifndef XY_ISO7816_ENABLE_T0
#define XY_ISO7816_ENABLE_T0 1
#endif

/**
 * @brief Enable T=1 protocol support (ISO7816-3)
 *
 * T=1 is an advanced block-oriented protocol.
 * Currently not implemented - reserved for future use.
 */
#ifndef XY_ISO7816_ENABLE_T1
#define XY_ISO7816_ENABLE_T1 0
#endif

/* ========================================================================== */
/* Buffer Size Configuration */
/* ========================================================================== */

/**
 * @brief Maximum ATR length (bytes)
 *
 * ISO7816-3 allows up to 33 bytes for ATR.
 * Reducing this can save memory on constrained systems.
 */
#ifndef XY_ISO7816_CFG_ATR_MAX_LEN
#define XY_ISO7816_CFG_ATR_MAX_LEN 33
#endif

/**
 * @brief Maximum APDU command length (bytes)
 *
 * Standard: 5 (header) + 255 (data) + 1 (Le) = 261 bytes
 * For extended APDU: can be up to 65539 bytes
 */
#ifndef XY_ISO7816_CFG_APDU_MAX_LEN
#define XY_ISO7816_CFG_APDU_MAX_LEN 261
#endif

/**
 * @brief Maximum APDU response data length (bytes)
 *
 * Standard: 256 (data) + 2 (SW1/SW2) = 258 bytes
 */
#ifndef XY_ISO7816_CFG_RESPONSE_MAX_LEN
#define XY_ISO7816_CFG_RESPONSE_MAX_LEN 258
#endif

/* ========================================================================== */
/* Timeout Configuration */
/* ========================================================================== */

/**
 * @brief Default command timeout (milliseconds)
 *
 * Used for most APDU commands.
 * Increase if dealing with slow cards.
 */
#ifndef XY_ISO7816_CFG_DEFAULT_TIMEOUT
#define XY_ISO7816_CFG_DEFAULT_TIMEOUT 1000
#endif

/**
 * @brief ATR timeout (milliseconds)
 *
 * ISO7816-3 specifies 20 seconds max for ATR.
 * Do not reduce below specification.
 */
#ifndef XY_ISO7816_CFG_ATR_TIMEOUT
#define XY_ISO7816_CFG_ATR_TIMEOUT 20000
#endif

/**
 * @brief Byte-to-byte timeout (milliseconds)
 *
 * Timeout between consecutive bytes in a transmission.
 */
#ifndef XY_ISO7816_CFG_BYTE_TIMEOUT
#define XY_ISO7816_CFG_BYTE_TIMEOUT 100
#endif

/**
 * @brief Reset pulse duration (milliseconds)
 *
 * Duration of the reset signal to the card.
 * Typical range: 50-200 ms
 */
#ifndef XY_ISO7816_CFG_RESET_DURATION
#define XY_ISO7816_CFG_RESET_DURATION 100
#endif

/* ========================================================================== */
/* Feature Configuration */
/* ========================================================================== */

/**
 * @brief Enable automatic GET RESPONSE handling
 *
 * When enabled, automatically issues GET RESPONSE commands
 * when receiving SW1=0x61 (more data available).
 */
#ifndef XY_ISO7816_CFG_AUTO_GET_RESPONSE
#define XY_ISO7816_CFG_AUTO_GET_RESPONSE 1
#endif

/**
 * @brief Enable debug logging
 *
 * When enabled, logs APDU commands and responses for debugging.
 * Requires printf or similar logging infrastructure.
 */
#ifndef XY_ISO7816_CFG_DEBUG_LOG
#define XY_ISO7816_CFG_DEBUG_LOG 0
#endif

/**
 * @brief Enable ATR parsing and validation
 *
 * When enabled, validates ATR structure and extracts protocol info.
 * Disable to save code space if ATR validation is not needed.
 */
#ifndef XY_ISO7816_CFG_ENABLE_ATR_PARSE
#define XY_ISO7816_CFG_ENABLE_ATR_PARSE 1
#endif

/**
 * @brief Enable SIM-specific operations
 *
 * When enabled, includes SIM/USIM card specific functions
 * (ICCID, IMSI, PIN verification, authentication).
 */
#ifndef XY_ISO7816_CFG_ENABLE_SIM_OPS
#define XY_ISO7816_CFG_ENABLE_SIM_OPS 1
#endif

/**
 * @brief Enable utility functions
 *
 * When enabled, includes BCD conversion and other utilities.
 */
#ifndef XY_ISO7816_CFG_ENABLE_UTILS
#define XY_ISO7816_CFG_ENABLE_UTILS 1
#endif

/* ========================================================================== */
/* UART Configuration Defaults */
/* ========================================================================== */

/**
 * @brief Default UART baudrate for ISO7816
 *
 * Standard ISO7816 uses 9600 bps initially.
 * Can be increased after ATR negotiation.
 */
#ifndef XY_ISO7816_CFG_DEFAULT_BAUDRATE
#define XY_ISO7816_CFG_DEFAULT_BAUDRATE 9600
#endif

/**
 * @brief Default UART word length
 *
 * ISO7816 uses 8 data bits.
 */
#ifndef XY_ISO7816_CFG_DEFAULT_WORDLEN
#define XY_ISO7816_CFG_DEFAULT_WORDLEN 8
#endif

/**
 * @brief Default UART stop bits
 *
 * ISO7816 uses 2 stop bits (or 1.5 for some implementations).
 */
#ifndef XY_ISO7816_CFG_DEFAULT_STOPBITS
#define XY_ISO7816_CFG_DEFAULT_STOPBITS 2
#endif

/**
 * @brief Default UART parity
 *
 * ISO7816 uses even parity.
 * 0 = None, 1 = Even, 2 = Odd
 */
#ifndef XY_ISO7816_CFG_DEFAULT_PARITY
#define XY_ISO7816_CFG_DEFAULT_PARITY 1 /* Even */
#endif

/* ========================================================================== */
/* Memory Optimization */
/* ========================================================================== */

/**
 * @brief Use static buffers instead of dynamic allocation
 *
 * When enabled, uses static buffers for APDU processing.
 * Useful for embedded systems without dynamic memory.
 */
#ifndef XY_ISO7816_CFG_STATIC_BUFFERS
#define XY_ISO7816_CFG_STATIC_BUFFERS 1
#endif

/**
 * @brief Maximum number of concurrent ISO7816 handles
 *
 * Only used when XY_ISO7816_CFG_STATIC_BUFFERS is enabled.
 */
#ifndef XY_ISO7816_CFG_MAX_HANDLES
#define XY_ISO7816_CFG_MAX_HANDLES 2
#endif

/* ========================================================================== */
/* Error Handling Configuration */
/* ========================================================================== */

/**
 * @brief Enable detailed error codes
 *
 * When enabled, provides more specific error codes.
 * Disable to reduce code size if generic errors are sufficient.
 */
#ifndef XY_ISO7816_CFG_DETAILED_ERRORS
#define XY_ISO7816_CFG_DETAILED_ERRORS 1
#endif

/**
 * @brief Enable error callbacks
 *
 * When enabled, allows registering error callback functions.
 */
#ifndef XY_ISO7816_CFG_ERROR_CALLBACK
#define XY_ISO7816_CFG_ERROR_CALLBACK 0
#endif

/* ========================================================================== */
/* Platform-Specific Configuration */
/* ========================================================================== */

/**
 * @brief Enable hardware flow control
 *
 * Some platforms support hardware flow control for ISO7816.
 */
#ifndef XY_ISO7816_CFG_HW_FLOW_CONTROL
#define XY_ISO7816_CFG_HW_FLOW_CONTROL 0
#endif

/**
 * @brief Enable DMA for UART transfers
 *
 * When enabled and supported by HAL, uses DMA for data transfers.
 */
#ifndef XY_ISO7816_CFG_USE_DMA
#define XY_ISO7816_CFG_USE_DMA 0
#endif

/* ========================================================================== */
/* Validation */
/* ========================================================================== */

/* Ensure at least one protocol is enabled */
#if !XY_ISO7816_ENABLE_T0 && !XY_ISO7816_ENABLE_T1
#error "At least one protocol (T=0 or T=1) must be enabled"
#endif

/* Validate buffer sizes */
#if XY_ISO7816_CFG_ATR_MAX_LEN < 2
#error "XY_ISO7816_CFG_ATR_MAX_LEN must be at least 2"
#endif

#if XY_ISO7816_CFG_APDU_MAX_LEN < 5
#error "XY_ISO7816_CFG_APDU_MAX_LEN must be at least 5"
#endif

#ifdef __cplusplus
}
#endif

#endif /* XY_ISO7816_CFG_H */
