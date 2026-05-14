/**
 * @file xy_sha256.h
 * @brief SHA256 Hash Interface
 * @version 1.0.0
 * @date 2026-03-02
 */

#ifndef XY_SHA256_H
#define XY_SHA256_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SHA256 上下文
 */
typedef struct {
    uint32_t h[8];          /* 哈希值 */
    uint64_t total;         /* 总字节数 */
    uint8_t buffer[64];     /* 数据缓冲区 */
    size_t buflen;          /* 缓冲区长度 */
} xy_sha256_ctx_t;

/**
 * @brief SHA256 哈希长度
 */
#define XY_SHA256_HASH_SIZE   32

/**
 * @brief 初始化 SHA256
 * @param ctx SHA256 上下文
 */
void xy_sha256_init(xy_sha256_ctx_t *ctx);

/**
 * @brief SHA256 更新
 * @param ctx SHA256 上下文
 * @param data 输入数据
 * @param len 数据长度
 */
void xy_sha256_update(xy_sha256_ctx_t *ctx, const uint8_t *data, size_t len);

/**
 * @brief SHA256 完成
 * @param ctx SHA256 上下文
 * @param hash 输出哈希 (32 字节)
 */
void xy_sha256_finish(xy_sha256_ctx_t *ctx, uint8_t *hash);

/**
 * @brief 计算 SHA256 哈希
 * @param data 输入数据
 * @param len 数据长度
 * @param hash 输出哈希 (32 字节)
 * @return 0 成功，-1 失败
 */
int xy_sha256(const uint8_t *data, size_t len, uint8_t *hash);

#ifdef __cplusplus
}
#endif

#endif /* XY_SHA256_H */
