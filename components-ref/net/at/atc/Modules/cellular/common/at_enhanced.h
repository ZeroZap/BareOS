/**
 * @file at_enhanced.h
 * @brief 增强版AT客户端接口
 */

#ifndef AT_ENHANCED_H
#define AT_ENHANCED_H

#include "at_client.h"
#include "at_urc.h"

/* 增强版AT命令上下文 */
typedef struct {
    const char *cmd;                                 /* AT命令 */
    const char *expect;                              /* 期望响应 */
    uint32_t timeout;                                /* 超时时间 */
    void *user_data;                                 /* 用户数据 */
    bool (*validator)(const char *resp, void *data); /* 响应验证器 */
} at_cmd_context_t;

/* 增强版API */

/**
 * @brief 发送AT命令并等待特定响应
 */
int at_send_and_wait(at_device_t *dev, at_cmd_context_t *ctx);

/**
 * @brief 自动重试发送AT命令
 */
int at_send_with_retry(at_device_t *dev, const char *cmd, int max_retries,
                       uint32_t retry_delay);

/**
 * @brief 批量执行AT命令序列
 */
int at_execute_sequence(at_device_t *dev, at_cmd_context_t *seq[], int count);

/**
 * @brief 设置URC自动应答
 */
int at_set_urc_auto_response(at_device_t *dev, const char *urc_pattern,
                             const char *response_cmd);

/**
 * @brief URC到事件转换器
 */
typedef void (*urc_event_callback_t)(int event_id, void *event_data);

int at_urc_to_event(at_device_t *dev, const char *urc_pattern, int event_id,
                    urc_event_callback_t callback);

#endif /* AT_ENHANCED_H */