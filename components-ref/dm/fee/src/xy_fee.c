/**
 * @file xy_fee.c
 * @brief Flash EEPROM Emulation 实现
 * @version 2.0.0
 * @date 2026-03-15
 *
 * 支持裸机（Bare-metal）和 RTOS 环境
 */

#include "fee.h"
#include <string.h>

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
    #define FEE_ENTER_CRITICAL()    do { } while (0)
    #define FEE_EXIT_CRITICAL()     do { } while (0)
#elif XY_OS_BACKEND_FREERTOS
    #define FEE_ENTER_CRITICAL()    taskENTER_CRITICAL()
    #define FEE_EXIT_CRITICAL()     taskEXIT_CRITICAL()
#elif XY_OS_BACKEND_RTTHREAD
    #define FEE_ENTER_CRITICAL()    rt_enter_critical()
    #define FEE_EXIT_CRITICAL()      rt_exit_critical()
#else
    #define FEE_ENTER_CRITICAL()    do { } while (0)
    #define FEE_EXIT_CRITICAL()     do { } while (0)
#endif

/*==============================================================================
 * 内部辅助函数
 *============================================================================*/

/**
 * @brief 获取工作缓冲区指针
 */
static inline uint8_t *get_work_buffer(fee_handle_t *h)
{
    /* work_buffer 存储在 handle 末尾的用户提供空间中 */
    return (uint8_t *)h + sizeof(fee_handle_t);
}

/**
 * @brief 获取 Flash 操作接口指针
 */
static inline const fee_flash_ops_t *get_flash_ops(fee_handle_t *h)
{
    return h->flash_ops;
}

/*==============================================================================
 * CRC16计算
 *============================================================================*/

/**
 * @brief CRC16-CCITT计算
 */
static uint16_t crc16_calc(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    while (len--) {
        crc ^= *data++;
        for (uint8_t i = 8; i; i--) {
            crc = (crc & 1) ? ((crc >> 1) ^ 0xA001) : (crc >> 1);
        }
    }
    return crc;
}

/*==============================================================================
 * Flash地址计算
 *============================================================================*/

static inline uint32_t get_fee_page_addr(fee_handle_t *h, uint8_t page_idx)
{
    return (uint32_t)(h->flash_base + (uint32_t)page_idx * h->fee_page_size);
}

static inline uint32_t get_data_area_addr(fee_handle_t *h, uint8_t page_idx)
{
    return get_fee_page_addr(h, page_idx) + h->aligned_header_size;
}

/*==============================================================================
 * Flash操作封装
 *============================================================================*/

static fee_status_t flash_read(fee_handle_t *h, uint32_t addr,
                               uint8_t *buf, uint16_t len)
{
    const fee_flash_ops_t *ops = get_flash_ops(h);
    return ops->read(addr, buf, len) == 0 ? FEE_OK : FEE_ERROR_FLASH;
}

static fee_status_t flash_write(fee_handle_t *h, uint32_t addr,
                                const uint8_t *buf, uint16_t len)
{
    const fee_flash_ops_t *ops = get_flash_ops(h);

    /* 地址对齐检查 */
    if (addr % h->write_granularity != 0) {
        return FEE_ERROR_FLASH;
    }

    /* 长度对齐检查 */
    if (len % h->write_granularity != 0) {
        return FEE_ERROR_FLASH;
    }

    return ops->write(addr, buf, len) == 0 ? FEE_OK : FEE_ERROR_FLASH;
}

static fee_status_t flash_erase_fee_page(fee_handle_t *h, uint8_t page_idx)
{
    const fee_flash_ops_t *ops = get_flash_ops(h);
    uint32_t base = get_fee_page_addr(h, page_idx);
    uint16_t flash_page_size = h->fee_page_size / h->pages_per_fee_page;

    for (uint8_t i = 0; i < h->pages_per_fee_page; i++) {
        if (ops->erase(base + (uint32_t)i * flash_page_size) != 0) {
            return FEE_ERROR_FLASH;
        }
    }
    return FEE_OK;
}

/*==============================================================================
 * FEE Page Header操作
 *============================================================================*/

static fee_status_t read_page_header(fee_handle_t *h, uint8_t page_idx,
                                     fee_page_header_t *hdr)
{
    uint32_t addr = get_fee_page_addr(h, page_idx);

    fee_status_t status =
        flash_read(h, addr, (uint8_t *)hdr, FEE_PAGE_HEADER_BASE_SIZE);
    if (status != FEE_OK)
        return status;

    /* 验证CRC */
    uint16_t crc =
        crc16_calc((uint8_t *)hdr, offsetof(fee_page_header_t, header_crc));
    return (crc == hdr->header_crc) ? FEE_OK : FEE_ERROR_FLASH;
}

static fee_status_t write_page_header(fee_handle_t *h, uint8_t page_idx,
                                      fee_page_header_t *hdr)
{
    uint32_t addr        = get_fee_page_addr(h, page_idx);
    uint8_t aligned_size = h->aligned_header_size;
    uint8_t *work_buffer = get_work_buffer(h);

    /* 计算CRC */
    hdr->header_crc =
        crc16_calc((uint8_t *)hdr, offsetof(fee_page_header_t, header_crc));

    /* 准备对齐缓冲区 */
    memset(work_buffer, 0xFF, aligned_size);
    memcpy(work_buffer, hdr, FEE_PAGE_HEADER_BASE_SIZE);

    /* 按颗粒写入 */
    uint8_t gran = h->write_granularity;
    for (uint16_t offset = 0; offset < aligned_size; offset += gran) {
        fee_status_t status =
            flash_write(h, addr + offset, work_buffer + offset, gran);
        if (status != FEE_OK)
            return status;
    }

    return FEE_OK;
}

/*==============================================================================
 * 写入偏移计算
 *============================================================================*/

/**
 * @brief 通过扫描Flash计算当前写入位置
 */
static uint16_t calc_write_offset(fee_handle_t *h, uint8_t page_idx)
{
    uint32_t addr     = get_data_area_addr(h, page_idx);
    uint16_t max_size = h->data_area_size;
    uint16_t offset   = 0;
    uint8_t rec_size  = h->write_granularity;

    while (offset + rec_size <= max_size) {
        fee_record_header_t rec;

        if (flash_read(h, addr + offset, (uint8_t *)&rec, FEE_RECORD_HEADER_SIZE)
            != FEE_OK) {
            break;
        }

        if (FEE_RECORD_IS_ERASED(&rec)) {
            return offset;
        }

        offset += rec_size;
    }

    return offset;
}

/*==============================================================================
 * Record操作
 *============================================================================*/

/**
 * @brief 写入一条Record
 * @note Record大小固定为1个write_granularity
 */
static fee_status_t write_record(fee_handle_t *h, uint16_t addr,
                                 const uint8_t *data, uint8_t len)
{
    uint16_t max_size = h->data_area_size;
    uint8_t gran      = h->write_granularity;
    uint8_t *work_buffer = get_work_buffer(h);

    /* 检查空间 */
    if (h->write_offset + gran > max_size) {
        return FEE_ERROR_FULL;
    }

    /* 检查数据长度 */
    if (len > h->record_data_size) {
        return FEE_ERROR_PARAM;
    }

    /* 构造Record */
    fee_record_header_t rec;
    rec.addr = addr;
    rec.crc  = crc16_calc(data, len);

    /* 组装到工作缓冲区（Header + Data，总大小=颗粒） */
    memset(work_buffer, 0xFF, gran);
    memcpy(work_buffer, &rec, FEE_RECORD_HEADER_SIZE);
    memcpy(work_buffer + FEE_RECORD_HEADER_SIZE, data, len);

    /* 一次写入（正好1个颗粒） */
    uint32_t write_addr =
        get_data_area_addr(h, h->active_page) + h->write_offset;
    fee_status_t status = flash_write(h, write_addr, work_buffer, gran);
    if (status != FEE_OK)
        return status;

    h->write_offset += gran;
    return FEE_OK;
}

/*==============================================================================
 * Cache重建
 *============================================================================*/

/**
 * @brief 从Flash重建Cache（掉电恢复的关键）
 */
static fee_status_t rebuild_cache(fee_handle_t *h, uint8_t page_idx)
{
    uint32_t addr     = get_data_area_addr(h, page_idx);
    uint16_t max_size = h->data_area_size;
    uint16_t offset   = 0;
    uint8_t gran      = h->write_granularity;
    uint8_t data_size = h->record_data_size;
    uint8_t data_buf[FEE_MAX_GRANULARITY];

    /* 清空Cache */
    memset(h->cache, 0xFF, h->cache_size);

    /* 遍历所有Record */
    while (offset + gran <= max_size) {
        fee_record_header_t rec;

        if (flash_read(h, addr + offset, (uint8_t *)&rec, FEE_RECORD_HEADER_SIZE)
            != FEE_OK) {
            break;
        }

        if (FEE_RECORD_IS_ERASED(&rec))
            break;

        /* 读取Record数据 */
        if (flash_read(h, addr + offset + FEE_RECORD_HEADER_SIZE, data_buf, data_size)
            == FEE_OK) {

            /* 验证CRC */
            uint16_t calc_crc = crc16_calc(data_buf, data_size);
            if (calc_crc == rec.crc) {
                /* 更新Cache（后面的Record会覆盖前面的，实现最新值） */
                if (rec.addr + data_size <= h->cache_size) {
                    memcpy(&h->cache[rec.addr], data_buf, data_size);
                }
            }
        }

        offset += gran;
    }

    h->write_offset = offset;
    h->flags |= FEE_FLAG_VALID;

    return FEE_OK;
}

/*==============================================================================
 * 2-Page管理
 *============================================================================*/

/**
 * @brief 查找活动FEE Page
 * @return 0或1，-1表示无有效页
 */
static int8_t find_active_page(fee_handle_t *h)
{
    fee_page_header_t hdr0, hdr1;
    bool valid0 = false, valid1 = false;

    if (read_page_header(h, 0, &hdr0) == FEE_OK) {
        if (hdr0.page_state == FEE_PAGE_STATE_ACTIVE
            || hdr0.page_state == FEE_PAGE_STATE_RECEIVING) {
            valid0 = true;
        }
    }

    if (read_page_header(h, 1, &hdr1) == FEE_OK) {
        if (hdr1.page_state == FEE_PAGE_STATE_ACTIVE
            || hdr1.page_state == FEE_PAGE_STATE_RECEIVING) {
            valid1 = true;
        }
    }

    /* 两个都有效，选择擦除次数大的（最新的） */
    if (valid0 && valid1) {
        return (hdr0.erase_count >= hdr1.erase_count) ? 0 : 1;
    }

    if (valid0)
        return 0;
    if (valid1)
        return 1;

    return -1;
}

/*==============================================================================
 * GC（垃圾回收）
 *============================================================================*/

/**
 * @brief 执行GC迁移
 * @note 将Cache中的有效数据迁移到另一个FEE Page
 */
static fee_status_t fee_gc_migrate(fee_handle_t *h)
{
    fee_page_header_t old_hdr, new_hdr;
    uint8_t new_page = h->active_page ^ 1; /* 0->1, 1->0 */
    uint8_t data_size = h->record_data_size;
    uint8_t *work_buffer = get_work_buffer(h);

    /* 读取旧页头 */
    if (read_page_header(h, h->active_page, &old_hdr) != FEE_OK) {
        return FEE_ERROR_FLASH;
    }

    /* 检查擦除次数限制 */
    if (old_hdr.erase_count >= h->max_erase_count) {
        return FEE_ERROR_ERASE_LIMIT;
    }

    /* 擦除新页 */
    if (flash_erase_fee_page(h, new_page) != FEE_OK) {
        return FEE_ERROR_FLASH;
    }

    /* 写入新页头（RECEIVING状态） */
    new_hdr.page_state  = FEE_PAGE_STATE_RECEIVING;
    new_hdr.erase_count = old_hdr.erase_count + 1;

    if (write_page_header(h, new_page, &new_hdr) != FEE_OK) {
        return FEE_ERROR_FLASH;
    }

    /* 切换活动页 */
    uint8_t old_page = h->active_page;
    h->active_page   = new_page;
    h->write_offset  = 0;

    /* 迁移Cache中的有效数据 */
    for (uint16_t addr = 0; addr < h->cache_size; addr += data_size) {
        bool has_data = false;
        uint8_t chunk = (addr + data_size <= h->cache_size)
                            ? data_size
                            : (h->cache_size - addr);

        /* 检查是否有非0xFF数据 */
        for (uint8_t i = 0; i < chunk; i++) {
            if (h->cache[addr + i] != 0xFF) {
                has_data = true;
                break;
            }
        }

        if (has_data) {
            fee_status_t status = write_record(h, addr, &h->cache[addr], chunk);
            if (status != FEE_OK && status != FEE_ERROR_FULL) {
                /* 失败，恢复旧页 */
                h->active_page  = old_page;
                h->write_offset = calc_write_offset(h, old_page);
                return status;
            }
        }
    }

    /* 更新新页状态为ACTIVE */
    new_hdr.page_state = FEE_PAGE_STATE_ACTIVE;
    write_page_header(h, new_page, &new_hdr);

    /* 标记旧页为INVALID */
    old_hdr.page_state = FEE_PAGE_STATE_INVALID;
    write_page_header(h, old_page, &old_hdr);

    (void)work_buffer; /* 避免未使用警告 */
    return FEE_OK;
}

/*==============================================================================
 * 公共API实现
 *============================================================================*/

/**
 * @brief 初始化FEE
 */
fee_status_t fee_init(fee_handle_t *handle, const fee_config_t *config,
                      uint8_t *cache_buffer, uint8_t *work_buffer)
{
    fee_page_header_t hdr;
    int8_t active;

    /* 参数检查 */
    if (!handle || !config || !cache_buffer || !work_buffer) {
        return FEE_ERROR_PARAM;
    }

    /* 检查write_granularity */
    uint8_t gran = config->write_granularity;
    if (gran < FEE_MIN_GRANULARITY || gran > FEE_MAX_GRANULARITY
        || (gran & (gran - 1)) != 0) {
        return FEE_ERROR_PARAM;
    }

    /* 计算对齐参数 */
    uint8_t aligned_header = FEE_ALIGNED_HEADER_SIZE(gran);

    /* 初始化句柄 */
    memset(handle, 0, sizeof(fee_handle_t));
    handle->flash_base         = config->flash_base;
    handle->pages_per_fee_page = config->pages_per_fee_page;
    handle->fee_page_size      = config->flash_page_size * config->pages_per_fee_page;
    handle->write_granularity  = gran;
    handle->aligned_header_size = aligned_header;
    handle->record_data_size   = gran - FEE_RECORD_HEADER_SIZE;
    handle->data_area_size     = handle->fee_page_size - aligned_header;
    handle->cache              = cache_buffer;
    handle->cache_size         = config->cache_size;
    handle->max_erase_count    = config->max_erase_count;

    /* 存储实例级的工作缓冲区和Flash操作接口 */
    handle->work_buffer = work_buffer;
    handle->flash_ops   = config->flash_ops;

    /* 查找活动FEE Page */
    active = find_active_page(handle);

    if (active >= 0) {
        /* 找到活动页，重建Cache */
        handle->active_page = (uint8_t)active;

        if (rebuild_cache(handle, (uint8_t)active) != FEE_OK) {
            return FEE_ERROR_FLASH;
        }

        /* 修复RECEIVING状态（如果GC中断） */
        if (read_page_header(handle, (uint8_t)active, &hdr) == FEE_OK) {
            if (hdr.page_state == FEE_PAGE_STATE_RECEIVING) {
                hdr.page_state = FEE_PAGE_STATE_ACTIVE;
                write_page_header(handle, (uint8_t)active, &hdr);
            }
        }
    } else {
        /* 没有活动页，格式化FEE Page 0 */
        handle->active_page = 0;

        if (flash_erase_fee_page(handle, 0) != FEE_OK) {
            return FEE_ERROR_FLASH;
        }

        hdr.page_state  = FEE_PAGE_STATE_ACTIVE;
        hdr.erase_count = 1;

        if (write_page_header(handle, 0, &hdr) != FEE_OK) {
            return FEE_ERROR_FLASH;
        }

        memset(handle->cache, 0xFF, handle->cache_size);
        handle->write_offset = 0;
        handle->flags |= FEE_FLAG_VALID;
    }

    handle->flags |= FEE_FLAG_INIT;
    return FEE_OK;
}

/**
 * @brief 写入数据到虚拟EEPROM
 */
fee_status_t fee_write(fee_handle_t *handle, uint16_t addr, const uint8_t *data,
                       uint16_t len)
{
    fee_status_t status;

    if (!handle || !data || !(handle->flags & FEE_FLAG_INIT)) {
        return FEE_ERROR_NOT_INIT;
    }

    if (addr + len > handle->cache_size) {
        return FEE_ERROR_PARAM;
    }

    /* 检查数据是否相同（优化：避免不必要的写入） */
    if (memcmp(&handle->cache[addr], data, len) == 0) {
        return FEE_OK;
    }

    FEE_ENTER_CRITICAL();

    /* 按record_data_size分块写入 */
    uint16_t remaining = len;
    uint16_t offset    = 0;
    uint8_t chunk_size = handle->record_data_size;

    while (remaining > 0) {
        uint8_t chunk =
            (remaining > chunk_size) ? chunk_size : (uint8_t)remaining;

        status = write_record(handle, addr + offset, data + offset, chunk);

        if (status == FEE_ERROR_FULL) {
            /* 空间不足，触发GC */
            status = fee_gc_migrate(handle);
            if (status != FEE_OK) {
                FEE_EXIT_CRITICAL();
                return status;
            }

            /* 重试写入 */
            status = write_record(handle, addr + offset, data + offset, chunk);
        }

        if (status != FEE_OK) {
            FEE_EXIT_CRITICAL();
            return status;
        }

        offset += chunk;
        remaining -= chunk;
    }

    /* 更新Cache */
    memcpy(&handle->cache[addr], data, len);

    FEE_EXIT_CRITICAL();
    return FEE_OK;
}

/**
 * @brief 从虚拟EEPROM读取数据
 */
fee_status_t fee_read(fee_handle_t *handle, uint16_t addr, uint8_t *data,
                      uint16_t len)
{
    if (!handle || !data || !(handle->flags & FEE_FLAG_INIT)) {
        return FEE_ERROR_NOT_INIT;
    }

    if (addr + len > handle->cache_size) {
        return FEE_ERROR_PARAM;
    }

    /* 直接从Cache复制 */
    memcpy(data, &handle->cache[addr], len);
    return FEE_OK;
}

/**
 * @brief 格式化FEE（擦除所有数据）
 */
fee_status_t fee_format(fee_handle_t *handle)
{
    fee_page_header_t hdr;

    if (!handle || !(handle->flags & FEE_FLAG_INIT)) {
        return FEE_ERROR_NOT_INIT;
    }

    FEE_ENTER_CRITICAL();

    /* 擦除两个FEE Page */
    flash_erase_fee_page(handle, 0);
    flash_erase_fee_page(handle, 1);

    /* 重新初始化FEE Page 0 */
    handle->active_page  = 0;
    handle->write_offset = 0;

    hdr.page_state  = FEE_PAGE_STATE_ACTIVE;
    hdr.erase_count = 1;
    write_page_header(handle, 0, &hdr);

    /* 清空Cache */
    memset(handle->cache, 0xFF, handle->cache_size);
    handle->flags |= FEE_FLAG_VALID;

    FEE_EXIT_CRITICAL();
    return FEE_OK;
}

/**
 * @brief 手动触发垃圾回收
 */
fee_status_t fee_gc(fee_handle_t *handle)
{
    if (!handle || !(handle->flags & FEE_FLAG_INIT)) {
        return FEE_ERROR_NOT_INIT;
    }

    FEE_ENTER_CRITICAL();
    fee_status_t status = fee_gc_migrate(handle);
    FEE_EXIT_CRITICAL();

    return status;
}

/**
 * @brief 获取FEE运行信息
 */
fee_status_t fee_get_info(fee_handle_t *handle, uint16_t *erase_count,
                          uint16_t *free_bytes, uint16_t *record_count)
{
    fee_page_header_t hdr;

    if (!handle || !(handle->flags & FEE_FLAG_INIT)) {
        return FEE_ERROR_NOT_INIT;
    }

    FEE_ENTER_CRITICAL();

    if (read_page_header(handle, handle->active_page, &hdr) != FEE_OK) {
        FEE_EXIT_CRITICAL();
        return FEE_ERROR_FLASH;
    }

    if (erase_count) {
        *erase_count = hdr.erase_count;
    }

    if (free_bytes) {
        *free_bytes = handle->data_area_size - handle->write_offset;
    }

    if (record_count) {
        *record_count = handle->write_offset / handle->write_granularity;
    }

    FEE_EXIT_CRITICAL();
    return FEE_OK;
}
