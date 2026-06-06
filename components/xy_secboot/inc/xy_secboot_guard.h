/**
 * @file xy_secboot_guard.h
 * @brief Low-cost secure boot guard helpers
 * @version 0.1.0
 */

#ifndef XY_SECBOOT_GUARD_H
#define XY_SECBOOT_GUARD_H

#include <stdint.h>
#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

int xy_secboot_ct_compare(const uint8_t *a, const uint8_t *b, size_t len);
void xy_secboot_secure_zero(void *buf, size_t len);
int xy_secboot_check_inverse_u32(uint32_t value, uint32_t value_inv);

#ifdef __cplusplus
}
#endif

#endif /* XY_SECBOOT_GUARD_H */
