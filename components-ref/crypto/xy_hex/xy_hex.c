#include <stdint.h>
#include "xy_tiny_crypto.h"

static const char hex_chars[] = "0123456789abcdef";

static int hex_char_to_value(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1; // invalid character
}

size_t xy_hex_encode_len(size_t input_len)
{
    return input_len * 2 + 1; // +1 for null terminator
}

size_t xy_hex_decode_len(size_t input_len)
{
    return input_len / 2;
}

int xy_hex_encode(const uint8_t *input, size_t input_len, char *output,
                  size_t output_len)
{
    if (!input || !output)
        return XY_CRYPTO_INVALID_PARAM;

    size_t required_len = xy_hex_encode_len(input_len);
    if (output_len < required_len)
        return XY_CRYPTO_BUFFER_TOO_SMALL;

    for (size_t i = 0; i < input_len; i++) {
        output[i * 2]     = hex_chars[(input[i] >> 4) & 0x0F];
        output[i * 2 + 1] = hex_chars[input[i] & 0x0F];
    }

    output[input_len * 2] = '\0';
    return XY_CRYPTO_SUCCESS;
}

int xy_hex_decode(const char *input, size_t input_len, uint8_t *output,
                  size_t output_len)
{
    if (!input || !output)
        return XY_CRYPTO_INVALID_PARAM;
    if (input_len % 2 != 0)
        return XY_CRYPTO_INVALID_PARAM;

    size_t required_len = xy_hex_decode_len(input_len);
    if (output_len < required_len)
        return XY_CRYPTO_BUFFER_TOO_SMALL;

    for (size_t i = 0; i < input_len; i += 2) {
        int high = hex_char_to_value(input[i]);
        int low  = hex_char_to_value(input[i + 1]);

        if (high < 0 || low < 0)
            return XY_CRYPTO_INVALID_PARAM;

        output[i / 2] = (high << 4) | low;
    }

    return XY_CRYPTO_SUCCESS;
}