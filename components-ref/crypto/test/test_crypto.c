#include <stdio.h>
#include <string.h>
#include "xy_tiny_crypto.h"
#include <stdio.h>
#include <string.h>

static void print_hex(const uint8_t *data, size_t len, const char *label)
{
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

void test_md5(void)
{
    printf("\n=== MD5 测试 ===\n");

    const char *test_data = "The quick brown fox jumps over the lazy dog";
    uint8_t digest[XY_MD5_DIGEST_SIZE];

    if (xy_md5_hash((const uint8_t *)test_data, strlen(test_data), digest)
        == XY_CRYPTO_SUCCESS) {
        print_hex(digest, XY_MD5_DIGEST_SIZE, "MD5");
        printf("期望: 9e107d9d372bb6826bd81d3542a419d6\n");
    } else {
        printf("MD5 计算失败\n");
    }
}

void test_sha256(void)
{
    printf("\n=== SHA256 测试 ===\n");

    const char *test_data = "The quick brown fox jumps over the lazy dog";
    uint8_t digest[XY_SHA256_DIGEST_SIZE];

    if (xy_sha256_hash((const uint8_t *)test_data, strlen(test_data), digest)
        == XY_CRYPTO_SUCCESS) {
        print_hex(digest, XY_SHA256_DIGEST_SIZE, "SHA256");
        printf(
            "期望: "
            "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592"
            "\n");
    } else {
        printf("SHA256 计算失败\n");
    }
}

void test_aes(void)
{
    printf("\n=== AES-128 测试 ===\n");

    const uint8_t key[16] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                              0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };

    const uint8_t plaintext[16] = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40,
                                    0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11,
                                    0x73, 0x93, 0x17, 0x2a };

    uint8_t ciphertext[16];
    uint8_t decrypted[16];

    xy_aes_ctx_t ctx;
    if (xy_aes_init(&ctx, key, XY_AES_KEY_SIZE_128) == XY_CRYPTO_SUCCESS) {
        if (xy_aes_encrypt_block(&ctx, plaintext, ciphertext)
            == XY_CRYPTO_SUCCESS) {
            print_hex(plaintext, 16, "明文");
            print_hex(ciphertext, 16, "密文");

            if (xy_aes_decrypt_block(&ctx, ciphertext, decrypted)
                == XY_CRYPTO_SUCCESS) {
                print_hex(decrypted, 16, "解密");

                if (memcmp(plaintext, decrypted, 16) == 0) {
                    printf("AES 加解密成功!\n");
                } else {
                    printf("AES 加解密失败!\n");
                }
            }
        }
    }
}

void test_base64(void)
{
    printf("\n=== Base64 测试 ===\n");

    const char *test_data = "Hello, World!";
    char encoded[64];
    uint8_t decoded[64];

    if (xy_base64_encode((const uint8_t *)test_data, strlen(test_data), encoded,
                         sizeof(encoded))
        == XY_CRYPTO_SUCCESS) {
        printf("原文: %s\n", test_data);
        printf("Base64编码: %s\n", encoded);

        if (xy_base64_decode(encoded, strlen(encoded), decoded, sizeof(decoded))
            == XY_CRYPTO_SUCCESS) {
            decoded[strlen(test_data)] = '\0'; // 添加字符串结束符
            printf("Base64解码: %s\n", (char *)decoded);

            if (strcmp(test_data, (char *)decoded) == 0) {
                printf("Base64 编解码成功!\n");
            } else {
                printf("Base64 编解码失败!\n");
            }
        }
    }
}

void test_hex(void)
{
    printf("\n=== Hex 测试 ===\n");

    const uint8_t test_data[] = { 0x48, 0x65, 0x6c, 0x6c, 0x6f };
    char encoded[32];
    uint8_t decoded[32];

    if (xy_hex_encode(test_data, sizeof(test_data), encoded, sizeof(encoded))
        == XY_CRYPTO_SUCCESS) {
        printf("原始数据: ");
        for (size_t i = 0; i < sizeof(test_data); i++) {
            printf("%02x ", test_data[i]);
        }
        printf("\n");
        printf("Hex编码: %s\n", encoded);

        if (xy_hex_decode(encoded, strlen(encoded), decoded, sizeof(decoded))
            == XY_CRYPTO_SUCCESS) {
            printf("Hex解码: ");
            for (size_t i = 0; i < sizeof(test_data); i++) {
                printf("%02x ", decoded[i]);
            }
            printf("\n");

            if (memcmp(test_data, decoded, sizeof(test_data)) == 0) {
                printf("Hex 编解码成功!\n");
            } else {
                printf("Hex 编解码失败!\n");
            }
        }
    }
}

void test_crc32(void)
{
    printf("\n=== CRC32 测试 ===\n");

    const char *test_data = "123456789";
    uint32_t crc = xy_crc32((const uint8_t *)test_data, strlen(test_data));

    printf("输入: %s\n", test_data);
    printf("CRC32: 0x%08x\n", crc);
    printf("期望: 0xcbf43926\n");
}

void test_hmac_sha256(void)
{
    printf("\n=== HMAC-SHA256 测试 ===\n");

    const char *key  = "key";
    const char *data = "The quick brown fox jumps over the lazy dog";
    uint8_t hmac[XY_SHA256_DIGEST_SIZE];

    if (xy_hmac_sha256((const uint8_t *)key, strlen(key), (const uint8_t *)data,
                       strlen(data), hmac)
        == XY_CRYPTO_SUCCESS) {
        printf("密钥: %s\n", key);
        printf("数据: %s\n", data);
        print_hex(hmac, XY_SHA256_DIGEST_SIZE, "HMAC-SHA256");
    }
}

int main(void)
{
    printf("XY Tiny Crypto 库测试\n");
    printf("=====================\n");

    test_md5();
    test_sha256();
    test_aes();
    test_base64();
    test_hex();
    test_crc32();
    test_hmac_sha256();

    printf("\n测试完成!\n");
    return 0;
}