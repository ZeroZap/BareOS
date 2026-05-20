#include <stdint.h>
#include "xy_tiny_crypto.h"

static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int base64_char_to_index(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 26;
    if (c >= '0' && c <= '9')
        return c - '0' + 52;
    if (c == '+')
        return 62;
    if (c == '/')
        return 63;
    if (c == '=')
        return -1; // padding
    return -2;     // invalid character
}

size_t xy_base64_encode_len(size_t input_len)
{
    return ((input_len + 2) / 3) * 4 + 1; // +1 for null terminator
}

size_t xy_base64_decode_len(size_t input_len)
{
    return (input_len / 4) * 3;
}

int xy_base64_encode(const uint8_t *input, size_t input_len, char *output,
                     size_t output_len)
{
    if (!input || !output)
        return XY_CRYPTO_INVALID_PARAM;

    size_t required_len = xy_base64_encode_len(input_len);
    if (output_len < required_len)
        return XY_CRYPTO_BUFFER_TOO_SMALL;

    size_t output_pos = 0;

    for (size_t i = 0; i < input_len; i += 3) {
        uint32_t triple = 0;
        int padding     = 0;

        // 构建24位组
        triple |= (uint32_t)input[i] << 16;
        if (i + 1 < input_len) {
            triple |= (uint32_t)input[i + 1] << 8;
        } else {
            padding++;
        }
        if (i + 2 < input_len) {
            triple |= (uint32_t)input[i + 2];
        } else {
            padding++;
        }

        // 输出4个6位字符
        output[output_pos++] = base64_chars[(triple >> 18) & 0x3F];
        output[output_pos++] = base64_chars[(triple >> 12) & 0x3F];
        output[output_pos++] =
            (padding >= 2) ? '=' : base64_chars[(triple >> 6) & 0x3F];
        output[output_pos++] =
            (padding >= 1) ? '=' : base64_chars[triple & 0x3F];
    }

    output[output_pos] = '\0';
    return XY_CRYPTO_SUCCESS;
}

int xy_base64_decode(const char *input, size_t input_len, uint8_t *output,
                     size_t output_len)
{
    if (!input || !output)
        return XY_CRYPTO_INVALID_PARAM;
    if (input_len % 4 != 0)
        return XY_CRYPTO_INVALID_PARAM;

    size_t max_output_len = xy_base64_decode_len(input_len);
    if (output_len < max_output_len)
        return XY_CRYPTO_BUFFER_TOO_SMALL;

    size_t output_pos = 0;

    for (size_t i = 0; i < input_len; i += 4) {
        int indices[4];
        int padding = 0;

        // 转换4个字符为索引
        for (int j = 0; j < 4; j++) {
            indices[j] = base64_char_to_index(input[i + j]);
            if (indices[j] == -1) { // padding
                padding++;
                indices[j] = 0;
            } else if (indices[j] == -2) { // invalid character
                return XY_CRYPTO_INVALID_PARAM;
            }
        }

        // 重构24位组
        uint32_t triple = (indices[0] << 18) | (indices[1] << 12)
                          | (indices[2] << 6) | indices[3];

        // 输出字节
        output[output_pos++] = (triple >> 16) & 0xFF;
        if (padding < 2)
            output[output_pos++] = (triple >> 8) & 0xFF;
        if (padding < 1)
            output[output_pos++] = triple & 0xFF;
    }

    return XY_CRYPTO_SUCCESS;
}