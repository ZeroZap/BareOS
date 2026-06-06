/**
 * @file xy_secboot_crypto.h
 * @brief Algorithm abstraction for secure boot and protected assets
 * @version 0.1.0
 */

#ifndef XY_SECBOOT_CRYPTO_H
#define XY_SECBOOT_CRYPTO_H

#include <stdint.h>
#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Common Limits ==================== */

#define XY_SECBOOT_HASH_MAX_SIZE       64u
#define XY_SECBOOT_SIG_MAX_SIZE        128u
#define XY_SECBOOT_MAC_MAX_SIZE        64u
#define XY_SECBOOT_KEY_ID_MAX_SIZE     16u
#define XY_SECBOOT_NONCE_MAX_SIZE      16u
#define XY_SECBOOT_HASH_CTX_MAX_SIZE   256u
#define XY_SECBOOT_MANIFEST_MAGIC      0x54425358u /* 'XSBT' */
#define XY_SECBOOT_MANIFEST_VERSION    1u

/* ==================== Status Codes ==================== */

typedef enum {
    XY_SECBOOT_OK = 0,
    XY_SECBOOT_ERR_PARAM = -1,
    XY_SECBOOT_ERR_VERIFY = -2,
    XY_SECBOOT_ERR_AUTH = -3,
    XY_SECBOOT_ERR_ROLLBACK = -4,
    XY_SECBOOT_ERR_UNSUPPORTED = -5,
    XY_SECBOOT_ERR_HW = -6,
    XY_SECBOOT_ERR_STATE = -7,
} xy_secboot_status_t;

/* ==================== Algorithm IDs ==================== */

typedef enum {
    XY_SECBOOT_ALG_NONE = 0,

    XY_SECBOOT_ALG_HASH_SHA256,
    XY_SECBOOT_ALG_HASH_BLAKE2S_256,
    XY_SECBOOT_ALG_HASH_SM3,

    XY_SECBOOT_ALG_SIG_ED25519,
    XY_SECBOOT_ALG_SIG_ECDSA_P256,
    XY_SECBOOT_ALG_SIG_SM2,

    XY_SECBOOT_ALG_MAC_HMAC_SHA256,
    XY_SECBOOT_ALG_MAC_BLAKE2S_KEYED,
    XY_SECBOOT_ALG_MAC_HMAC_SM3,

    XY_SECBOOT_ALG_CIPHER_CHACHA20,
    XY_SECBOOT_ALG_CIPHER_AES_CTR,
    XY_SECBOOT_ALG_CIPHER_SM4_CTR,
    XY_SECBOOT_ALG_CIPHER_SM4_CBC,

    XY_SECBOOT_ALG_AEAD_CHACHA20_POLY1305,
    XY_SECBOOT_ALG_AEAD_AES_GCM,
    XY_SECBOOT_ALG_AEAD_SM4_GCM,
    XY_SECBOOT_ALG_AEAD_SM4_CBC_HMAC_SM3,
} xy_secboot_alg_t;

typedef enum {
    XY_SECBOOT_KEY_BOOT_PUBLIC = 0,
    XY_SECBOOT_KEY_BOOT_MAC,
    XY_SECBOOT_KEY_MODEL_ENC,
    XY_SECBOOT_KEY_MODEL_MAC,
    XY_SECBOOT_KEY_TRANSPORT,
    XY_SECBOOT_KEY_OUTPUT_MAC,
    XY_SECBOOT_KEY_DEVICE_ROOT,
} xy_secboot_key_type_t;

typedef enum {
    XY_SECBOOT_IMAGE_APP = 0,
    XY_SECBOOT_IMAGE_BOOT_PATCH,
    XY_SECBOOT_IMAGE_MODEL,
    XY_SECBOOT_IMAGE_PARAM,
    XY_SECBOOT_IMAGE_FACTORY,
} xy_secboot_image_type_t;

/* ==================== Suite Description ==================== */

typedef struct {
    uint32_t suite_id;
    xy_secboot_alg_t hash_alg;
    xy_secboot_alg_t sig_alg;
    xy_secboot_alg_t firmware_mac_alg;
    xy_secboot_alg_t model_cipher_alg;
    xy_secboot_alg_t model_mac_alg;
    xy_secboot_alg_t transport_aead_alg;
    xy_secboot_alg_t output_mac_alg;
    uint8_t hash_size;
    uint8_t sig_size;
    uint8_t mac_size;
    uint8_t nonce_size;
    uint8_t require_anti_rollback;
    uint8_t require_constant_time;
    uint8_t require_canary;
    uint8_t require_double_run;
} xy_secboot_suite_t;

/* ==================== Manifest Wire Format ==================== */

typedef struct {
    uint32_t magic;
    uint16_t header_version;
    uint16_t header_len;
    uint32_t product_id;
    uint32_t image_type;
    uint32_t image_addr;
    uint32_t image_size;
    uint32_t entry_addr;
    uint32_t image_version;
    uint32_t min_boot_version;
    uint32_t security_counter;
    uint8_t key_id[XY_SECBOOT_KEY_ID_MAX_SIZE];
    uint8_t nonce[XY_SECBOOT_NONCE_MAX_SIZE];
    uint8_t image_hash[XY_SECBOOT_HASH_MAX_SIZE];
    uint8_t signature[XY_SECBOOT_SIG_MAX_SIZE];
    uint32_t header_crc32;
} xy_secboot_manifest_t;

typedef struct {
    uint32_t device_id;
    uint32_t message_type;
    uint32_t boot_counter;
    uint32_t output_counter;
    uint32_t timestamp;
    uint32_t payload_len;
} xy_secboot_output_aad_t;

/* ==================== Crypto Backend Operations ==================== */

typedef struct {
    int (*init)(void);

    int (*hash_init)(void *ctx, xy_secboot_alg_t alg);
    int (*hash_update)(void *ctx, const uint8_t *data, size_t len);
    int (*hash_final)(void *ctx, uint8_t *digest, size_t *digest_len);

    int (*verify)(xy_secboot_alg_t alg,
                  const uint8_t *pub_key, size_t pub_key_len,
                  const uint8_t *msg, size_t msg_len,
                  const uint8_t *sig, size_t sig_len);

    int (*mac)(xy_secboot_alg_t alg,
               const uint8_t *key, size_t key_len,
               const uint8_t *data, size_t data_len,
               uint8_t *tag, size_t *tag_len);

    int (*cipher_crypt)(xy_secboot_alg_t alg,
                        const uint8_t *key, size_t key_len,
                        const uint8_t *nonce, size_t nonce_len,
                        const uint8_t *input, uint8_t *output,
                        size_t len);

    int (*aead_decrypt)(xy_secboot_alg_t alg,
                        const uint8_t *key, size_t key_len,
                        const uint8_t *nonce, size_t nonce_len,
                        const uint8_t *aad, size_t aad_len,
                        const uint8_t *ciphertext, size_t ciphertext_len,
                        const uint8_t *tag, size_t tag_len,
                        uint8_t *plaintext);

    int (*get_key)(xy_secboot_key_type_t type,
                   const uint8_t *key_id, size_t key_id_len,
                   uint8_t *key, size_t *key_len);

    int (*rollback_read)(uint32_t *counter);
    int (*rollback_write)(uint32_t counter);

    int (*ct_compare)(const uint8_t *a, const uint8_t *b, size_t len);
    void (*secure_zero)(void *buf, size_t len);
    int (*canary_check)(void);
} xy_secboot_crypto_ops_t;

const xy_secboot_suite_t *xy_secboot_get_suite(void);
void xy_secboot_set_crypto_ops(const xy_secboot_crypto_ops_t *ops);
const xy_secboot_crypto_ops_t *xy_secboot_get_crypto_ops(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_SECBOOT_CRYPTO_H */
