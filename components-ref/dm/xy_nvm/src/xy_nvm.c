/**
 * @file xy_nvm.c
 * @brief NVM Key-Value Storage Implementation
 * @version 1.0.0
 * @date 2026-03-05
 */

#include "xy_nvm.h"
#include <string.h>
#include <stdlib.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG
#include "xy_log.h"

/* KV 头结构 */
typedef struct {
    uint32_t head;      // 头标志 (0xAA55AA55)
    uint8_t key_id;     // 键 ID
    uint8_t is_en;      // 使能标志 (0xFF=有效)
    uint16_t len;       // 数据长度
    uint8_t sum;        // 校验和
} kv_header_t;

#define KV_HEAD_MAGIC   0xAA55AA55
#define KV_HEAD_SIZE    8

/**
 * @brief 计算校验和
 */
static uint8_t calc_checksum(const kv_header_t *hdr, const uint8_t *data)
{
    uint16_t sum = hdr->key_id + hdr->is_en + hdr->len;
    for (uint16_t i = 0; i < hdr->len; i++) {
        sum += data[i];
    }
    return (uint8_t)(sum & 0xFF);
}

/**
 * @brief Flash 读取 (模拟)
 */
static void flash_read(uint32_t addr, void *buf, size_t len)
{
    /* 实际实现需要调用底层 Flash 读取 */
    memcpy(buf, (void *)addr, len);
}

/**
 * @brief Flash 写入 (模拟)
 */
static int flash_write(uint32_t addr, const void *buf, size_t len)
{
    /* 实际实现需要调用底层 Flash 写入 */
    (void)addr;
    (void)buf;
    (void)len;
    return 0;
}

/**
 * @brief Flash 擦除 (模拟)
 */
static int flash_erase(uint32_t addr, size_t len)
{
    /* 实际实现需要调用底层 Flash 擦除 */
    (void)addr;
    (void)len;
    return 0;
}

/**
 * @brief 初始化 NVM
 */
xy_nvm_status_t xy_nvm_init(xy_nvm_t *nvm, const xy_nvm_config_t *cfg)
{
    if (!nvm || !cfg || !cfg->flash_base) {
        return XY_NVM_ERROR_INVALID_PARAM;
    }
    
    memset(nvm, 0, sizeof(*nvm));
    nvm->config = *cfg;
    nvm->initialized = true;
    
    xy_log_i("NVM initialized: base=0x%x, pages=%d\n", 
             (uint32_t)cfg->flash_base, cfg->num_pages);
    
    return XY_NVM_OK;
}

/**
 * @brief 反初始化 NVM
 */
xy_nvm_status_t xy_nvm_deinit(xy_nvm_t *nvm)
{
    if (!nvm || !nvm->initialized) {
        return XY_NVM_ERROR_INVALID_PARAM;
    }
    
    nvm->initialized = false;
    return XY_NVM_OK;
}

/**
 * @brief 查找 KV 地址
 */
static uint32_t find_kv_addr(xy_nvm_t *nvm, uint8_t key_id)
{
    uint32_t addr = (uint32_t)nvm->config.flash_base;
    uint32_t end_addr = addr + nvm->config.page_size * nvm->config.num_pages;
    
    while (addr < end_addr) {
        kv_header_t hdr;
        flash_read(addr, &hdr, sizeof(hdr));
        
        /* 检查头标志 */
        if (hdr.head != KV_HEAD_MAGIC) {
            addr += 4;
            continue;
        }
        
        /* 检查校验和 */
        uint8_t *data = (uint8_t *)(addr + KV_HEAD_SIZE);
        if (calc_checksum(&hdr, data) != hdr.sum) {
            addr += KV_HEAD_SIZE;
            continue;
        }
        
        /* 检查键 ID 和使能 */
        if (hdr.key_id == key_id && hdr.is_en == 0xFF) {
            return addr;
        }
        
        /* 移动到下一个 KV */
        addr += KV_HEAD_SIZE + hdr.len;
    }
    
    return 0;
}

/**
 * @brief 读取 KV (返回实体)
 */
xy_nvm_result_t xy_nvm_get(xy_nvm_t *nvm, uint8_t key_id)
{
    xy_nvm_result_t result = {0};
    
    if (!nvm || !nvm->initialized) {
        result.status = XY_NVM_ERROR_INVALID_PARAM;
        return result;
    }
    
    /* 查找地址 */
    uint32_t addr = find_kv_addr(nvm, key_id);
    if (addr == 0) {
        result.status = XY_NVM_ERROR_NOT_FOUND;
        return result;
    }
    
    /* 读取头 */
    kv_header_t hdr;
    flash_read(addr, &hdr, sizeof(hdr));
    
    /* 读取数据部分 */
    uint32_t data_addr = addr + KV_HEAD_SIZE;
    flash_read(data_addr, result.data, hdr.len);
    
    result.len = hdr.len;
    result.status = XY_NVM_OK;
    
    return result;
}

/**
 * @brief 写入 KV
 */
xy_nvm_status_t xy_nvm_set(xy_nvm_t *nvm, uint8_t key_id, 
                           const uint8_t *data, uint16_t len)
{
    if (!nvm || !nvm->initialized || !data) {
        return XY_NVM_ERROR_INVALID_PARAM;
    }
    
    /* 构建 KV 头 */
    kv_header_t hdr;
    hdr.head = KV_HEAD_MAGIC;
    hdr.key_id = key_id;
    hdr.is_en = 0xFF;
    hdr.len = len;
    hdr.sum = calc_checksum(&hdr, data);
    
    /* 查找写入位置 */
    uint32_t addr = (uint32_t)nvm->config.flash_base;
    uint32_t end_addr = addr + nvm->config.page_size * nvm->config.num_pages;
    
    while (addr < end_addr) {
        kv_header_t existing;
        flash_read(addr, &existing, sizeof(existing));
        
        /* 找到空闲空间 */
        if (existing.head == 0xFFFFFFFF) {
            /* 写入头 */
            flash_write(addr, &hdr, sizeof(hdr));
            
            /* 写入数据部分 */
            flash_write(addr + KV_HEAD_SIZE, data, len);
            
            return XY_NVM_OK;
        }
        
        /* 跳过已用空间 */
        addr += KV_HEAD_SIZE + existing.len;
        /* 对齐到 4 字节 */
        if (addr % 4 != 0) {
            addr += 4 - (addr % 4);
        }
    }
    
    return XY_NVM_ERROR_FULL;
}

/**
 * @brief 删除 KV
 */
xy_nvm_status_t xy_nvm_delete(xy_nvm_t *nvm, uint8_t key_id)
{
    if (!nvm || !nvm->initialized) {
        return XY_NVM_ERROR_INVALID_PARAM;
    }
    
    /* 查找地址 */
    uint32_t addr = find_kv_addr(nvm, key_id);
    if (addr == 0) {
        return XY_NVM_ERROR_NOT_FOUND;
    }
    
    /* 标记为删除 (is_en = 0x00) */
    uint8_t is_en = 0x00;
    flash_write(addr + 4, &is_en, 1);
    
    return XY_NVM_OK;
}

/**
 * @brief 格式化 NVM
 */
xy_nvm_status_t xy_nvm_format(xy_nvm_t *nvm)
{
    if (!nvm || !nvm->initialized) {
        return XY_NVM_ERROR_INVALID_PARAM;
    }
    
    uint32_t addr = (uint32_t)nvm->config.flash_base;
    size_t size = nvm->config.page_size * nvm->config.num_pages;
    
    /* 擦除所有区域 */
    flash_erase(addr, size);
    
    xy_log_i("NVM formatted\n");
    return XY_NVM_OK;
}

/**
 * @brief 获取使用统计
 */
void xy_nvm_get_stats(xy_nvm_t *nvm, uint16_t *used, uint16_t *free)
{
    if (!nvm || !nvm->initialized) {
        return;
    }
    
    uint32_t addr = (uint32_t)nvm->config.flash_base;
    uint32_t end_addr = addr + nvm->config.page_size * nvm->config.num_pages;
    uint16_t used_bytes = 0;
    
    while (addr < end_addr) {
        kv_header_t hdr;
        flash_read(addr, &hdr, sizeof(hdr));
        
        if (hdr.head == 0xFFFFFFFF) {
            break;
        }
        
        used_bytes += KV_HEAD_SIZE + hdr.len;
        
        /* 对齐到 4 字节 */
        addr += KV_HEAD_SIZE + hdr.len;
        if (addr % 4 != 0) {
            addr += 4 - (addr % 4);
        }
    }
    
    uint16_t total = nvm->config.page_size * nvm->config.num_pages;
    
    if (used) *used = used_bytes;
    if (free) *free = total - used_bytes;
}
