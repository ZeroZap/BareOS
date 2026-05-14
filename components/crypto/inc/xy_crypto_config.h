/**
 * @file xy_crypto_config.h
 * @brief Configuration file for xy_tiny_crypto library
 *
 * This file allows users to choose between pure software implementation
 * and hardware-accelerated (HAL-based) cryptographic operations.
 */

#ifndef XY_CRYPTO_CONFIG_H
#define XY_CRYPTO_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Platform Selection ==================== */

/**
 * @brief Platform selection for cryptographic operations
 *
 * Choose one of the following platforms:
 * - XY_CRYPTO_PLATFORM_SOFTWARE: Pure C software implementation (default)
 * - XY_CRYPTO_PLATFORM_HAL:      Hardware acceleration via HAL interface
 * - XY_CRYPTO_PLATFORM_SDK:      Platform SDK (e.g., STM32, ESP32, Nordic)
 */
#ifndef XY_CRYPTO_PLATFORM
#define XY_CRYPTO_PLATFORM_SOFTWARE 0
#define XY_CRYPTO_PLATFORM_HAL      1
#define XY_CRYPTO_PLATFORM_SDK      2

// Default to software implementation
#define XY_CRYPTO_PLATFORM XY_CRYPTO_PLATFORM_SOFTWARE
#endif

/* ==================== Software Implementation Options ==================== */

/**
 * @brief Enable software division for platforms without hardware divide
 *
 * When enabled, crypto algorithms will use software-based division
 * compatible with XY_USE_SOFT_DIV option from xy_clib.
 *
 * 0 = Use hardware division
 * 1 = Use software division (shift and multiply approximations)
 */
#ifndef XY_CRYPTO_USE_SOFT_DIV
#ifdef XY_USE_SOFT_DIV
#define XY_CRYPTO_USE_SOFT_DIV XY_USE_SOFT_DIV
#else
#define XY_CRYPTO_USE_SOFT_DIV 0
#endif
#endif

/**
 * @brief Enable optimized software implementations
 *
 * When enabled, use optimized algorithms for better performance
 * at the cost of slightly larger code size.
 */
#ifndef XY_CRYPTO_OPTIMIZE_SPEED
#define XY_CRYPTO_OPTIMIZE_SPEED 1
#endif

/* ==================== Hardware Acceleration Options ==================== */

/**
 * @brief Enable AES hardware acceleration
 *
 * Requires HAL or SDK support for AES operations.
 */
#ifndef XY_CRYPTO_HW_AES
#define XY_CRYPTO_HW_AES 0
#endif

/**
 * @brief Enable SHA hardware acceleration
 *
 * Requires HAL or SDK support for SHA operations.
 */
#ifndef XY_CRYPTO_HW_SHA
#define XY_CRYPTO_HW_SHA 0
#endif

/**
 * @brief Enable CRC hardware acceleration
 *
 * Requires HAL or SDK support for CRC operations.
 */
#ifndef XY_CRYPTO_HW_CRC
#define XY_CRYPTO_HW_CRC 0
#endif

/**
 * @brief Enable RNG hardware acceleration
 *
 * Requires HAL or SDK support for hardware random number generator.
 */
#ifndef XY_CRYPTO_HW_RNG
#define XY_CRYPTO_HW_RNG 0
#endif

/* ==================== Feature Enablement ==================== */

/**
 * @brief Enable MD5 algorithm
 * Set to 0 to exclude MD5 from compilation
 */
#ifndef XY_CRYPTO_ENABLE_MD5
#define XY_CRYPTO_ENABLE_MD5 1
#endif

/**
 * @brief Enable SHA1 algorithm
 * Set to 0 to exclude SHA1 from compilation
 */
#ifndef XY_CRYPTO_ENABLE_SHA1
#define XY_CRYPTO_ENABLE_SHA1 1
#endif

/**
 * @brief Enable SHA256 algorithm
 * Set to 0 to exclude SHA256 from compilation
 */
#ifndef XY_CRYPTO_ENABLE_SHA256
#define XY_CRYPTO_ENABLE_SHA256 1
#endif

/**
 * @brief Enable AES algorithm
 * Set to 0 to exclude AES from compilation
 */
#ifndef XY_CRYPTO_ENABLE_AES
#define XY_CRYPTO_ENABLE_AES 1
#endif

/**
 * @brief Enable RSA algorithm
 * Set to 0 to exclude RSA from compilation (saves significant code space)
 */
#ifndef XY_CRYPTO_ENABLE_RSA
#define XY_CRYPTO_ENABLE_RSA 0
#endif

/**
 * @brief Enable HMAC functions
 * Set to 0 to exclude HMAC from compilation
 */
#ifndef XY_CRYPTO_ENABLE_HMAC
#define XY_CRYPTO_ENABLE_HMAC 1
#endif

/**
 * @brief Enable Curve25519 (X25519 + Ed25519)
 * Set to 0 to exclude both X25519 and Ed25519 from compilation
 * This provides unified implementation with shared field arithmetic
 */
#ifndef XY_CRYPTO_ENABLE_CURVE25519
#define XY_CRYPTO_ENABLE_CURVE25519 1
#endif

/* Legacy compatibility - individual enable flags */
#ifndef XY_CRYPTO_ENABLE_X25519
#define XY_CRYPTO_ENABLE_X25519 XY_CRYPTO_ENABLE_CURVE25519
#endif

#ifndef XY_CRYPTO_ENABLE_ED25519
#define XY_CRYPTO_ENABLE_ED25519 XY_CRYPTO_ENABLE_CURVE25519
#endif

/**
 * @brief Enable CSPRNG (Cryptographically Secure Pseudo-Random Number Generator)
 * Set to 0 to exclude ChaCha20-based CSPRNG from compilation
 *
 * This provides cryptographically secure random number generation suitable for:
 * - Generating ephemeral keys (X25519, Ed25519)
 * - Nonce and IV generation
 * - Random padding
 * - Session tokens
 */
#ifndef XY_CRYPTO_ENABLE_CSPRNG
#define XY_CRYPTO_ENABLE_CSPRNG 1
#endif

/* ==================== Platform-Specific Includes ==================== */

#if XY_CRYPTO_PLATFORM == XY_CRYPTO_PLATFORM_HAL
/* Include HAL crypto interface */
#include "xy_hal_crypto.h"
#elif XY_CRYPTO_PLATFORM == XY_CRYPTO_PLATFORM_SDK
/* Include platform SDK crypto headers */
#if defined(STM32_HAL)
#include "stm32_crypto.h"
#elif defined(ESP_PLATFORM)
#include "esp_crypto.h"
#elif defined(NRF_SDK)
#include "nrf_crypto.h"
#else
#warning "Platform SDK selected but not defined. Falling back to software."
#undef XY_CRYPTO_PLATFORM
#define XY_CRYPTO_PLATFORM XY_CRYPTO_PLATFORM_SOFTWARE
#endif
#endif

/* ==================== Debug and Validation ==================== */

/**
 * @brief Enable crypto operation validation
 *
 * When enabled, performs additional checks on crypto operations.
 * Disable in production for better performance.
 */
#ifndef XY_CRYPTO_ENABLE_VALIDATION
#define XY_CRYPTO_ENABLE_VALIDATION 1
#endif

/**
 * @brief Enable crypto debug output
 *
 * Requires a logging mechanism (e.g., xy_log)
 */
#ifndef XY_CRYPTO_ENABLE_DEBUG
#define XY_CRYPTO_ENABLE_DEBUG 0
#endif

#ifdef __cplusplus
}
#endif

#endif /* XY_CRYPTO_CONFIG_H */
