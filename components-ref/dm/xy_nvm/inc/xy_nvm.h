/**
 * @file xy_nvm.h
 * @brief NVM Key-Value Storage Interface
 * @version 1.0.0
 * @date 2026-03-05
 */

#ifndef XY_NVM_H
#define XY_NVM_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 配置 ==================== */

#ifndef XY_NVM_MAX_KEY
#define XY_NVM_MAX_KEY      255     // 最大键值 ID
#endif

#ifndef XY_NVM_MAX_DATA_LEN
#define XY_NVM_MAX_DATA_LEN 64      // 最大数据长度
#endif

/* ==================== 状态码 ==================== */

typedef enum {
    XY_NVM_OK = 0,
    XY_NVM_ERROR,
    XY_NVM_ERROR_NOT_FOUND,
    XY_NVM_ERROR_FULL,
    XY_NVM_ERROR_INVALID_PARAM,
    XY_NVM_ERROR_CRC,
} xy_nvm_status_t;

/* ==================== 数据结构 ==================== */

/**
 * @brief KV 读取结果
 */
typedef struct {
    xy_nvm_status_t status;     // 状态码
    uint8_t data[XY_NVM_MAX_DATA_LEN];  // 数据缓冲区
    uint16_t len;               // 数据长度
} xy_nvm_result_t;

/**
 * @brief KV 存储配置
 */
typedef struct {
    uint8_t *flash_base;        // Flash 基地址
    uint16_t page_size;         // Flash 页大小
    uint8_t num_pages;          // 可用页数
} xy_nvm_config_t;

/**
 * @brief KV 存储句柄
 */
typedef struct {
    xy_nvm_config_t config;     // 配置
    uint8_t *cache;             // 缓存
    uint16_t cache_size;        // 缓存大小
    bool initialized;           // 初始化标志
} xy_nvm_t;

/* ==================== API ==================== */

/**
 * @brief 初始化 NVM
 */
xy_nvm_status_t xy_nvm_init(xy_nvm_t *nvm, const xy_nvm_config_t *cfg);

/**
 * @brief 反初始化 NVM
 */
xy_nvm_status_t xy_nvm_deinit(xy_nvm_t *nvm);

/**
 * @brief 读取 KV (返回实体)
 * @param nvm NVM 句柄
 * @param key_id 键 ID
 * @return 读取结果
 */
xy_nvm_result_t xy_nvm_get(xy_nvm_t *nvm, uint8_t key_id);

/**
 * @brief 写入 KV
 * @param nvm NVM 句柄
 * @param key_id 键 ID
 * @param data 数据指针
 * @param len 数据长度
 * @return 状态码
 */
xy_nvm_status_t xy_nvm_set(xy_nvm_t *nvm, uint8_t key_id, 
                           const uint8_t *data, uint16_t len);

/**
 * @brief 删除 KV
 * @param nvm NVM 句柄
 * @param key_id 键 ID
 * @return 状态码
 */
xy_nvm_status_t xy_nvm_delete(xy_nvm_t *nvm, uint8_t key_id);

/**
 * @brief 格式化 NVM
 * @param nvm NVM 句柄
 * @return 状态码
 */
xy_nvm_status_t xy_nvm_format(xy_nvm_t *nvm);

/**
 * @brief 获取使用统计
 * @param nvm NVM 句柄
 * @param used 已使用字节
 * @param free 剩余字节
 */
void xy_nvm_get_stats(xy_nvm_t *nvm, uint16_t *used, uint16_t *free);

/* ==================== 便捷 API ==================== */

/**
 * @brief 读取 8 位数据
 */
static inline xy_nvm_result_t xy_nvm_get_u8(xy_nvm_t *nvm, uint8_t key_id)
{
    xy_nvm_result_t result = xy_nvm_get(nvm, key_id);
    return result;
}

/**
 * @brief 写入 8 位数据
 */
static inline xy_nvm_status_t xy_nvm_set_u8(xy_nvm_t *nvm, uint8_t key_id, uint8_t value)
{
    return xy_nvm_set(nvm, key_id, &value, 1);
}

/**
 * @brief 读取 16 位数据
 */
static inline xy_nvm_result_t xy_nvm_get_u16(xy_nvm_t *nvm, uint8_t key_id)
{
    return xy_nvm_get(nvm, key_id);
}

/**
 * @brief 写入 16 位数据
 */
static inline xy_nvm_status_t xy_nvm_set_u16(xy_nvm_t *nvm, uint8_t key_id, uint16_t value)
{
    return xy_nvm_set(nvm, key_id, (uint8_t *)&value, 2);
}

/**
 * @brief 读取 32 位数据
 */
static inline xy_nvm_result_t xy_nvm_get_u32(xy_nvm_t *nvm, uint8_t key_id)
{
    return xy_nvm_get(nvm, key_id);
}

/**
 * @brief 写入 32 位数据
 */
static inline xy_nvm_status_t xy_nvm_set_u32(xy_nvm_t *nvm, uint8_t key_id, uint32_t value)
{
    return xy_nvm_set(nvm, key_id, (uint8_t *)&value, 4);
}

#ifdef __cplusplus
}
#endif

#endif /* XY_NVM_H */
