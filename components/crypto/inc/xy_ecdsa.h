/**
 * @file xy_ecdsa.h
 * @brief ECDSA P-256 Signature Verification
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_ECDSA_H
#define XY_ECDSA_H

#include <stdint.h>
#include "xy_typedef.h"
#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 密钥长度
 */
#define XY_ECDSA_P256_KEY_SIZE      32
#define XY_ECDSA_P256_SIG_SIZE      64
#define XY_ECDSA_P256_PUB_KEY_SIZE  64

/**
 * @brief P-256 公钥
 */
typedef struct {
    uint8_t x[XY_ECDSA_P256_KEY_SIZE];
    uint8_t y[XY_ECDSA_P256_KEY_SIZE];
} xy_ecdsa_pub_key_t;

/**
 * @brief ECDSA 签名
 */
typedef struct {
    uint8_t r[XY_ECDSA_P256_KEY_SIZE];
    uint8_t s[XY_ECDSA_P256_KEY_SIZE];
} xy_ecdsa_sig_t;

/**
 * @brief 验证 ECDSA P-256 签名
 * @param pub_key 公钥 (64 字节)
 * @param message 消息数据
 * @param msg_len 消息长度
 * @param sig 签名 (64 字节)
 * @return 0 成功，-1 失败
 */
int xy_ecdsa_p256_verify(const xy_ecdsa_pub_key_t *pub_key,
                         const uint8_t *message, size_t msg_len,
                         const xy_ecdsa_sig_t *sig);

/**
 * @brief 简化版验证接口
 * @param pub_key 公钥 (64 字节)
 * @param message 消息数据
 * @param msg_len 消息长度
 * @param sig 签名 (64 字节)
 * @return 0 成功，-1 失败
 */
int xy_ecdsa_verify_simple(const uint8_t *pub_key,
                           const uint8_t *message, size_t msg_len,
                           const uint8_t *sig);

#ifdef __cplusplus
}
#endif

#endif
