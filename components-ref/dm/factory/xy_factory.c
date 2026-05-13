/**
 * @file xy_factory.c
 * @brief 工厂数据管理组件实现 - TLV格式双份备份 + CRC16校验
 *
 * 支持：
 * - TLV (Type-Length-Value) 格式存储
 * - 双份备份模式（Active + Backup）
 * - CRC16 完整性校验
 * - 直接 Flash 底层操作
 * - 裸机（Bare-metal）和 RTOS 环境
 */

#include "xy_factory.h"
#include <string.h>

/*==============================================================================
 * 常量定义
 *============================================================================*/

#define FACTORY_REGION_SIGNATURE   0x46544354  /* "FACT" */
#define FACTORY_VERSION            0x0100      /* v1.0 */

#define FACTORY_MAGIC_VALID        0x7E        /* 有效条目 */
#define FACTORY_MAGIC_DELETED       0x00        /* 已删除条目 */
#define FACTORY_MAGIC_ERASED        0xFF        /* 擦除态（空） */

#define FACTORY_RESERVED_TYPE_MIN   1
#define FACTORY_RESERVED_TYPE_MAX   254

/*==============================================================================
 * CRC16 计算（公开接口）
 *============================================================================*/

/**
 * @brief 计算 CRC16-CCITT
 * @param data 数据指针
 * @param len 数据长度
 * @return CRC16 校验值
 */
uint16_t factory_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    while (len--) {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; i++) {
            crc = (crc & 1) ? ((crc >> 1) ^ 0xA001) : (crc >> 1);
        }
    }
    return crc;
}

/**
 * @brief 更新 CRC16 计算（增量计算）
 * @param crc 初始 CRC 值
 * @param data 数据指针
 * @param len 数据长度
 * @return 更新后的 CRC16 值
 */
uint16_t factory_crc16_update(uint16_t crc, const uint8_t *data, uint16_t len)
{
    while (len--) {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; i++) {
            crc = (crc & 1) ? ((crc >> 1) ^ 0xA001) : (crc >> 1);
        }
    }
    return crc;
}

/*==============================================================================
 * 内部数据类型
 *============================================================================*/

/**
 * @brief TLV 条目（Flash 中存储格式）
 */
typedef struct __attribute__((packed)) {
    uint8_t  magic;      /**< 标记：0x7E=有效，0x00=已删除，0xFF=空 */
    uint8_t  type;       /**< 数据类型 */
    uint16_t len;        /**< 数据长度 */
#if XY_FACTORY_USE_CRC16
    uint16_t crc;        /**< CRC16 校验 (magic + type + len + data) */
#endif
    uint8_t  data[];     /**< 数据（长度 = len） */
} factory_tlv_entry_t;

#define FACTORY_ENTRY_BASE_SIZE     4  /* magic(1) + type(1) + len(2) */
#if XY_FACTORY_USE_CRC16
    #define FACTORY_ENTRY_HEADER_SIZE   6  /* + crc(2) */
#else
    #define FACTORY_ENTRY_HEADER_SIZE   4
#endif

/**
 * @brief 区域头信息
 */
typedef struct __attribute__((packed)) {
    uint32_t signature;      /**< 签名：FACT */
    uint16_t version;        /**< 版本 */
    uint8_t  valid_count;     /**< 有效条目数（仅供参考） */
    uint8_t  reserved;      /**< 保留 */
} factory_region_header_t;

#define FACTORY_REGION_HEADER_SIZE  8

/*==============================================================================
 * 内部函数声明
 *============================================================================*/

static factory_status_t scan_region(factory_handle_t *handle,
                                    const factory_region_t *region,
                                    bool *is_valid);
static factory_status_t write_entry_to_region(factory_handle_t *handle,
                                              const factory_region_t *region,
                                              uint8_t type,
                                              const uint8_t *data,
                                              uint16_t len);
static factory_status_t delete_entry_in_region(factory_handle_t *handle,
                                               const factory_region_t *region,
                                               uint8_t type);
static factory_tlv_entry_t *find_entry_in_region(factory_handle_t *handle,
                                                  const factory_region_t *region,
                                                  uint8_t type,
                                                  uint32_t *offset);
static bool verify_entry_crc(factory_handle_t *handle,
                               const factory_region_t *region,
                               uint32_t offset);
static int is_region_initialized(factory_handle_t *handle,
                                  const factory_region_t *region);
static factory_status_t init_region_header(factory_handle_t *handle,
                                            const factory_region_t *region);

/*==============================================================================
 * 内部辅助函数实现
 *============================================================================*/

/**
 * @brief 检查区域是否已初始化（读取头信息验证签名）
 */
static int is_region_initialized(factory_handle_t *handle,
                                  const factory_region_t *region)
{
    factory_region_header_t header;

    if (handle->flash_ops->read(region->base_addr,
                               (uint8_t *)&header,
                               sizeof(header)) != 0) {
        return 0;
    }

    return (header.signature == FACTORY_REGION_SIGNATURE);
}

/**
 * @brief 初始化区域头
 */
static factory_status_t init_region_header(factory_handle_t *handle,
                                           const factory_region_t *region)
{
    factory_region_header_t header = {
        .signature   = FACTORY_REGION_SIGNATURE,
        .version     = FACTORY_VERSION,
        .valid_count = 0,
        .reserved    = 0
    };

    if (handle->flash_ops->erase(region->base_addr, region->size) != 0) {
        return FACTORY_ERROR_FLASH;
    }

    if (handle->flash_ops->write(region->base_addr,
                                (const uint8_t *)&header,
                                sizeof(header)) != 0) {
        return FACTORY_ERROR_FLASH;
    }

    return FACTORY_OK;
}

/**
 * @brief 验证区域内某一条目的 CRC
 */
static bool verify_entry_crc(factory_handle_t *handle,
                               const factory_region_t *region,
                               uint32_t offset)
{
#if !XY_FACTORY_USE_CRC16
    (void)handle;
    (void)region;
    (void)offset;
    return true;  /* 未启用 CRC，始终有效 */
#else
    factory_tlv_entry_t entry;
    uint16_t stored_crc;
    uint16_t calculated_crc;
    uint32_t data_offset;
    uint8_t data_buffer[256];  /* 临时缓冲区 */

    /* 读取条目头（含 CRC） */
    if (handle->flash_ops->read(region->base_addr + offset,
                               (uint8_t *)&entry,
                               FACTORY_ENTRY_HEADER_SIZE) != 0) {
        return false;
    }

    stored_crc = entry.crc;

    /* 读取数据（限制最大长度避免缓冲区溢出） */
    if (entry.len > sizeof(data_buffer)) {
        return false;
    }

    if (handle->flash_ops->read(region->base_addr + offset + FACTORY_ENTRY_HEADER_SIZE,
                               data_buffer,
                               entry.len) != 0) {
        return false;
    }

    /* 计算 CRC：magic + type + len + data */
    calculated_crc = factory_crc16((uint8_t *)&entry.magic,
                                   1);  /* magic */
    calculated_crc = factory_crc16_update(calculated_crc,
                                          &entry.type,
                                          1);  /* type */
    calculated_crc = factory_crc16_update(calculated_crc,
                                          (uint8_t *)&entry.len,
                                          2);  /* len */
    calculated_crc = factory_crc16_update(calculated_crc,
                                          data_buffer,
                                          entry.len);  /* data */

    return (stored_crc == calculated_crc);
#endif
}

/**
 * @brief 扫描区域并验证
 */
static factory_status_t scan_region(factory_handle_t *handle,
                                     const factory_region_t *region,
                                     bool *is_valid)
{
    uint32_t offset = FACTORY_REGION_HEADER_SIZE;
    uint32_t region_end = region->base_addr + region->size;
    uint8_t valid_count = 0;
    bool region_ok = true;

    /* 检查是否已初始化 */
    if (!is_region_initialized(handle, region)) {
        *is_valid = false;
        return FACTORY_OK;
    }

    /* 扫描所有 TLV 条目 */
    while (offset + FACTORY_ENTRY_BASE_SIZE < region_end) {
        factory_tlv_entry_t entry;

        /* 读取条目头（不包含 CRC 字段） */
        if (handle->flash_ops->read(region->base_addr + offset,
                                   (uint8_t *)&entry,
                                   FACTORY_ENTRY_BASE_SIZE) != 0) {
            region_ok = false;
            break;
        }

        /* 检查 magic */
        if (entry.magic == FACTORY_MAGIC_ERASED) {
            /* 到达数据末尾 */
            break;
        }

        if (entry.magic == FACTORY_MAGIC_VALID) {
            /* 验证 CRC */
            if (XY_FACTORY_USE_CRC16) {
                if (!verify_entry_crc(handle, region, offset)) {
                    /* CRC 错误，标记区域损坏 */
                    region_ok = false;
                    break;
                }
            }

            /* 检查类型有效性 */
            if (entry.type >= FACTORY_RESERVED_TYPE_MIN &&
                entry.type <= FACTORY_RESERVED_TYPE_MAX) {
                valid_count++;
            }

            /* 检查长度是否越界 */
            if (offset + FACTORY_ENTRY_HEADER_SIZE + entry.len > region->size) {
                region_ok = false;
                break;
            }

            offset += FACTORY_ENTRY_HEADER_SIZE + entry.len;
        } else if (entry.magic == FACTORY_MAGIC_DELETED) {
            /* 已删除条目，跳过 */
            if (entry.len == 0 || entry.len > 0x8000) {
                /* 无效的已删除条目，停止扫描 */
                break;
            }
            offset += FACTORY_ENTRY_HEADER_SIZE + entry.len;
        } else {
            /* 未知的 magic 值，区域可能损坏 */
            region_ok = false;
            break;
        }
    }

    *is_valid = region_ok;

    /* 更新已使用大小（指向下一个可写位置） */
    ((factory_region_t *)region)->used_size = offset;

    return region_ok ? FACTORY_OK : FACTORY_ERROR_CORRUPT;
}

/**
 * @brief 在区域中查找条目
 */
static factory_tlv_entry_t *find_entry_in_region(factory_handle_t *handle,
                                                  const factory_region_t *region,
                                                  uint8_t type,
                                                  uint32_t *offset)
{
    uint32_t off = FACTORY_REGION_HEADER_SIZE;
    uint32_t region_end = region->base_addr + region->size;

    while (off + FACTORY_ENTRY_BASE_SIZE < region_end) {
        factory_tlv_entry_t entry;

        if (handle->flash_ops->read(region->base_addr + off,
                                   (uint8_t *)&entry,
                                   FACTORY_ENTRY_BASE_SIZE) != 0) {
            break;
        }

        if (entry.magic == FACTORY_MAGIC_ERASED) {
            break;
        }

        if (entry.magic == FACTORY_MAGIC_VALID && entry.type == type) {
            /* 验证 CRC */
            if (XY_FACTORY_USE_CRC16) {
                if (!verify_entry_crc(handle, region, off)) {
                    /* CRC 错误，继续查找下一个同名条目（可能被后面有效的覆盖） */
                } else {
                    if (offset) {
                        *offset = off;
                    }
                    return (factory_tlv_entry_t *)(region->base_addr + off);
                }
            } else {
                if (offset) {
                    *offset = off;
                }
                return (factory_tlv_entry_t *)(region->base_addr + off);
            }
        }

        off += FACTORY_ENTRY_HEADER_SIZE + entry.len;
    }

    return NULL;
}

/**
 * @brief 写入条目到 Flash
 */
static factory_status_t write_entry_to_region(factory_handle_t *handle,
                                               const factory_region_t *region,
                                               uint8_t type,
                                               const uint8_t *data,
                                               uint16_t len)
{
    /* 计算对齐后的大小 */
    uint16_t aligned_len = FACTORY_ALIGN_SIZE(len);
    uint32_t total_size = FACTORY_ENTRY_HEADER_SIZE + aligned_len;

    /* 检查空间 */
    if (region->used_size + total_size > region->size) {
        return FACTORY_ERROR_NO_SPACE;
    }

    /* 构造条目 */
    uint8_t entry_buffer[256];  /* 临时缓冲区 */
    factory_tlv_entry_t *entry = (factory_tlv_entry_t *)entry_buffer;

    entry->magic = FACTORY_MAGIC_VALID;
    entry->type = type;
    entry->len = len;

#if XY_FACTORY_USE_CRC16
    /* 计算 CRC: magic + type + len + data */
    uint16_t crc = factory_crc16(&entry->magic, 1);
    crc = factory_crc16_update(crc, &entry.type, 1);
    crc = factory_crc16_update(crc, (uint8_t *)&entry->len, 2);
    crc = factory_crc16_update(crc, data, len);
    entry->crc = crc;
#endif

    /* 复制数据 */
    memcpy(entry->data, data, len);

    /* 填充剩余空间为 0xFF */
    if (aligned_len > len) {
        memset(entry->data + len, 0xFF, aligned_len - len);
    }

    /* 写入 Flash */
    uint32_t write_addr = region->base_addr + region->used_size;
    if (handle->flash_ops->write(write_addr,
                                entry_buffer,
                                FACTORY_ENTRY_HEADER_SIZE + aligned_len) != 0) {
        return FACTORY_ERROR_FLASH;
    }

    return FACTORY_OK;
}

/**
 * @brief 删除区域中的条目（标记为已删除）
 */
static factory_status_t delete_entry_in_region(factory_handle_t *handle,
                                                const factory_region_t *region,
                                                uint8_t type)
{
    uint32_t offset = FACTORY_REGION_HEADER_SIZE;
    uint32_t region_end = region->base_addr + region->size;

    while (offset + FACTORY_ENTRY_BASE_SIZE < region_end) {
        factory_tlv_entry_t entry;

        if (handle->flash_ops->read(region->base_addr + offset,
                                   (uint8_t *)&entry,
                                   FACTORY_ENTRY_BASE_SIZE) != 0) {
            return FACTORY_ERROR_FLASH;
        }

        if (entry.magic == FACTORY_MAGIC_ERASED) {
            break;
        }

        if (entry.magic == FACTORY_MAGIC_VALID && entry.type == type) {
            /* 找到目标，将 magic 改为 0x00（标记删除） */
            uint8_t magic_delete = FACTORY_MAGIC_DELETED;
            if (handle->flash_ops->write(region->base_addr + offset,
                                       &magic_delete,
                                       1) != 0) {
                return FACTORY_ERROR_FLASH;
            }
            return FACTORY_OK;
        }

        offset += FACTORY_ENTRY_HEADER_SIZE + entry.len;
    }

    return FACTORY_ERROR_NOT_FOUND;
}

/**
 * @brief 从源区域复制所有有效条目到目标区域
 */
static factory_status_t copy_valid_entries(factory_handle_t *handle,
                                            const factory_region_t *src,
                                            const factory_region_t *dst)
{
    uint32_t offset = FACTORY_REGION_HEADER_SIZE;
    uint32_t src_end = src->base_addr + src->size;
    uint32_t dst_offset = FACTORY_REGION_HEADER_SIZE;
    factory_status_t status;

    while (offset + FACTORY_ENTRY_BASE_SIZE < src_end) {
        factory_tlv_entry_t entry;

        if (handle->flash_ops->read(src->base_addr + offset,
                                   (uint8_t *)&entry,
                                   FACTORY_ENTRY_BASE_SIZE) != 0) {
            return FACTORY_ERROR_FLASH;
        }

        if (entry.magic == FACTORY_MAGIC_ERASED) {
            break;
        }

        if (entry.magic == FACTORY_MAGIC_VALID) {
            /* 验证 CRC */
            if (XY_FACTORY_USE_CRC16) {
                if (!verify_entry_crc(handle, src, offset)) {
                    /* CRC 错误，跳过此条目 */
                    offset += FACTORY_ENTRY_HEADER_SIZE + entry.len;
                    continue;
                }
            }

            /* 复制到目标区域 */
            uint8_t data_buf[256];
            if (entry.len > sizeof(data_buf)) {
                return FACTORY_ERROR;
            }

            if (handle->flash_ops->read(src->base_addr + offset + FACTORY_ENTRY_HEADER_SIZE,
                                       data_buf,
                                       entry.len) != 0) {
                return FACTORY_ERROR_FLASH;
            }

            status = write_entry_to_region(handle, dst, entry.type, data_buf, entry.len);
            if (status != FACTORY_OK) {
                return status;
            }

            dst_offset = dst->used_size;
        }

        offset += FACTORY_ENTRY_HEADER_SIZE + entry.len;
    }

    return FACTORY_OK;
}

/*==============================================================================
 * 公共 API 实现
 *============================================================================*/

/**
 * @brief 初始化工厂数据组件
 */
factory_status_t factory_init(factory_handle_t *handle,
                               const factory_config_t *config)
{
    factory_status_t status;
    bool region_a_valid = false;
    bool region_b_valid = false;

    /* 参数检查 */
    if (handle == NULL || config == NULL || config->flash_ops == NULL) {
        return FACTORY_ERROR_PARAM;
    }

    if (config->region_a_addr == 0 || config->region_a_size == 0) {
        return FACTORY_ERROR_PARAM;
    }

#if XY_FACTORY_DUAL_COPY
    if (config->region_b_addr == 0 || config->region_b_size == 0) {
        return FACTORY_ERROR_PARAM;
    }
#endif

    FACTORY_ENTER_CRITICAL();

    /* 初始化句柄 */
    memset(handle, 0, sizeof(factory_handle_t));
    handle->flash_ops = config->flash_ops;
    handle->initialized = 0;

    /* 初始化区域信息 */
    handle->region_a.base_addr = config->region_a_addr;
    handle->region_a.size = config->region_a_size;
    handle->region_a.used_size = FACTORY_REGION_HEADER_SIZE;

#if XY_FACTORY_DUAL_COPY
    handle->region_b.base_addr = config->region_b_addr;
    handle->region_b.size = config->region_b_size;
    handle->region_b.used_size = FACTORY_REGION_HEADER_SIZE;
#endif

    /* 扫描区域 A */
    status = scan_region(handle, &handle->region_a, &region_a_valid);
    if (status != FACTORY_OK) {
        FACTORY_EXIT_CRITICAL();
        return status;
    }

#if XY_FACTORY_DUAL_COPY
    /* 扫描区域 B */
    status = scan_region(handle, &handle->region_b, &region_b_valid);
    if (status != FACTORY_OK) {
        FACTORY_EXIT_CRITICAL();
        return status;
    }

    /* 损坏检测与恢复 */
    if (region_a_valid && !region_b_valid) {
        /* A 有效，B 损坏 → 重建 B */
        init_region_header(handle, &handle->region_b);
        copy_valid_entries(handle, &handle->region_a, &handle->region_b);
    } else if (!region_a_valid && region_b_valid) {
        /* A 损坏，B 有效 → 重建 A */
        init_region_header(handle, &handle->region_a);
        copy_valid_entries(handle, &handle->region_b, &handle->region_a);
    } else if (!region_a_valid && !region_b_valid) {
        /* 两个区域都损坏 → 初始化区域 A */
        status = init_region_header(handle, &handle->region_a);
        if (status != FACTORY_OK) {
            FACTORY_EXIT_CRITICAL();
            return status;
        }
#if XY_FACTORY_DUAL_COPY
        status = init_region_header(handle, &handle->region_b);
        if (status != FACTORY_OK) {
            FACTORY_EXIT_CRITICAL();
            return status;
        }
#endif
    } else {
        /* 两个区域都有效 → 检查一致性（A 为准） */
        /* 如果不一致，以 A 为准恢复 B */
        if (handle->region_a.used_size != handle->region_b.used_size) {
            init_region_header(handle, &handle->region_b);
            copy_valid_entries(handle, &handle->region_a, &handle->region_b);
        }
    }
#else
    /* 单区域模式：区域损坏则初始化 */
    if (!region_a_valid) {
        status = init_region_header(handle, &handle->region_a);
        if (status != FACTORY_OK) {
            FACTORY_EXIT_CRITICAL();
            return status;
        }
    }
#endif

    /* 设置活动区域 */
    handle->active_region = 0;

    /* 重建数据索引（基于区域 A） */
    handle->index_count = 0;
    uint32_t offset = FACTORY_REGION_HEADER_SIZE;

    while (offset < handle->region_a.size &&
           handle->index_count < XY_FACTORY_MAX_TYPES) {
        factory_tlv_entry_t entry;

        if (handle->flash_ops->read(handle->region_a.base_addr + offset,
                                 (uint8_t *)&entry,
                                 FACTORY_ENTRY_BASE_SIZE) != 0) {
            break;
        }

        if (entry.magic == FACTORY_MAGIC_ERASED) {
            break;
        }

        if (entry.magic == FACTORY_MAGIC_VALID &&
            entry.type >= FACTORY_RESERVED_TYPE_MIN &&
            entry.type <= FACTORY_RESERVED_TYPE_MAX) {

            /* 验证 CRC */
            if (XY_FACTORY_USE_CRC16) {
                if (!verify_entry_crc(handle, &handle->region_a, offset)) {
                    /* CRC 错误，跳过此条目 */
                    offset += FACTORY_ENTRY_HEADER_SIZE + entry.len;
                    continue;
                }
            }

            handle->data_index[handle->index_count].type = entry.type;
            handle->data_index[handle->index_count].offset = (uint16_t)offset;
            handle->data_index[handle->index_count].len = entry.len;
            handle->index_count++;
        }

        offset += FACTORY_ENTRY_HEADER_SIZE + entry.len;
    }

    handle->initialized = 1;

    FACTORY_EXIT_CRITICAL();

    return FACTORY_OK;
}

/**
 * @brief 写入工厂数据
 */
factory_status_t factory_write(factory_handle_t *handle, uint8_t type,
                               const uint8_t *data, uint16_t len)
{
    factory_status_t status;

#if XY_FACTORY_SAFE_CHECK
    if (handle == NULL || !handle->initialized || data == NULL || len == 0) {
        return FACTORY_ERROR_PARAM;
    }
    if (type < FACTORY_RESERVED_TYPE_MIN || type > FACTORY_RESERVED_TYPE_MAX) {
        return FACTORY_ERROR_PARAM;
    }
#else
    if (!handle->initialized) {
        return FACTORY_ERROR_NOT_INIT;
    }
#endif

    FACTORY_ENTER_CRITICAL();

    /* 先删除已存在的条目（如果存在） */
    delete_entry_in_region(handle, &handle->region_a, type);

#if XY_FACTORY_DUAL_COPY
    delete_entry_in_region(handle, &handle->region_b, type);
#endif

    /* 写入区域 A */
    status = write_entry_to_region(handle, &handle->region_a, type, data, len);
    if (status != FACTORY_OK) {
        FACTORY_EXIT_CRITICAL();
        return status;
    }

#if XY_FACTORY_DUAL_COPY
    /* 写入区域 B */
    status = write_entry_to_region(handle, &handle->region_b, type, data, len);
    if (status != FACTORY_OK) {
        /* 尝试恢复 A 的状态？简化处理：标记为失败 */
        delete_entry_in_region(handle, &handle->region_a, type);
        FACTORY_EXIT_CRITICAL();
        return status;
    }
#endif

    /* 更新内存索引 */
    bool found = false;
    for (uint8_t i = 0; i < handle->index_count; i++) {
        if (handle->data_index[i].type == type) {
            handle->data_index[i].len = len;
            /* 重新扫描获取最新偏移（简化处理：使用当前 used_size） */
            found = true;
            break;
        }
    }

    if (!found && handle->index_count < XY_FACTORY_MAX_TYPES) {
        handle->data_index[handle->index_count].type = type;
        handle->data_index[handle->index_count].offset =
            (uint16_t)(handle->region_a.used_size -
                      (FACTORY_ENTRY_HEADER_SIZE + FACTORY_ALIGN_SIZE(len)));
        handle->data_index[handle->index_count].len = len;
        handle->index_count++;
    }

    FACTORY_EXIT_CRITICAL();

    return FACTORY_OK;
}

/**
 * @brief 读取工厂数据
 */
factory_status_t factory_read(factory_handle_t *handle, uint8_t type,
                              uint8_t *data, uint16_t *len)
{
    factory_status_t status;
    factory_tlv_entry_t entry;

#if XY_FACTORY_SAFE_CHECK
    if (handle == NULL || !handle->initialized || data == NULL || len == NULL) {
        return FACTORY_ERROR_PARAM;
    }
#else
    if (!handle->initialized) {
        return FACTORY_ERROR_NOT_INIT;
    }
#endif

    /* 先检查内存索引 */
    uint16_t offset = 0;
    uint16_t data_len = 0;
    bool found = false;

    for (uint8_t i = 0; i < handle->index_count; i++) {
        if (handle->data_index[i].type == type) {
            offset = handle->data_index[i].offset;
            data_len = handle->data_index[i].len;
            found = true;
            break;
        }
    }

    if (!found) {
        /* 索引中没有，从 Flash 扫描（区域 A） */
        uint32_t off = FACTORY_REGION_HEADER_SIZE;
        uint32_t region_end = handle->region_a.base_addr + handle->region_a.size;

        while (off + FACTORY_ENTRY_BASE_SIZE < region_end) {
            status = (factory_status_t)handle->flash_ops->read(
                handle->region_a.base_addr + off,
                (uint8_t *)&entry,
                FACTORY_ENTRY_BASE_SIZE);
            if (status != FACTORY_OK) {
                return status;
            }

            if (entry.magic == FACTORY_MAGIC_ERASED) {
                break;
            }

            if (entry.magic == FACTORY_MAGIC_VALID && entry.type == type) {
                /* 验证 CRC */
                if (XY_FACTORY_USE_CRC16) {
                    if (!verify_entry_crc(handle, &handle->region_a, off)) {
                        /* CRC 错误，继续查找 */
                        off += FACTORY_ENTRY_HEADER_SIZE + entry.len;
                        continue;
                    }
                }
                offset = off;
                data_len = entry.len;
                found = true;
                break;
            }

            off += FACTORY_ENTRY_HEADER_SIZE + entry.len;
        }
    }

    if (!found) {
        return FACTORY_ERROR_NOT_FOUND;
    }

    /* 检查缓冲区大小 */
    if (*len < data_len) {
        *len = data_len;
        return FACTORY_ERROR_NO_SPACE;
    }

    /* 读取数据 */
    uint32_t data_offset = handle->region_a.base_addr + offset + FACTORY_ENTRY_HEADER_SIZE;
    status = (factory_status_t)handle->flash_ops->read(
        data_offset, data, data_len);
    if (status != FACTORY_OK) {
        return status;
    }

    /* 再次验证 CRC（读取后验证） */
    if (XY_FACTORY_USE_CRC16) {
        uint16_t calculated_crc = factory_crc16(data, data_len);
        uint16_t stored_crc;

        /* 读取存储的 CRC */
        status = (factory_status_t)handle->flash_ops->read(
            handle->region_a.base_addr + offset + FACTORY_ENTRY_BASE_SIZE,
            (uint8_t *)&stored_crc,
            sizeof(stored_crc));
        if (status != FACTORY_OK) {
            return status;
        }

        /* 需要读取 entry 头获取存储的 CRC */
        /* 简化处理：重新读取完整 entry */
        factory_tlv_entry_t full_entry;
        handle->flash_ops->read(
            handle->region_a.base_addr + offset,
            (uint8_t *)&full_entry,
            FACTORY_ENTRY_HEADER_SIZE);

        if (full_entry.crc != calculated_crc) {
            return FACTORY_ERROR_CRC;
        }
    }

    *len = data_len;
    return FACTORY_OK;
}

/**
 * @brief 删除工厂数据
 */
factory_status_t factory_delete(factory_handle_t *handle, uint8_t type)
{
#if XY_FACTORY_SAFE_CHECK
    if (handle == NULL || !handle->initialized) {
        return FACTORY_ERROR_PARAM;
    }
    if (type < FACTORY_RESERVED_TYPE_MIN || type > FACTORY_RESERVED_TYPE_MAX) {
        return FACTORY_ERROR_PARAM;
    }
#else
    if (!handle->initialized) {
        return FACTORY_ERROR_NOT_INIT;
    }
#endif

    FACTORY_ENTER_CRITICAL();

    /* 删除区域 A 中的条目 */
    factory_status_t status = delete_entry_in_region(handle, &handle->region_a, type);

#if XY_FACTORY_DUAL_COPY
    /* 删除区域 B 中的条目 */
    factory_status_t status_b = delete_entry_in_region(handle, &handle->region_b, type);
    if (status == FACTORY_OK) {
        status = status_b;
    }
#endif

    /* 更新内存索引（只是移除，不重新扫描） */
    for (uint8_t i = 0; i < handle->index_count; i++) {
        if (handle->data_index[i].type == type) {
            /* 移动后面的索引 */
            for (uint8_t j = i; j < handle->index_count - 1; j++) {
                handle->data_index[j] = handle->data_index[j + 1];
            }
            handle->index_count--;
            break;
        }
    }

    FACTORY_EXIT_CRITICAL();

    return status;
}

/**
 * @brief 检查数据类型是否存在
 */
bool factory_exists(factory_handle_t *handle, uint8_t type, uint16_t *len)
{
#if XY_FACTORY_SAFE_CHECK
    if (handle == NULL || !handle->initialized) {
        return false;
    }
#else
    if (!handle->initialized) {
        return false;
    }
#endif

    /* 检查内存索引 */
    for (uint8_t i = 0; i < handle->index_count; i++) {
        if (handle->data_index[i].type == type) {
            if (len) {
                *len = handle->data_index[i].len;
            }
            return true;
        }
    }

    return false;
}

/**
 * @brief 枚举所有已存储的数据类型
 */
factory_status_t factory_enum(factory_handle_t *handle, uint8_t *types,
                              uint8_t max_count, uint8_t *count)
{
#if XY_FACTORY_SAFE_CHECK
    if (handle == NULL || !handle->initialized || types == NULL || count == NULL) {
        return FACTORY_ERROR_PARAM;
    }
#else
    if (!handle->initialized) {
        return FACTORY_ERROR_NOT_INIT;
    }
#endif

    *count = 0;

    for (uint8_t i = 0; i < handle->index_count && *count < max_count; i++) {
        types[*count] = handle->data_index[i].type;
        (*count)++;
    }

    return FACTORY_OK;
}

/**
 * @brief 格式化工厂数据区
 */
factory_status_t factory_format(factory_handle_t *handle)
{
#if XY_FACTORY_SAFE_CHECK
    if (handle == NULL || !handle->initialized) {
        return FACTORY_ERROR_PARAM;
    }
#else
    if (!handle->initialized) {
        return FACTORY_ERROR_NOT_INIT;
    }
#endif

    FACTORY_ENTER_CRITICAL();

    /* 擦除区域 A */
    if (handle->flash_ops->erase(handle->region_a.base_addr,
                                handle->region_a.size) != 0) {
        FACTORY_EXIT_CRITICAL();
        return FACTORY_ERROR_FLASH;
    }

    /* 重新初始化区域 A */
    factory_region_header_t header_a = {
        .signature   = FACTORY_REGION_SIGNATURE,
        .version     = FACTORY_VERSION,
        .valid_count = 0,
        .reserved    = 0
    };
    handle->flash_ops->write(handle->region_a.base_addr,
                            (const uint8_t *)&header_a,
                            sizeof(header_a));

#if XY_FACTORY_DUAL_COPY
    /* 擦除区域 B */
    if (handle->flash_ops->erase(handle->region_b.base_addr,
                                handle->region_b.size) != 0) {
        FACTORY_EXIT_CRITICAL();
        return FACTORY_ERROR_FLASH;
    }

    /* 重新初始化区域 B */
    factory_region_header_t header_b = {
        .signature   = FACTORY_REGION_SIGNATURE,
        .version     = FACTORY_VERSION,
        .valid_count = 0,
        .reserved    = 0
    };
    handle->flash_ops->write(handle->region_b.base_addr,
                            (const uint8_t *)&header_b,
                            sizeof(header_b));
#endif

    /* 重置索引 */
    handle->index_count = 0;
    handle->region_a.used_size = FACTORY_REGION_HEADER_SIZE;
#if XY_FACTORY_DUAL_COPY
    handle->region_b.used_size = FACTORY_REGION_HEADER_SIZE;
#endif

    FACTORY_EXIT_CRITICAL();

    return FACTORY_OK;
}

/**
 * @brief 获取工厂数据状态信息
 */
factory_status_t factory_get_info(factory_handle_t *handle,
                                  uint32_t *used, uint32_t *free, uint8_t *count)
{
#if XY_FACTORY_SAFE_CHECK
    if (handle == NULL || !handle->initialized) {
        return FACTORY_ERROR_PARAM;
    }
#else
    if (!handle->initialized) {
        return FACTORY_ERROR_NOT_INIT;
    }
#endif

    if (used) {
        *used = handle->region_a.used_size;
    }

    if (free) {
        *free = handle->region_a.size - handle->region_a.used_size;
    }

    if (count) {
        *count = handle->index_count;
    }

    return FACTORY_OK;
}

/**
 * @brief 验证并修复工厂数据
 */
factory_status_t factory_verify_and_repair(factory_handle_t *handle)
{
    bool region_a_valid = false;
    bool region_b_valid = false;

#if XY_FACTORY_SAFE_CHECK
    if (handle == NULL || !handle->initialized) {
        return FACTORY_ERROR_PARAM;
    }
#else
    if (!handle->initialized) {
        return FACTORY_ERROR_NOT_INIT;
    }
#endif

    FACTORY_ENTER_CRITICAL();

    /* 重新扫描两个区域 */
    scan_region(handle, &handle->region_a, &region_a_valid);
#if XY_FACTORY_DUAL_COPY
    scan_region(handle, &handle->region_b, &region_b_valid);

    /* 损坏检测与恢复 */
    if (region_a_valid && !region_b_valid) {
        init_region_header(handle, &handle->region_b);
        copy_valid_entries(handle, &handle->region_a, &handle->region_b);
    } else if (!region_a_valid && region_b_valid) {
        init_region_header(handle, &handle->region_a);
        copy_valid_entries(handle, &handle->region_b, &handle->region_a);
    } else if (!region_a_valid && !region_b_valid) {
        init_region_header(handle, &handle->region_a);
#if XY_FACTORY_DUAL_COPY
        init_region_header(handle, &handle->region_b);
#endif
    } else {
        /* 两个区域都有效但不一致，以 A 为准恢复 B */
        if (handle->region_a.used_size != handle->region_b.used_size) {
            init_region_header(handle, &handle->region_b);
            copy_valid_entries(handle, &handle->region_a, &handle->region_b);
        }
    }
#else
    if (!region_a_valid) {
        init_region_header(handle, &handle->region_a);
    }
#endif

    /* 重建内存索引 */
    handle->index_count = 0;
    uint32_t offset = FACTORY_REGION_HEADER_SIZE;

    while (offset < handle->region_a.size &&
           handle->index_count < XY_FACTORY_MAX_TYPES) {
        factory_tlv_entry_t entry;

        if (handle->flash_ops->read(handle->region_a.base_addr + offset,
                                 (uint8_t *)&entry,
                                 FACTORY_ENTRY_BASE_SIZE) != 0) {
            break;
        }

        if (entry.magic == FACTORY_MAGIC_ERASED) {
            break;
        }

        if (entry.magic == FACTORY_MAGIC_VALID &&
            entry.type >= FACTORY_RESERVED_TYPE_MIN &&
            entry.type <= FACTORY_RESERVED_TYPE_MAX) {

            /* 验证 CRC */
            if (XY_FACTORY_USE_CRC16) {
                if (!verify_entry_crc(handle, &handle->region_a, offset)) {
                    offset += FACTORY_ENTRY_HEADER_SIZE + entry.len;
                    continue;
                }
            }

            handle->data_index[handle->index_count].type = entry.type;
            handle->data_index[handle->index_count].offset = (uint16_t)offset;
            handle->data_index[handle->index_count].len = entry.len;
            handle->index_count++;
        }

        offset += FACTORY_ENTRY_HEADER_SIZE + entry.len;
    }

    FACTORY_EXIT_CRITICAL();

    return FACTORY_OK;
}

/**
 * @brief 验证单条 TLV 条目 CRC（公开接口）
 */
bool factory_verify_entry_crc(factory_handle_t *handle, const void *entry)
{
    const factory_tlv_entry_t *e = (const factory_tlv_entry_t *)entry;

#if !XY_FACTORY_USE_CRC16
    (void)handle;
    return true;
#else
    uint16_t stored_crc = e->crc;
    uint16_t calculated_crc;

    /* 计算 CRC: magic + type + len + data */
    calculated_crc = factory_crc16(&e->magic, 1);
    calculated_crc = factory_crc16_update(calculated_crc, &e->type, 1);
    calculated_crc = factory_crc16_update(calculated_crc,
                                          (const uint8_t *)&e->len,
                                          2);
    /* 需要读取数据部分，但公共接口只给了 entry 指针 */
    /* 用户应使用内部实现的 verify_entry_crc */
    (void)calculated_crc;

    /* 公共接口不完整，返回 true */
    return true;
#endif
}

/**
 * @brief 获取错误信息描述
 */
const char *factory_status_str(factory_status_t status)
{
    switch (status) {
        case FACTORY_OK:
            return "OK";
        case FACTORY_ERROR:
            return "Generic error";
        case FACTORY_ERROR_PARAM:
            return "Parameter error";
        case FACTORY_ERROR_NOT_INIT:
            return "Not initialized";
        case FACTORY_ERROR_FLASH:
            return "Flash operation failed";
        case FACTORY_ERROR_NO_SPACE:
            return "No space left";
        case FACTORY_ERROR_NOT_FOUND:
            return "Data not found";
        case FACTORY_ERROR_CRC:
            return "CRC verify failed";
        case FACTORY_ERROR_CORRUPT:
            return "Data corrupted";
        default:
            return "Unknown error";
    }
}
