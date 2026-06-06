/**
 * @file xy_secboot_single_slot.h
 * @brief Single-slot secure boot and streaming update interface
 * @version 0.1.0
 */

#ifndef XY_SECBOOT_SINGLE_SLOT_H
#define XY_SECBOOT_SINGLE_SLOT_H

#include <stdint.h>
#include "xy_typedef.h"
#include "xy_secboot_crypto.h"
#include "xy_secboot_partition.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    XY_SECBOOT_SINGLE_STATE_IDLE = 0,
    XY_SECBOOT_SINGLE_STATE_VERIFY_EXISTING,
    XY_SECBOOT_SINGLE_STATE_BOOT_APP,
    XY_SECBOOT_SINGLE_STATE_RECOVERY,
    XY_SECBOOT_SINGLE_STATE_RECEIVE_MANIFEST,
    XY_SECBOOT_SINGLE_STATE_RECEIVE_IMAGE,
    XY_SECBOOT_SINGLE_STATE_VERIFY_NEW_IMAGE,
    XY_SECBOOT_SINGLE_STATE_ACTIVATE_NEW_IMAGE,
    XY_SECBOOT_SINGLE_STATE_FAIL,
} xy_secboot_single_state_t;

typedef enum {
    XY_SECBOOT_BOOT_ACTION_JUMP_APP = 0,
    XY_SECBOOT_BOOT_ACTION_ENTER_RECOVERY,
    XY_SECBOOT_BOOT_ACTION_WAIT_DOWNLOAD,
    XY_SECBOOT_BOOT_ACTION_RESET,
    XY_SECBOOT_BOOT_ACTION_FAIL_SAFE,
} xy_secboot_boot_action_t;

typedef struct {
    uint32_t magic;
    uint32_t seq;
    uint32_t state;
    uint32_t state_inv;
    uint32_t image_version;
    uint32_t image_version_inv;
    uint32_t received_size;
    uint32_t crc32;
} xy_secboot_single_record_t;

typedef struct {
    int (*flash_read)(uint32_t address, uint8_t *data, size_t len);
    int (*flash_write)(uint32_t address, const uint8_t *data, size_t len);
    int (*flash_erase)(uint32_t address, size_t len);
    int (*uart_read)(uint8_t *data, size_t len, uint32_t timeout_ms);
    int (*uart_write)(const uint8_t *data, size_t len, uint32_t timeout_ms);
    void (*system_reset)(void);
    void (*jump_to_app)(uint32_t entry_addr);
} xy_secboot_single_port_t;

typedef struct {
    const xy_secboot_partition_table_t *partition_table;
    const xy_secboot_crypto_ops_t *crypto;
    const xy_secboot_single_port_t *port;
    uint32_t product_id;
    uint32_t recovery_timeout_ms;
    uint32_t packet_timeout_ms;
} xy_secboot_single_ctx_t;

int xy_secboot_single_init(xy_secboot_single_ctx_t *ctx);
xy_secboot_boot_action_t xy_secboot_single_check_boot(
    xy_secboot_single_ctx_t *ctx,
    xy_secboot_manifest_t *manifest);
int xy_secboot_single_receive_uart(xy_secboot_single_ctx_t *ctx);
int xy_secboot_single_verify_active(xy_secboot_single_ctx_t *ctx,
                                    xy_secboot_manifest_t *manifest);

#ifdef __cplusplus
}
#endif

#endif /* XY_SECBOOT_SINGLE_SLOT_H */
