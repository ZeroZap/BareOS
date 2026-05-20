/**
 * @file xy_rng.h
 * @brief Random Number Generation API
 * @version 1.0.0
 * @date 2025-11-01
 *
 * This header provides both simple (non-cryptographic) and cryptographically
 * secure random number generation capabilities.
 */

#ifndef XY_RNG_H
#define XY_RNG_H

#include <stdint.h>
#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Error Codes ==================== */
#define XY_RNG_SUCCESS          0
#define XY_RNG_ERROR           -1
#define XY_RNG_INVALID_PARAM   -2
#define XY_RNG_NOT_INITIALIZED -3

/* ==================== Simple RNG (Non-Cryptographic) ==================== */

/**
 * @brief Generate random bytes using simple LCG
 *
 * WARNING: NOT cryptographically secure. Use only for testing/simulation.
 * Falls back to system RNG when available (RtlGenRandom on Windows,
 * /dev/urandom on Unix).
 *
 * @param buffer Output buffer
 * @param len Number of bytes to generate
 * @return 0 on success, negative on error
 */
int xy_random_bytes(uint8_t *buffer, size_t len);

/**
 * @brief Generate a random 32-bit unsigned integer
 *
 * WARNING: NOT cryptographically secure.
 *
 * @return Random uint32_t value
 */
uint32_t xy_random_uint32(void);

/* ==================== CSPRNG (Cryptographically Secure) ==================== */

/**
 * @brief Initialize CSPRNG with seed material
 *
 * Must be called before using any CSPRNG functions.
 *
 * @param seed Seed data (minimum 32 bytes recommended)
 * @param seed_len Length of seed data
 * @return 0 on success, negative on error
 *
 * @note Use high-quality entropy sources:
 *       - Hardware RNG (TRNG)
 *       - System entropy pools
 *       - Environmental noise (ADC, timing jitter)
 */
int xy_csprng_init(const uint8_t *seed, size_t seed_len);

/**
 * @brief Reseed CSPRNG with additional entropy
 *
 * Recommended to call periodically (every 1 MB of output) or when
 * additional entropy becomes available.
 *
 * @param entropy Additional entropy data
 * @param entropy_len Length of entropy data
 * @return 0 on success, negative on error
 */
int xy_csprng_reseed(const uint8_t *entropy, size_t entropy_len);

/**
 * @brief Generate cryptographically secure random bytes
 *
 * @param output Output buffer
 * @param output_len Number of bytes to generate
 * @return 0 on success, negative on error
 *
 * @note Automatically warns when 1 MB threshold is reached.
 *       Consider reseeding at that point.
 */
int xy_csprng_generate(uint8_t *output, size_t output_len);

/**
 * @brief Generate a random 32-bit unsigned integer
 *
 * @return Random uint32_t value (cryptographically secure)
 */
uint32_t xy_csprng_uint32(void);

/**
 * @brief Generate a random 64-bit unsigned integer
 *
 * @return Random uint64_t value (cryptographically secure)
 */
uint64_t xy_csprng_uint64(void);

/**
 * @brief Generate uniform random number in range [0, upper_bound)
 *
 * Uses rejection sampling to avoid modulo bias, ensuring uniform
 * distribution across the entire range.
 *
 * @param upper_bound Exclusive upper bound (must be > 0)
 * @return Random value in [0, upper_bound)
 */
uint32_t xy_csprng_uniform(uint32_t upper_bound);

/**
 * @brief Securely cleanup CSPRNG state
 *
 * Zeros all internal state. Must call xy_csprng_init() again before use.
 */
void xy_csprng_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_RNG_H */
