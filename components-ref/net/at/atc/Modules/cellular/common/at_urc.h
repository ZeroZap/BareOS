/**
 * @file at_urc.h
 * @brief AT客户端动态URC解析扩展
 */

#ifndef AT_URC_H
#define AT_URC_H

#include "at_client.h"

/* URC匹配模式类型 */
typedef enum {
    URC_MATCH_PREFIX = 0, /* 前缀匹配（默认） */
    URC_MATCH_SUFFIX,     /* 后缀匹配 */
    URC_MATCH_CONTAIN,    /* 包含匹配 */
    URC_MATCH_EXACT,      /* 精确匹配 */
    URC_MATCH_REGEX       /* 正则表达式匹配 */
} urc_match_type_t;

/* URC回调函数类型 */
typedef void (*urc_callback_t)(at_device_t *dev, const char *data,
                               void *user_data);

/* URC处理项 */
typedef struct at_urc_handler {
    char *pattern;               /* 匹配模式 */
    urc_match_type_t match_type; /* 匹配类型 */
    urc_callback_t callback;     /* 回调函数 */
    void *user_data;             /* 用户数据 */
    bool enabled;                /* 是否启用 */
    uint32_t hit_count;          /* 命中次数 */
    struct at_urc_handler *next; /* 下一个 */
} at_urc_handler_t;

/* URC管理器 */
typedef struct {
    at_urc_handler_t *handlers; /* URC处理器链表 */
    uint16_t handler_count;     /* 处理器数量 */
    uint32_t urc_total;         /* URC总数 */
    bool processing;            /* 是否正在处理 */
} at_urc_manager_t;

/* API函数声明 */

/**
 * @brief 初始化URC管理器
 */
int at_urc_init(at_device_t *dev);

/**
 * @brief 注册URC处理器
 * @param dev AT设备
 * @param pattern 匹配模式
 * @param match_type 匹配类型
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return 处理器ID，失败返回-1
 */
int at_urc_register(at_device_t *dev, const char *pattern,
                    urc_match_type_t match_type, urc_callback_t callback,
                    void *user_data);

/**
 * @brief 注销URC处理器
 */
int at_urc_unregister(at_device_t *dev, int handler_id);

/**
 * @brief 启用/禁用URC处理器
 */
int at_urc_set_enable(at_device_t *dev, int handler_id, bool enable);

/**
 * @brief 处理URC数据
 * @param dev AT设备
 * @param data 接收到的数据
 * @param len 数据长度
 * @return 是否被处理
 */
bool at_urc_process(at_device_t *dev, const char *data, uint32_t len);

/**
 * @brief URC后台处理任务（RTOS环境）
 */
void at_urc_task(void *param);

/**
 * @brief URC轮询处理（裸机环境）
 */
void at_urc_poll(at_device_t *dev);

/**
 * @brief 获取URC统计信息
 */
void at_urc_get_stats(at_device_t *dev, uint32_t *total, uint32_t *handled);

/**
 * @brief 清除所有URC处理器
 */
void at_urc_clear(at_device_t *dev);

#endif /* AT_URC_H */