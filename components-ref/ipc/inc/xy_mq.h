/**
 * @file xy_mq.h
 * @brief Message Queue for IPC
 * @version 2.0.0
 * @date 2026-03-02
 */

#ifndef XY_MQ_H
#define XY_MQ_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 错误码
 */
#define XY_MQ_OK              0
#define XY_MQ_ERROR           (-1)
#define XY_MQ_INVALID_PARAM   (-2)
#define XY_MQ_FULL            (-3)
#define XY_MQ_EMPTY           (-4)
#define XY_MQ_TIMEOUT         (-5)
#define XY_MQ_NO_MEM          (-6)

/**
 * @brief 消息优先级
 */
typedef enum {
    XY_MQ_PRIORITY_LOW = 0,
    XY_MQ_PRIORITY_NORMAL = 1,
    XY_MQ_PRIORITY_HIGH = 2,
    XY_MQ_PRIORITY_URGENT = 3,
} xy_mq_priority_t;

/**
 * @brief 消息结构
 */
typedef struct {
    uint32_t id;                /* 消息 ID */
    xy_mq_priority_t priority;  /* 优先级 */
    uint32_t timestamp;         /* 时间戳 */
    uint8_t *data;              /* 数据指针 */
    uint16_t len;               /* 数据长度 */
} xy_mq_msg_t;

/**
 * @brief 消息队列配置
 */
typedef struct {
    uint16_t msg_size;          /* 消息大小 */
    uint16_t max_msgs;          /* 最大消息数 */
    bool priority_enabled;      /* 启用优先级 */
    bool overwrite_old;         /* 满时覆盖旧消息 */
} xy_mq_config_t;

/**
 * @brief 消息队列句柄
 */
typedef struct {
    xy_mq_config_t config;
    uint8_t *buffer;
    uint16_t head;
    uint16_t tail;
    uint16_t count;
    uint32_t send_count;
    uint32_t recv_count;
    uint32_t drop_count;
    bool initialized;
} xy_mq_t;

/**
 * @brief 初始化消息队列
 */
int xy_mq_init(xy_mq_t *mq, const xy_mq_config_t *config);

/**
 * @brief 反初始化
 */
int xy_mq_deinit(xy_mq_t *mq);

/**
 * @brief 发送消息
 */
int xy_mq_send(xy_mq_t *mq, const xy_mq_msg_t *msg, uint32_t timeout);

/**
 * @brief 接收消息
 */
int xy_mq_recv(xy_mq_t *mq, xy_mq_msg_t *msg, uint32_t timeout);

/**
 * @brief 尝试发送 (不等待)
 */
int xy_mq_try_send(xy_mq_t *mq, const xy_mq_msg_t *msg);

/**
 * @brief 尝试接收 (不等待)
 */
int xy_mq_try_recv(xy_mq_t *mq, xy_mq_msg_t *msg);

/**
 * @brief 获取消息数量
 */
uint16_t xy_mq_get_count(const xy_mq_t *mq);

/**
 * @brief 获取剩余空间
 */
uint16_t xy_mq_get_free(const xy_mq_t *mq);

/**
 * @brief 清空队列
 */
int xy_mq_clear(xy_mq_t *mq);

/**
 * @brief 获取统计信息
 */
int xy_mq_get_stats(const xy_mq_t *mq, uint32_t *send, uint32_t *recv, uint32_t *drop);

#ifdef __cplusplus
}
#endif

#endif
