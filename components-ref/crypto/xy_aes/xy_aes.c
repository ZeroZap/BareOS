#include <stdint.h>
#include <string.h>
#include "xy_tiny_crypto.h"

// AES S-box
static const uint8_t aes_sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b,
    0xfe, 0xd7, 0xab, 0x76, 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
    0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, 0xb7, 0xfd, 0x93, 0x26,
    0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2,
    0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
    0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed,
    0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f,
    0x50, 0x3c, 0x9f, 0xa8, 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
    0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 0xcd, 0x0c, 0x13, 0xec,
    0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14,
    0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
    0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, 0xe7, 0xc8, 0x37, 0x6d,
    0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f,
    0x4b, 0xbd, 0x8b, 0x8a, 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
    0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 0xe1, 0xf8, 0x98, 0x11,
    0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f,
    0xb0, 0x54, 0xbb, 0x16
};

// AES 逆 S-box
static const uint8_t aes_inv_sbox[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e,
    0x81, 0xf3, 0xd7, 0xfb, 0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87,
    0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb, 0x54, 0x7b, 0x94, 0x32,
    0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49,
    0x6d, 0x8b, 0xd1, 0x25, 0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16,
    0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92, 0x6c, 0x70, 0x48, 0x50,
    0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05,
    0xb8, 0xb3, 0x45, 0x06, 0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02,
    0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b, 0x3a, 0x91, 0x11, 0x41,
    0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8,
    0x1c, 0x75, 0xdf, 0x6e, 0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89,
    0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b, 0xfc, 0x56, 0x3e, 0x4b,
    0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59,
    0x27, 0x80, 0xec, 0x5f, 0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d,
    0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef, 0xa0, 0xe0, 0x3b, 0x4d,
    0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63,
    0x55, 0x21, 0x0c, 0x7d
};

// Rcon 常量
static const uint8_t aes_rcon[10] = { 0x01, 0x02, 0x04, 0x08, 0x10,
                                      0x20, 0x40, 0x80, 0x1b, 0x36 };

static uint32_t aes_sub_word(uint32_t word)
{
    return (aes_sbox[(word >> 24) & 0xff] << 24)
           | (aes_sbox[(word >> 16) & 0xff] << 16)
           | (aes_sbox[(word >> 8) & 0xff] << 8) | aes_sbox[word & 0xff];
}

static uint32_t aes_rot_word(uint32_t word)
{
    return (word << 8) | (word >> 24);
}

static void aes_key_expansion(const uint8_t *key, int key_size,
                              uint32_t *round_keys, int rounds)
{
    int i;
    uint32_t temp;
    int nk = key_size / 4; // 密钥字数

    // 复制初始密钥
    for (i = 0; i < nk; i++) {
        round_keys[i] = (key[i * 4] << 24) | (key[i * 4 + 1] << 16)
                        | (key[i * 4 + 2] << 8) | key[i * 4 + 3];
    }

    // 生成轮密钥
    for (i = nk; i < 4 * (rounds + 1); i++) {
        temp = round_keys[i - 1];
        if (i % nk == 0) {
            temp =
                aes_sub_word(aes_rot_word(temp)) ^ (aes_rcon[i / nk - 1] << 24);
        } else if (nk > 6 && i % nk == 4) {
            temp = aes_sub_word(temp);
        }
        round_keys[i] = round_keys[i - nk] ^ temp;
    }
}

static void aes_sub_bytes(uint8_t state[16])
{
    for (int i = 0; i < 16; i++) {
        state[i] = aes_sbox[state[i]];
    }
}

static void aes_inv_sub_bytes(uint8_t state[16])
{
    for (int i = 0; i < 16; i++) {
        state[i] = aes_inv_sbox[state[i]];
    }
}

static void aes_shift_rows(uint8_t state[16])
{
    uint8_t temp;

    // 第二行左移1位
    temp      = state[1];
    state[1]  = state[5];
    state[5]  = state[9];
    state[9]  = state[13];
    state[13] = temp;

    // 第三行左移2位
    temp      = state[2];
    state[2]  = state[10];
    state[10] = temp;
    temp      = state[6];
    state[6]  = state[14];
    state[14] = temp;

    // 第四行左移3位
    temp      = state[3];
    state[3]  = state[15];
    state[15] = state[11];
    state[11] = state[7];
    state[7]  = temp;
}

static void aes_inv_shift_rows(uint8_t state[16])
{
    uint8_t temp;

    // 第二行右移1位
    temp      = state[13];
    state[13] = state[9];
    state[9]  = state[5];
    state[5]  = state[1];
    state[1]  = temp;

    // 第三行右移2位
    temp      = state[2];
    state[2]  = state[10];
    state[10] = temp;
    temp      = state[6];
    state[6]  = state[14];
    state[14] = temp;

    // 第四行右移3位
    temp      = state[7];
    state[7]  = state[11];
    state[11] = state[15];
    state[15] = state[3];
    state[3]  = temp;
}

static uint8_t aes_gmul(uint8_t a, uint8_t b)
{
    uint8_t p = 0;
    for (int i = 0; i < 8; i++) {
        if (b & 1)
            p ^= a;
        int hi_bit_set = (a & 0x80);
        a <<= 1;
        if (hi_bit_set)
            a ^= 0x1b;
        b >>= 1;
    }
    return p;
}

static void aes_mix_columns(uint8_t state[16])
{
    for (int c = 0; c < 4; c++) {
        uint8_t s0 = state[c * 4];
        uint8_t s1 = state[c * 4 + 1];
        uint8_t s2 = state[c * 4 + 2];
        uint8_t s3 = state[c * 4 + 3];

        state[c * 4]     = aes_gmul(s0, 2) ^ aes_gmul(s1, 3) ^ s2 ^ s3;
        state[c * 4 + 1] = s0 ^ aes_gmul(s1, 2) ^ aes_gmul(s2, 3) ^ s3;
        state[c * 4 + 2] = s0 ^ s1 ^ aes_gmul(s2, 2) ^ aes_gmul(s3, 3);
        state[c * 4 + 3] = aes_gmul(s0, 3) ^ s1 ^ s2 ^ aes_gmul(s3, 2);
    }
}

static void aes_inv_mix_columns(uint8_t state[16])
{
    for (int c = 0; c < 4; c++) {
        uint8_t s0 = state[c * 4];
        uint8_t s1 = state[c * 4 + 1];
        uint8_t s2 = state[c * 4 + 2];
        uint8_t s3 = state[c * 4 + 3];

        state[c * 4] = aes_gmul(s0, 14) ^ aes_gmul(s1, 11) ^ aes_gmul(s2, 13)
                       ^ aes_gmul(s3, 9);
        state[c * 4 + 1] = aes_gmul(s0, 9) ^ aes_gmul(s1, 14) ^ aes_gmul(s2, 11)
                           ^ aes_gmul(s3, 13);
        state[c * 4 + 2] = aes_gmul(s0, 13) ^ aes_gmul(s1, 9) ^ aes_gmul(s2, 14)
                           ^ aes_gmul(s3, 11);
        state[c * 4 + 3] = aes_gmul(s0, 11) ^ aes_gmul(s1, 13) ^ aes_gmul(s2, 9)
                           ^ aes_gmul(s3, 14);
    }
}

static void aes_add_round_key(uint8_t state[16], const uint32_t *round_key)
{
    for (int i = 0; i < 4; i++) {
        uint32_t k = round_key[i];
        state[i * 4] ^= (k >> 24) & 0xff;
        state[i * 4 + 1] ^= (k >> 16) & 0xff;
        state[i * 4 + 2] ^= (k >> 8) & 0xff;
        state[i * 4 + 3] ^= k & 0xff;
    }
}

int xy_aes_init(xy_aes_ctx_t *ctx, const uint8_t *key, int key_size)
{
    if (!ctx || !key)
        return XY_CRYPTO_INVALID_PARAM;

    switch (key_size) {
    case XY_AES_KEY_SIZE_128:
        ctx->rounds = 10;
        break;
    case XY_AES_KEY_SIZE_192:
        ctx->rounds = 12;
        break;
    case XY_AES_KEY_SIZE_256:
        ctx->rounds = 14;
        break;
    default:
        return XY_CRYPTO_INVALID_PARAM;
    }

    aes_key_expansion(key, key_size, ctx->round_keys, ctx->rounds);
    return XY_CRYPTO_SUCCESS;
}

int xy_aes_encrypt_block(xy_aes_ctx_t *ctx, const uint8_t *plaintext,
                         uint8_t *ciphertext)
{
    if (!ctx || !plaintext || !ciphertext)
        return XY_CRYPTO_INVALID_PARAM;

    uint8_t state[16];
    memcpy(state, plaintext, 16);

    // 初始轮密钥加
    aes_add_round_key(state, &ctx->round_keys[0]);

    // 前 rounds-1 轮
    for (int round = 1; round < ctx->rounds; round++) {
        aes_sub_bytes(state);
        aes_shift_rows(state);
        aes_mix_columns(state);
        aes_add_round_key(state, &ctx->round_keys[round * 4]);
    }

    // 最后一轮（没有列混合）
    aes_sub_bytes(state);
    aes_shift_rows(state);
    aes_add_round_key(state, &ctx->round_keys[ctx->rounds * 4]);

    memcpy(ciphertext, state, 16);
    return XY_CRYPTO_SUCCESS;
}

int xy_aes_decrypt_block(xy_aes_ctx_t *ctx, const uint8_t *ciphertext,
                         uint8_t *plaintext)
{
    if (!ctx || !ciphertext || !plaintext)
        return XY_CRYPTO_INVALID_PARAM;

    uint8_t state[16];
    memcpy(state, ciphertext, 16);

    // 初始轮密钥加
    aes_add_round_key(state, &ctx->round_keys[ctx->rounds * 4]);

    // 前 rounds-1 轮
    for (int round = ctx->rounds - 1; round > 0; round--) {
        aes_inv_shift_rows(state);
        aes_inv_sub_bytes(state);
        aes_add_round_key(state, &ctx->round_keys[round * 4]);
        aes_inv_mix_columns(state);
    }

    // 最后一轮（没有逆列混合）
    aes_inv_shift_rows(state);
    aes_inv_sub_bytes(state);
    aes_add_round_key(state, &ctx->round_keys[0]);

    memcpy(plaintext, state, 16);
    return XY_CRYPTO_SUCCESS;
}

int xy_aes_cbc_encrypt(xy_aes_ctx_t *ctx, const uint8_t *iv,
                       const uint8_t *plaintext, size_t len,
                       uint8_t *ciphertext)
{
    if (!ctx || !iv || !plaintext || !ciphertext
        || len % XY_AES_BLOCK_SIZE != 0) {
        return XY_CRYPTO_INVALID_PARAM;
    }

    uint8_t prev_block[XY_AES_BLOCK_SIZE];
    memcpy(prev_block, iv, XY_AES_BLOCK_SIZE);

    for (size_t i = 0; i < len; i += XY_AES_BLOCK_SIZE) {
        uint8_t block[XY_AES_BLOCK_SIZE];

        // XOR with previous ciphertext block
        for (int j = 0; j < XY_AES_BLOCK_SIZE; j++) {
            block[j] = plaintext[i + j] ^ prev_block[j];
        }

        // Encrypt block
        int ret = xy_aes_encrypt_block(ctx, block, &ciphertext[i]);
        if (ret != XY_CRYPTO_SUCCESS)
            return ret;

        // Update previous block
        memcpy(prev_block, &ciphertext[i], XY_AES_BLOCK_SIZE);
    }

    return XY_CRYPTO_SUCCESS;
}

int xy_aes_cbc_decrypt(xy_aes_ctx_t *ctx, const uint8_t *iv,
                       const uint8_t *ciphertext, size_t len,
                       uint8_t *plaintext)
{
    if (!ctx || !iv || !ciphertext || !plaintext
        || len % XY_AES_BLOCK_SIZE != 0) {
        return XY_CRYPTO_INVALID_PARAM;
    }

    uint8_t prev_block[XY_AES_BLOCK_SIZE];
    memcpy(prev_block, iv, XY_AES_BLOCK_SIZE);

    for (size_t i = 0; i < len; i += XY_AES_BLOCK_SIZE) {
        uint8_t block[XY_AES_BLOCK_SIZE];

        // Decrypt block
        int ret = xy_aes_decrypt_block(ctx, &ciphertext[i], block);
        if (ret != XY_CRYPTO_SUCCESS)
            return ret;

        // XOR with previous ciphertext block
        for (int j = 0; j < XY_AES_BLOCK_SIZE; j++) {
            plaintext[i + j] = block[j] ^ prev_block[j];
        }

        // Update previous block
        memcpy(prev_block, &ciphertext[i], XY_AES_BLOCK_SIZE);
    }

    return XY_CRYPTO_SUCCESS;
}