/**
 * @file xy_fee_nano.h
 * @brief Flash EEPROM Emulation - Nano 精简版
 * @version 2.0.0
 * @date 2026-03-15
 *
 * @note 设计约束：
 *       - 极简实现，适用于资源受限MCU
 *       - 不使用动态内存分配（malloc/free）
 *       - 读取直接访问Flash，不使用Cache
 *       - 支持多实例
 *
 * =============================================================================
 * 与完整版 FEE 的区别：
 *
 * | 特性           | FEE 完整版     | FEE Nano       |
 * |---------------|---------------|-----------------|
 * | Cache         | 有（全镜像）   | 无（直接读Flash）|
 * | 读取性能      | 快（Cache）   | 慢（访问Flash） |
 * | RAM 占用      | 高            | 低              |
 * | 掉电恢复      | 自动（Cache） | 依赖Flash数据   |
 * | 动态内存      | 无            | 无              |
 * | 多实例支持    | 是            | 是              |
 *
 * =============================================================================
 */

#ifndef EFLASH_H
#define EFLASH_H

#include <stdint.h>
#include "xy_typedef.h"
#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 平台和后端配置
 *============================================================================*/

/**
 * @brief 检测 RTOS 后端
 */
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
    /** 裸机下由调用者保证禁中断，此处为空 */
    #define EFLASH_ENTER_CRITICAL()    do { } while (0)
    #define EFLASH_EXIT_CRITICAL()     do { } while (0)
#elif XY_OS_BACKEND_FREERTOS
    #define EFLASH_ENTER_CRITICAL()    taskENTER_CRITICAL()
    #define EFLASH_EXIT_CRITICAL()     taskEXIT_CRITICAL()
#elif XY_OS_BACKEND_RTTHREAD
    #define EFLASH_ENTER_CRITICAL()    rt_enter_critical()
    #define EFLASH_EXIT_CRITICAL()      rt_exit_critical()
#else
    #define EFLASH_ENTER_CRITICAL()    do { } while (0)
    #define EFLASH_EXIT_CRITICAL()     do { } while (0)
#endif

/*==============================================================================
 * 配置常量
 *============================================================================*/

/** @brief 最大页数 */
#define EFLASH_MAX_PAGES         64
/** @brief 最大页大小（字节） */
#define EFLASH_MAX_PAGE_SIZE     4096
/** @brief 默认页大小（字节） */
#define EFLASH_DEFAULT_PAGE_SIZE 512

/**
 * @brief 写入单元大小枚举
 */
typedef enum {
    EFLASH_WRITE_UNIT_32BIT  = 4,  /**< 32-bit 写入单元 (4 字节) */
    EFLASH_WRITE_UNIT_64BIT  = 8,  /**< 64-bit 写入单元 (8 字节) */
    EFLASH_WRITE_UNIT_128BIT = 16, /**< 128-bit 写入单元 (16 字节) */
} eflash_write_unit_t;

/**
 * @brief 写入粒度选项（用于 FEE 包装）
 */
typedef enum {
    EFLASH_GRANULARITY_8   = 8,   /**< 8 字节颗粒 */
    EFLASH_GRANULARITY_16  = 16,  /**< 16 字节颗粒 */
    EFLASH_GRANULARITY_32  = 32,  /**< 32 字节颗粒 */
    EFLASH_GRANULARITY_64  = 64,  /**< 64 字节颗粒 */
} eflash_granularity_t;

/*==============================================================================
 * 错误码定义
 *============================================================================*/

typedef enum {
    EFLASH_OK = 0,               /**< 操作成功 */
    EFLASH_ERROR_INVALID_PARAM,  /**< 参数无效 */
    EFLASH_ERROR_OUT_OF_RANGE,   /**< 地址越界 */
    EFLASH_ERROR_ALIGNMENT,      /**< 地址/大小对齐错误 */
    EFLASH_ERROR_WRITE_FAIL,     /**< 写入失败 */
    EFLASH_ERROR_ERASE_FAIL,     /**< 擦除失败 */
    EFLASH_ERROR_NOT_INIT,       /**< 未初始化 */
    EFLASH_ERROR_BUSY,           /**< 设备忙 */
} eflash_result_t;

/*==============================================================================
 * Flash 配置和状态结构
 *============================================================================*/

/**
 * @brief Flash 配置结构
 */
typedef struct {
    uint32_t total_size;            /**< 总大小（字节） */
    uint32_t page_size;             /**< 页大小（字节） */
    uint32_t page_count;            /**< 页数量 */
    eflash_write_unit_t write_unit;  /**< 最小写入单元 */
    bool auto_erase;                /**< 写入前自动擦除 */
} eflash_config_t;

/**
 * @brief Flash 句柄（多实例安全）
 *
 * @note 所有缓冲区由用户提供，不使用动态内存分配
 */
typedef struct {
    eflash_config_t config;         /**< Flash 配置 */
    uint8_t *memory;                /**< 模拟 Flash 内存（用户提供） */
    bool *page_erased;              /**< 页擦除状态数组（用户提供） */
    bool initialized;               /**< 初始化状态 */
    bool user_provided_buffer;       /**< 是否使用用户提供的 buffer */
} eflash_handle_t;

/*==============================================================================
 * API 函数声明
 *============================================================================*/

/**
 * @brief 使用用户提供的 buffer 初始化 Flash
 *
 * @param handle Flash 句柄
 * @param config 配置参数
 * @param memory_buffer 用户提供的模拟 Flash 内存区域
 * @param page_erased_buffer 用户提供的擦除状态数组
 *
 * @return EFLASH_OK 成功，其他失败
 *
 * @note 适用于裸机环境，不使用 malloc
 */
eflash_result_t eflash_init_with_buffer(eflash_handle_t *handle,
                                         const eflash_config_t *config,
                                         uint8_t *memory_buffer,
                                         bool *page_erased_buffer);

/**
 * @brief 初始化 Flash 设备（仅 PC 模拟器使用）
 *
 * @param handle Flash 句柄
 * @param config 配置参数
 *
 * @return EFLASH_OK 成功，其他失败
 *
 * @note 仅在 PC 模拟环境使用，嵌入式环境请用 eflash_init_with_buffer
 */
eflash_result_t eflash_init(eflash_handle_t *handle,
                             const eflash_config_t *config);

/**
 * @brief 反初始化 Flash 设备
 *
 * @param handle Flash 句柄
 * @return EFLASH_OK 成功，其他失败
 */
eflash_result_t eflash_deinit(eflash_handle_t *handle);

/**
 * @brief 读取 Flash 数据
 *
 * @param handle Flash 句柄
 * @param address 起始地址
 * @param data 数据缓冲区
 * @param size 读取长度（字节）
 * @return EFLASH_OK 成功，其他失败
 */
eflash_result_t eflash_read(eflash_handle_t *handle, uint32_t address,
                            uint8_t *data, size_t size);

/**
 * @brief 写入 Flash 数据
 *
 * @param handle Flash 句柄
 * @param address 起始地址
 * @param data 数据指针
 * @param size 写入长度（字节）
 * @return EFLASH_OK 成功，其他失败
 *
 * @note 地址和大小必须按 write_unit 对齐
 */
eflash_result_t eflash_write(eflash_handle_t *handle, uint32_t address,
                             const uint8_t *data, size_t size);

/**
 * @brief 擦除 Flash 页
 *
 * @param handle Flash 句柄
 * @param page_index 页索引
 * @return EFLASH_OK 成功，其他失败
 */
eflash_result_t eflash_erase_page(eflash_handle_t *handle, uint32_t page_index);

/**
 * @brief 擦除 Flash 扇区（基于地址）
 *
 * @param handle Flash 句柄
 * @param address 扇区内任意地址
 * @return EFLASH_OK 成功，其他失败
 */
eflash_result_t eflash_erase_sector(eflash_handle_t *handle, uint32_t address);

/**
 * @brief 擦除整个 Flash
 *
 * @param handle Flash 句柄
 * @return EFLASH_OK 成功，其他失败
 */
eflash_result_t eflash_erase_all(eflash_handle_t *handle);

/**
 * @brief 获取 Flash 信息
 *
 * @param handle Flash 句柄
 * @param config 输出配置数据缓冲区
 * @return EFLASH_OK 成功，其他失败
 */
eflash_result_t eflash_get_info(eflash_handle_t *handle,
                                eflash_config_t *config);

/**
 * @brief 检查地址范围是否有效
 *
 * @param handle Flash 句柄
 * @param address 起始地址
 * @param size 大小（字节）
 * @return true 有效，false 无效
 */
bool eflash_is_address_valid(eflash_handle_t *handle, uint32_t address,
                             size_t size);

/**
 * @brief 获取地址对应的页索引
 *
 * @param handle Flash 句柄
 * @param address 任意地址
 * @return 页索引
 */
uint32_t eflash_get_page_index(eflash_handle_t *handle, uint32_t address);

/**
 * @brief 检查页是否已擦除
 *
 * @param handle Flash 句柄
 * @param page_index 页索引
 * @return true 已擦除，false 未擦除
 */
bool eflash_is_page_erased(eflash_handle_t *handle, uint32_t page_index);

/*==============================================================================
 * 辅助宏
 *============================================================================*/

/**
 * @brief 向上对齐到指定边界
 */
#define EFLASH_ALIGN_UP(addr, align)   (((addr) + (align) - 1) & ~((align) - 1))

/**
 * @brief 向下对齐到指定边界
 */
#define EFLASH_ALIGN_DOWN(addr, align) ((addr) & ~((align) - 1))

/**
 * @brief 检查地址是否对齐
 */
#define EFLASH_IS_ALIGNED(addr, align) (((addr) & ((align) - 1)) == 0)

/*==============================================================================
 * 调用约定速查表
 *============================================================================*/

/*
 * +-------------------+---------------------------+---------------------------+
 * | 函数              | 裸机（禁中断）            | RTOS（持有锁）            |
 * +-------------------+---------------------------+---------------------------+
 * | eflash_init       | 禁中断                   | 持有互斥锁                |
 * | eflash_deinit     | 禁中断                   | 持有互斥锁                |
 * | eflash_read       | 任意                     | 任意                      |
 * | eflash_write      | 禁中断                   | 持有互斥锁                |
 * | eflash_erase_*    | 禁中断                   | 持有互斥锁                |
 * | eflash_get_info   | 任意                     | 任意                      |
 * +-------------------+---------------------------+---------------------------+
 */

#ifdef __cplusplus
}
#endif

#endif /* EFLASH_H */
