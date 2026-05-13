/**
 * @file xy_mux_spi.c
 * @brief MUX SPI Interface Implementation
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_mux_spi.h"
#include "xy_mux.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ==================== SPI 操作接口 ==================== */

/**
 * @brief SPI 初始化回调
 */
static int32_t xy_mux_spi_init(uint8_t channel, const void *config)
{
    xy_log_d("SPI channel %d initialized\n", channel);
    return XY_MUX_OK;
}

/**
 * @brief SPI 反初始化回调
 */
static int32_t xy_mux_spi_deinit(uint8_t channel)
{
    xy_log_d("SPI channel %d deinitialized\n", channel);
    return XY_MUX_OK;
}

/**
 * @brief SPI 读取回调
 */
static int32_t xy_mux_spi_read(uint8_t channel, void *data, size_t len)
{
    if (!data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    xy_log_w("SPI read should use xy_mux_spi_read()\n");
    return XY_MUX_ERROR_NOT_SUPPORTED;
}

/**
 * @brief SPI 写入回调
 */
static int32_t xy_mux_spi_write(uint8_t channel, const void *data, size_t len)
{
    if (!data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    xy_log_w("SPI write should use xy_mux_spi_write()\n");
    return XY_MUX_ERROR_NOT_SUPPORTED;
}

/**
 * @brief SPI 控制回调
 */
static int32_t xy_mux_spi_ioctl(uint8_t channel, int cmd, void *arg)
{
    xy_log_d("SPI channel %d ioctl: cmd=%d\n", channel, cmd);
    return XY_MUX_OK;
}

/* SPI 操作接口 */
static const xy_mux_ops_t g_spi_ops = {
    .init = xy_mux_spi_init,
    .deinit = xy_mux_spi_deinit,
    .read = xy_mux_spi_read,
    .write = xy_mux_spi_write,
    .ioctl = xy_mux_spi_ioctl,
};

/* ==================== SPI API 实现 ==================== */

int32_t xy_mux_spi_register(xy_mux_manager_t *mgr, uint8_t channel,
                            const xy_mux_ops_t *ops, void *user_data)
{
    if (!mgr) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    /* 合并操作接口 */
    static xy_mux_ops_t combined_ops;
    memcpy(&combined_ops, &g_spi_ops, sizeof(xy_mux_ops_t));

    if (ops) {
        if (ops->init) combined_ops.init = ops->init;
        if (ops->deinit) combined_ops.deinit = ops->deinit;
        if (ops->read) combined_ops.read = ops->read;
        if (ops->write) combined_ops.write = ops->write;
        if (ops->ioctl) combined_ops.ioctl = ops->ioctl;
    }

    xy_log_i("Registering SPI channel %d\n", channel);
    return xy_mux_register(mgr, XY_MUX_TYPE_SPI, channel, &combined_ops, user_data);
}

int32_t xy_mux_spi_config(xy_mux_manager_t *mgr, uint8_t channel,
                          const xy_mux_spi_config_t *config)
{
    if (!mgr || !config) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    return xy_mux_ioctl(mgr, XY_MUX_TYPE_SPI, channel,
                        XY_MUX_SPI_CMD_SET_CONFIG, (void *)config);
}

int32_t xy_mux_spi_transfer(xy_mux_manager_t *mgr, uint8_t channel,
                            const void *tx_data, void *rx_data, size_t len)
{
    if (!mgr || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    /* SPI 全双工传输: 同时收发 */
    /* 构建数据包: 长度(2字节) + 数据 */
    uint8_t header[2];
    header[0] = (uint8_t)(len & 0xFF);
    header[1] = (uint8_t)((len >> 8) & 0xFF);

    /* 发送数据 */
    int32_t ret = xy_mux_write(mgr, XY_MUX_TYPE_SPI, channel, header, 2);
    if (ret < 0) {
        return ret;
    }

    ret = xy_mux_write(mgr, XY_MUX_TYPE_SPI, channel, tx_data, len);
    if (ret < 0) {
        return ret;
    }

    /* 接收数据 */
    if (rx_data) {
        ret = xy_mux_read(mgr, XY_MUX_TYPE_SPI, channel, rx_data, len);
        if (ret < 0) {
            return ret;
        }
    }

    return len;
}

int32_t xy_mux_spi_read(xy_mux_manager_t *mgr, uint8_t channel,
                        void *data, size_t len)
{
    if (!mgr || !data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    /* SPI 读操作: 发送空数据以产生时钟信号 */
    static uint8_t null_buf[256];
    if (len > sizeof(null_buf)) {
        return XY_MUX_ERROR_NO_MEMORY;
    }

    return xy_mux_spi_transfer(mgr, channel, null_buf, data, len);
}

int32_t xy_mux_spi_write(xy_mux_manager_t *mgr, uint8_t channel,
                         const void *data, size_t len)
{
    if (!mgr || !data || len == 0) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    /* SPI 写操作: 只发送不接收 */
    uint8_t header[2];
    header[0] = (uint8_t)(len & 0xFF);
    header[1] = (uint8_t)((len >> 8) & 0xFF);

    int32_t ret = xy_mux_write(mgr, XY_MUX_TYPE_SPI, channel, header, 2);
    if (ret < 0) {
        return ret;
    }

    return xy_mux_write(mgr, XY_MUX_TYPE_SPI, channel, data, len);
}
