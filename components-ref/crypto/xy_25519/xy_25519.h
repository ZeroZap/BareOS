/**
 * @file xy_25519.h
 * @brief Unified Curve25519 library (X25519 ECDH + Ed25519 signatures)
 * @version 1.0.0
 * @date 2025-11-02
 *
 * This module provides both X25519 key exchange (RFC 7748) and Ed25519
 * digital signatures (RFC 8032) with shared field arithmetic operations.
 */

#ifndef XY_25519_H
#define XY_25519_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== X25519 ECDH ==================== */

/**
 * @brief Size of X25519 public key in bytes
 */
#define XY_X25519_PUBLIC_KEY_SIZE 32

/**
 * @brief Size of X25519 private key in bytes
 */
#define XY_X25519_PRIVATE_KEY_SIZE 32

/**
 * @brief Size of X25519 shared secret in bytes
 */
#define XY_X25519_SHARED_SECRET_SIZE 32

/* X25519 Error Codes */
#define XY_X25519_SUCCESS 0
#define XY_X25519_ERROR_INVALID_PARAM -1
#define XY_X25519_ERROR_WEAK_KEY -2
#define XY_X25519_ERROR -3

/**
 * @brief Generate X25519 key pair
 *
 * @param private_key Output buffer for private key (32 bytes)
 * @param public_key Output buffer for public key (32 bytes)
 * @return XY_X25519_SUCCESS on success, error code otherwise
 */
int xy_x25519_generate_keypair(uint8_t private_key[XY_X25519_PRIVATE_KEY_SIZE],
                                uint8_t public_key[XY_X25519_PUBLIC_KEY_SIZE]);

/**
 * @brief Compute X25519 public key from private key
 *
 * @param private_key Input private key (32 bytes)
 * @param public_key Output buffer for public key (32 bytes)
 * @return XY_X25519_SUCCESS on success, error code otherwise
 */
int xy_x25519_public_key(const uint8_t private_key[XY_X25519_PRIVATE_KEY_SIZE],
                          uint8_t public_key[XY_X25519_PUBLIC_KEY_SIZE]);

/**
 * @brief Compute X25519 shared secret
 *
 * @param shared_secret Output buffer for shared secret (32 bytes)
 * @param our_private_key Our private key (32 bytes)
 * @param their_public_key Their public key (32 bytes)
 * @return XY_X25519_SUCCESS on success, error code otherwise
 */
int xy_x25519_shared_secret(
    uint8_t shared_secret[XY_X25519_SHARED_SECRET_SIZE],
    const uint8_t our_private_key[XY_X25519_PRIVATE_KEY_SIZE],
    const uint8_t their_public_key[XY_X25519_PUBLIC_KEY_SIZE]);

/**
 * @brief Validate X25519 public key
 *
 * @param public_key Public key to validate (32 bytes)
 * @return XY_X25519_SUCCESS if valid, error code otherwise
 */
int xy_x25519_validate_public_key(
    const uint8_t public_key[XY_X25519_PUBLIC_KEY_SIZE]);

/* ==================== Ed25519 Signatures ==================== */

/**
 * @brief Size of Ed25519 public key in bytes
 */
#define XY_ED25519_PUBLIC_KEY_SIZE 32

/**
 * @brief Size of Ed25519 private key (seed) in bytes
 */
#define XY_ED25519_PRIVATE_KEY_SIZE 32

/**
 * @brief Size of Ed25519 signature in bytes
 */
#define XY_ED25519_SIGNATURE_SIZE 64

/**
 * @brief Size of Ed25519 seed in bytes
 */
#define XY_ED25519_SEED_SIZE 32

/* Ed25519 Error Codes */
#define XY_ED25519_SUCCESS 0
#define XY_ED25519_ERROR_INVALID_PARAM -1
#define XY_ED25519_ERROR_VERIFY_FAILED -2
#define XY_ED25519_ERROR_INVALID_SIGNATURE -3
#define XY_ED25519_ERROR -4

/**
 * @brief Generate Ed25519 key pair
 *
 * @param public_key Output buffer for public key (32 bytes)
 * @param private_key Output buffer for private key seed (32 bytes)
 * @return XY_ED25519_SUCCESS on success, error code otherwise
 */
int xy_ed25519_generate_keypair(
    uint8_t public_key[XY_ED25519_PUBLIC_KEY_SIZE],
    uint8_t private_key[XY_ED25519_PRIVATE_KEY_SIZE]);

/**
 * @brief Derive Ed25519 public key from private key seed
 *
 * @param private_key Input private key seed (32 bytes)
 * @param public_key Output buffer for public key (32 bytes)
 * @return XY_ED25519_SUCCESS on success, error code otherwise
 */
int xy_ed25519_public_key(
    const uint8_t private_key[XY_ED25519_PRIVATE_KEY_SIZE],
    uint8_t public_key[XY_ED25519_PUBLIC_KEY_SIZE]);

/**
 * @brief Sign a message using Ed25519
 *
 * @param signature Output buffer for signature (64 bytes)
 * @param message Message to sign
 * @param message_len Length of message in bytes
 * @param public_key Signer's public key (32 bytes)
 * @param private_key Signer's private key seed (32 bytes)
 * @return XY_ED25519_SUCCESS on success, error code otherwise
 */
int xy_ed25519_sign(uint8_t signature[XY_ED25519_SIGNATURE_SIZE],
                     const uint8_t *message,
                     size_t message_len,
                     const uint8_t public_key[XY_ED25519_PUBLIC_KEY_SIZE],
                     const uint8_t private_key[XY_ED25519_PRIVATE_KEY_SIZE]);

/**
 * @brief Verify an Ed25519 signature
 *
 * @param signature Signature to verify (64 bytes)
 * @param message Message that was signed
 * @param message_len Length of message in bytes
 * @param public_key Signer's public key (32 bytes)
 * @return XY_ED25519_SUCCESS if valid, error code otherwise
 */
int xy_ed25519_verify(const uint8_t signature[XY_ED25519_SIGNATURE_SIZE],
                       const uint8_t *message,
                       size_t message_len,
                       const uint8_t public_key[XY_ED25519_PUBLIC_KEY_SIZE]);

/**
 * @brief Sign a message (simple interface without public key)
 *
 * @param signature Output buffer for signature (64 bytes)
 * @param message Message to sign
 * @param message_len Length of message in bytes
 * @param private_key Signer's private key seed (32 bytes)
 * @return XY_ED25519_SUCCESS on success, error code otherwise
 */
int xy_ed25519_sign_simple(uint8_t signature[XY_ED25519_SIGNATURE_SIZE],
                            const uint8_t *message,
                            size_t message_len,
                            const uint8_t private_key[XY_ED25519_PRIVATE_KEY_SIZE]);

#ifdef __cplusplus
}
#endif

#endif
