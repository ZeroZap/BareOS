#include <stdio.h>
#include <string.h>
#include "xy_tiny_crypto.h"
#include <stdio.h>
#include <string.h>

// 示例：文件哈希计算器
void example_file_hash(void)
{
    printf("=== File Hash Example ===\n");

    const char *content = "This is a sample file content for hashing.";

    // Simulate file content
    printf("File content: %s\n", content);

    // 计算多种哈希
    uint8_t md5_hash[XY_MD5_DIGEST_SIZE];
    uint8_t sha256_hash[XY_SHA256_DIGEST_SIZE];

    xy_md5_hash((const uint8_t *)content, strlen(content), md5_hash);
    xy_sha256_hash((const uint8_t *)content, strlen(content), sha256_hash);

    printf("MD5:    ");
    for (int i = 0; i < XY_MD5_DIGEST_SIZE; i++) {
        printf("%02x", md5_hash[i]);
    }
    printf("\n");

    printf("SHA256: ");
    for (int i = 0; i < XY_SHA256_DIGEST_SIZE; i++) {
        printf("%02x", sha256_hash[i]);
    }
    printf("\n\n");
}

// 示例：消息认证
void example_message_auth(void)
{
    printf("=== 消息认证示例 ===\n");

    const char *secret_key = "my_secret_key_123";
    const char *message    = "Important message that needs authentication";

    uint8_t hmac_result[XY_SHA256_DIGEST_SIZE];

    if (xy_hmac_sha256((const uint8_t *)secret_key, strlen(secret_key),
                       (const uint8_t *)message, strlen(message), hmac_result)
        == XY_CRYPTO_SUCCESS) {

        printf("密钥: %s\n", secret_key);
        printf("消息: %s\n", message);
        printf("HMAC-SHA256: ");
        for (int i = 0; i < XY_SHA256_DIGEST_SIZE; i++) {
            printf("%02x", hmac_result[i]);
        }
        printf("\n\n");
    }
}

// 示例：数据加密传输
void example_data_encryption(void)
{
    printf("=== 数据加密传输示例 ===\n");

    // 原始数据
    const char *original_data = "Confidential Data!";
    printf("原始数据: %s\n", original_data);

    // AES 密钥和 IV
    uint8_t aes_key[XY_AES_KEY_SIZE_128] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae,
                                             0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88,
                                             0x09, 0xcf, 0x4f, 0x3c };

    uint8_t iv[XY_AES_BLOCK_SIZE] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                                      0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
                                      0x0c, 0x0d, 0x0e, 0x0f };

    // 准备数据（PKCS#7 填充）
    size_t data_len = strlen(original_data);
    size_t padded_len =
        ((data_len / XY_AES_BLOCK_SIZE) + 1) * XY_AES_BLOCK_SIZE;
    uint8_t padded_data[64];
    uint8_t encrypted_data[64];
    uint8_t decrypted_data[64];

    memcpy(padded_data, original_data, data_len);
    uint8_t padding = padded_len - data_len;
    for (size_t i = data_len; i < padded_len; i++) {
        padded_data[i] = padding;
    }

    // 初始化 AES
    xy_aes_ctx_t aes_ctx;
    if (xy_aes_init(&aes_ctx, aes_key, XY_AES_KEY_SIZE_128)
        == XY_CRYPTO_SUCCESS) {

        // 加密
        if (xy_aes_cbc_encrypt(
                &aes_ctx, iv, padded_data, padded_len, encrypted_data)
            == XY_CRYPTO_SUCCESS) {

            // 转换为 Base64 用于传输
            char b64_encrypted[128];
            xy_base64_encode(encrypted_data, padded_len, b64_encrypted,
                             sizeof(b64_encrypted));
            printf("加密后(Base64): %s\n", b64_encrypted);

            // 模拟接收方：Base64 解码
            uint8_t received_encrypted[64];
            xy_base64_decode(b64_encrypted, strlen(b64_encrypted),
                             received_encrypted, sizeof(received_encrypted));

            // 解密
            if (xy_aes_cbc_decrypt(&aes_ctx, iv, received_encrypted, padded_len,
                                   decrypted_data)
                == XY_CRYPTO_SUCCESS) {

                // 去除填充
                uint8_t pad_len = decrypted_data[padded_len - 1];
                decrypted_data[padded_len - pad_len] = '\0';

                printf("解密后: %s\n", (char *)decrypted_data);

                if (strcmp(original_data, (char *)decrypted_data) == 0) {
                    printf("加密传输成功！\n");
                } else {
                    printf("加密传输失败！\n");
                }
            }
        }
    }
    printf("\n");
}

// 示例：数据完整性校验
void example_data_integrity(void)
{
    printf("=== 数据完整性校验示例 ===\n");

    const char *data_packet = "Network packet data with some content";
    printf("数据包: %s\n", data_packet);

    // 计算 CRC32 校验和
    uint32_t checksum =
        xy_crc32((const uint8_t *)data_packet, strlen(data_packet));
    printf("CRC32校验和: 0x%08x\n", checksum);

    // 模拟数据传输后的校验
    const char *received_data =
        "Network packet data with some content"; // 正确数据
    const char *corrupted_data =
        "Network packet data with some cont3nt"; // 损坏数据

    uint32_t received_checksum =
        xy_crc32((const uint8_t *)received_data, strlen(received_data));
    uint32_t corrupted_checksum =
        xy_crc32((const uint8_t *)corrupted_data, strlen(corrupted_data));

    printf("接收数据校验: %s\n",
           (checksum == received_checksum) ? "通过" : "失败");
    printf("损坏数据校验: %s\n",
           (checksum == corrupted_checksum) ? "通过" : "失败");
    printf("\n");
}

// 示例：随机数生成
void example_random_generation(void)
{
    printf("=== 随机数生成示例 ===\n");

    // 生成随机字节
    uint8_t random_bytes[16];
    if (xy_random_bytes(random_bytes, sizeof(random_bytes))
        == XY_CRYPTO_SUCCESS) {
        printf("随机字节: ");
        for (int i = 0; i < 16; i++) {
            printf("%02x ", random_bytes[i]);
        }
        printf("\n");
    }

    // 生成随机整数
    printf("随机整数: ");
    for (int i = 0; i < 5; i++) {
        printf("%u ", xy_random_uint32());
    }
    printf("\n\n");
}

int main(void)
{
    printf("XY Tiny Crypto 使用示例\n");
    printf("========================\n\n");

    example_file_hash();
    example_message_auth();
    example_data_encryption();
    example_data_integrity();
    example_random_generation();

    printf("所有示例执行完成！\n");
    return 0;
}