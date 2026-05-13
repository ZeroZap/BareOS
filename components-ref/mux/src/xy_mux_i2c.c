/**
 * @file xy_mux_i2c.c
 * @brief MUX I2C Interface Implementation
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_mux_i2c.h"
#include "xy_mux.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ==================== I2C 操作接口 ==================== */

/**
 * @brief I2C 初始化回调
 */
static int32_t xy_mux_i2c_init(uint8_t channel, const void *config)
{
    xy_log_d("I2C channel %d initialized\n", channel);
    return XY_MUX_OK;
}

/**
 * @brief I2C 反初始化回调
 */
static int32_t xy_mux_i2c_deinit(uint8_t channel)
{
    xy_log_d("I2C channel %d deinitialized\n", channel);
    return XY_MUX_OK;
}

/**
 * @brief I2C 读取回调
 */
static int32_t xy_mux_i2c_read(uint8_t channel, void *data, size_t len)
{
    if (!data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    xy_log_w("I2C read should use xy_mux_i2c_read()\n");
    return XY_MUX_ERROR_NOT_SUPPORTED;
}

/**
 * @brief I2C 写入回调
 */
static int32_t xy_mux_i2c_write(uint8_t channel, const void *data, size_t len)
{
    if (!data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    xy_log_w("I2C write should use xy_mux_i2c_write()\n");
    return XY_MUX_ERROR_NOT_SUPPORTED;
}

/**
 * @brief I2C 控制回调
 */
static int32_t xy_mux_i2c_ioctl(uint8_t channel, int cmd, void *arg)
{
    xy_log_d("I2C channel %d ioctl: cmd=%d\n", channel, cmd);
    return XY_MUX_OK;
}

/* I2C 操作接口 */
static const xy_mux_ops_t g_i2c_ops = {
    .init = xy_mux_i2c_init,
    .deinit = xy_mux_i2c_deinit,
    .read = xy_mux_i2c_read,
    .write = xy_mux_i2c_write,
    .ioctl = xy_mux_i2c_ioctl,
};

/* ==================== I2C API 实现 ==================== */

int32_t xy_mux_i2c_register(xy_mux_manager_t *mgr, uint8_t channel,
                            const xy_mux_ops_t *ops, void *user_data)
{
    if (!mgr) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    /* 合并操作接口 */
    static xy_mux_ops_t combined_ops;
    memcpy(&combined_ops, &g_i2c_ops, sizeof(xy_mux_ops_t));

    if (ops) {
        if (ops->init) combined_ops.init = ops->init;
        if (ops->deinit) combined_ops.deinit = ops->deinit;
        if (ops->read) combined_ops.read = ops->read;
        if (ops->write) combined_ops.write = ops->write;
        if (ops->ioctl) combined_ops.ioctl = ops->ioctl;
    }

    xy_log_i("Registering I2C channel %d\n", channel);
    return xy_mux_register(mgr, XY_MUX_TYPE_I2C, channel, &combined_ops, user_data);
}

int32_t xy_mux_i2c_config(xy_mux_manager_t *mgr, uint8_t channel,
                          const xy_mux_i2c_config_t *config)
{
    if (!mgr || !config) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    return xy_mux_ioctl(mgr, XY_MUX_TYPE_I2C, channel,
                        XY_MUX_I2C_CMD_SET_CONFIG, (void *)config);
}

int32_t xy_mux_i2c_transfer(xy_mux_manager_t *mgr, uint8_t channel,
                            xy_mux_i2c_msg_t *msgs, int count)
{
    if (!mgr || !msgs || count <= 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    int32_t total_len = 0;

    for (int i = 0; i < count; i++) {
        xy_mux_i2c_msg_t *msg = &msgs[i];

        if (msg->flags & XY_MUX_I2C_M_RD) {
            /* 读操作 */
            int32_t ret = xy_mux_read(mgr, XY_MUX_TYPE_I2C, channel,
                                      msg->buf, msg->len);
            if (ret < 0) {
                return ret;
            }
            total_len += ret;
        } else {
            /* 写操作 */
            /* 构建数据包: 地址(2字节) + 数据 */
            uint8_t packet[256];
            if (msg->len + 2 > sizeof(packet)) {
                return XY_MUX_ERROR_NO_MEMORY;
            }

            packet[0] = (uint8_t)(msg->addr & 0xFF);
            packet[1] = (uint8_t)((msg->addr >> 8) & 0xFF);
            memcpy(&packet[2], msg->buf, msg->len);

            int32_t ret = xy_mux_write(mgr, XY_MUX_TYPE_I2C, channel,
                                       packet, msg->len + 2);
            if (ret < 0) {
                return ret;
            }
            total_len += ret;
        }
    }

    return total_len;
}

int32_t xy_mux_i2c_read(xy_mux_manager_t *mgr, uint8_t channel,
                        uint16_t addr, void *data, size_t len)
{
    if (!mgr || !data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    /* 构建读请求包 */
    uint8_t req_packet[3];
    req_packet[0] = (uint8_t)(addr & 0xFF);
    req_packet[1] = (uint8_t)((addr >> 8) & 0xFF);
    req_packet[2] = (uint8_t)len;

    int32_t ret = xy_mux_write(mgr, XY_MUX_TYPE_I2C, channel, req_packet, 3);
    if (ret < 0) {
        return ret;
    }

    /* 读取响应数据 */
    ret = xy_mux_read(mgr, XY_MUX_TYPE_I2C, channel, data, len);
    return ret;
}

int32_t xy_mux_i2c_write(xy_mux_manager_t *mgr, uint8_t channel,
                         uint16_t addr, const void *data, size_t len)
{
    if (!mgr || !data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    /* 构建写请求包: 地址(2字节) + 数据 */
    uint8_t *packet = (uint8_t *)malloc(len + 2);
    if (!packet) {
        return XY_MUX_ERROR_NO_MEMORY;
    }

    packet[0] = (uint8_t)(addr & 0xFF);
    packet[1] = (uint8_t)((addr >> 8) & 0xFF);
    memcpy(&packet[2], data, len);

    int32_t ret = xy_mux_write(mgr, XY_MUX_TYPE_I2C, channel, packet, len + 2);

    free(packet);
    return ret;
}

int32_t xy_mux_i2c_scan(xy_mux_manager_t *mgr, uint8_t channel,
                        uint16_t *addrs, size_t max_count)
{
    if (!mgr || !addrs || max_count == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    /* 发送扫描命令 */
    int32_t ret = xy_mux_ioctl(mgr, XY_MUX_TYPE_I2C, channel,
                               XY_MUX_I2C_CMD_SCAN, addrs);
    if (ret < 0) {
        return ret;
    }

    /* 如果驱动不支持扫描，则返回错误 */
    return XY_MUX_ERROR_NOT_SUPPORTED;
}
