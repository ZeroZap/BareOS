/**
 * @file xy_mux_uart.c
 * @brief MUX UART Interface Implementation
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_mux_uart.h"
#include "xy_mux.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ==================== UART 操作接口 ==================== */

/**
 * @brief UART 初始化回调
 */
static int32_t xy_mux_uart_init(uint8_t channel, const void *config)
{
    xy_log_d("UART channel %d initialized\n", channel);
    return XY_MUX_OK;
}

/**
 * @brief UART 反初始化回调
 */
static int32_t xy_mux_uart_deinit(uint8_t channel)
{
    xy_log_d("UART channel %d deinitialized\n", channel);
    return XY_MUX_OK;
}

/**
 * @brief UART 读取回调
 */
static int32_t xy_mux_uart_read(uint8_t channel, void *data, size_t len)
{
    if (!data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    xy_log_w("UART read should use xy_mux_uart_read()\n");
    return XY_MUX_ERROR_NOT_SUPPORTED;
}

/**
 * @brief UART 写入回调
 */
static int32_t xy_mux_uart_write(uint8_t channel, const void *data, size_t len)
{
    if (!data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    xy_log_w("UART write should use xy_mux_uart_write()\n");
    return XY_MUX_ERROR_NOT_SUPPORTED;
}

/**
 * @brief UART 控制回调
 */
static int32_t xy_mux_uart_ioctl(uint8_t channel, int cmd, void *arg)
{
    xy_log_d("UART channel %d ioctl: cmd=%d\n", channel, cmd);
    return XY_MUX_OK;
}

/* UART 操作接口 */
static const xy_mux_ops_t g_uart_ops = {
    .init = xy_mux_uart_init,
    .deinit = xy_mux_uart_deinit,
    .read = xy_mux_uart_read,
    .write = xy_mux_uart_write,
    .ioctl = xy_mux_uart_ioctl,
};

/* ==================== UART API 实现 ==================== */

int32_t xy_mux_uart_register(xy_mux_manager_t *mgr, uint8_t channel,
                             const xy_mux_ops_t *ops, void *user_data)
{
    if (!mgr) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    /* 合并操作接口 */
    static xy_mux_ops_t combined_ops;
    memcpy(&combined_ops, &g_uart_ops, sizeof(xy_mux_ops_t));

    if (ops) {
        if (ops->init) combined_ops.init = ops->init;
        if (ops->deinit) combined_ops.deinit = ops->deinit;
        if (ops->read) combined_ops.read = ops->read;
        if (ops->write) combined_ops.write = ops->write;
        if (ops->ioctl) combined_ops.ioctl = ops->ioctl;
    }

    xy_log_i("Registering UART channel %d\n", channel);
    return xy_mux_register(mgr, XY_MUX_TYPE_UART, channel, &combined_ops, user_data);
}

int32_t xy_mux_uart_config(xy_mux_manager_t *mgr, uint8_t channel,
                           const xy_mux_uart_config_t *config)
{
    if (!mgr || !config) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    return xy_mux_ioctl(mgr, XY_MUX_TYPE_UART, channel,
                        XY_MUX_UART_CMD_SET_CONFIG, (void *)config);
}

int32_t xy_mux_uart_read(xy_mux_manager_t *mgr, uint8_t channel,
                         void *data, size_t len, uint32_t timeout)
{
    if (!mgr || !data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    /* UART 读取操作
     * 协议: 先发送读请求 (包含长度和超时), 然后读取响应数据
     */
    uint8_t req[6];
    req[0] = (uint8_t)(len & 0xFF);
    req[1] = (uint8_t)((len >> 8) & 0xFF);
    req[2] = (uint8_t)((len >> 16) & 0xFF);
    req[3] = (uint8_t)((len >> 24) & 0xFF);
    req[4] = (uint8_t)(timeout & 0xFF);
    req[5] = (uint8_t)((timeout >> 8) & 0xFF);

    int32_t ret = xy_mux_write(mgr, XY_MUX_TYPE_UART, channel, req, sizeof(req));
    if (ret < 0) {
        return ret;
    }

    ret = xy_mux_read(mgr, XY_MUX_TYPE_UART, channel, data, len);
    return ret;
}

int32_t xy_mux_uart_write(xy_mux_manager_t *mgr, uint8_t channel,
                          const void *data, size_t len, uint32_t timeout)
{
    if (!mgr || !data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    /* UART 写入操作
     * 协议: 发送写入请求 (包含长度和超时) + 数据
     */
    uint8_t header[6];
    header[0] = (uint8_t)(len & 0xFF);
    header[1] = (uint8_t)((len >> 8) & 0xFF);
    header[2] = (uint8_t)((len >> 16) & 0xFF);
    header[3] = (uint8_t)((len >> 24) & 0xFF);
    header[4] = (uint8_t)(timeout & 0xFF);
    header[5] = (uint8_t)((timeout >> 8) & 0xFF);

    int32_t ret = xy_mux_write(mgr, XY_MUX_TYPE_UART, channel, header, sizeof(header));
    if (ret < 0) {
        return ret;
    }

    return xy_mux_write(mgr, XY_MUX_TYPE_UART, channel, data, len);
}
