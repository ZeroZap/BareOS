/**
 * @file xy_mem.c
 * @brief 动态内存管理组件实现
 *
 * 支持：
 * - 多内存池管理（类似 lwIP mem pools）
 * - 不定长内存分配（类似 cJSON）
 * - 裸机（Bare-metal）和 RTOS 环境
 */

#include "xy_mem.h"
#include <string.h>

/*==============================================================================
 * 后端检测（与 xy_os_cfg.h 保持一致）
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
 * 平台适配层
 *============================================================================*/

#if XY_OS_BACKEND_BAREMETAL
    #define MEM_ENTER_CRITICAL()    do { } while (0)
    #define MEM_EXIT_CRITICAL()     do { } while (0)
#elif XY_OS_BACKEND_FREERTOS
    #define MEM_ENTER_CRITICAL()    taskENTER_CRITICAL()
    #define MEM_EXIT_CRITICAL()     taskEXIT_CRITICAL()
#elif XY_OS_BACKEND_RTTHREAD
    #define MEM_ENTER_CRITICAL()    rt_enter_critical()
    #define MEM_EXIT_CRITICAL()     rt_exit_critical()
#else
    #define MEM_ENTER_CRITICAL()    do { } while (0)
    #define MEM_EXIT_CRITICAL()     do { } while (0)
#endif

/*==============================================================================
 * 内部常量
 *============================================================================*/

#define MEM_ALIGN_SIZE         XY_MEM_ALIGN_SIZE
#define MEM_ALIGN_MASK         (MEM_ALIGN_SIZE - 1)
#define MEM_BLOCK_HEADER_SIZE  (sizeof(xy_mem_block_header_t))

/* 最小块大小（能容纳一个指针） */
#define MEM_MIN_BLOCK_SIZE     (sizeof(void *) * 2)

/*==============================================================================
 * 内部数据类型
 *============================================================================*/

/**
 * @brief 内存块描述（池内部分配）
 */
typedef struct _mem_block {
    xy_mem_size_t         size;       /* 块大小（用户可用） */
    struct _mem_block    *next;       /* 下一个空闲块 */
} mem_block_t;

/**
 * @brief 已分配块追踪（用于释放验证）
 */
typedef struct _alloc_entry {
    void                 *ptr;
    xy_mem_size_t         size;
    xy_mem_pool_t        *pool;
#if XY_MEM_TRACKING
    const char           *file;
    int                   line;
#endif
    struct _alloc_entry  *next;
} alloc_entry_t;

/*==============================================================================
 * 内部变量
 *============================================================================*/

static xy_mem_pool_t    *g_default_pool = NULL;
static xy_mem_pool_t    *g_pool_list[XY_MEM_MAX_POOLS] = { NULL };
static int               g_pool_count = 0;
static alloc_entry_t    *g_alloc_list = NULL;

/* 内置默认池（用于小量分配） */
#ifdef XY_MEM_USE_INTERNAL_POOL
static unsigned char     g_internal_buffer[XY_MEM_INTERNAL_POOL_SIZE];
static xy_mem_pool_t     g_internal_pool;
#endif

/*==============================================================================
 * 内部函数声明
 *============================================================================*/

static void *pool_alloc_from_free_list(xy_mem_pool_t *pool, xy_mem_size_t size);
static void pool_free_to_free_list(xy_mem_pool_t *pool, void *ptr, xy_mem_size_t size);
static xy_mem_size_t align_size(xy_mem_size_t size);
static int add_alloc_record(void *ptr, xy_mem_size_t size, xy_mem_pool_t *pool,
                           const char *file, int line);
static int remove_alloc_record(void *ptr);
static alloc_entry_t *find_alloc_record(void *ptr);
static xy_mem_pool_t *find_pool_by_ptr(void *ptr);

/*==============================================================================
 * 辅助函数实现
 *============================================================================*/

static xy_mem_size_t align_size(xy_mem_size_t size)
{
    return (size + MEM_ALIGN_MASK) & ~MEM_ALIGN_MASK;
}

static alloc_entry_t *find_alloc_record(void *ptr)
{
    alloc_entry_t *p;
    for (p = g_alloc_list; p != NULL; p = p->next) {
        if (p->ptr == ptr) {
            return p;
        }
    }
    return NULL;
}

static int add_alloc_record(void *ptr, xy_mem_size_t size, xy_mem_pool_t *pool,
                           const char *file, int line)
{
#if XY_MEM_TRACKING
    alloc_entry_t *entry;
    entry = (alloc_entry_t *)xy_mem_pool_alloc(NULL, sizeof(alloc_entry_t));
    if (entry == NULL) {
        return -1;
    }
    entry->ptr = ptr;
    entry->size = size;
    entry->pool = pool;
    entry->file = file;
    entry->line = line;
    entry->next = g_alloc_list;
    g_alloc_list = entry;
#endif
    (void)ptr;
    (void)size;
    (void)pool;
    (void)file;
    (void)line;
    return 0;
}

static int remove_alloc_record(void *ptr)
{
#if XY_MEM_TRACKING
    alloc_entry_t **pp;
    for (pp = &g_alloc_list; *pp != NULL; pp = &(*pp)->next) {
        if ((*pp)->ptr == ptr) {
            alloc_entry_t *tmp = *pp;
            *pp = tmp->next;
            xy_mem_pool_free(NULL, tmp);
            return 0;
        }
    }
#endif
    (void)ptr;
    return -1;
}

static xy_mem_pool_t *find_pool_by_ptr(void *ptr)
{
#if XY_MEM_TRACKING
    alloc_entry_t *entry = find_alloc_record(ptr);
    return entry ? entry->pool : NULL;
#else
    xy_mem_pool_t *p;
    for (int i = 0; i < g_pool_count; i++) {
        p = g_pool_list[i];
        if (p && p->start <= (unsigned char *)ptr &&
            (unsigned char *)ptr < p->start + p->size) {
            return p;
        }
    }
    return NULL;
#endif
}

/*==============================================================================
 * 内存池管理实现
 *============================================================================*/

int xy_mem_pool_init(xy_mem_pool_t *pool, const char *name,
                    void *buffer, xy_mem_size_t size)
{
    if (pool == NULL || buffer == NULL || size < MEM_MIN_BLOCK_SIZE) {
        return -1;
    }

    MEM_ENTER_CRITICAL();

    /* 初始化池描述符 */
    pool->start = (unsigned char *)buffer;
    pool->size = size;
    pool->used = 0;
    pool->alloc_count = 0;
    pool->name = name;
    pool->next = NULL;

    /* 初始化空闲链表（整个池作为一个空闲块） */
    mem_block_t *first_block = (mem_block_t *)buffer;
    first_block->size = size;
    first_block->next = NULL;
    pool->free_head = first_block;
    pool->block_min = size;

    MEM_EXIT_CRITICAL();

    /* 注册到全局池列表 */
    xy_mem_register_pool(pool);

    return 0;
}

void *xy_mem_pool_alloc(xy_mem_pool_t *pool, xy_mem_size_t size)
{
    void *ptr = NULL;
    xy_mem_size_t alloc_size;

    if (size == 0) {
        return NULL;
    }

    /* 使用默认池 */
    if (pool == NULL) {
        pool = g_default_pool;
        if (pool == NULL) {
#ifdef XY_MEM_USE_INTERNAL_POOL
            pool = &g_internal_pool;
#else
            return NULL;
#endif
        }
    }

    /* 对齐大小 */
    alloc_size = align_size(size);

    MEM_ENTER_CRITICAL();

    ptr = pool_alloc_from_free_list(pool, alloc_size);

    if (ptr != NULL) {
        pool->alloc_count++;

        /* 记录分配信息 */
        add_alloc_record(ptr, size, pool, NULL, 0);
    }

    MEM_EXIT_CRITICAL();

    return ptr;
}

static void *pool_alloc_from_free_list(xy_mem_pool_t *pool, xy_mem_size_t size)
{
    mem_block_t **pp;
    mem_block_t *p;

    if (pool == NULL || pool->free_head == NULL) {
        return NULL;
    }

    /* 最佳适配算法 */
    pp = (mem_block_t **)&pool->free_head;
    p = *pp;

    while (p != NULL) {
        if (p->size >= size) {
            /* 找到合适的块 */
            xy_mem_size_t remaining = p->size - size;

            /* 如果剩余空间足够创建一个新块（包含头部） */
            if (remaining >= MEM_MIN_BLOCK_SIZE) {
                /* 从当前块分割 */
                mem_block_t *new_block = (mem_block_t *)((unsigned char *)p + size);
                new_block->size = remaining;
                new_block->next = p->next;

                /* 更新当前块 */
                p->size = size;
                p->next = new_block;
            }

            /* 从空闲链表移除 */
            *pp = p->next;

            pool->used += size;
            if (pool->used > pool->size) {
                pool->used = pool->size;
            }

            /* 返回用户数据地址（块头部之后） */
            return (unsigned char *)p + sizeof(mem_block_t);
        }

        pp = (mem_block_t **)&p->next;
        p = *pp;
    }

    return NULL;
}

int xy_mem_pool_free(void *ptr)
{
    xy_mem_pool_t *pool;
    mem_block_t *block;
    xy_mem_size_t block_size;
    alloc_entry_t *record;

    if (ptr == NULL) {
        return -1;
    }

#if XY_MEM_SAFE_CHECK
    /* 查找分配记录 */
    record = find_alloc_record(ptr);
    if (record == NULL) {
        /* 未找到记录，可能是双重释放 */
        return -1;
    }
    pool = record->pool;
    block_size = record->size;
#else
    pool = find_pool_by_ptr(ptr);
    if (pool == NULL) {
        return -1;
    }
    /* 计算块大小（需要估算或从记录获取） */
    block_size = 0;  /* 如果没有追踪则无法精确释放 */
#endif

    /* 计算块头部地址 */
    block = (mem_block_t *)((unsigned char *)ptr - sizeof(mem_block_t));

    MEM_ENTER_CRITICAL();

    /* 归还到空闲链表（按地址排序合并） */
    pool_free_to_free_list(pool, block, block_size);

    /* 更新统计 */
    pool->used -= block_size;
    if (pool->used < 0) {
        pool->used = 0;
    }

    /* 移除分配记录 */
    remove_alloc_record(ptr);

    MEM_EXIT_CRITICAL();

    return 0;
}

static void pool_free_to_free_list(xy_mem_pool_t *pool, void *ptr, xy_mem_size_t size)
{
    mem_block_t *block = (mem_block_t *)((unsigned char *)ptr - sizeof(mem_block_t));
    mem_block_t *p;
    mem_block_t *prev = NULL;

    block->size = size;
    block->next = NULL;

    /* 按地址顺序插入空闲链表 */
    p = (mem_block_t *)pool->free_head;

    while (p != NULL) {
        if (block < p) {
            /* 插入到 p 之前 */
            break;
        }
        prev = p;
        p = p->next;
    }

    if (prev == NULL) {
        /* 插入到链表头 */
        block->next = pool->free_head;
        pool->free_head = block;
    } else {
        /* 插入到 prev 之后 */
        block->next = prev->next;
        prev->next = block;
    }

    /* 尝试与前后块合并 */
    /* 与前一个合并 */
    if (prev != NULL) {
        unsigned char *prev_end = (unsigned char *)prev + prev->size;
        if (prev_end == (unsigned char *)block) {
            prev->size += block->size;
            prev->next = block->next;
            block = prev;
        }
    }

    /* 与后一个合并 */
    if (block->next != NULL) {
        unsigned char *block_end = (unsigned char *)block + block->size;
        if (block_end == (unsigned char *)block->next) {
            block->size += block->next->size;
            block->next = block->next->next;
        }
    }
}

int xy_mem_pool_info(xy_mem_pool_t *pool, xy_mem_info_t *info)
{
    if (pool == NULL || info == NULL) {
        return -1;
    }

    MEM_ENTER_CRITICAL();

    info->total = pool->size;
    info->used = pool->used;
    info->alloc_count = pool->alloc_count;
    info->free_count = 0;  /* 需要追踪才能计算 */
    info->largest_free = 0;

    /* 遍历空闲链表找最大块 */
    mem_block_t *p = (mem_block_t *)pool->free_head;
    while (p != NULL) {
        if (p->size > info->largest_free) {
            info->largest_free = p->size;
        }
        info->free_count++;
        p = p->next;
    }

    MEM_EXIT_CRITICAL();

    return 0;
}

void xy_mem_pool_reset(xy_mem_pool_t *pool)
{
    if (pool == NULL) {
        return;
    }

    MEM_ENTER_CRITICAL();

    /* 整个池作为一个空闲块 */
    mem_block_t *first_block = (mem_block_t *)pool->start;
    first_block->size = pool->size;
    first_block->next = NULL;
    pool->free_head = first_block;
    pool->used = 0;
    pool->alloc_count = 0;

    MEM_EXIT_CRITICAL();
}

/*==============================================================================
 * 默认池操作实现
 *============================================================================*/

void *xy_malloc(xy_mem_size_t size)
{
    return xy_mem_pool_alloc(g_default_pool, size);
}

void *xy_malloc_debug(xy_mem_size_t size, const char *file, int line)
{
    void *ptr = xy_mem_pool_alloc(g_default_pool, size);

#if XY_MEM_TRACKING
    if (ptr != NULL && file != NULL) {
        alloc_entry_t *entry = find_alloc_record(ptr);
        if (entry != NULL) {
            entry->file = file;
            entry->line = line;
        }
    }
#endif

    (void)file;
    (void)line;
    return ptr;
}

void xy_free(void *ptr)
{
    xy_mem_pool_free(ptr);
}

void *xy_realloc(void *ptr, xy_mem_size_t size)
{
    void *new_ptr;
    xy_mem_size_t old_size = 0;
    alloc_entry_t *record;

    if (ptr == NULL) {
        return xy_malloc(size);
    }

    if (size == 0) {
        xy_free(ptr);
        return NULL;
    }

    /* 获取旧大小 */
    record = find_alloc_record(ptr);
    if (record != NULL) {
        old_size = record->size;
    }

    /* 分配新内存 */
    new_ptr = xy_malloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }

    /* 复制旧数据 */
    if (old_size > 0 && size > 0) {
        xy_mem_copy(new_ptr, ptr, old_size < size ? old_size : size);
    }

    /* 释放旧内存 */
    xy_free(ptr);

    return new_ptr;
}

/*==============================================================================
 * 不定长内存分配实现
 *============================================================================*/

void *xy_malloc_variable(xy_mem_size_t size)
{
    xy_mem_block_header_t *header;
    void *user_ptr;

    if (size == 0) {
        return NULL;
    }

    /* 分配：头部 + 对齐 + 用户数据 */
    header = (xy_mem_block_header_t *)xy_malloc(
        sizeof(xy_mem_block_header_t) + MEM_ALIGN_SIZE + size);

    if (header == NULL) {
        return NULL;
    }

    /* 初始化头部 */
    header->size = size;
    header->pool = g_default_pool;
    header->next = NULL;

    /* 用户指针：对齐后跳过头部 */
    user_ptr = (void *)((unsigned char *)header +
                        sizeof(xy_mem_block_header_t));

    /* 记录分配 */
#if XY_MEM_TRACKING
    add_alloc_record(user_ptr, size, header->pool, NULL, 0);
#endif

    return user_ptr;
}

void xy_free_variable(void *ptr)
{
    xy_mem_block_header_t *header;

    if (ptr == NULL) {
        return;
    }

    /* 回推头部地址 */
    header = (xy_mem_block_header_t *)
             ((unsigned char *)ptr - sizeof(xy_mem_block_header_t));

#if XY_MEM_SAFE_CHECK
    /* 验证头部有效性 */
    if (header->pool == NULL) {
        /* 已经被释放 */
        return;
    }
#endif

    /* 清空池标记（防止二次释放） */
    header->pool = NULL;

#if XY_MEM_TRACKING
    remove_alloc_record(ptr);
#endif

    /* 释放整个块（包括头部） */
    xy_free(header);
}

xy_mem_size_t xy_malloc_variable_size(void *ptr)
{
    xy_mem_block_header_t *header;

    if (ptr == NULL) {
        return 0;
    }

    header = (xy_mem_block_header_t *)
             ((unsigned char *)ptr - sizeof(xy_mem_block_header_t));

    return header->size;
}

/*==============================================================================
 * 内存操作辅助函数实现
 *============================================================================*/

void xy_mem_set(void *p, int val, xy_mem_size_t len)
{
    unsigned char *pp = (unsigned char *)p;
    while (len--) {
        *pp++ = (unsigned char)val;
    }
}

void xy_mem_copy(void *dest, const void *src, xy_mem_size_t len)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    if (dest == NULL || src == NULL || len == 0) {
        return;
    }

    /* 处理重叠情况 */
    if (dest < src) {
        while (len--) {
            *d++ = *s++;
        }
    } else {
        d += len;
        s += len;
        while (len--) {
            *--d = *--s;
        }
    }
}

int xy_mem_cmp(const void *s1, const void *s2, xy_mem_size_t len)
{
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    if (s1 == s2) {
        return 0;
    }

    while (len--) {
        if (*p1 != *p2) {
            return (*p1 > *p2) ? 1 : -1;
        }
        p1++;
        p2++;
    }

    return 0;
}

void xy_mem_move(void *dest, const void *src, xy_mem_size_t len)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    if (dest == NULL || src == NULL || len == 0) {
        return;
    }

    /* 使用 memmove 语义：允许重叠 */
    if (dest < src) {
        while (len--) {
            *d++ = *s++;
        }
    } else {
        d += len;
        s += len;
        while (len--) {
            *--d = *--s;
        }
    }
}

/*==============================================================================
 * 内存池注册和管理实现
 *============================================================================*/

int xy_mem_register_pool(xy_mem_pool_t *pool)
{
    if (pool == NULL) {
        return -1;
    }

    if (g_pool_count >= XY_MEM_MAX_POOLS) {
        return -1;
    }

    MEM_ENTER_CRITICAL();

    g_pool_list[g_pool_count++] = pool;

    /* 如果还没有默认池，设为第一个 */
    if (g_default_pool == NULL) {
        g_default_pool = pool;
    }

    MEM_EXIT_CRITICAL();

    return 0;
}

int xy_mem_unregister_pool(xy_mem_pool_t *pool)
{
    int i;
    int found = 0;

    if (pool == NULL) {
        return -1;
    }

    MEM_ENTER_CRITICAL();

    for (i = 0; i < g_pool_count; i++) {
        if (g_pool_list[i] == pool) {
            /* 移除：后面的池前移 */
            for (; i < g_pool_count - 1; i++) {
                g_pool_list[i] = g_pool_list[i + 1];
            }
            g_pool_count--;
            found = 1;
            break;
        }
    }

    /* 如果移除的是默认池，更新默认池 */
    if (found && g_default_pool == pool) {
        g_default_pool = (g_pool_count > 0) ? g_pool_list[0] : NULL;
    }

    MEM_EXIT_CRITICAL();

    return found ? 0 : -1;
}

void xy_mem_set_default_pool(xy_mem_pool_t *pool)
{
    MEM_ENTER_CRITICAL();
    g_default_pool = pool;
    MEM_EXIT_CRITICAL();
}

xy_mem_pool_t *xy_mem_get_default_pool(void)
{
    return g_default_pool;
}

int xy_mem_all_pools_info(xy_mem_info_t *info, int max)
{
    int count = 0;
    int i;

    MEM_ENTER_CRITICAL();

    for (i = 0; i < g_pool_count && count < max; i++) {
        if (info != NULL) {
            xy_mem_pool_info(g_pool_list[i], &info[count]);
        }
        count++;
    }

    MEM_EXIT_CRITICAL();

    return count;
}

/*==============================================================================
 * 调试和诊断实现
 *============================================================================*/

int xy_mem_check(xy_mem_pool_t *pool)
{
    mem_block_t *p;
    int ret = 0;

    MEM_ENTER_CRITICAL();

    if (pool == NULL) {
        /* 检查所有池 */
        for (int i = 0; i < g_pool_count; i++) {
            if (xy_mem_check(g_pool_list[i]) != 0) {
                ret = -1;
            }
        }
    } else {
        /* 检查空闲链表完整性 */
        p = (mem_block_t *)pool->free_head;
        while (p != NULL) {
            /* 验证块在池范围内 */
            if ((unsigned char *)p < pool->start ||
                (unsigned char *)p + p->size > pool->start + pool->size) {
                ret = -1;
                break;
            }
            p = p->next;
        }
    }

    MEM_EXIT_CRITICAL();

    return ret;
}

xy_mem_count_t xy_mem_leak_report(xy_mem_pool_t *pool)
{
    xy_mem_count_t count = 0;

#if XY_MEM_TRACKING
    alloc_entry_t *p;

    MEM_ENTER_CRITICAL();

    for (p = g_alloc_list; p != NULL; p = p->next) {
        if (pool == NULL || p->pool == pool) {
            count++;
#if defined(_XY_DEBUG_) || XY_DEBUG
            /* 打印泄漏信息 */
            /* printf("LEAK: %p, size=%d, file=%s, line=%d\n",
                   p->ptr, p->size, p->file ? p->file : "unknown", p->line); */
#endif
        }
    }

    MEM_EXIT_CRITICAL();
#else
    (void)pool;
#endif

    return count;
}

/*==============================================================================
 * 初始化（在系统初始化时调用）
 *============================================================================*/

#ifdef XY_MEM_USE_INTERNAL_POOL

void xy_mem_init_internal_pool(void)
{
    static int inited = 0;
    if (!inited) {
        xy_mem_pool_init(&g_internal_pool, "internal",
                        g_internal_buffer, sizeof(g_internal_buffer));
        inited = 1;
    }
}

#endif

/*==============================================================================
 * 单元测试（仅在 PLATFORM == PLATFORM_X86 时编译）
 *============================================================================*/

#if (PLATFORM == PLATFORM_X86)

#include <stdio.h>

void xy_mem_test(void)
{
    unsigned char ram[1024];
    xy_mem_pool_t pool;
    void *p1, *p2, *p3;
    xy_mem_info_t info;

    printf("\n=== Memory Pool Test ===\n");

    /* 初始化池 */
    xy_mem_pool_init(&pool, "test", ram, sizeof(ram));
    printf("Pool init: %d bytes\n", pool.size);

    /* 分配测试 */
    p1 = xy_mem_pool_alloc(&pool, 100);
    printf("Alloc p1: %p (100 bytes)\n", p1);

    p2 = xy_mem_pool_alloc(&pool, 200);
    printf("Alloc p2: %p (200 bytes)\n", p2);

    p3 = xy_mem_pool_alloc(&pool, 50);
    printf("Alloc p3: %p (50 bytes)\n", p3);

    xy_mem_pool_info(&pool, &info);
    printf("Used: %d/%d, Alloc count: %d\n", info.used, info.total, info.alloc_count);

    /* 释放测试 */
    printf("\nFree p2...\n");
    xy_mem_pool_free(p2);

    xy_mem_pool_info(&pool, &info);
    printf("Used: %d, Largest free: %d\n", info.used, info.largest_free);

    /* 重新分配到刚释放的空间 */
    p2 = xy_mem_pool_alloc(&pool, 80);
    printf("Realloc p2: %p (80 bytes)\n", p2);

    /* 不定长分配测试 */
    printf("\n=== Variable Size Alloc Test ===\n");
    void *v1 = xy_malloc_variable(256);
    printf("Variable alloc v1: %p (256 bytes)\n", v1);
    printf("Variable size: %d\n", xy_malloc_variable_size(v1));
    xy_free_variable(v1);
    printf("Variable freed\n");

    /* 泄漏检测 */
    printf("\n=== Leak Report ===\n");
    xy_mem_count_t leaks = xy_mem_leak_report(&pool);
    printf("Leaks: %d\n", leaks);

    printf("\n=== Test Complete ===\n");
}

#endif /* PLATFORM_X86 */
