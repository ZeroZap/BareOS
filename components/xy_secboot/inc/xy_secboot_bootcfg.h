/**
 * @file xy_secboot_bootcfg.h
 * @brief Redundant A/B boot configuration storage
 * @version 0.1.0
 */

#ifndef XY_SECBOOT_BOOTCFG_H
#define XY_SECBOOT_BOOTCFG_H

#include <stdint.h>
#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XY_SECBOOT_BOOTCFG_MAGIC          0x31434258u /* 'XBC1' */
#define XY_SECBOOT_BOOTCFG_FORMAT_VERSION 1u

typedef enum {
    XY_SECBOOT_BOOTCFG_COPY_NONE = 0,
    XY_SECBOOT_BOOTCFG_COPY_A = 1,
    XY_SECBOOT_BOOTCFG_COPY_B = 2,
} xy_secboot_bootcfg_copy_t;

typedef struct {
    uint32_t magic;
    uint16_t format_version;
    uint16_t header_size;
    uint32_t seq;
    uint32_t seq_inv;
    uint32_t payload_len;
    uint32_t payload_crc32;
    uint32_t header_crc32;
} xy_secboot_bootcfg_header_t;

typedef struct {
    int (*flash_read)(uint32_t address, uint8_t *data, size_t len);
    int (*flash_erase)(uint32_t address, size_t len);
    int (*flash_write)(uint32_t address, const uint8_t *data, size_t len);
} xy_secboot_bootcfg_port_t;

typedef struct {
    uint32_t copy_a_addr;
    uint32_t copy_b_addr;
    uint32_t copy_size;
    const xy_secboot_bootcfg_port_t *port;
} xy_secboot_bootcfg_ctx_t;

typedef struct {
    xy_secboot_bootcfg_copy_t active_copy;
    uint32_t seq;
    uint32_t payload_len;
} xy_secboot_bootcfg_info_t;

int xy_secboot_bootcfg_load(const xy_secboot_bootcfg_ctx_t *ctx,
                            uint8_t *payload,
                            size_t payload_cap,
                            size_t *payload_len,
                            xy_secboot_bootcfg_info_t *info);

int xy_secboot_bootcfg_save(const xy_secboot_bootcfg_ctx_t *ctx,
                            const uint8_t *payload,
                            size_t payload_len,
                            xy_secboot_bootcfg_info_t *info);

#ifdef __cplusplus
}
#endif

#endif /* XY_SECBOOT_BOOTCFG_H */
