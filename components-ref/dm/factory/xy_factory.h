XinYi\components\dm\factory\xy_factory.h
```

```markdown
/**
 * @file xy_factory.h
 * @brief 工厂数据管理组件 - TLV格式双份备份 + CRC16校验
 *
 * 特性：
 * - TLV (Type-Length-Value) 格式存储
 * - 双份备份模式（Active + Backup），提高可靠性
 * - CRC16 完整性校验，确保数据有效
 * - 直接 Flash 底层操作，无 Cache
 * - 支持裸机（Bare-metal）和 RTOS 环境
 *
 * =============================================================================
 * TLV Entry 结构：
 * +--------+--------+--------+-------+-------+------------+
 * | Magic | Type  | Len    | CRC16_H| CRC16_L| Data ...  |
 * +--------+--------+--------+-------+-------+------------+
 *   1B      1B      2B        1B       1B        N
 *
 * Magic: 0x7E=有效, 0x00=已删除, 0xFF=空（擦除态）
 * CRC16: 对 Magic+Type+Len+Data 计算的校验和
 * =============================================================================
 *
 * 双份备份架构：
 *
 * +---------------------+    +---------------------+
 * |    Factory A       |    |    Factory B       |
 * |  (Active Region)   |    |  (Backup Region)   |
 * +---------------------+    +---------------------+
 *         |                        |
 *         v                        v
 *    [TLV Area 0]             [TLV Area 0]
 *    [TLV Area 1]             [TLV Area 1]
 *       ...                       ...
 *    [TLV Area N]             [TLV Area N]
 *
 * 写入流程：
 *   1. 计算 CRC16
 *   2. 写入到 Active Region
 *   3. 如果成功，再写入到 Backup Region
 *   4. 如果 Active 损坏，从 Backup 恢复
 *
 * 读取流程：
 *   1. 从 Active Region 读取
 *   2. 验证 CRC16
 *   3. 如果 CRC 错误，从 Backup 恢复
 *
 * =============================================================================
 */

#ifndef XY_FACTORY_H
#define XY_FACTORY_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
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
 * 编译时配置
 *============================================================================*/

/**
 * @brief 是否启用双份备份模式
 *
 * 双份模式：
 *   - 写入时同时更新两个区域
 *   - 读取时优先读取 Active，失败则读 Backup
 *   - 损坏检测时从 Backup 恢复 Active
 *
 * 禁用后：仅使用单个区域，节省 Flash 空间
 */
#ifndef XY_FACTORY_DUAL_COPY
#define XY_FACTORY_DUAL_COPY        1
#endif

/**
 * @brief 是否启用安全检查
 */
#ifndef XY_FACTORY_SAFE_CHECK
#define XY_FACTORY_SAFE_CHECK       1
#endif

/**
 * @brief 是否启用 CRC16 校验
 *
 * 启用后：
 *   - 每条 TLV Entry 包含 CRC16 校验字段
 *   - 读取时验证 CRC，错误则从备份恢复
 *   - 写入时计算并存储 CRC
 *
 * 禁用后：节省 2 字节/TLV，但不检测数据损坏
 */
#ifndef XY_FACTORY_USE_CRC16
#define XY_FACTORY_USE_CRC16        1
#endif

/**
 * @brief 最小 TLV 写入对齐字节数
 *
 * Flash 写入必须按此对齐（通常是 4/8 字节）
 */
#ifndef XY_FACTORY_ALIGN_SIZE
#define XY_FACTORY_ALIGN_SIZE       8
#endif

/**
 * @brief 最大支持的数据类型数
 */
#ifndef XY_FACTORY_MAX_TYPES
#define XY_FACTORY_MAX_TYPES        32
#endif

/*==============================================================================
 * 临界区适配
 *============================================================================*/

#if XY_OS_BACKEND_BAREMETAL
    /** 裸机下由调用者保证禁中断，此处为空 */
    #define FACTORY_ENTER_CRITICAL()    do { } while (0)
    #define FACTORY_EXIT_CRITICAL()     do { } while (0)
#elif XY_OS_BACKEND_FREERTOS
    #define FACTORY_ENTER_CRITICAL()    taskENTER_CRITICAL()
    #define FACTORY_EXIT_CRITICAL()     taskEXIT_CRITICAL()
#elif XY_OS_BACKEND_RTTHREAD
    #define FACTORY_ENTER_CRITICAL()    rt_enter_critical()
    #define FACTORY_EXIT_CRITICAL()      rt_exit_critical()
#else
    #define FACTORY_ENTER_CRITICAL()    do { } while (0)
    #define FACTORY_EXIT_CRITICAL()     do { } while (0)
#endif

/*==============================================================================
 * Flash 操作接口
 *============================================================================*/

/**
 * @brief Flash 操作接口
 *
 * 用户需要实现此接口，工厂组件通过此接口操作 Flash
 */
typedef struct {
    /**
     * @brief 擦除 Flash 区域
     * @param addr 起始地址
     * @param size 要擦除的大小（字节）
     * @return 0 成功，非0 失败
     */
    int (*erase)(uint32_t addr, uint32_t size);

    /**
     * @brief 写入 Flash（按 ALIGN_SIZE 对齐）
     * @param addr 起始地址
     * @param data 数据指针
     * @param size 数据大小（必须是 ALIGN_SIZE 的整数倍）
     * @return 0 成功，非0 失败
     */
    int (*write)(uint32_t addr, const uint8_t *data, uint32_t size);

    /**
     * @brief 读取 Flash
     * @param addr 起始地址
     * @param data 数据缓冲区
     * @param size 数据大小
     * @return 0 成功，非0 失败
     */
    int (*read)(uint32_t addr, uint8_t *data, uint32_t size);
} factory_flash_ops_t;

/*==============================================================================
 * TLV 数据结构
 *============================================================================*/

/**
 * @brief TLV 头部
 *
 * 结构：
 *   +--------+--------+--------+
 *   |  Type  |  Len   | CRC16  |
 *   +--------+--------+--------+
 *     1B      2B       2B
 */
typedef struct __attribute__((packed)) {
    uint8_t  type;   /**< 数据类型 (1-255, 0/255 保留) */
    uint16_t len;    /**< Value 长度（不含头部和 CRC） */
#if XY_FACTORY_USE_CRC16
    uint16_t crc;    /**< CRC16 校验 (type + len + data) */
#endif
} factory_tlv_header_t;

#define FACTORY_TLV_HEADER_SIZE   (3 + (XY_FACTORY_USE_CRC16 ? 2 : 0))

/**
 * @brief 区域描述符
 */
typedef struct {
    uint32_t base_addr;      /**< 区域起始地址 */
    uint32_t size;           /**< 区域总大小 */
    uint32_t used_size;      /**< 已使用大小 */
} factory_region_t;

/**
 * @brief 工厂数据句柄
 */
typedef struct {
    /* Flash 操作接口 */
    const factory_flash_ops_t *flash_ops;

    /* 区域信息 */
    factory_region_t region_a;  /**< 区域 A (Active) */
#if XY_FACTORY_DUAL_COPY
    factory_region_t region_b;  /**< 区域 B (Backup) */
#endif

    /* 运行状态 */
    uint8_t  active_region;    /**< 当前活动区域 (0=A, 1=B) */
    uint8_t  initialized;       /**< 初始化标志 */

    /* 数据表（用于快速查找） */
    struct {
        uint8_t  type;
        uint16_t offset;
        uint16_t len;
    } data_index[XY_FACTORY_MAX_TYPES];
    uint8_t index_count;
} factory_handle_t;

/**
 * @brief 工厂数据配置
 */
typedef struct {
    const factory_flash_ops_t *flash_ops;  /**< Flash 操作接口 */

    uint32_t region_a_addr;   /**< 区域 A 起始地址 */
    uint32_t region_a_size;  /**< 区域 A 大小 */
#if XY_FACTORY_DUAL_COPY
    uint32_t region_b_addr;   /**< 区域 B 起始地址 (双份模式) */
    uint32_t region_b_size;  /**< 区域 B 大小 (双份模式) */
#endif
} factory_config_t;

/*==============================================================================
 * 错误码定义
 *============================================================================*/

typedef enum {
    FACTORY_OK               =  0,  /**< 成功 */
    FACTORY_ERROR            = -1,  /**< 通用错误 */
    FACTORY_ERROR_PARAM      = -2,  /**< 参数错误 */
    FACTORY_ERROR_NOT_INIT   = -3,  /**< 未初始化 */
    FACTORY_ERROR_FLASH     = -4,  /**< Flash 操作失败 */
    FACTORY_ERROR_NO_SPACE   = -5,  /**< 空间不足 */
    FACTORY_ERROR_NOT_FOUND  = -6,  /**< 数据不存在 */
    FACTORY_ERROR_CRC        = -7,  /**< CRC 校验失败 */
    FACTORY_ERROR_CORRUPT    = -8   /**< 数据损坏 */
} factory_status_t;

/*==============================================================================
 * API 函数声明
 *============================================================================*/

/**
 * @brief 初始化工厂数据组件
 *
 * @param handle 工厂数据句柄
 * @param config 配置参数
 * @return FACTORY_OK 成功，其他失败
 *
 * @note 调用上下文：禁中断/持有锁
 *
 * 执行流程：
 *   1. 扫描区域 A，建立数据索引
 *   2. 如果启用双份模式，扫描区域 B
 *   3. 损坏检测：如果区域 A 损坏，尝试从 B 恢复
 */
factory_status_t factory_init(factory_handle_t *handle,
                              const factory_config_t *config);

/**
 * @brief 写入工厂数据
 *
 * @param handle 工厂数据句柄
 * @param type 数据类型 (1-254)
 * @param data 数据指针
 * @param len 数据长度
 * @return FACTORY_OK 成功，其他失败
 *
 * @note 调用上下文：禁中断/持有锁
 *
 * @note 如果数据已存在，会更新值（原地更新，如果空间足够）
 *       如果不存在，会追加到末尾
 *
 * @note 写入流程（双份模式）：
 *       1. 计算 CRC16
 *       2. 写入 Active 区域
 *       3. 写入 Backup 区域
 *       4. 如果任何一步失败，返回错误
 */
factory_status_t factory_write(factory_handle_t *handle, uint8_t type,
                               const uint8_t *data, uint16_t len);

/**
 * @brief 读取工厂数据
 *
 * @param handle 工厂数据句柄
 * @param type 数据类型
 * @param data 数据缓冲区
 * @param len 数据缓冲区长度（输入：最大长度，输出：实际长度）
 * @return FACTORY_OK 成功，其他失败
 *
 * @note 调用上下文：禁中断/持有锁
 *
 * @note 如果缓冲区长度不足，返回 FACTORY_ERROR_NO_SPACE
 *       并在 len 中返回所需长度
 *
 * @note 读取时会验证 CRC16，错误则返回 FACTORY_ERROR_CRC
 */
factory_status_t factory_read(factory_handle_t *handle, uint8_t type,
                              uint8_t *data, uint16_t *len);

/**
 * @brief 删除工厂数据
 *
 * @param handle 工厂数据句柄
 * @param type 数据类型
 * @return FACTORY_OK 成功，其他失败
 *
 * @note 调用上下文：禁中断/持有锁
 *
 * @note 删除只是标记为无效，不会回收空间
 *       空间会在下次全擦除时回收
 */
factory_status_t factory_delete(factory_handle_t *handle, uint8_t type);

/**
 * @brief 检查数据类型是否存在
 *
 * @param handle 工厂数据句柄
 * @param type 数据类型
 * @param len 输出：数据长度（可为 NULL）
 * @return true 存在，false 不存在
 *
 * @note 调用上下文：禁中断/持有锁
 */
bool factory_exists(factory_handle_t *handle, uint8_t type, uint16_t *len);

/**
 * @brief 枚举所有已存储的数据类型
 *
 * @param handle 工厂数据句柄
 * @param types 输出类型数组
 * @param max_count 最大数量
 * @param count 输出：实际数量
 * @return FACTORY_OK 成功，其他失败
 *
 * @note 调用上下文：禁中断/持有锁
 */
factory_status_t factory_enum(factory_handle_t *handle, uint8_t *types,
                              uint8_t max_count, uint8_t *count);

/**
 * @brief 格式化工厂数据区（擦除所有数据）
 *
 * @param handle 工厂数据句柄
 * @return FACTORY_OK 成功，其他失败
 *
 * @note 调用上下文：禁中断/持有锁
 *
 * @note 此操作会擦除区域 A 和区域 B
 */
factory_status_t factory_format(factory_handle_t *handle);

/**
 * @brief 获取工厂数据状态信息
 *
 * @param handle 工厂数据句柄
 * @param used 输出：已使用字节数（可为 NULL）
 * @param free 输出：剩余空间字节数（可为 NULL）
 * @param count 输出：数据项数量（可为 NULL）
 * @return FACTORY_OK 成功，其他失败
 *
 * @note 调用上下文：任意
 */
factory_status_t factory_get_info(factory_handle_t *handle,
                                  uint32_t *used, uint32_t *free, uint8_t *count);

/**
 * @brief 验证并修复工厂数据（从 Backup 恢复 Active）
 *
 * @param handle 工厂数据句柄
 * @return FACTORY_OK 成功，其他失败
 *
 * @note 调用上下文：禁中断/持有锁
 *
 * @note 建议在启动时调用此函数，确保数据完整性
 *
 * @note 会验证每条 TLV 的 CRC16，错误则从备份恢复
 */
factory_status_t factory_verify_and_repair(factory_handle_t *handle);

/**
 * @brief 获取错误信息描述
 *
 * @param status 错误码
 * @return 错误描述字符串
 */
const char *factory_status_str(factory_status_t status);

/*==============================================================================
 * 辅助宏
 *============================================================================*/

/**
 * @brief 计算对齐后的大小
 */
#define FACTORY_ALIGN_SIZE(size) \
    (((size) + XY_FACTORY_ALIGN_SIZE - 1) & ~(XY_FACTORY_ALIGN_SIZE - 1))

/**
 * @brief 计算 TLV 总大小（含头部）
 */
#define FACTORY_TLV_TOTAL_SIZE(data_len) \
    (FACTORY_TLV_HEADER_SIZE + (data_len))

/**
 * @brief 获取 TLV 中的 Data 指针
 */
#define FACTORY_TLV_DATA(header) \
    ((uint8_t *)(header) + FACTORY_TLV_HEADER_SIZE)

/**
 * @brief 判断 TLV 是否有效（检查 magic 或标记）
 */
#define FACTORY_TLV_IS_VALID(entry) \
    ((entry)->magic == 0x7E && (entry)->type != 0 && (entry)->type != 255)

/**
 * @brief 计算双份模式所需总 Flash 空间
 */
#if XY_FACTORY_DUAL_COPY
    #define FACTORY_TOTAL_SIZE(region_size)   ((region_size) * 2)
#else
    #define FACTORY_TOTAL_SIZE(region_size)   (region_size)
#endif

/*==============================================================================
 * 预定义数据类型
 *============================================================================*/

/**
 * @brief 预定义的数据类型
 */
typedef enum {
    FACTORY_TYPE_DEVICE_ID     = 1,   /**< 设备 ID */
    FACTORY_TYPE_CALIBRATION   = 2,   /**< 校准数据 */
    FACTORY_TYPE_CONFIG        = 3,   /**< 配置参数 */
    FACTORY_TYPE_SECURITY_KEY  = 4,   /**< 安全密钥 */
    FACTORY_TYPE_MANUFACTURE   = 5,   /**< 制造信息 */
    FACTORY_TYPE_RESERVED      = 254, /**< 保留 */
    FACTORY_TYPE_END_MARK      = 255, /**< 结束标记 */
} factory_type_t;

/*==============================================================================
 * CRC16 计算接口（供外部使用）
 *============================================================================*/

/**
 * @brief 计算 CRC16
 *
 * @param data 数据指针
 * @param len 数据长度
 * @return CRC16 校验值
 *
 * @note 使用 CRC16-CCITT 算法 (0xA001 多项式)
 */
uint16_t factory_crc16(const uint8_t *data, uint16_t len);

/**
 * @brief 更新 CRC16 计算（增量计算）
 *
 * @param crc 初始 CRC 值
 * @param data 数据指针
 * @param len 数据长度
 * @return 更新后的 CRC16 值
 */
uint16_t factory_crc16_update(uint16_t crc, const uint8_t *data, uint16_t len);

/**
 * @brief 验证 TLV 条目 CRC
 *
 * @param entry TLV 条目指针（Flash 地址）
 * @param handle 句柄（用于读取 Flash）
 * @return true CRC 正确，false CRC 错误
 *
 * @note 此函数会读取完整的 entry（包括 data）进行校验
 */
bool factory_verify_entry_crc(factory_handle_t *handle, const void *entry);

/*==============================================================================
 * 调用约定速查表
 *============================================================================*/

/*
 * +-------------------+---------------------------+---------------------------+
 * | 函数                  | 裸机（禁中断）            | RTOS（持有锁）            |
 * +-------------------+---------------------------+---------------------------+
 * | factory_init          | 禁中断                   | 持有互斥锁                |
 * | factory_write         | 禁中断                   | 持有互斥锁                |
 * | factory_read          | 禁中断                   | 持有互斥锁                |
 * | factory_delete        | 禁中断                   | 持有互斥锁                |
 * | factory_exists        | 禁中断                   | 持有互斥锁                |
 * | factory_enum          | 禁中断                   | 持有互斥锁                |
 * | factory_format        | 禁中断                   | 持有互斥锁                |
 * | factory_get_info      | 任意（已保护）           | 任意（已保护）            |
 * | factory_verify_repair | 禁中断                   | 持有互斥锁                |
 * | factory_crc16         | 任意                     | 任意                      |
 * | factory_verify_entry_crc | 禁中断                | 持有锁                    |
 * +-------------------+---------------------------+---------------------------+
 */

#ifdef __cplusplus
}
#endif

#endif /* XY_FACTORY_H */
