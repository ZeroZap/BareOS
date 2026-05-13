#include "xy_fota_flash.h"
/**
 * @file xy_fota_secure.h
 * @brief Secure FOTA with MCUboot + WireGuard Style Encryption
 * @version 2.0.0
 * @date 2026-03-02
 */

#ifndef XY_FOTA_SECURE_H
#define XY_FOTA_SECURE_H

#include <stdint.h>
#include <stdbool.h>
#include "xy_fota.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 固件包魔数
 */
#define XY_FOTA_SECURE_MAGIC        0x58494E59  /* "XINY" */

/**
 * @brief 加密算法常量
 */
#define XY_FOTA_ECDSA_P256_SIG_SIZE 64
#define XY_FOTA_CHACHA20_NONCE_SIZE 12
#define XY_FOTA_POLY1305_TAG_SIZE   16
#define XY_FOTA_PUB_KEY_SIZE        64

/**
 * @brief 固件包头
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;                 /* 魔数 */
    uint32_t version;               /* 版本号 */
    uint32_t fw_size;               /* 固件大小 */
    uint32_t timestamp;             /* 时间戳 */
    uint8_t ecdsa_sig[64];          /* ECDSA P-256 签名 */
    uint8_t chacha_nonce[12];       /* ChaCha20 Nonce */
    uint8_t reserved[164];          /* 保留 */
} xy_fota_secure_header_t;

/**
 * @brief 安全 FOTA 配置
 */
typedef struct {
    const uint8_t *pub_key;         /* 公钥 (64 字节) */
    uint32_t slot0_addr;            /* Slot 0 地址 */
    uint32_t slot1_addr;            /* Slot 1 地址 */
    uint32_t slot_size;             /* 每个 Slot 大小 */
    bool dual_bank;                 /* 双 Bank 模式 */
} xy_fota_secure_config_t;

/**
 * @brief 安全 FOTA 句柄
 */
typedef struct {
    xy_fota_secure_config_t config; /* 配置 */
    xy_fota_secure_header_t header; /* 当前固件头 */
    xy_fota_flash_ops_t *flash;     /* Flash 操作接口 */
    uint32_t decrypted_size;        /* 已解密大小 */
    uint32_t total_size;            /* 总大小 */
    uint8_t chacha_key[32];         /* ChaCha20 密钥 */
    bool verified;                  /* 已验证 */
    bool initialized;               /* 已初始化 */
} xy_fota_secure_t;

/**
 * @brief 初始化安全 FOTA
 * @param fota 安全 FOTA 句柄
 * @param config 配置
 * @param flash Flash 操作接口
 * @return XY_FOTA_OK 成功，其他值失败
 */
int xy_fota_secure_init(xy_fota_secure_t *fota,
                        const xy_fota_secure_config_t *config,
                        xy_fota_flash_ops_t *flash);

/**
 * @brief 反初始化
 * @param fota 安全 FOTA 句柄
 * @return XY_FOTA_OK 成功，其他值失败
 */
int xy_fota_secure_deinit(xy_fota_secure_t *fota);

/**
 * @brief 验证固件包
 * @param fota 安全 FOTA 句柄
 * @param fw_pkg 固件包数据
 * @param pkg_size 固件包大小
 * @return XY_FOTA_OK 成功，其他值失败
 */
int xy_fota_secure_verify(xy_fota_secure_t *fota,
                          const uint8_t *fw_pkg,
                          uint32_t pkg_size);

/**
 * @brief 解密并写入固件
 * @param fota 安全 FOTA 句柄
 * @param encrypted_data 加密数据
 * @param data_size 数据大小
 * @param offset 偏移量
 * @return XY_FOTA_OK 成功，其他值失败
 */
int xy_fota_secure_decrypt_and_write(xy_fota_secure_t *fota,
                                     const uint8_t *encrypted_data,
                                     uint32_t data_size,
                                     uint32_t offset);

/**
 * @brief 验证并解密整个固件包
 * @param fota 安全 FOTA 句柄
 * @param fw_pkg 固件包
 * @param pkg_size 固件包大小
 * @param output 输出缓冲区
 * @param output_size 输出缓冲区大小
 * @return XY_FOTA_OK 成功，其他值失败
 */
int xy_fota_secure_verify_and_decrypt(xy_fota_secure_t *fota,
                                      const uint8_t *fw_pkg,
                                      uint32_t pkg_size,
                                      uint8_t *output,
                                      uint32_t output_size);

/**
 * @brief 交换 Slot (双 Bank 模式)
 * @param fota 安全 FOTA 句柄
 * @return XY_FOTA_OK 成功，其他值失败
 */
int xy_fota_secure_swap(xy_fota_secure_t *fota);

/**
 * @brief 标记 Slot 为有效
 * @param fota 安全 FOTA 句柄
 * @param slot Slot 编号 (0/1)
 * @return XY_FOTA_OK 成功，其他值失败
 */
int xy_fota_secure_mark_valid(xy_fota_secure_t *fota, uint8_t slot);

/**
 * @brief 检查 Slot 是否有效
 * @param fota 安全 FOTA 句柄
 * @param slot Slot 编号 (0/1)
 * @param valid 有效性指针
 * @return XY_FOTA_OK 成功，其他值失败
 */
int xy_fota_secure_is_valid(xy_fota_secure_t *fota, uint8_t slot, bool *valid);

/**
 * @brief ECDSA P-256 签名验证
 * @param pub_key 公钥 (64 字节)
 * @param message 消息数据
 * @param msg_size 消息大小
 * @param signature 签名 (64 字节)
 * @return 0 成功，-1 失败
 */
int xy_fota_ecdsa_verify(const uint8_t *pub_key,
                         const uint8_t *message,
                         uint32_t msg_size,
                         const uint8_t *signature);

/**
 * @brief ChaCha20-Poly1305 解密
 * @param key ChaCha20 密钥 (32 字节)
 * @param nonce Nonce (12 字节)
 * @param ciphertext 密文
 * @param ct_len 密文长度
 * @param plaintext 明文输出
 * @param tag Poly1305 Tag (16 字节)
 * @return 0 成功，-1 失败
 */
int xy_fota_chacha20_decrypt(const uint8_t *key,
                             const uint8_t *nonce,
                             const uint8_t *ciphertext,
                             uint32_t ct_len,
                             uint8_t *plaintext,
                             const uint8_t *tag);

#ifdef __cplusplus
}
#endif

#endif
