/**
 * @file xy_mem.h
 * @brief 动态内存管理组件
 *
 * 支持：
 * - 多内存池管理（类似 lwIP mem pools）
 * - 不定长内存分配（类似 cJSON）
 * - 裸机（Bare-metal）和 RTOS 环境
 *
 * =============================================================================
 * 多池架构：
 *
 * 每个内存池独立管理，用户可通过宏 XY_MEM_POOL_CREATE() 注册多个池。
 * 分配时会按池顺序依次尝试，直到找到可用池。
 *
 * 示例：
 *
 * @code
 * // 定义两个内存池
 * XY_MEM_POOL_DECLARE(app_pool, 1024);    // 1KB 应用池
 * XY_MEM_POOL_DECLARE(large_pool, 4096);  // 4KB 大对象池
 *
 * // 初始化
 * void sys_init(void) {
 *     XY_MEM_POOL_INIT(app_pool);
 *     XY_MEM_POOL_INIT(large_pool);
 * }
 *
 * // 使用
 * void *p = xy_malloc(100);              // 从默认池分配
 * void *p = xy_malloc_from_pool(100, app_pool);  // 从指定池分配
 * @endcode
 *
 * =============================================================================
 * 不定长分配：
 *
 * 用于不确定大小的数据（如 JSON、协议包），按实际大小分配。
 *
 * @code
 * char *json_str = xy_malloc_variable(256);  // 分配256字节
 * xy_free_variable(json_str);                // 释放时无需知道大小
 * @endcode
 *
 * =============================================================================
 */

#ifndef _XY_MEM_H_
#define _XY_MEM_H_

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
 * 编译时配置
 *============================================================================*/

/**
 * @brief 是否启用安全检查（NULL检查、双重释放检测）
 * @note 建议在调试阶段启用，生产环境可禁用以提升性能
 */
#ifndef XY_MEM_SAFE_CHECK
#define XY_MEM_SAFE_CHECK       1
#endif

/**
 * @brief 是否启用内存池追踪（统计分配信息）
 * @note 会占用额外内存，用于调试
 */
#ifndef XY_MEM_TRACKING
#define XY_MEM_TRACKING         0
#endif

/**
 * @brief 默认内存池数量上限
 */
#ifndef XY_MEM_MAX_POOLS
#define XY_MEM_MAX_POOLS        4
#endif

/**
 * @brief 默认内存池最小对齐字节数
 */
#ifndef XY_MEM_ALIGN_SIZE
#define XY_MEM_ALIGN_SIZE       sizeof(void *)
#endif

/*==============================================================================
 * 基础类型和结构
 *============================================================================*/

typedef unsigned int    xy_mem_count_t;
typedef unsigned int    xy_mem_size_t;

/**
 * @brief 内存块头信息（不定长分配用）
 *
 * 结构：| block_header | user_data... |
 *
 * block_header 存储实际分配大小，释放时据此计算基础地址
 */
typedef struct _xy_mem_block_header {
    xy_mem_size_t size;      /**< 用户数据大小（不含头部） */
    void         *pool;      /**< 所属内存池（用于校验） */
#if XY_MEM_TRACKING
    const char   *file;
    int           line;
#endif
    struct _xy_mem_block_header *next;
} xy_mem_block_header_t;

/**
 * @brief 内存池描述符
 */
typedef struct _xy_mem_pool {
    unsigned char         *start;     /**< 池起始地址 */
    xy_mem_size_t          size;     /**< 池总大小 */
    xy_mem_size_t          used;     /**< 已使用大小 */
    xy_mem_count_t         alloc_count; /**< 分配次数 */
    const char            *name;     /**< 池名称（用于调试） */
    struct _xy_mem_pool   *next;     /**< 下一个池（链表） */

    void                  *free_head; /**< 空闲块链表头 */
    xy_mem_size_t          block_min; /**< 最小块大小 */
} xy_mem_pool_t;

/**
 * @brief 内存信息结构
 */
typedef struct _xy_mem_info {
    xy_mem_size_t total;       /**< 池总大小 */
    xy_mem_size_t used;        /**< 已使用大小 */
    xy_mem_size_t peak;        /**< 峰值使用 */
    xy_mem_count_t alloc_count;/**< 分配次数 */
    xy_mem_count_t free_count; /**< 释放次数 */
    xy_mem_size_t largest_free;/**< 最大空闲块 */
} xy_mem_info_t;

/*==============================================================================
 * 内存池管理接口
 *============================================================================*/

/**
 * @brief 声明一个内存池（编译时）
 * @param name  池变量名
 * @param size  池大小（字节数）
 *
 * @note 池必须使用 XY_MEM_POOL_INIT() 初始化后才能使用
 *
 * 示例：
 * @code
 * XY_MEM_POOL_DECLARE(my_pool, 1024);
 * @endcode
 */
#define XY_MEM_POOL_DECLARE(name, size) \
    static unsigned char name##_buffer[size]; \
    static xy_mem_pool_t name##_pool_inst; \
    static int name##_inited = 0

/**
 * @brief 初始化一个静态声明的内存池
 * @param name 池变量名（XY_MEM_POOL_DECLARE 定义的）
 * @param name_str 池名称字符串（用于调试）
 */
#define XY_MEM_POOL_INIT(name) \
    do { \
        if (!name##_inited) { \
            xy_mem_pool_init(&name##_pool_inst, #name, name##_buffer, sizeof(name##_buffer)); \
            name##_inited = 1; \
        } \
    } while (0)

/**
 * @brief 获取内存池实例指针
 * @param name 池变量名
 */
#define XY_MEM_POOL_PTR(name)    (&name##_pool_inst)

/**
 * @brief 初始化内存池
 * @param pool   池描述符指针
 * @param name   池名称（用于调试，可为NULL）
 * @param buffer 内存区域起始地址
 * @param size   内存区域大小
 *
 * @return 0 成功，-1 失败
 */
int xy_mem_pool_init(xy_mem_pool_t *pool, const char *name,
                    void *buffer, xy_mem_size_t size);

/**
 * @brief 从指定内存池分配
 * @param pool  目标池（NULL表示使用默认池）
 * @param size  需要的大小（字节）
 *
 * @return 分配到的内存地址，失败返回NULL
 *
 * @note 分配的大小会被记录，释放时无需传入大小
 */
void *xy_mem_pool_alloc(xy_mem_pool_t *pool, xy_mem_size_t size);

/**
 * @brief 释放内存（从任意池分配的都可用此释放）
 * @param ptr  分配的内存地址
 *
 * @return 0 成功，-1 失败（无效指针）
 */
int xy_mem_pool_free(void *ptr);

/**
 * @brief 获取内存池使用信息
 * @param pool 池描述符（NULL表示使用默认池）
 * @param info 输出信息结构
 *
 * @return 0 成功，-1 失败
 */
int xy_mem_pool_info(xy_mem_pool_t *pool, xy_mem_info_t *info);

/**
 * @brief 重置内存池（释放所有块）
 * @param pool 池描述符
 */
void xy_mem_pool_reset(xy_mem_pool_t *pool);

/*==============================================================================
 * 默认池操作（便捷接口）
 *============================================================================*/

/**
 * @brief 从默认内存池分配
 * @param size 需要的大小
 * @return 分配到的内存地址，失败返回NULL
 */
void *xy_malloc(xy_mem_size_t size);

/**
 * @brief 从默认内存池分配（支持调试信息）
 * @param size 需要的大小
 * @param file 来源文件名
 * @param line 来源行号
 * @return 分配到的内存地址
 */
void *xy_malloc_debug(xy_mem_size_t size, const char *file, int line);

/**
 * @brief 释放内存（默认池）
 * @param ptr 分配的内存地址
 */
void xy_free(void *ptr);

/**
 * @brief 重新分配（默认池）
 * @param ptr  原内存地址
 * @param size 新大小
 * @return 新内存地址，失败返回NULL
 */
void *xy_realloc(void *ptr, xy_mem_size_t size);

/*==============================================================================
 * 不定长内存分配（类似 cJSON）
 *============================================================================*/

/**
 * @brief 分配不定长内存
 * @param size 需要的字节数
 * @return 分配到的内存地址，失败返回NULL
 *
 * @note 释放时使用 xy_free_variable()，无需传入大小
 *
 * 示例：
 * @code
 * char *buf = xy_malloc_variable(256);
 * strcpy(buf, "hello world");
 * xy_free_variable(buf);
 * @endcode
 */
void *xy_malloc_variable(xy_mem_size_t size);

/**
 * @brief 释放不定长分配的内存
 * @param ptr 分配的内存地址
 *
 * @note 内部存储了块大小信息，无需用户传入
 */
void xy_free_variable(void *ptr);

/**
 * @brief 获取不定长分配的原始大小
 * @param ptr 分配的内存地址
 * @return 实际分配的字节数
 */
xy_mem_size_t xy_malloc_variable_size(void *ptr);

/*==============================================================================
 * 内存操作辅助函数
 *============================================================================*/

/**
 * @brief 内存设置
 * @param p    目标地址
 * @param val  设置值（0-255）
 * @param len  字节数
 */
void xy_mem_set(void *p, int val, xy_mem_size_t len);

/**
 * @brief 内存复制
 * @param dest 目标地址
 * @param src  源地址
 * @param len  字节数
 */
void xy_mem_copy(void *dest, const void *src, xy_mem_size_t len);

/**
 * @brief 内存比较
 * @param s1   字符串1
 * @param s2   字符串2
 * @param len  比较字节数
 * @return 0 相等，-1 s1<s2，1 s1>s2
 */
int xy_mem_cmp(const void *s1, const void *s2, xy_mem_size_t len);

/**
 * @brief 内存移动（允许重叠）
 * @param dest 目标地址
 * @param src  源地址
 * @param len  字节数
 */
void xy_mem_move(void *dest, const void *src, xy_mem_size_t len);

/*==============================================================================
 * 内存池注册和管理（运行时多池）
 *============================================================================*/

/**
 * @brief 注册一个内存池（运行时）
 * @param pool 池描述符
 * @return 0 成功，-1 失败（池已满或参数无效）
 */
int xy_mem_register_pool(xy_mem_pool_t *pool);

/**
 * @brief 注销一个内存池
 * @param pool 池描述符
 * @return 0 成功，-1 失败（池未注册）
 */
int xy_mem_unregister_pool(xy_mem_pool_t *pool);

/**
 * @brief 设置默认内存池
 * @param pool 池描述符（NULL使用内部默认池）
 */
void xy_mem_set_default_pool(xy_mem_pool_t *pool);

/**
 * @brief 获取默认内存池
 * @return 默认池描述符
 */
xy_mem_pool_t *xy_mem_get_default_pool(void);

/**
 * @brief 遍历所有已注册池的信息
 * @param info 输出信息数组
 * @param max  数组最大长度
 * @return 实际返回的池数量
 */
int xy_mem_all_pools_info(xy_mem_info_t *info, int max);

/*==============================================================================
 * 调试和诊断
 *============================================================================*/

/**
 * @brief 内存完整性检查
 * @param pool 池描述符（NULL检查所有）
 * @return 0 正常，-1 损坏
 */
int xy_mem_check(xy_mem_pool_t *pool);

/**
 * @brief 内存泄漏检测报告
 * @param pool 池描述符
 * @return 未释放的块数量
 */
xy_mem_count_t xy_mem_leak_report(xy_mem_pool_t *pool);

#ifdef __cplusplus
}
#endif

/*==============================================================================
 * 便捷宏（可选参数）
 *============================================================================*/

/**
 * @brief 调试分配宏（自动记录文件行号）
 */
#if XY_MEM_SAFE_CHECK
    #define XY_MALLOC(size)        xy_malloc_debug(size, __FILE__, __LINE__)
#else
    #define XY_MALLOC(size)        xy_malloc(size)
#endif

/**
 * @brief 从指定池分配的便捷宏
 */
#define XY_POOL_ALLOC(pool, size) xy_mem_pool_alloc(pool, size)

/**
 * @brief 释放宏
 */
#define XY_FREE(ptr)               xy_free(ptr)

/**
 * @brief 不定长分配宏
 */
#define XY_ALLOC_VAR(size)         xy_malloc_variable(size)
#define XY_FREE_VAR(ptr)           xy_free_variable(ptr)

#endif /* _XY_MEM_H_ */
