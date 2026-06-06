/**
 * @file xy_secboot_policy.c
 * @brief Secure boot suite policy selection
 */

#include "xy_secboot_config.h"
#include "xy_secboot_crypto.h"

static const xy_secboot_crypto_ops_t *s_crypto_ops = NULL;

static const xy_secboot_suite_t s_suite_minimal = {
    XY_SECBOOT_SUITE_MINIMAL,
    XY_SECBOOT_ALG_HASH_BLAKE2S_256,
    XY_SECBOOT_ALG_NONE,
    XY_SECBOOT_ALG_MAC_BLAKE2S_KEYED,
    XY_SECBOOT_ALG_NONE,
    XY_SECBOOT_ALG_NONE,
    XY_SECBOOT_ALG_NONE,
    XY_SECBOOT_ALG_NONE,
    32u,
    0u,
    32u,
    0u,
    1u,
    1u,
    1u,
    0u,
};

static const xy_secboot_suite_t s_suite_modern = {
    XY_SECBOOT_SUITE_MODERN,
    XY_SECBOOT_ALG_HASH_BLAKE2S_256,
    XY_SECBOOT_ALG_SIG_ED25519,
    XY_SECBOOT_ALG_NONE,
    XY_SECBOOT_ALG_CIPHER_CHACHA20,
    XY_SECBOOT_ALG_MAC_BLAKE2S_KEYED,
    XY_SECBOOT_ALG_AEAD_CHACHA20_POLY1305,
    XY_SECBOOT_ALG_MAC_BLAKE2S_KEYED,
    32u,
    64u,
    32u,
    12u,
    1u,
    1u,
    1u,
    0u,
};

static const xy_secboot_suite_t s_suite_market = {
    XY_SECBOOT_SUITE_MARKET,
    XY_SECBOOT_ALG_HASH_SHA256,
    XY_SECBOOT_ALG_SIG_ECDSA_P256,
    XY_SECBOOT_ALG_NONE,
    XY_SECBOOT_ALG_CIPHER_AES_CTR,
    XY_SECBOOT_ALG_MAC_HMAC_SHA256,
    XY_SECBOOT_ALG_AEAD_AES_GCM,
    XY_SECBOOT_ALG_MAC_HMAC_SHA256,
    32u,
    64u,
    32u,
    12u,
    1u,
    1u,
    1u,
    0u,
};

static const xy_secboot_suite_t s_suite_gm = {
    XY_SECBOOT_SUITE_GM,
    XY_SECBOOT_ALG_HASH_SM3,
    XY_SECBOOT_ALG_SIG_SM2,
    XY_SECBOOT_ALG_NONE,
    XY_SECBOOT_ALG_CIPHER_SM4_CTR,
    XY_SECBOOT_ALG_MAC_HMAC_SM3,
    XY_SECBOOT_ALG_AEAD_SM4_GCM,
    XY_SECBOOT_ALG_MAC_HMAC_SM3,
    32u,
    64u,
    32u,
    12u,
    1u,
    1u,
    1u,
    0u,
};

static const xy_secboot_suite_t s_suite_plb_gm = {
    XY_SECBOOT_SUITE_PLB_GM,
    XY_SECBOOT_ALG_HASH_SM3,
    XY_SECBOOT_ALG_SIG_SM2,
    XY_SECBOOT_ALG_NONE,
    XY_SECBOOT_ALG_NONE,
    XY_SECBOOT_ALG_MAC_HMAC_SM3,
    XY_SECBOOT_ALG_AEAD_SM4_GCM,
    XY_SECBOOT_ALG_MAC_HMAC_SM3,
    32u,
    64u,
    32u,
    12u,
    1u,
    1u,
    1u,
    0u,
};

static const xy_secboot_suite_t s_suite_model_secure = {
    XY_SECBOOT_SUITE_MODEL_SECURE,
    XY_SECBOOT_ALG_HASH_BLAKE2S_256,
    XY_SECBOOT_ALG_SIG_ED25519,
    XY_SECBOOT_ALG_NONE,
    XY_SECBOOT_ALG_CIPHER_CHACHA20,
    XY_SECBOOT_ALG_MAC_BLAKE2S_KEYED,
    XY_SECBOOT_ALG_AEAD_CHACHA20_POLY1305,
    XY_SECBOOT_ALG_MAC_BLAKE2S_KEYED,
    32u,
    64u,
    32u,
    12u,
    1u,
    1u,
    1u,
    1u,
};

const xy_secboot_suite_t *xy_secboot_get_suite(void)
{
#if XY_SECBOOT_SUITE == XY_SECBOOT_SUITE_MINIMAL
    return &s_suite_minimal;
#elif XY_SECBOOT_SUITE == XY_SECBOOT_SUITE_MODERN
    return &s_suite_modern;
#elif XY_SECBOOT_SUITE == XY_SECBOOT_SUITE_MARKET
    return &s_suite_market;
#elif XY_SECBOOT_SUITE == XY_SECBOOT_SUITE_GM
    return &s_suite_gm;
#elif XY_SECBOOT_SUITE == XY_SECBOOT_SUITE_MODEL_SECURE
    return &s_suite_model_secure;
#else
    return &s_suite_plb_gm;
#endif
}

void xy_secboot_set_crypto_ops(const xy_secboot_crypto_ops_t *ops)
{
    s_crypto_ops = ops;
}

const xy_secboot_crypto_ops_t *xy_secboot_get_crypto_ops(void)
{
    return s_crypto_ops;
}
