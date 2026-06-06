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
    XY_SECBOOT_PART_APP_A_MANIFEST,
    XY_SECBOOT_PART_APP_A_IMAGE,
    XY_SECBOOT_PART_APP_B_MANIFEST,
    XY_SECBOOT_PART_APP_B_IMAGE,
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
} xy_secboot_partition_flag_t;

typedef struct {
    xy_secboot_partition_id_t id;
    uint32_t address;
    uint32_t size;
    uint32_t erase_size;
    uint32_t flags;
} xy_secboot_partition_t;

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
