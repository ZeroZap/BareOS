#include "xy_eeprom.h"
#include "xy_string.h"

#define XY_EEPROM_PAGE_STATUS_VALID    0x0au
#define XY_EEPROM_PAGE_STATUS_TRANSFER 0x5au
#define XY_EEPROM_PAGE_STATUS_ERASED   0xffu
#define XY_EEPROM_ERASED_WORD          0xffffffffu

typedef union {
    struct {
        uint32_t status : 8;
        uint32_t cycle : 24;
    } bits;
    uint32_t word;
} xy_eeprom_page_header_t;

static uint32_t align_up(uint32_t value, uint32_t align)
{
    return (value + align - 1u) & ~(align - 1u);
}

static uint32_t record_size(xy_eeprom_t *eep)
{
    return align_up(4u + eep->config.value_size, eep->config.write_unit);
}

static uint32_t page_header_size(xy_eeprom_t *eep)
{
    return align_up(sizeof(uint32_t), eep->config.write_unit);
}

#if XY_EEPROM_ENABLE_CRC16
static uint16_t crc16_update(uint16_t crc, const uint8_t *data, size_t size)
{
    while (size-- > 0u) {
        crc ^= (uint16_t)(*data++) << 8;
        for (uint32_t i = 0; i < 8u; i++) {
            if ((crc & 0x8000u) != 0u) {
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }
    return crc;
}
#endif

static uint16_t record_crc(uint16_t index, const uint8_t *data, size_t size)
{
#if XY_EEPROM_ENABLE_CRC16
    uint16_t crc = 0xffffu;
    crc = crc16_update(crc, (const uint8_t *)&index, sizeof(index));
    crc = crc16_update(crc, data, size);
    return crc;
#else
    (void)index;
    (void)data;
    (void)size;
    return 0xffffu;
#endif
}

static bool is_power_of_two(uint32_t value)
{
    return value != 0u && (value & (value - 1u)) == 0u;
}

static void bitmap_set(uint8_t *bitmap, uint32_t index)
{
    bitmap[index >> 3] |= (uint8_t)(1u << (index & 7u));
}

static bool bitmap_get(const uint8_t *bitmap, uint32_t index)
{
    return (bitmap[index >> 3] & (uint8_t)(1u << (index & 7u))) != 0u;
}

static bool page_is_erased(xy_eeprom_t *eep, uint32_t page);

static xy_eeprom_result_t raw_read(xy_eeprom_t *eep, uint32_t offset,
                                   uint8_t *data, size_t size)
{
    return eep->ops->read(eep->ops_ctx, offset, data, size);
}

static xy_eeprom_result_t raw_write(xy_eeprom_t *eep, uint32_t offset,
                                    const uint8_t *data, size_t size)
{
    return eep->ops->write(eep->ops_ctx, offset, data, size);
}

static xy_eeprom_result_t raw_erase_page(xy_eeprom_t *eep, uint32_t page)
{
    return eep->ops->erase_page(eep->ops_ctx, page);
}

static xy_eeprom_result_t read_page_header(xy_eeprom_t *eep, uint32_t page,
                                           xy_eeprom_page_header_t *hdr)
{
    return raw_read(eep, page * eep->config.page_size, (uint8_t *)&hdr->word,
                    sizeof(hdr->word));
}

static xy_eeprom_result_t write_page_header(xy_eeprom_t *eep, uint32_t page,
                                            uint8_t status, uint32_t cycle)
{
    xy_eeprom_page_header_t hdr;
    uint8_t buf[XY_EEPROM_MAX_WRITE_UNIT];
    hdr.bits.status = status;
    hdr.bits.cycle = cycle & 0x00ffffffu;
    memset(buf, 0xff, page_header_size(eep));
    memcpy(buf, &hdr.word, sizeof(hdr.word));
    return raw_write(eep, page * eep->config.page_size, buf,
                     page_header_size(eep));
}

static xy_eeprom_result_t find_active_page(xy_eeprom_t *eep)
{
    bool found = false;
    uint32_t best_cycle = 0;
    uint32_t best_page = 0;
    bool transfer_found = false;
    uint32_t transfer_cycle = 0;
    uint32_t transfer_page = 0;

    for (uint32_t page = 0; page < eep->config.page_count; page++) {
        xy_eeprom_page_header_t hdr;
        if (read_page_header(eep, page, &hdr) != XY_EEPROM_OK) {
            return XY_EEPROM_ERROR_READ;
        }
        if (hdr.bits.status == XY_EEPROM_PAGE_STATUS_VALID
            && (!found || hdr.bits.cycle >= best_cycle)) {
            found = true;
            best_cycle = hdr.bits.cycle;
            best_page = page;
        } else if (hdr.bits.status == XY_EEPROM_PAGE_STATUS_TRANSFER) {
            transfer_found = true;
            if (hdr.bits.cycle >= transfer_cycle) {
                transfer_cycle = hdr.bits.cycle;
                transfer_page = page;
            }
        } else if (hdr.bits.status != XY_EEPROM_PAGE_STATUS_ERASED
                   && !page_is_erased(eep, page)) {
            return XY_EEPROM_ERROR_CORRUPT;
        }
    }

    if (!found) {
        if (transfer_found) {
            xy_eeprom_result_t ret = write_page_header(eep, transfer_page,
                                                       XY_EEPROM_PAGE_STATUS_VALID,
                                                       transfer_cycle);
            if (ret != XY_EEPROM_OK) {
                return ret;
            }
            found = true;
            best_cycle = transfer_cycle;
            best_page = transfer_page;
        } else {
            return XY_EEPROM_ERROR_CORRUPT;
        }
    }

    for (uint32_t page = 0; page < eep->config.page_count; page++) {
        xy_eeprom_page_header_t hdr;
        if (read_page_header(eep, page, &hdr) == XY_EEPROM_OK
            && hdr.bits.status == XY_EEPROM_PAGE_STATUS_TRANSFER
            && page != best_page) {
            (void)raw_erase_page(eep, page);
        }
    }

    eep->active_page = best_page;
    eep->cycle = best_cycle;
    return XY_EEPROM_OK;
}

static bool page_is_erased(xy_eeprom_t *eep, uint32_t page)
{
    uint32_t word;
    uint32_t offset = page * eep->config.page_size;
    uint32_t end = offset + eep->config.page_size;

    while (offset < end) {
        if (raw_read(eep, offset, (uint8_t *)&word, sizeof(word)) != XY_EEPROM_OK) {
            return false;
        }
        if (word != XY_EEPROM_ERASED_WORD) {
            return false;
        }
        offset += sizeof(word);
    }
    return true;
}

static xy_eeprom_result_t format_empty(xy_eeprom_t *eep)
{
    for (uint32_t page = 0; page < eep->config.page_count; page++) {
        xy_eeprom_result_t ret = raw_erase_page(eep, page);
        if (ret != XY_EEPROM_OK) {
            return ret;
        }
    }
    eep->active_page = 0;
    eep->cycle = 1;
    eep->write_offset = page_header_size(eep);
    memset(eep->valid_bitmap, 0, eep->valid_bitmap_size);
    return write_page_header(eep, 0, XY_EEPROM_PAGE_STATUS_VALID, eep->cycle);
}

static xy_eeprom_result_t scan_active_page(xy_eeprom_t *eep)
{
    uint32_t rec_size = record_size(eep);
    uint32_t page_base = eep->active_page * eep->config.page_size;
    uint32_t offset = page_base + page_header_size(eep);
    uint32_t end = page_base + eep->config.page_size;

    memset(eep->valid_bitmap, 0, eep->valid_bitmap_size);

    while (offset + rec_size <= end) {
        uint16_t index;
        if (raw_read(eep, offset, (uint8_t *)&index, sizeof(index)) != XY_EEPROM_OK) {
            return XY_EEPROM_ERROR_READ;
        }
        if (index == 0xffffu) {
            break;
        }
        uint16_t crc;
        if (raw_read(eep, offset + sizeof(index), (uint8_t *)&crc,
                     sizeof(crc)) != XY_EEPROM_OK) {
            return XY_EEPROM_ERROR_READ;
        }
        if (index < eep->config.item_count) {
            uint8_t *dst = eep->shadow + index * eep->config.value_size;
            if (raw_read(eep, offset + sizeof(index) + sizeof(crc), dst,
                         eep->config.value_size) != XY_EEPROM_OK) {
                return XY_EEPROM_ERROR_READ;
            }
            if (crc == record_crc(index, dst, eep->config.value_size)) {
                bitmap_set(eep->valid_bitmap, index);
            } else {
                break;
            }
        }
        offset += rec_size;
    }

    eep->write_offset = offset - page_base;
    return XY_EEPROM_OK;
}

static xy_eeprom_result_t append_record(xy_eeprom_t *eep, uint16_t index,
                                        const uint8_t *data)
{
    uint32_t rec_size = record_size(eep);
    uint32_t page_base = eep->active_page * eep->config.page_size;
    uint32_t offset = page_base + eep->write_offset;
    uint32_t page_end = page_base + eep->config.page_size;
    uint8_t rec_buf[XY_EEPROM_MAX_RECORD_SIZE];
    uint16_t crc = record_crc(index, data, eep->config.value_size);

    if (rec_size > sizeof(rec_buf)) {
        return XY_EEPROM_ERROR_INVALID_PARAM;
    }
    if (offset + rec_size > page_end) {
        return XY_EEPROM_ERROR_NO_SPACE;
    }

    memset(rec_buf, 0xff, rec_size);
    memcpy(rec_buf, &index, sizeof(index));
    memcpy(rec_buf + sizeof(index), &crc, sizeof(crc));
    memcpy(rec_buf + sizeof(index) + sizeof(crc), data, eep->config.value_size);

    xy_eeprom_result_t ret = raw_write(eep, offset, rec_buf, rec_size);
    if (ret != XY_EEPROM_OK) {
        return ret;
    }
    eep->write_offset += rec_size;
    return XY_EEPROM_OK;
}

static xy_eeprom_result_t page_transfer(xy_eeprom_t *eep)
{
    uint32_t old_page = eep->active_page;
    uint32_t new_page = (old_page + 1u) % eep->config.page_count;

    xy_eeprom_result_t ret = raw_erase_page(eep, new_page);
    if (ret != XY_EEPROM_OK) {
        return ret;
    }

    eep->active_page = new_page;
    eep->cycle = (eep->cycle + 1u) & 0x00ffffffu;
    if (eep->cycle == 0u) {
        eep->cycle = 1u;
    }
    eep->write_offset = page_header_size(eep);

    ret = write_page_header(eep, new_page, XY_EEPROM_PAGE_STATUS_TRANSFER,
                            eep->cycle);
    if (ret != XY_EEPROM_OK) {
        return ret;
    }

    for (uint32_t index = 0; index < eep->config.item_count; index++) {
        if (bitmap_get(eep->valid_bitmap, index)) {
            ret = append_record(eep, (uint16_t)index,
                                eep->shadow + index * eep->config.value_size);
            if (ret != XY_EEPROM_OK) {
                return ret;
            }
        }
    }

    ret = write_page_header(eep, new_page, XY_EEPROM_PAGE_STATUS_VALID,
                            eep->cycle);
    if (ret != XY_EEPROM_OK) {
        return ret;
    }

    return raw_erase_page(eep, old_page);
}

xy_eeprom_result_t xy_eeprom_init(xy_eeprom_t *eep,
                                  const xy_eeprom_config_t *config,
                                  const xy_eeprom_ops_t *ops,
                                  void *ops_ctx,
                                  uint8_t *shadow,
                                  uint8_t *valid_bitmap,
                                  uint32_t valid_bitmap_size)
{
    if (eep == NULL || config == NULL || ops == NULL || ops->read == NULL
        || ops->write == NULL || ops->erase_page == NULL || shadow == NULL
        || valid_bitmap == NULL) {
        return XY_EEPROM_ERROR_INVALID_PARAM;
    }
    if (config->write_unit > XY_EEPROM_MAX_WRITE_UNIT
        || !is_power_of_two(config->write_unit)) {
        return XY_EEPROM_ERROR_INVALID_PARAM;
    }
    if (config->page_size <= align_up(sizeof(uint32_t), config->write_unit)
        || config->page_count < 2u
        || (config->page_count & 1u) != 0u || config->item_count == 0u
        || config->item_count > 0xffffu || config->value_size == 0u
        || record_size((xy_eeprom_t *)&(xy_eeprom_t){ .config = *config }) > XY_EEPROM_MAX_RECORD_SIZE
        || valid_bitmap_size < XY_EEPROM_BITMAP_SIZE(config->item_count)) {
        return XY_EEPROM_ERROR_INVALID_PARAM;
    }

    eep->config = *config;
    eep->ops = ops;
    eep->ops_ctx = ops_ctx;
    eep->shadow = shadow;
    eep->valid_bitmap = valid_bitmap;
    eep->valid_bitmap_size = valid_bitmap_size;
    eep->initialized = false;

    if (find_active_page(eep) != XY_EEPROM_OK) {
        bool all_erased = true;
        for (uint32_t page = 0; page < eep->config.page_count; page++) {
            if (!page_is_erased(eep, page)) {
                all_erased = false;
                break;
            }
        }
        if (!all_erased) {
            return XY_EEPROM_ERROR_CORRUPT;
        }
        xy_eeprom_result_t ret = format_empty(eep);
        if (ret != XY_EEPROM_OK) {
            return ret;
        }
    }

    xy_eeprom_result_t ret = scan_active_page(eep);
    if (ret != XY_EEPROM_OK) {
        return ret;
    }

    eep->initialized = true;
    return XY_EEPROM_OK;
}

xy_eeprom_result_t xy_eeprom_read(xy_eeprom_t *eep, uint16_t index,
                                  void *data)
{
    if (eep == NULL || data == NULL) {
        return XY_EEPROM_ERROR_INVALID_PARAM;
    }
    if (!eep->initialized) {
        return XY_EEPROM_ERROR_CORRUPT;
    }
    if (index >= eep->config.item_count) {
        return XY_EEPROM_ERROR_OUT_OF_RANGE;
    }
    if (!bitmap_get(eep->valid_bitmap, index)) {
        return XY_EEPROM_ERROR_CORRUPT;
    }

    memcpy(data, eep->shadow + index * eep->config.value_size,
           eep->config.value_size);
    return XY_EEPROM_OK;
}

xy_eeprom_result_t xy_eeprom_write(xy_eeprom_t *eep, uint16_t index,
                                   const void *data)
{
    if (eep == NULL || data == NULL) {
        return XY_EEPROM_ERROR_INVALID_PARAM;
    }
    if (!eep->initialized) {
        return XY_EEPROM_ERROR_CORRUPT;
    }
    if (index >= eep->config.item_count) {
        return XY_EEPROM_ERROR_OUT_OF_RANGE;
    }

    uint8_t *dst = eep->shadow + index * eep->config.value_size;
    memcpy(dst, data, eep->config.value_size);
    bitmap_set(eep->valid_bitmap, index);

    xy_eeprom_result_t ret = append_record(eep, index, dst);
    if (ret == XY_EEPROM_ERROR_NO_SPACE) {
        ret = page_transfer(eep);
    }
    return ret;
}

xy_eeprom_result_t xy_eeprom_reset(xy_eeprom_t *eep)
{
    if (eep == NULL || eep->ops == NULL) {
        return XY_EEPROM_ERROR_INVALID_PARAM;
    }
    return format_empty(eep);
}

uint32_t xy_eeprom_get_cycle(xy_eeprom_t *eep)
{
    if (eep == NULL) {
        return 0;
    }
    return eep->cycle;
}
