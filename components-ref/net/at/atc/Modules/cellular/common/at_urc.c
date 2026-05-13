/**
 * @file at_urc.c
 * @brief 动态URC解析实现
 */

#include "at_urc.h"
#include <stdlib.h>
#include <string.h>
#ifdef USE_REGEX
#include <regex.h>
#endif

/* 内部数据结构 */
typedef struct {
    at_urc_manager_t urc_manager;
} at_device_private_t;

/* 默认URC处理器 */
static void default_urc_echo_handler(at_device_t *dev, const char *data,
                                     void *user_data);
static void default_urc_ready_handler(at_device_t *dev, const char *data,
                                      void *user_data);

/* 匹配函数 */
static bool urc_match_prefix(const char *data, const char *pattern);
static bool urc_match_suffix(const char *data, const char *pattern);
static bool urc_match_contain(const char *data, const char *pattern);
static bool urc_match_exact(const char *data, const char *pattern);
#ifdef USE_REGEX
static bool urc_match_regex(const char *data, const char *pattern);
#endif

/* 初始化URC管理器 */
int at_urc_init(at_device_t *dev)
{
    if (!dev) {
        return -1;
    }

    /* 分配私有数据 */
    at_device_private_t *priv =
        (at_device_private_t *)malloc(sizeof(at_device_private_t));
    if (!priv) {
        return -1;
    }

    memset(priv, 0, sizeof(at_device_private_t));
    dev->user_data = priv; /* 存储到设备私有数据 */

    /* 注册默认URC处理器 */
    at_urc_register(
        dev, "ATE", URC_MATCH_PREFIX, default_urc_echo_handler, NULL);
    at_urc_register(
        dev, "RDY", URC_MATCH_EXACT, default_urc_ready_handler, NULL);

    return 0;
}

/* 注册URC处理器 */
int at_urc_register(at_device_t *dev, const char *pattern,
                    urc_match_type_t match_type, urc_callback_t callback,
                    void *user_data)
{
    if (!dev || !pattern || !callback) {
        return -1;
    }

    at_device_private_t *priv = (at_device_private_t *)dev->user_data;
    if (!priv) {
        return -1;
    }

    /* 创建新的处理器 */
    at_urc_handler_t *handler =
        (at_urc_handler_t *)malloc(sizeof(at_urc_handler_t));
    if (!handler) {
        return -1;
    }

    handler->pattern    = strdup(pattern);
    handler->match_type = match_type;
    handler->callback   = callback;
    handler->user_data  = user_data;
    handler->enabled    = true;
    handler->hit_count  = 0;
    handler->next       = NULL;

    /* 添加到链表 */
    if (!priv->urc_manager.handlers) {
        priv->urc_manager.handlers = handler;
    } else {
        at_urc_handler_t *last = priv->urc_manager.handlers;
        while (last->next) {
            last = last->next;
        }
        last->next = handler;
    }

    priv->urc_manager.handler_count++;
    return priv->urc_manager.handler_count - 1; /* 返回处理器ID */
}

/* 处理URC数据 */
bool at_urc_process(at_device_t *dev, const char *data, uint32_t len)
{
    if (!dev || !data || len == 0) {
        return false;
    }

    at_device_private_t *priv = (at_device_private_t *)dev->user_data;
    if (!priv || priv->urc_manager.processing) {
        return false;
    }

    priv->urc_manager.processing = true;
    bool processed               = false;
    char line[256];
    uint32_t line_idx = 0;

    /* 按行分割处理 */
    for (uint32_t i = 0; i < len; i++) {
        if (data[i] == '\r' || data[i] == '\n') {
            if (line_idx > 0) {
                line[line_idx] = '\0';
                processed |= urc_process_line(dev, line, priv);
                line_idx = 0;
            }
        } else if (line_idx < sizeof(line) - 1) {
            line[line_idx++] = data[i];
        }
    }

    /* 处理剩余数据 */
    if (line_idx > 0) {
        line[line_idx] = '\0';
        processed |= urc_process_line(dev, line, priv);
    }

    if (processed) {
        priv->urc_manager.urc_total++;
    }

    priv->urc_manager.processing = false;
    return processed;
}

/* 处理单行URC数据 */
static bool urc_process_line(at_device_t *dev, const char *line,
                             at_device_private_t *priv)
{
    at_urc_handler_t *handler = priv->urc_manager.handlers;
    bool processed            = false;

    while (handler) {
        if (handler->enabled) {
            bool matched = false;

            switch (handler->match_type) {
            case URC_MATCH_PREFIX:
                matched = urc_match_prefix(line, handler->pattern);
                break;
            case URC_MATCH_SUFFIX:
                matched = urc_match_suffix(line, handler->pattern);
                break;
            case URC_MATCH_CONTAIN:
                matched = urc_match_contain(line, handler->pattern);
                break;
            case URC_MATCH_EXACT:
                matched = urc_match_exact(line, handler->pattern);
                break;
#ifdef USE_REGEX
            case URC_MATCH_REGEX:
                matched = urc_match_regex(line, handler->pattern);
                break;
#endif
            default:
                matched = urc_match_prefix(line, handler->pattern);
                break;
            }

            if (matched) {
                handler->hit_count++;
                if (handler->callback) {
                    handler->callback(dev, line, handler->user_data);
                }
                processed = true;
                break; /* 匹配成功后停止，避免重复处理 */
            }
        }
        handler = handler->next;
    }

    return processed;
}

/* 匹配函数实现 */
static bool urc_match_prefix(const char *data, const char *pattern)
{
    return strncmp(data, pattern, strlen(pattern)) == 0;
}

static bool urc_match_suffix(const char *data, const char *pattern)
{
    size_t data_len    = strlen(data);
    size_t pattern_len = strlen(pattern);

    if (pattern_len > data_len) {
        return false;
    }

    return strcmp(data + data_len - pattern_len, pattern) == 0;
}

static bool urc_match_contain(const char *data, const char *pattern)
{
    return strstr(data, pattern) != NULL;
}

static bool urc_match_exact(const char *data, const char *pattern)
{
    return strcmp(data, pattern) == 0;
}

#ifdef USE_REGEX
static bool urc_match_regex(const char *data, const char *pattern)
{
    regex_t regex;
    int ret;

    ret = regcomp(&regex, pattern, REG_EXTENDED | REG_NOSUB);
    if (ret) {
        return false;
    }

    ret = regexec(&regex, data, 0, NULL, 0);
    regfree(&regex);

    return ret == 0;
}
#endif

/* 默认URC处理器 */
static void default_urc_echo_handler(at_device_t *dev, const char *data,
                                     void *user_data)
{
    (void)user_data;
    printf("URC Echo: %s\n", data);
}

static void default_urc_ready_handler(at_device_t *dev, const char *data,
                                      void *user_data)
{
    (void)user_data;
    printf("URC Ready: %s\n", data);
}

/* URC后台处理任务（RTOS） */
void at_urc_task(void *param)
{
    at_device_t *dev = (at_device_t *)param;

    while (1) {
        char buffer[256];
        int len = at_readline(dev, buffer, sizeof(buffer));

        if (len > 0) {
            at_urc_process(dev, buffer, len);
        }

#ifdef USE_RTOS
        vTaskDelay(pdMS_TO_TICKS(10));
#else
        // 裸机延迟
#endif
    }
}

/* URC轮询处理（裸机） */
void at_urc_poll(at_device_t *dev)
{
    if (!dev) {
        return;
    }

    char buffer[256];
    int len = at_readline(dev, buffer, sizeof(buffer));

    if (len > 0) {
        at_urc_process(dev, buffer, len);
    }
}