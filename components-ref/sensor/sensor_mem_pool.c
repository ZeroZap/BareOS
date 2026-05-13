#include "sensor_core.h"

#if !SENSOR_USE_MALLOC

/* 内存池配置 */
#define MEM_POOL_BLOCK_SIZE  64
#define MEM_POOL_BLOCK_COUNT 32

/* 内存块结构 */
typedef struct mem_block {
    struct mem_block *next;
    uint8_t data[MEM_POOL_BLOCK_SIZE];
} mem_block_t;

/* 内存池 */
static mem_block_t mem_pool[MEM_POOL_BLOCK_COUNT];
static mem_block_t *free_list    = NULL;
static bool mem_pool_initialized = false;

/**
 * @brief 初始化内存池
 */
static void sensor_mem_pool_init(void)
{
    if (mem_pool_initialized) {
        return;
    }

    /* 初始化空闲链表 */
    free_list = &mem_pool[0];
    for (int i = 0; i < MEM_POOL_BLOCK_COUNT - 1; i++) {
        mem_pool[i].next = &mem_pool[i + 1];
    }
    mem_pool[MEM_POOL_BLOCK_COUNT - 1].next = NULL;

    mem_pool_initialized = true;

    SENSOR_LOG("Memory pool initialized: %d blocks x %d bytes",
               MEM_POOL_BLOCK_COUNT, MEM_POOL_BLOCK_SIZE);
}

/**
 * @brief 从内存池分配内存
 */
void *sensor_mem_alloc(size_t size)
{
    if (!mem_pool_initialized) {
        sensor_mem_pool_init();
    }

    if (size > MEM_POOL_BLOCK_SIZE || free_list == NULL) {
        SENSOR_LOG("Memory allocation failed: size=%u", size);
        return NULL;
    }

    /* 从空闲链表取出一个块 */
    mem_block_t *block = free_list;
    free_list          = block->next;

    return block->data;
}

/**
 * @brief 释放内存到内存池
 */
void sensor_mem_free(void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    /* 计算块地址 */
    mem_block_t *block =
        (mem_block_t *)((uint8_t *)ptr - offsetof(mem_block_t, data));

    /* 放回空闲链表 */
    block->next = free_list;
    free_list   = block;
}

/**
 * @brief 获取内存池使用情况
 */
void sensor_mem_pool_stats(uint32_t *total, uint32_t *used, uint32_t *free)
{
    if (!mem_pool_initialized) {
        if (total)
            *total = 0;
        if (used)
            *used = 0;
        if (free)
            *free = 0;
        return;
    }

    /* 统计空闲块数量 */
    uint32_t free_count = 0;
    mem_block_t *p      = free_list;
    while (p != NULL) {
        free_count++;
        p = p->next;
    }

    if (total)
        *total = MEM_POOL_BLOCK_COUNT;
    if (used)
        *used = MEM_POOL_BLOCK_COUNT - free_count;
    if (free)
        *free = free_count;
}

#endif /* !SENSOR_USE_MALLOC */