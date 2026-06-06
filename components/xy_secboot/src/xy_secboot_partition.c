/**
 * @file xy_secboot_partition.c
 * @brief Secure boot partition helpers
 */

#include "xy_secboot_partition.h"

const xy_secboot_partition_t *xy_secboot_partition_find(
    const xy_secboot_partition_table_t *table,
    xy_secboot_partition_id_t id)
{
    if (!table || !table->parts) {
        return NULL;
    }

    for (size_t i = 0; i < table->count; i++) {
        if (table->parts[i].id == id) {
            return &table->parts[i];
        }
    }

    return NULL;
}

int xy_secboot_partition_check_range(const xy_secboot_partition_t *part,
                                     uint32_t address,
                                     uint32_t size)
{
    uint32_t part_end;
    uint32_t range_end;

    if (!part || size == 0u) {
        return -1;
    }

    part_end = part->address + part->size;
    range_end = address + size;

    if (part_end < part->address || range_end < address) {
        return -1;
    }

    if (address < part->address || range_end > part_end) {
        return -1;
    }

    return 0;
}
