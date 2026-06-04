/**
 * @file xy_fee_nano.c
 * @brief Flash EEPROM Emulation - Nano 精简版实现
 * @version 2.0.0
 * @date 2026-03-15
 *
 * 支持裸机（Bare-metal）和 RTOS 环境
 * 不使用动态内存分配（malloc/free）
 */

#include "xy_fee_nano.h"
#include "xy_string.h"

#if defined(_WIN32) || defined(__unix__) || defined(__APPLE__)
#include <stdlib.h>
#endif

/*==============================================================================
 * 平台和后端配置
 *============================================================================*/

#ifndef XY_OS_BACKEND_BAREMETAL
    #define XY_OS_BACKEND_BAREMETAL  1
    #define XY_OS_BACKEND_FREERTOS   0
    #define XY_OS_BACKEND_RTTHREAD   0
#endif

#if !defined(XY_OS_BACKEND_FREERTOS)
    #define XY_OS_BACKEND_FREERTOS   0
#endif
#if !defined(XY_OS_BACKEND_RTTHREAD)
    #define XY_OS_BACKEND_RTTHREAD   0
#endif

/*==============================================================================
 * 临界区适配
 *============================================================================*/

#if XY_OS_BACKEND_BAREMETAL
    #define EFLASH_ENTER_CRITICAL()    do { } while (0)
    #define EFLASH_EXIT_CRITICAL()     do { } while (0)
#elif XY_OS_BACKEND_FREERTOS
    #define EFLASH_ENTER_CRITICAL()    taskENTER_CRITICAL()
    #define EFLASH_EXIT_CRITICAL()     taskEXIT_CRITICAL()
#elif XY_OS_BACKEND_RTTHREAD
    #define EFLASH_ENTER_CRITICAL()    rt_enter_critical()
    #define EFLASH_EXIT_CRITICAL()     rt_exit_critical()
#else
    #define EFLASH_ENTER_CRITICAL()    do { } while (0)
    #define EFLASH_EXIT_CRITICAL()     do { } while (0)
#endif

/*==============================================================================
 * 常量定义
 *============================================================================*/

#define EFLASH_ERASED_VALUE 0xFF /**< 擦除后的 Flash 值 */

#define FEE_REC_MAGIC 0x58464545u
#define FEE_MIN_PAGES 2u
#define FEE_MAX_VALUE_SIZE 128u
#define FEE_PAGE_STATUS_VALID    0x0au
#define FEE_PAGE_STATUS_TRANSFER 0x5au
#define FEE_PAGE_STATUS_ERASED   0xffu

typedef union {
    struct {
        uint32_t status : 8;
        uint32_t cycle : 24;
    } bits;
    uint32_t word;
} fee_page_header_t;

typedef struct {
    uint32_t magic;
    uint32_t key;
    uint16_t len;
    uint16_t total;
    uint32_t seq;
    uint32_t crc;
} fee_record_hdr_t;

static eflash_result_t fee_raw_read(eflash_handle_t *handle, uint32_t offset,
                                    uint8_t *data, size_t size);
static eflash_result_t fee_raw_write(eflash_handle_t *handle, uint32_t offset,
                                     const uint8_t *data, size_t size);
static eflash_result_t fee_raw_erase_page(eflash_handle_t *handle,
                                          uint32_t page_index);

/*==============================================================================
 * 内部辅助函数
 *============================================================================*/

/**
 * @brief 检查地址范围是否有效（内部调用）
 */
static bool is_address_valid_internal(eflash_handle_t *handle,
                                       uint32_t address, size_t size)
{
    if (handle == NULL || !handle->initialized) {
        return false;
    }

    if (address >= handle->config.total_size) {
        return false;
    }

    if (size == 0) {
        return true;
    }

    if (address + size > handle->config.total_size) {
        return false;
    }

    return true;
}

/**
 * @brief 获取地址对应的页索引（内部调用）
 */
static uint32_t get_page_index_internal(eflash_handle_t *handle,
                                          uint32_t address)
{
    if (handle == NULL || !handle->initialized) {
        return 0;
    }
    return address / handle->config.page_size;
}

static uint32_t fee_align4(uint32_t value)
{
    return (value + 3u) & ~3u;
}

static uint32_t fee_page_header_size(void)
{
    return sizeof(uint32_t);
}

static uint32_t fee_page_base(eflash_handle_t *handle, uint32_t page)
{
    return page * handle->config.page_size;
}

static eflash_result_t fee_read_page_header(eflash_handle_t *handle,
                                            uint32_t page,
                                            fee_page_header_t *hdr)
{
    return fee_raw_read(handle, fee_page_base(handle, page),
                        (uint8_t *)&hdr->word, sizeof(hdr->word));
}

static eflash_result_t fee_write_page_header(eflash_handle_t *handle,
                                             uint32_t page, uint8_t status,
                                             uint32_t cycle)
{
    fee_page_header_t hdr;
    hdr.bits.status = status;
    hdr.bits.cycle = cycle & 0x00ffffffu;
    return fee_raw_write(handle, fee_page_base(handle, page),
                         (const uint8_t *)&hdr.word, sizeof(hdr.word));
}

static bool fee_page_header_is_active(const fee_page_header_t *hdr)
{
    return hdr->bits.status == FEE_PAGE_STATUS_VALID
           || hdr->bits.status == FEE_PAGE_STATUS_TRANSFER;
}

static bool fee_page_is_legacy(eflash_handle_t *handle, uint32_t page)
{
    uint32_t magic;
    if (fee_raw_read(handle, fee_page_base(handle, page),
                     (uint8_t *)&magic, sizeof(magic)) != EFLASH_OK) {
        return false;
    }
    return magic == FEE_REC_MAGIC;
}

static bool fee_page_is_scannable(eflash_handle_t *handle, uint32_t page)
{
    fee_page_header_t hdr;
    if (fee_read_page_header(handle, page, &hdr) != EFLASH_OK) {
        return false;
    }
    return fee_page_header_is_active(&hdr) || fee_page_is_legacy(handle, page);
}

static uint32_t fee_page_record_start(eflash_handle_t *handle,
                                      uint32_t page)
{
    if (fee_page_is_legacy(handle, page)) {
        return fee_page_base(handle, page);
    }
    return fee_page_base(handle, page) + fee_page_header_size();
}

static bool fee_page_erased(eflash_handle_t *handle, uint32_t page)
{
    uint32_t word;
    uint32_t offset = fee_page_base(handle, page);
    uint32_t end = offset + handle->config.page_size;
    while (offset < end) {
        if (fee_raw_read(handle, offset, (uint8_t *)&word, sizeof(word)) != EFLASH_OK) {
            return false;
        }
        if (word != 0xffffffffu) {
            return false;
        }
        offset += sizeof(word);
    }
    return true;
}

static uint32_t fee_page_cycle(eflash_handle_t *handle, uint32_t page)
{
    fee_page_header_t hdr;
    if (fee_read_page_header(handle, page, &hdr) != EFLASH_OK) {
        return 0;
    }
    if (hdr.bits.status != FEE_PAGE_STATUS_VALID
        && hdr.bits.status != FEE_PAGE_STATUS_TRANSFER) {
        return 0;
    }
    return hdr.bits.cycle;
}

static uint32_t fee_crc32_update(uint32_t crc, const uint8_t *data, size_t size)
{
    while (size-- > 0u) {
        crc ^= *data++;
        for (uint32_t i = 0; i < 8u; i++) {
            if ((crc & 1u) != 0u) {
                crc = (crc >> 1) ^ 0xedb88320u;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

static uint32_t fee_record_crc(const fee_record_hdr_t *hdr,
                               const uint8_t *data)
{
    uint32_t crc = 0xffffffffu;
    crc = fee_crc32_update(crc, (const uint8_t *)&hdr->key, sizeof(hdr->key));
    crc = fee_crc32_update(crc, (const uint8_t *)&hdr->len, sizeof(hdr->len));
    crc = fee_crc32_update(crc, (const uint8_t *)&hdr->seq, sizeof(hdr->seq));
    crc = fee_crc32_update(crc, data, hdr->len);
    return ~crc;
}

static eflash_result_t fee_raw_read(eflash_handle_t *handle, uint32_t offset,
                                    uint8_t *data, size_t size)
{
    if (handle->ops != NULL) {
        return handle->ops->read(handle->ops_ctx, offset, data, size);
    }
    memcpy(data, &handle->memory[offset], size);
    return EFLASH_OK;
}

static eflash_result_t fee_raw_write(eflash_handle_t *handle, uint32_t offset,
                                     const uint8_t *data, size_t size)
{
    if (handle->ops != NULL) {
        return handle->ops->write(handle->ops_ctx, offset, data, size);
    }
    for (size_t i = 0; i < size; i++) {
        uint8_t current = handle->memory[offset + i];
        uint8_t new_val = data[i];
        if ((current & new_val) != new_val) {
            return EFLASH_ERROR_WRITE_FAIL;
        }
        handle->memory[offset + i] = current & new_val;
    }
    return EFLASH_OK;
}

static eflash_result_t fee_raw_erase_page(eflash_handle_t *handle,
                                          uint32_t page_index)
{
    if (handle->ops != NULL) {
        return handle->ops->erase_page(handle->ops_ctx, page_index);
    }
    memset(&handle->memory[page_index * handle->config.page_size],
           EFLASH_ERASED_VALUE, handle->config.page_size);
    return EFLASH_OK;
}

static bool fee_record_valid(eflash_handle_t *handle, uint32_t offset,
                             fee_record_hdr_t *hdr)
{
    if (fee_raw_read(handle, offset, (uint8_t *)hdr, sizeof(*hdr)) != EFLASH_OK) {
        return false;
    }
    if (hdr->magic != FEE_REC_MAGIC) {
        return false;
    }
    if (hdr->len == 0u || hdr->total < sizeof(*hdr)
        || (hdr->total & 0x3u) != 0u
        || hdr->total > handle->config.page_size) {
        return false;
    }
    if (offset + hdr->total > handle->config.total_size) {
        return false;
    }

    uint8_t tmp[128];
    uint32_t data_offset = offset + sizeof(*hdr);
    uint32_t remaining = hdr->len;
    uint32_t crc = 0xffffffffu;
    crc = fee_crc32_update(crc, (const uint8_t *)&hdr->key, sizeof(hdr->key));
    crc = fee_crc32_update(crc, (const uint8_t *)&hdr->len, sizeof(hdr->len));
    crc = fee_crc32_update(crc, (const uint8_t *)&hdr->seq, sizeof(hdr->seq));
    while (remaining > 0u) {
        uint32_t chunk = remaining > sizeof(tmp) ? sizeof(tmp) : remaining;
        if (fee_raw_read(handle, data_offset, tmp, chunk) != EFLASH_OK) {
            return false;
        }
        crc = fee_crc32_update(crc, tmp, chunk);
        data_offset += chunk;
        remaining -= chunk;
    }
    crc = ~crc;

    return crc == hdr->crc;
}

static uint32_t fee_page_write_offset(eflash_handle_t *handle,
                                      uint32_t page_index)
{
    uint32_t offset = fee_page_record_start(handle, page_index);
    uint32_t end = fee_page_base(handle, page_index) + handle->config.page_size;

    while (offset + sizeof(fee_record_hdr_t) <= end) {
        uint32_t magic;
        fee_raw_read(handle, offset, (uint8_t *)&magic, sizeof(magic));
        if (magic == 0xffffffffu) {
            return offset;
        }
        fee_record_hdr_t hdr;
        if (!fee_record_valid(handle, offset, &hdr)) {
            return offset;
        }
        offset += hdr.total;
    }

    return end;
}

static uint32_t fee_active_page(eflash_handle_t *handle)
{
    uint32_t best_page = 0;
    uint32_t best_seq = 0;
    bool found = false;

    for (uint32_t page = 0; page < handle->config.page_count; page++) {
        if (!fee_page_is_scannable(handle, page)) {
            continue;
        }
        uint32_t offset = fee_page_record_start(handle, page);
        uint32_t end = fee_page_base(handle, page) + handle->config.page_size;
        while (offset + sizeof(fee_record_hdr_t) <= end) {
            uint32_t magic;
            fee_raw_read(handle, offset, (uint8_t *)&magic, sizeof(magic));
            if (magic == 0xffffffffu) {
                break;
            }
            fee_record_hdr_t hdr;
            if (!fee_record_valid(handle, offset, &hdr)) {
                break;
            }
            if (!found || hdr.seq >= best_seq) {
                best_seq = hdr.seq;
                best_page = page;
                found = true;
            }
            offset += hdr.total;
        }
    }

    return best_page;
}

static uint32_t fee_next_seq(eflash_handle_t *handle)
{
    uint32_t max_seq = 0;

    for (uint32_t page = 0; page < handle->config.page_count; page++) {
        if (!fee_page_is_scannable(handle, page)) {
            continue;
        }
        uint32_t offset = fee_page_record_start(handle, page);
        uint32_t end = fee_page_base(handle, page) + handle->config.page_size;
        while (offset + sizeof(fee_record_hdr_t) <= end) {
            uint32_t magic;
            fee_raw_read(handle, offset, (uint8_t *)&magic, sizeof(magic));
            if (magic == 0xffffffffu) {
                break;
            }
            fee_record_hdr_t hdr;
            if (!fee_record_valid(handle, offset, &hdr)) {
                break;
            }
            if (hdr.seq > max_seq) {
                max_seq = hdr.seq;
            }
            offset += hdr.total;
        }
    }

    return max_seq + 1u;
}

static eflash_result_t fee_append_record(eflash_handle_t *handle,
                                         uint32_t page_index, uint32_t key,
                                         const uint8_t *data, size_t size,
                                         uint32_t seq)
{
    uint32_t total = fee_align4((uint32_t)sizeof(fee_record_hdr_t)
                                + (uint32_t)size);
    uint32_t offset = fee_page_write_offset(handle, page_index);
    uint32_t page_end = (page_index + 1u) * handle->config.page_size;
    if (offset + total > page_end) {
        return EFLASH_ERROR_OUT_OF_RANGE;
    }

    fee_record_hdr_t hdr;
    hdr.magic = FEE_REC_MAGIC;
    hdr.key = key;
    hdr.len = (uint16_t)size;
    hdr.total = (uint16_t)total;
    hdr.seq = seq;
    hdr.crc = fee_record_crc(&hdr, data);

    eflash_result_t result = fee_raw_write(handle, offset, (const uint8_t *)&hdr,
                                           sizeof(hdr));
    if (result != EFLASH_OK) {
        return result;
    }

    uint32_t data_offset = offset + sizeof(hdr);
    size_t remaining = size;
    while (remaining >= 4u) {
        result = fee_raw_write(handle, data_offset, data, 4u);
        if (result != EFLASH_OK) {
            return result;
        }
        data += 4u;
        data_offset += 4u;
        remaining -= 4u;
    }

    if (remaining > 0u) {
        uint8_t tail[4] = {0xff, 0xff, 0xff, 0xff};
        memcpy(tail, data, remaining);
        result = fee_raw_write(handle, data_offset, tail, sizeof(tail));
        if (result != EFLASH_OK) {
            return result;
        }
    }

    return EFLASH_OK;
}

static eflash_result_t fee_find_latest(eflash_handle_t *handle, uint32_t key,
                                       fee_record_hdr_t *out_hdr,
                                       uint32_t *out_offset)
{
    bool found = false;
    fee_record_hdr_t best_hdr;
    uint32_t best_offset = 0;

    for (uint32_t page = 0; page < handle->config.page_count; page++) {
        if (!fee_page_is_scannable(handle, page)) {
            continue;
        }
        uint32_t offset = fee_page_record_start(handle, page);
        uint32_t end = fee_page_base(handle, page) + handle->config.page_size;
        while (offset + sizeof(fee_record_hdr_t) <= end) {
            uint32_t magic;
            fee_raw_read(handle, offset, (uint8_t *)&magic, sizeof(magic));
            if (magic == 0xffffffffu) {
                break;
            }
            fee_record_hdr_t hdr;
            if (!fee_record_valid(handle, offset, &hdr)) {
                break;
            }
            if (hdr.key == key && (!found || hdr.seq >= best_hdr.seq)) {
                best_hdr = hdr;
                best_offset = offset;
                found = true;
            }
            offset += hdr.total;
        }
    }

    if (!found) {
        return EFLASH_ERROR_NOT_INIT;
    }
    if (out_hdr != NULL) {
        *out_hdr = best_hdr;
    }
    if (out_offset != NULL) {
        *out_offset = best_offset;
    }
    return EFLASH_OK;
}

static eflash_result_t fee_copy_latest_records(eflash_handle_t *handle,
                                               uint32_t dst_page,
                                               uint32_t skip_key,
                                               uint32_t *seq)
{
    uint8_t value[FEE_MAX_VALUE_SIZE];

    for (uint32_t page = 0; page < handle->config.page_count; page++) {
        if (page == dst_page || !fee_page_is_scannable(handle, page)) {
            continue;
        }
        uint32_t offset = fee_page_record_start(handle, page);
        uint32_t end = fee_page_base(handle, page) + handle->config.page_size;
        while (offset + sizeof(fee_record_hdr_t) <= end) {
            uint32_t magic;
            fee_raw_read(handle, offset, (uint8_t *)&magic, sizeof(magic));
            if (magic == 0xffffffffu) {
                break;
            }
            fee_record_hdr_t hdr;
            if (!fee_record_valid(handle, offset, &hdr)) {
                break;
            }

            fee_record_hdr_t latest;
            uint32_t latest_offset;
            if (hdr.key != skip_key
                && hdr.len <= sizeof(value)
                && fee_find_latest(handle, hdr.key, &latest, &latest_offset) == EFLASH_OK
                && latest_offset == offset) {
                eflash_result_t result = fee_raw_read(handle,
                                                      offset + sizeof(hdr),
                                                      value, hdr.len);
                if (result != EFLASH_OK) {
                    return result;
                }
                result = fee_append_record(handle, dst_page, hdr.key, value,
                                           hdr.len, (*seq)++);
                if (result != EFLASH_OK) {
                    return result;
                }
            }
            offset += hdr.total;
        }
    }

    return EFLASH_OK;
}

static eflash_result_t fee_read_key(eflash_handle_t *handle, uint32_t key,
                                    uint8_t *data, size_t size)
{
    fee_record_hdr_t hdr;
    uint32_t offset;
    eflash_result_t result = fee_find_latest(handle, key, &hdr, &offset);
    if (result != EFLASH_OK) {
        return result;
    }

    uint32_t copy_len = hdr.len < size ? hdr.len : (uint32_t)size;
    result = fee_raw_read(handle, offset + sizeof(hdr), data, copy_len);
    if (result != EFLASH_OK) {
        return result;
    }
    if (copy_len < size) {
        memset(data + copy_len, 0, size - copy_len);
    }
    return EFLASH_OK;
}

static eflash_result_t fee_write_key(eflash_handle_t *handle, uint32_t key,
                                     const uint8_t *data, size_t size)
{
    if (size == 0u || size > FEE_MAX_VALUE_SIZE) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    uint32_t active = fee_active_page(handle);
    uint32_t seq = fee_next_seq(handle);
    eflash_result_t result = fee_append_record(handle, active, key, data, size,
                                               seq++);
    if (result == EFLASH_OK) {
        return EFLASH_OK;
    }
    if (result != EFLASH_ERROR_OUT_OF_RANGE) {
        return result;
    }

    uint32_t dst_page = (active + 1u) % handle->config.page_count;
    result = fee_raw_erase_page(handle, dst_page);
    if (result != EFLASH_OK) {
        return result;
    }
    handle->page_erased[dst_page] = true;

    uint32_t cycle = fee_page_cycle(handle, active) + 1u;
    if (cycle == 0u || cycle > 0x00ffffffu) {
        cycle = 1u;
    }
    result = fee_write_page_header(handle, dst_page,
                                   FEE_PAGE_STATUS_TRANSFER, cycle);
    if (result != EFLASH_OK) {
        return result;
    }
    handle->page_erased[dst_page] = false;

    result = fee_copy_latest_records(handle, dst_page, key, &seq);
    if (result != EFLASH_OK) {
        return result;
    }

    result = fee_append_record(handle, dst_page, key, data, size, seq++);
    if (result != EFLASH_OK) {
        return result;
    }

    result = fee_write_page_header(handle, dst_page, FEE_PAGE_STATUS_VALID,
                                   cycle);
    if (result != EFLASH_OK) {
        return result;
    }

    result = fee_raw_erase_page(handle, active);
    if (result != EFLASH_OK) {
        return result;
    }
    handle->page_erased[active] = true;

    return EFLASH_OK;
}

static eflash_result_t fee_recover_pages(eflash_handle_t *handle)
{
    bool valid_found = false;
    uint32_t best_valid_page = 0;
    uint32_t best_valid_cycle = 0;
    bool transfer_found = false;
    uint32_t best_transfer_page = 0;
    uint32_t best_transfer_cycle = 0;
    bool all_erased = true;

    for (uint32_t page = 0; page < handle->config.page_count; page++) {
        bool erased = fee_page_erased(handle, page);
        handle->page_erased[page] = erased;
        if (erased) {
            continue;
        }
        all_erased = false;

        fee_page_header_t hdr;
        if (fee_read_page_header(handle, page, &hdr) != EFLASH_OK) {
            return EFLASH_ERROR_WRITE_FAIL;
        }
        if (hdr.bits.status == FEE_PAGE_STATUS_VALID) {
            if (!valid_found || hdr.bits.cycle >= best_valid_cycle) {
                valid_found = true;
                best_valid_cycle = hdr.bits.cycle;
                best_valid_page = page;
            }
        } else if (hdr.bits.status == FEE_PAGE_STATUS_TRANSFER) {
            transfer_found = true;
            if (hdr.bits.cycle >= best_transfer_cycle) {
                best_transfer_cycle = hdr.bits.cycle;
                best_transfer_page = page;
            }
        } else if (!fee_page_is_legacy(handle, page)) {
            return EFLASH_ERROR_WRITE_FAIL;
        }
    }

    if (all_erased) {
        eflash_result_t result = fee_write_page_header(handle, 0,
                                                       FEE_PAGE_STATUS_VALID,
                                                       1u);
        if (result == EFLASH_OK) {
            handle->page_erased[0] = false;
        }
        return result;
    }

    if (!valid_found && transfer_found) {
        eflash_result_t result = fee_write_page_header(handle,
                                                       best_transfer_page,
                                                       FEE_PAGE_STATUS_VALID,
                                                       best_transfer_cycle);
        if (result != EFLASH_OK) {
            return result;
        }
        valid_found = true;
        best_valid_page = best_transfer_page;
    }

    if (valid_found) {
        for (uint32_t page = 0; page < handle->config.page_count; page++) {
            fee_page_header_t hdr;
            if (page == best_valid_page
                || fee_read_page_header(handle, page, &hdr) != EFLASH_OK) {
                continue;
            }
            if (hdr.bits.status == FEE_PAGE_STATUS_TRANSFER
                || hdr.bits.status == FEE_PAGE_STATUS_VALID) {
                eflash_result_t result = fee_raw_erase_page(handle, page);
                if (result != EFLASH_OK) {
                    return result;
                }
                handle->page_erased[page] = true;
            }
        }
    }

    return EFLASH_OK;
}

/*==============================================================================
 * API 函数实现
 *============================================================================*/

/**
 * @brief 使用用户提供的 buffer 初始化 Flash
 *
 * @note 适用于裸机环境，不使用 malloc
 */
eflash_result_t eflash_init_with_buffer(eflash_handle_t *handle,
                                         const eflash_config_t *config,
                                         uint8_t *memory_buffer,
                                         bool *page_erased_buffer)
{
    if (handle == NULL || config == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    /* 验证配置 */
    if (config->page_size == 0 || config->page_size > EFLASH_MAX_PAGE_SIZE) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (config->page_count == 0 || config->page_count > EFLASH_MAX_PAGES) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    /* 计算总大小 */
    uint32_t total_size = config->page_size * config->page_count;
    if (config->total_size > 0 && config->total_size != total_size) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    /* 验证写入单元 */
    if (config->write_unit != EFLASH_WRITE_UNIT_32BIT
        && config->write_unit != EFLASH_WRITE_UNIT_64BIT
        && config->write_unit != EFLASH_WRITE_UNIT_128BIT) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    /* 验证用户提供 buffer */
    if (memory_buffer == NULL || page_erased_buffer == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    EFLASH_ENTER_CRITICAL();

    /* 初始化配置 */
    handle->config = *config;
    handle->config.total_size = total_size;

    /* 使用用户提供的 buffer */
    handle->memory = memory_buffer;
    handle->page_erased = page_erased_buffer;
    handle->ops = NULL;
    handle->ops_ctx = NULL;
    handle->user_provided_buffer = true;

    /* 初始化 Flash 内存为擦除态 */
    memset(handle->memory, EFLASH_ERASED_VALUE, total_size);

    /* 标记所有页为已擦除 */
    for (uint32_t i = 0; i < config->page_count; i++) {
        handle->page_erased[i] = true;
    }

    handle->initialized = true;

    EFLASH_EXIT_CRITICAL();

    return EFLASH_OK;
}

eflash_result_t eflash_init_with_ops(eflash_handle_t *handle,
                                      const eflash_config_t *config,
                                      uint8_t *memory_base,
                                      bool *page_erased_buffer,
                                      const eflash_ops_t *ops,
                                      void *ops_ctx)
{
    if (handle == NULL || config == NULL || memory_base == NULL
        || page_erased_buffer == NULL || ops == NULL || ops->read == NULL
        || ops->write == NULL || ops->erase_page == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (config->page_size == 0 || config->page_size > EFLASH_MAX_PAGE_SIZE) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (config->page_count == 0 || config->page_count > EFLASH_MAX_PAGES) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    uint32_t total_size = config->page_size * config->page_count;
    if (config->total_size > 0 && config->total_size != total_size) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (config->write_unit != EFLASH_WRITE_UNIT_32BIT
        && config->write_unit != EFLASH_WRITE_UNIT_64BIT
        && config->write_unit != EFLASH_WRITE_UNIT_128BIT) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    EFLASH_ENTER_CRITICAL();

    handle->config = *config;
    handle->config.total_size = total_size;
    handle->memory = memory_base;
    handle->page_erased = page_erased_buffer;
    handle->ops = ops;
    handle->ops_ctx = ops_ctx;
    handle->user_provided_buffer = true;

    if (config->page_count >= FEE_MIN_PAGES) {
        eflash_result_t result = fee_recover_pages(handle);
        if (result != EFLASH_OK) {
            EFLASH_EXIT_CRITICAL();
            return result;
        }
    } else {
        for (uint32_t page = 0; page < config->page_count; page++) {
            handle->page_erased[page] = fee_page_erased(handle, page);
        }
    }

    handle->initialized = true;

    EFLASH_EXIT_CRITICAL();

    return EFLASH_OK;
}

/**
 * @brief 初始化 Flash 设备（仅 PC 模拟器使用）
 *
 * @note 仅在 PC 模拟环境使用，嵌入式环境请用 eflash_init_with_buffer
 */
eflash_result_t eflash_init(eflash_handle_t *handle,
                            const eflash_config_t *config)
{
#if defined(_WIN32) || defined(__unix__) || defined(__APPLE__)
    /* PC 环境：使用动态分配模拟 */
    if (handle == NULL || config == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    /* 验证配置 */
    if (config->page_size == 0 || config->page_size > EFLASH_MAX_PAGE_SIZE) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (config->page_count == 0 || config->page_count > EFLASH_MAX_PAGES) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    /* 计算总大小 */
    uint32_t total_size = config->page_size * config->page_count;

    /* 分配内存 */
    handle->memory = (uint8_t *)malloc(total_size);
    if (handle->memory == NULL) {
        return EFLASH_ERROR_WRITE_FAIL;
    }

    /* 分配擦除状态数组 */
    handle->page_erased = (bool *)malloc(config->page_count * sizeof(bool));
    if (handle->page_erased == NULL) {
        free(handle->memory);
        handle->memory = NULL;
        return EFLASH_ERROR_WRITE_FAIL;
    }

    /* 初始化配置 */
    handle->config = *config;
    handle->config.total_size = total_size;
    handle->user_provided_buffer = false;

    /* 初始化 Flash 内存为擦除态 */
    memset(handle->memory, EFLASH_ERASED_VALUE, total_size);

    /* 标记所有页为已擦除 */
    for (uint32_t i = 0; i < config->page_count; i++) {
        handle->page_erased[i] = true;
    }

    handle->initialized = true;

    return EFLASH_OK;
#else
    /* 嵌入式环境：回退到用户 buffer 接口 */
    return EFLASH_ERROR_NOT_INIT;
#endif
}

/**
 * @brief 反初始化 Flash 设备
 */
eflash_result_t eflash_deinit(eflash_handle_t *handle)
{
    if (handle == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    EFLASH_ENTER_CRITICAL();

    if (!handle->initialized) {
        EFLASH_EXIT_CRITICAL();
        return EFLASH_ERROR_NOT_INIT;
    }

    /* 仅在非用户提供的 buffer 情况下释放内存（PC 模拟器） */
#if defined(_WIN32) || defined(__unix__) || defined(__APPLE__)
    if (!handle->user_provided_buffer) {
        if (handle->memory != NULL) {
            free(handle->memory);
            handle->memory = NULL;
        }

        if (handle->page_erased != NULL) {
            free(handle->page_erased);
            handle->page_erased = NULL;
        }
    }
#endif

    handle->initialized = false;
    handle->ops = NULL;
    handle->ops_ctx = NULL;

    EFLASH_EXIT_CRITICAL();

    return EFLASH_OK;
}

/**
 * @brief 读取 Flash 数据
 */
eflash_result_t eflash_read(eflash_handle_t *handle, uint32_t address,
                            uint8_t *data, size_t size)
{
    if (handle == NULL || data == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (!handle->initialized) {
        return EFLASH_ERROR_NOT_INIT;
    }

    if (size == 0) {
        return EFLASH_OK;
    }

    if (handle->ops != NULL && handle->config.page_count >= FEE_MIN_PAGES) {
        return fee_read_key(handle, address, data, size);
    }

    /* 检查地址范围 */
    if (!is_address_valid_internal(handle, address, size)) {
        return EFLASH_ERROR_OUT_OF_RANGE;
    }

    if (handle->ops != NULL) {
        return handle->ops->read(handle->ops_ctx, address, data, size);
    }

    /* 读取数据 */
    memcpy(data, &handle->memory[address], size);

    return EFLASH_OK;
}

/**
 * @brief 写入 Flash 数据
 */
eflash_result_t eflash_write(eflash_handle_t *handle, uint32_t address,
                             const uint8_t *data, size_t size)
{
    if (handle == NULL || data == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (!handle->initialized) {
        return EFLASH_ERROR_NOT_INIT;
    }

    if (size == 0) {
        return EFLASH_OK;
    }

    if (handle->ops != NULL && handle->config.page_count >= FEE_MIN_PAGES) {
        EFLASH_ENTER_CRITICAL();
        eflash_result_t result = fee_write_key(handle, address, data, size);
        EFLASH_EXIT_CRITICAL();
        return result;
    }

    /* 检查地址范围 */
    if (!is_address_valid_internal(handle, address, size)) {
        return EFLASH_ERROR_OUT_OF_RANGE;
    }

    /* 检查对齐 */
    uint32_t write_unit = (uint32_t)handle->config.write_unit;
    if (!EFLASH_IS_ALIGNED(address, write_unit)
        || !EFLASH_IS_ALIGNED(size, write_unit)) {
        return EFLASH_ERROR_ALIGNMENT;
    }

    EFLASH_ENTER_CRITICAL();

    if (handle->ops != NULL) {
        if (handle->config.auto_erase) {
            uint32_t start_page = get_page_index_internal(handle, address);
            uint32_t end_page = get_page_index_internal(handle,
                                                        address + size - 1);
            for (uint32_t page = start_page; page <= end_page; page++) {
                if (page >= handle->config.page_count) {
                    EFLASH_EXIT_CRITICAL();
                    return EFLASH_ERROR_OUT_OF_RANGE;
                }
                if (!handle->page_erased[page]) {
                    eflash_result_t erase_result =
                        handle->ops->erase_page(handle->ops_ctx, page);
                    if (erase_result != EFLASH_OK) {
                        EFLASH_EXIT_CRITICAL();
                        return erase_result;
                    }
                    handle->page_erased[page] = true;
                }
            }
        }

        eflash_result_t result = handle->ops->write(handle->ops_ctx, address,
                                                    data, size);
        if (result == EFLASH_OK) {
            uint32_t start_page = get_page_index_internal(handle, address);
            uint32_t end_page = get_page_index_internal(handle,
                                                        address + size - 1);
            for (uint32_t page = start_page; page <= end_page; page++) {
                handle->page_erased[page] = false;
            }
        }
        EFLASH_EXIT_CRITICAL();
        return result;
    }

    /* 自动擦除页（如果启用） */
    if (handle->config.auto_erase) {
        uint32_t start_page = get_page_index_internal(handle, address);
        uint32_t end_page = get_page_index_internal(handle,
                                                     address + size - 1);

        for (uint32_t page = start_page; page <= end_page; page++) {
            if (page >= handle->config.page_count) {
                EFLASH_EXIT_CRITICAL();
                return EFLASH_ERROR_OUT_OF_RANGE;
            }
            if (!handle->page_erased[page]) {
                eflash_result_t result = eflash_erase_page(handle, page);
                if (result != EFLASH_OK) {
                    EFLASH_EXIT_CRITICAL();
                    return result;
                }
            }
        }
    }

    /* 模拟 Flash 写入操作（只能将 1 变为 0） */
    for (size_t i = 0; i < size; i++) {
        uint32_t addr = address + i;
        uint8_t current = handle->memory[addr];
        uint8_t new_val = data[i];

        /* Flash 写入只能清除位（1 -> 0），不能设置位（0 -> 1） */
        if ((current & new_val) != new_val) {
            /* 尝试写入 1 但当前位置是 0 - 需要先擦除 */
            if (!handle->config.auto_erase) {
                EFLASH_EXIT_CRITICAL();
                return EFLASH_ERROR_WRITE_FAIL;
            }
        }

        handle->memory[addr] = current & new_val;
    }

    /* 标记受影响的页为未擦除 */
    uint32_t start_page = get_page_index_internal(handle, address);
    uint32_t end_page = get_page_index_internal(handle, address + size - 1);
    for (uint32_t page = start_page; page <= end_page; page++) {
        handle->page_erased[page] = false;
    }

    EFLASH_EXIT_CRITICAL();

    return EFLASH_OK;
}

/**
 * @brief 擦除 Flash 页
 */
eflash_result_t eflash_erase_page(eflash_handle_t *handle, uint32_t page_index)
{
    if (handle == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (!handle->initialized) {
        return EFLASH_ERROR_NOT_INIT;
    }

    EFLASH_ENTER_CRITICAL();

    if (page_index >= handle->config.page_count) {
        EFLASH_EXIT_CRITICAL();
        return EFLASH_ERROR_OUT_OF_RANGE;
    }

    if (handle->ops != NULL) {
        eflash_result_t result = handle->ops->erase_page(handle->ops_ctx,
                                                         page_index);
        if (result == EFLASH_OK) {
            handle->page_erased[page_index] = true;
        }
        EFLASH_EXIT_CRITICAL();
        return result;
    }

    /* 擦除页（设置所有字节为 0xFF） */
    uint32_t page_offset = page_index * handle->config.page_size;
    memset(&handle->memory[page_offset], EFLASH_ERASED_VALUE,
           handle->config.page_size);

    /* 标记页为已擦除 */
    handle->page_erased[page_index] = true;

    EFLASH_EXIT_CRITICAL();

    return EFLASH_OK;
}

/**
 * @brief 擦除 Flash 扇区（基于地址）
 */
eflash_result_t eflash_erase_sector(eflash_handle_t *handle, uint32_t address)
{
    if (handle == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (!handle->initialized) {
        return EFLASH_ERROR_NOT_INIT;
    }

    /* 获取地址对应的页索引 */
    uint32_t page_index = get_page_index_internal(handle, address);

    return eflash_erase_page(handle, page_index);
}

/**
 * @brief 擦除整个 Flash
 */
eflash_result_t eflash_erase_all(eflash_handle_t *handle)
{
    if (handle == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (!handle->initialized) {
        return EFLASH_ERROR_NOT_INIT;
    }

    EFLASH_ENTER_CRITICAL();

    if (handle->ops != NULL) {
        for (uint32_t page = 0; page < handle->config.page_count; page++) {
            eflash_result_t result = handle->ops->erase_page(handle->ops_ctx,
                                                             page);
            if (result != EFLASH_OK) {
                EFLASH_EXIT_CRITICAL();
                return result;
            }
            handle->page_erased[page] = true;
        }
        EFLASH_EXIT_CRITICAL();
        return EFLASH_OK;
    }

    /* 擦除所有 Flash 内存 */
    memset(handle->memory, EFLASH_ERASED_VALUE, handle->config.total_size);

    /* 标记所有页为已擦除 */
    for (uint32_t i = 0; i < handle->config.page_count; i++) {
        handle->page_erased[i] = true;
    }

    EFLASH_EXIT_CRITICAL();

    return EFLASH_OK;
}

/**
 * @brief 获取 Flash 信息
 */
eflash_result_t eflash_get_info(eflash_handle_t *handle,
                                eflash_config_t *config)
{
    if (handle == NULL || config == NULL) {
        return EFLASH_ERROR_INVALID_PARAM;
    }

    if (!handle->initialized) {
        return EFLASH_ERROR_NOT_INIT;
    }

    EFLASH_ENTER_CRITICAL();
    *config = handle->config;
    EFLASH_EXIT_CRITICAL();

    return EFLASH_OK;
}

/**
 * @brief 检查地址范围是否有效
 */
bool eflash_is_address_valid(eflash_handle_t *handle, uint32_t address,
                             size_t size)
{
    EFLASH_ENTER_CRITICAL();
    bool result = is_address_valid_internal(handle, address, size);
    EFLASH_EXIT_CRITICAL();
    return result;
}

/**
 * @brief 获取地址对应的页索引
 */
uint32_t eflash_get_page_index(eflash_handle_t *handle, uint32_t address)
{
    EFLASH_ENTER_CRITICAL();
    uint32_t result = get_page_index_internal(handle, address);
    EFLASH_EXIT_CRITICAL();
    return result;
}

/**
 * @brief 检查页是否已擦除
 */
bool eflash_is_page_erased(eflash_handle_t *handle, uint32_t page_index)
{
    if (handle == NULL || !handle->initialized) {
        return false;
    }

    EFLASH_ENTER_CRITICAL();

    if (page_index >= handle->config.page_count) {
        EFLASH_EXIT_CRITICAL();
        return false;
    }

    bool result = handle->page_erased[page_index];

    EFLASH_EXIT_CRITICAL();

    return result;
}
