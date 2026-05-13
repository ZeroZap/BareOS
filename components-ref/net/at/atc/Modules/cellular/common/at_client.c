/**
 * @file at_client.c
 * @brief AT客户端框架实现
 */

#include "at_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 全局客户端实例 */
static at_client_t g_at_client;

/* 内部函数声明 */
static at_tick_t at_get_time_diff(at_tick_t start, at_tick_t current);
static int at_process_response(at_device_t *dev);
static void at_reset_resp_buffer(at_resp_buffer_t *buf);

/* 默认响应处理器 */
static bool default_resp_handler(at_device_t *dev, const char *resp_line,
                                 void *user_data, at_resp_type_t *type)
{
    (void)dev;
    (void)user_data;

    if (strstr(resp_line, "OK")) {
        *type = AT_RESP_OK;
        return true; /* 停止处理 */
    } else if (strstr(resp_line, "ERROR")) {
        *type = AT_RESP_ERROR;
        return true;
    }

    return false; /* 继续处理下一行 */
}

/* 初始化AT客户端 */
at_client_t *at_client_init(void)
{
    memset(&g_at_client, 0, sizeof(g_at_client));
    return &g_at_client;
}

/* 注册AT设备 */
at_device_t *at_device_register(const char *name, at_read_byte_t read_byte,
                                at_write_t write_data,
                                at_tick_t (*get_tick)(void), void *user_data)
{
    if (g_at_client.device_count >= AT_MAX_DEVICES) {
        return NULL;
    }

    at_device_t *dev = (at_device_t *)malloc(sizeof(at_device_t));
    if (!dev) {
        return NULL;
    }

    memset(dev, 0, sizeof(at_device_t));
    dev->id = g_at_client.device_count;
    strncpy(dev->name, name, sizeof(dev->name) - 1);
    dev->read_byte    = read_byte;
    dev->write_data   = write_data;
    dev->get_tick     = get_tick;
    dev->user_data    = user_data;
    dev->timeout_ms   = AT_DEFAULT_TIMEOUT;
    dev->resp_handler = default_resp_handler;

    AT_MUTEX_INIT(dev->mutex);

    g_at_client.devices[g_at_client.device_count++] = dev;

    /* 第一个设备设为默认设备 */
    if (g_at_client.device_count == 1) {
        g_at_client.default_device = 0;
    }

    return dev;
}

/* 发送AT命令 */
at_resp_type_t at_send_command(at_device_t *dev, const char *cmd,
                               at_resp_handler_t handler, void *user_data,
                               uint32_t timeout)
{
    if (!dev || !cmd) {
        return AT_RESP_ERROR;
    }

    AT_MUTEX_LOCK(dev->mutex);

    if (dev->is_busy) {
        AT_MUTEX_UNLOCK(dev->mutex);
        return AT_RESP_ERROR;
    }

    dev->is_busy        = true;
    dev->resp_handler   = handler ? handler : default_resp_handler;
    dev->resp_user_data = user_data;
    
    /* 使用传入的超时值，如果为0则使用设备默认超时 */
    dev->timeout_ms = (timeout > 0) ? timeout : AT_DEFAULT_TIMEOUT;
    
    at_reset_resp_buffer(&dev->resp_buf);

    /* 发送命令 */
    uint32_t cmd_len = strlen(cmd);
    dev->write_data(dev, (uint8_t *)cmd, cmd_len);
    dev->write_data(dev, (uint8_t *)"\r\n", 2);
    dev->tx_count += cmd_len + 2;

    /* 等待并处理响应（带超时） */
    at_resp_type_t result = at_process_response(dev);

    dev->is_busy = false;
    AT_MUTEX_UNLOCK(dev->mutex);

    return result;
}

/* 处理响应 */
static int at_process_response(at_device_t *dev)
{
    at_tick_t start_time = dev->get_tick();
    at_tick_t timeout    = dev->timeout_ms;
    char line_buf[256];
    at_resp_type_t resp_type = AT_RESP_NONE;

    while (at_get_time_diff(start_time, dev->get_tick()) < timeout) {
        int line_len = at_readline(dev, line_buf, sizeof(line_buf));

        if (line_len > 0) {
            dev->rx_count += line_len;

            /* 调用响应处理器 */
            if (dev->resp_handler) {
                bool stop = dev->resp_handler(
                    dev, line_buf, dev->resp_user_data, &resp_type);
                if (stop) {
                    return resp_type;
                }
            }

            /* 保存到响应缓冲区 */
            if (dev->resp_buf.length + line_len < AT_MAX_RESP_LEN) {
                strcat(dev->resp_buf.buffer, line_buf);
                strcat(dev->resp_buf.buffer, "\n");
                dev->resp_buf.length += line_len + 1;
            } else {
                dev->resp_buf.overflow = true;
            }
        } else if (line_len == 0) {
            /* 无数据，短暂延时 */
            AT_DELAY_MS(1);
        }
    }

    return AT_RESP_TIMEOUT;
}

/* 读取一行 */
int at_readline(at_device_t *dev, char *buffer, uint32_t size)
{
    if (!dev || !buffer || size == 0) {
        return -1;
    }

    uint32_t idx = 0;
    bool got_cr  = false;

    while (idx < size - 1) {
        int ch = dev->read_byte(dev);

        if (ch < 0) {
            /* 无数据 */
            if (idx > 0 && got_cr) {
                buffer[idx] = '\0';
                return idx;
            }
            return 0;
        }

        if (ch == '\r') {
            got_cr = true;
            continue;
        }

        if (ch == '\n') {
            if (got_cr || idx > 0) {
                buffer[idx] = '\0';
                return idx;
            }
            continue;
        }

        buffer[idx++] = (char)ch;
        got_cr        = false;
    }

    buffer[size - 1] = '\0';
    return size - 1;
}

/* 发送原始数据 */
int at_send_raw(at_device_t *dev, const uint8_t *data, uint32_t len)
{
    if (!dev || !data || len == 0) {
        return -1;
    }

    AT_MUTEX_LOCK(dev->mutex);
    dev->write_data(dev, data, len);
    dev->tx_count += len;
    AT_MUTEX_UNLOCK(dev->mutex);

    return len;
}

/* 等待特定响应 */
bool at_expect(at_device_t *dev, const char *expect, uint32_t timeout)
{
    at_tick_t start = dev->get_tick();
    char buffer[256];

    while (at_get_time_diff(start, dev->get_tick()) < timeout) {
        int len = at_readline(dev, buffer, sizeof(buffer));
        if (len > 0 && strstr(buffer, expect)) {
            return true;
        }
        AT_DELAY_MS(1);
    }

    return false;
}

/* 工具函数 */
static at_tick_t at_get_time_diff(at_tick_t start, at_tick_t current)
{
    if (current >= start) {
        return current - start;
    } else {
        /* 处理tick溢出 */
        return (0xFFFFFFFF - start) + current + 1;
    }
}

static void at_reset_resp_buffer(at_resp_buffer_t *buf)
{
    memset(buf->buffer, 0, AT_MAX_RESP_LEN);
    buf->length   = 0;
    buf->overflow = false;
}

/* 简化的API函数 */
at_resp_type_t at_send_cmd(const char *cmd, at_resp_handler_t handler,
                           void *user_data, uint32_t timeout)
{
    if (g_at_client.device_count == 0) {
        return AT_RESP_ERROR;
    }

    at_device_t *dev = g_at_client.devices[g_at_client.default_device];
    return at_send_command(dev, cmd, handler, user_data, timeout);
}

void at_get_stats(at_device_t *dev, uint32_t *tx, uint32_t *rx, uint32_t *err)
{
    if (dev) {
        if (tx)
            *tx = dev->tx_count;
        if (rx)
            *rx = dev->rx_count;
        if (err)
            *err = dev->error_count;
    }
}