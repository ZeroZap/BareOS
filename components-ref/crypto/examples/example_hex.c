/**
 * @file example_hex.c
 * @brief Hex Encode/Decode Example
 */

#include <stdio.h>
#include <string.h>
#include "xy_tiny_crypto.h"

/* Print buffer as hex string */
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
    printf("=== Hex Encode/Decode Example ===\n\n");

    /* ========== Encode examples ========== */
    printf("--- Hex Encoding ---\n");

    /* String to hex */
    const char *test_string = "Hello, Hex!";
    size_t out_len = xy_hex_encode_len(strlen(test_string));
    char encoded[64] = {0};

    xy_hex_encode((const uint8_t *)test_string, strlen(test_string),
                  encoded, out_len);
    printf("Original: %s\n", test_string);
    printf("Hex encoded: %s\n\n", encoded);

    /* Binary data to hex */
    uint8_t binary_data[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
                             0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    size_t bin_out_len = xy_hex_encode_len(sizeof(binary_data));
    char encoded_binary[64] = {0};

    xy_hex_encode(binary_data, sizeof(binary_data),
                  encoded_binary, bin_out_len);
    printf("Binary data: ");
    print_hex("", binary_data, sizeof(binary_data));
    printf("Hex encoded: %s\n\n", encoded_binary);

    /* ========== Decode examples ========== */
    printf("--- Hex Decoding ---\n");

    /* Hex string to bytes */
    const char *hex_string = "48656c6c6f576f726c64";  // "HelloWorld" in hex
    size_t dec_out_len = xy_hex_decode_len(strlen(hex_string));
    uint8_t decoded[32] = {0};

    xy_hex_decode(hex_string, strlen(hex_string),
                  decoded, dec_out_len);
    printf("Hex string: %s\n", hex_string);
    printf("Decoded: ");
    print_hex("", decoded, dec_out_len / 2);
    decoded[dec_out_len / 2] = '\0';
    printf("As string: %s\n\n", decoded);

    /* Decode the binary hex we encoded earlier */
    uint8_t decoded_binary[32] = {0};
    xy_hex_decode(encoded_binary, strlen(encoded_binary),
                  decoded_binary, sizeof(decoded_binary));

    printf("Hex: %s\n", encoded_binary);
    printf("Decoded binary: ");
    print_hex("", decoded_binary, sizeof(binary_data));
    printf("Match: %s\n\n",
           memcmp(binary_data, decoded_binary, sizeof(binary_data)) == 0 ? "YES" : "NO");

    /* ========== Round-trip test ========== */
    printf("--- Round-trip Test ---\n");

    const char *test_cases[] = {
        "a",
        "ab",
        "abc",
        "abcd",
        "Hello",
        "48656c6c6f",  // "Hello" in hex
        "",
        "00112233AABBCCDD"
    };

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        const char *input = test_cases[i];

        /* Decode hex to binary */
        size_t dec_len = xy_hex_decode_len(strlen(input));
        uint8_t dec_buf[64] = {0};
        xy_hex_decode(input, strlen(input), dec_buf, dec_len);

        /* Encode binary back to hex */
        size_t enc_len = xy_hex_encode_len(dec_len / 2);
        char enc_buf[128] = {0};
        xy_hex_encode(dec_buf, dec_len / 2, enc_buf, enc_len);

        /* Verify */
        printf("Input:    '%s'\n", input);
        printf("Decoded:  ");
        print_hex("", dec_buf, dec_len / 2);
        printf("Re-encoded: '%s'\n\n", enc_buf);
    }

    printf("Hex example completed!\n");
    return 0;
}
