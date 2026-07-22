/**
 * @file xy_secboot_security.h
 * @brief Secure boot hardware security settings abstraction
 * @version 0.1.0
 */

#ifndef XY_SECBOOT_SECURITY_H
#define XY_SECBOOT_SECURITY_H

#include <stdint.h>
#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    XY_SECBOOT_RDP_LEVEL_0 = 0,
    XY_SECBOOT_RDP_LEVEL_1,
    XY_SECBOOT_RDP_LEVEL_2,
} xy_secboot_rdp_level_t;

typedef enum {
    XY_SECBOOT_SECURITY_FLAG_NONE = 0u,
    XY_SECBOOT_SECURITY_FLAG_ALLOW_RDP2 = 1u << 0,
} xy_secboot_security_flag_t;

typedef struct {
    uint32_t address;
    uint32_t size;
} xy_secboot_wrp_range_t;

typedef struct {
    xy_secboot_rdp_level_t rdp_level;
    const xy_secboot_wrp_range_t *wrp_ranges;
    size_t wrp_range_count;
    uint32_t flags;
} xy_secboot_security_config_t;

typedef struct {
    xy_secboot_rdp_level_t rdp_level;
    uint32_t wrp_mask;
} xy_secboot_security_status_t;

typedef struct {
    int (*get_status)(xy_secboot_security_status_t *status);
    int (*apply)(const xy_secboot_security_config_t *config);
} xy_secboot_security_ops_t;

int xy_secboot_security_get_status(const xy_secboot_security_ops_t *ops,
                                   xy_secboot_security_status_t *status);
int xy_secboot_security_apply(const xy_secboot_security_ops_t *ops,
                              const xy_secboot_security_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* XY_SECBOOT_SECURITY_H */
