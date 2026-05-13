/**
 * @file example_base64.c
 * @brief Base64 Encode/Decode Example
 */

#include <stdio.h>
#include <string.h>
#include "xy_tiny_crypto.h"

/* Print buffer as hex */
static void print_hex(const char *label, const uint8_t *data, size_t len)
{
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

int main(void)
{
    printf("=== Base64 Encode/Decode Example ===\n\n");

    /* ========== Encode examples ========== */
    printf("--- Base64 Encoding ---\n");

    /* String encoding */
    const char *test_string = "Hello, Base64!";
    size_t out_len = xy_base64_encode_len(strlen(test_string));
    char encoded[128] = {0};

    xy_base64_encode((const uint8_t *)test_string, strlen(test_string),
                     encoded, out_len);
    printf("Original: %s\n", test_string);
    printf("Encoded:  %s\n\n", encoded);

    /* Binary data encoding */
    uint8_t binary_data[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                             0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    size_t bin_out_len = xy_base64_encode_len(sizeof(binary_data));
    char encoded_binary[64] = {0};

    xy_base64_encode(binary_data, sizeof(binary_data),
                     encoded_binary, bin_out_len);
    printf("Binary data: ");
    print_hex("", binary_data, sizeof(binary_data));
    printf("Encoded: %s\n\n", encoded_binary);

    /* ========== Decode examples ========== */
    printf("--- Base64 Decoding ---\n");

    /* Decode string */
    const char *base64_string = "SGVsbG8sIFdvcmxkIQ==";  // "Hello, World!" in Base64
    size_t dec_out_len = xy_base64_decode_len(strlen(base64_string));
    uint8_t decoded[64] = {0};

    xy_base64_decode(base64_string, strlen(base64_string),
                     decoded, dec_out_len);
    printf("Encoded: %s\n", base64_string);
    printf("Decoded: %s\n\n", decoded);

    /* Decode binary encoded data */
    uint8_t decoded_binary[32] = {0};
    xy_base64_decode(encoded_binary, strlen(encoded_binary),
                     decoded_binary, sizeof(decoded_binary));

    printf("Encoded: %s\n", encoded_binary);
    printf("Decoded: ");
    print_hex("", decoded_binary, sizeof(binary_data));
    printf("Match:   %s\n\n",
           memcmp(binary_data, decoded_binary, sizeof(binary_data)) == 0 ? "YES" : "NO");

    /* ========== Round-trip test ========== */
    printf("--- Round-trip Test ---\n");

    const char *messages[] = {
        "a",
        "ab",
        "abc",
        "abcd",
        "Hello, World!",
        "Base64 encoding is commonly used for transmitting binary data over text-only channels.",
        "1234567890",
        ""
    };

    for (size_t i = 0; i < sizeof(messages) / sizeof(messages[0]); i++) {
        const char *msg = messages[i];
        size_t msg_len = strlen(msg);

        /* Encode */
        size_t enc_len = xy_base64_encode_len(msg_len);
        char enc_buf[256] = {0};
        xy_base64_encode((const uint8_t *)msg, msg_len, enc_buf, enc_len);

        /* Decode */
        size_t dec_len = xy_base64_decode_len(enc_len);
        uint8_t dec_buf[256] = {0};
        xy_base64_decode(enc_buf, strlen(enc_buf), dec_buf, dec_len);

        /* Verify */
        int match = (msg_len == dec_len) && (memcmp(msg, dec_buf, msg_len) == 0);
        printf("'%s' -> '%s' -> '%s' [%s]\n",
               msg, enc_buf, dec_buf, match ? "OK" : "FAIL");
    }

    printf("\nBase64 example completed!\n");
    return 0;
}
