/**
 * @file xy_secboot_guard.c
 * @brief Low-cost secure boot guard helpers
 */

#include "xy_secboot_guard.h"

int xy_secboot_ct_compare(const uint8_t *a, const uint8_t *b, size_t len)
{
    uint8_t diff = 0u;

    if (!a || !b) {
        return -1;
    }

    for (size_t i = 0; i < len; i++) {
        diff |= (uint8_t)(a[i] ^ b[i]);
    }

    return (diff == 0u) ? 0 : -1;
}

void xy_secboot_secure_zero(void *buf, size_t len)
{
    volatile uint8_t *p = (volatile uint8_t *)buf;

    if (!p) {
        return;
    }

    while (len--) {
        *p++ = 0u;
    }
}

int xy_secboot_check_inverse_u32(uint32_t value, uint32_t value_inv)
{
    return (value == ~value_inv) ? 0 : -1;
}
