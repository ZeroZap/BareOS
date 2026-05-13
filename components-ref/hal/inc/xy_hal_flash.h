/**
 * @file xy_hal_flash.h
 * @brief Flash Memory Hardware Abstraction Layer
 * @version 2.0
 * @date 2026-02-27
 */

#ifndef XY_HAL_FLASH_H
#define XY_HAL_FLASH_H
#include "xy_hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_hal.h"
#include <stdint.h>

/**
 * @brief Flash 扇区信息结构
 */
typedef struct {
    uint32_t start_addr;    /**< 扇区起始地址 */
    uint32_t size;          /**< 扇区大小 (字节) */
    uint8_t  is_protected;  /**< 写保护标志 */
} xy_hal_flash_sector_info_t;

/**
 * @brief Flash 信息结构
 */
typedef struct {
    uint32_t flash_size;        /**< Flash 总大小 (字节) */
    uint32_t sector_count;      /**< 扇区数量 */
    uint32_t page_size;         /**< 页大小 (字节) */
    uint32_t write_alignment;   /**< 写入对齐大小 (字节) */
    const xy_hal_flash_sector_info_t *sectors;  /**< 扇区信息数组 */
} xy_hal_flash_info_t;

/**
 * @brief Flash 读保护级别
 */
typedef enum {
    XY_HAL_FLASH_RDP_LEVEL_0 = 0,   /**< 无保护 */
    XY_HAL_FLASH_RDP_LEVEL_1,       /**< 级别 1 保护 */
    XY_HAL_FLASH_RDP_LEVEL_2,       /**< 级别 2 保护 (最高) */
} xy_hal_flash_rdp_level_t;

/**
 * @brief 初始化 Flash
 * @param flash Flash 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_flash_init(void *flash);

/**
 * @brief 反初始化 Flash
 * @param flash Flash 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_flash_deinit(void *flash);

/**
 * @brief 从 Flash 读取数据
 * @param flash Flash 实例
 * @param addr 读取地址
 * @param data 数据输出缓冲区
 * @param size 读取字节数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_flash_read(void *flash, uint32_t addr, uint8_t *data,
                                 uint32_t size);

/**
 * @brief 向 Flash 写入数据
 * @param flash Flash 实例
 * @param addr 写入地址
 * @param data 数据缓冲区
 * @param size 写入字节数
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_flash_write(void *flash, uint32_t addr,
                                  const uint8_t *data, uint32_t size);

/**
 * @brief 擦除 Flash 扇区
 * @param flash Flash 实例
 * @param addr 起始地址
 * @param size 擦除大小
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_flash_erase(void *flash, uint32_t addr, uint32_t size);

/**
 * @brief 批量擦除 Flash (整片擦除)
 * @param flash Flash 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_flash_mass_erase(void *flash);

/**
 * @brief 锁定 Flash 写入
 * @param flash Flash 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_flash_lock(void *flash);

/**
 * @brief 解锁 Flash 写入
 * @param flash Flash 实例
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_flash_unlock(void *flash);

/**
 * @brief 设置 Flash 读保护级别
 * @param flash Flash 实例
 * @param level 读保护级别
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_flash_set_read_protect(void *flash,
                                             xy_hal_flash_rdp_level_t level);

/**
 * @brief 获取 Flash 读保护级别
 * @param flash Flash 实例
 * @param level 读保护级别输出
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_flash_get_read_protect(void *flash,
                                             xy_hal_flash_rdp_level_t *level);

/**
 * @brief 获取 Flash 信息
 * @param flash Flash 实例
 * @param info Flash 信息输出结构
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_flash_get_info(void *flash, xy_hal_flash_info_t *info);

/**
 * @brief 检查地址是否在 Flash 范围内
 * @param flash Flash 实例
 * @param addr 检查地址
 * @return 1 在范围内，0 不在范围内
 */
int xy_hal_flash_is_valid_address(void *flash, uint32_t addr);

/**
 * @brief 获取扇区信息
 * @param flash Flash 实例
 * @param sector_num 扇区号
 * @param info 扇区信息输出结构
 * @return XY_HAL_OK 成功，其他值失败
 */
xy_hal_error_t xy_hal_flash_get_sector_info(void *flash, uint32_t sector_num,
                                            xy_hal_flash_sector_info_t *info);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_FLASH_H */
