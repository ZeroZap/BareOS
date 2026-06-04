#ifndef XY_EEPROM_H
#define XY_EEPROM_H

#include <stdint.h>
#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XY_EEPROM_MAX_WRITE_UNIT
#define XY_EEPROM_MAX_WRITE_UNIT 32u
#endif

#ifndef XY_EEPROM_MAX_RECORD_SIZE
#define XY_EEPROM_MAX_RECORD_SIZE 128u
#endif

#ifndef XY_EEPROM_ENABLE_CRC16
#define XY_EEPROM_ENABLE_CRC16 1u
#endif

typedef enum {
    XY_EEPROM_OK = 0,
    XY_EEPROM_ERROR_INVALID_PARAM = -1,
    XY_EEPROM_ERROR_OUT_OF_RANGE = -2,
    XY_EEPROM_ERROR_NO_SPACE = -3,
    XY_EEPROM_ERROR_READ = -4,
    XY_EEPROM_ERROR_WRITE = -5,
    XY_EEPROM_ERROR_ERASE = -6,
    XY_EEPROM_ERROR_CORRUPT = -7,
} xy_eeprom_result_t;

typedef xy_eeprom_result_t (*xy_eeprom_read_fn)(void *ctx, uint32_t offset,
                                                uint8_t *data, size_t size);
typedef xy_eeprom_result_t (*xy_eeprom_write_fn)(void *ctx, uint32_t offset,
                                                 const uint8_t *data,
                                                 size_t size);
typedef xy_eeprom_result_t (*xy_eeprom_erase_page_fn)(void *ctx,
                                                      uint32_t page_index);

typedef struct {
    xy_eeprom_read_fn read;
    xy_eeprom_write_fn write;
    xy_eeprom_erase_page_fn erase_page;
} xy_eeprom_ops_t;

typedef struct {
    uint32_t page_size;
    uint32_t page_count;
    uint32_t item_count;
    uint16_t value_size;
    uint16_t write_unit;
} xy_eeprom_config_t;

typedef struct {
    xy_eeprom_config_t config;
    const xy_eeprom_ops_t *ops;
    void *ops_ctx;
    uint8_t *shadow;
    uint8_t *valid_bitmap;
    uint32_t valid_bitmap_size;
    uint32_t active_page;
    uint32_t write_offset;
    uint32_t cycle;
    bool initialized;
} xy_eeprom_t;

xy_eeprom_result_t xy_eeprom_init(xy_eeprom_t *eep,
                                  const xy_eeprom_config_t *config,
                                  const xy_eeprom_ops_t *ops,
                                  void *ops_ctx,
                                  uint8_t *shadow,
                                  uint8_t *valid_bitmap,
                                  uint32_t valid_bitmap_size);

xy_eeprom_result_t xy_eeprom_read(xy_eeprom_t *eep, uint16_t index,
                                  void *data);
xy_eeprom_result_t xy_eeprom_write(xy_eeprom_t *eep, uint16_t index,
                                   const void *data);
xy_eeprom_result_t xy_eeprom_reset(xy_eeprom_t *eep);
uint32_t xy_eeprom_get_cycle(xy_eeprom_t *eep);

#define XY_EEPROM_BITMAP_SIZE(item_count) (((item_count) + 7u) / 8u)

#ifdef __cplusplus
}
#endif

#endif /* XY_EEPROM_H */
