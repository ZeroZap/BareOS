/**
 * @file xy_secboot_partition.h
 * @brief Flash partition descriptors for secure boot policies
 * @version 0.1.0
 */

#ifndef XY_SECBOOT_PARTITION_H
#define XY_SECBOOT_PARTITION_H

#include <stdint.h>
#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    XY_SECBOOT_PART_BOOTLOADER = 0,
    XY_SECBOOT_PART_BOOT_STATE,
    XY_SECBOOT_PART_ROLLBACK,
    XY_SECBOOT_PART_DOWNLOAD_MANIFEST,
    XY_SECBOOT_PART_DOWNLOAD_IMAGE,
    XY_SECBOOT_PART_APP_A_MANIFEST,
    XY_SECBOOT_PART_APP_A_IMAGE,
    XY_SECBOOT_PART_APP_B_MANIFEST,
    XY_SECBOOT_PART_APP_B_IMAGE,
    XY_SECBOOT_PART_SWAP,
    XY_SECBOOT_PART_MODEL_MANIFEST,
    XY_SECBOOT_PART_MODEL_IMAGE,
    XY_SECBOOT_PART_PARAM,
    XY_SECBOOT_PART_EVTLOG,
    XY_SECBOOT_PART_FACTORY,
    XY_SECBOOT_PART_RESERVED,
} xy_secboot_partition_id_t;

typedef enum {
    XY_SECBOOT_PART_FLAG_READ_ONLY   = 1u << 0,
    XY_SECBOOT_PART_FLAG_WRITE_ONCE  = 1u << 1,
    XY_SECBOOT_PART_FLAG_BOOT_PRIV   = 1u << 2,
    XY_SECBOOT_PART_FLAG_ENCRYPTED   = 1u << 3,
    XY_SECBOOT_PART_FLAG_SIGNED      = 1u << 4,
    XY_SECBOOT_PART_FLAG_ROLLBACK    = 1u << 5,
    XY_SECBOOT_PART_FLAG_EXECUTABLE  = 1u << 6,
    XY_SECBOOT_PART_FLAG_STAGING     = 1u << 7,
} xy_secboot_partition_flag_t;

typedef enum {
    XY_SECBOOT_STORAGE_INTERNAL_FLASH = 0,
    XY_SECBOOT_STORAGE_EXTERNAL_FLASH,
    XY_SECBOOT_STORAGE_OTP,
    XY_SECBOOT_STORAGE_SECURE_ELEMENT,
} xy_secboot_storage_t;

typedef enum {
    XY_SECBOOT_SLOT_MODE_SINGLE = 0,
    XY_SECBOOT_SLOT_MODE_SINGLE_WITH_STAGING,
    XY_SECBOOT_SLOT_MODE_AB_INTERNAL,
    XY_SECBOOT_SLOT_MODE_AB_MIXED,
    XY_SECBOOT_SLOT_MODE_AB_EXTERNAL,
    XY_SECBOOT_SLOT_MODE_SWAP,
} xy_secboot_slot_mode_t;

typedef enum {
    XY_SECBOOT_SLOT_NONE = 0,
    XY_SECBOOT_SLOT_A,
    XY_SECBOOT_SLOT_B,
    XY_SECBOOT_SLOT_DOWNLOAD,
    XY_SECBOOT_SLOT_SWAP,
} xy_secboot_slot_id_t;

typedef enum {
    XY_SECBOOT_DOWNLOAD_NONE = 0,
    XY_SECBOOT_DOWNLOAD_UART,
    XY_SECBOOT_DOWNLOAD_USB,
    XY_SECBOOT_DOWNLOAD_CELLULAR,
    XY_SECBOOT_DOWNLOAD_SATELLITE,
    XY_SECBOOT_DOWNLOAD_EXTERNAL_FLASH,
} xy_secboot_download_channel_t;

typedef struct {
    xy_secboot_partition_id_t id;
    xy_secboot_storage_t storage;
    xy_secboot_slot_id_t slot;
    uint32_t address;
    uint32_t size;
    uint32_t erase_size;
    uint32_t flags;
} xy_secboot_partition_t;

typedef struct {
    xy_secboot_slot_mode_t mode;
    xy_secboot_download_channel_t download_channel;
    xy_secboot_slot_id_t active_slot;
    xy_secboot_slot_id_t pending_slot;
    xy_secboot_slot_id_t confirmed_slot;
    uint32_t boot_attempts;
    uint32_t max_boot_attempts;
} xy_secboot_slot_policy_t;

typedef struct {
    const xy_secboot_partition_t *parts;
    size_t count;
} xy_secboot_partition_table_t;

const xy_secboot_partition_t *xy_secboot_partition_find(
    const xy_secboot_partition_table_t *table,
    xy_secboot_partition_id_t id);

int xy_secboot_partition_check_range(const xy_secboot_partition_t *part,
                                     uint32_t address,
                                     uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* XY_SECBOOT_PARTITION_H */
