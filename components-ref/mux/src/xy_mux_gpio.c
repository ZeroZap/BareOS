/**
 * @file xy_mux_gpio.c
 * @brief MUX GPIO Interface Implementation
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_mux_gpio.h"
#include "xy_mux.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/* ==================== GPIO 操作接口 ==================== */

/**
 * @brief GPIO 初始化回调
 */
static int32_t xy_mux_gpio_init(uint8_t channel, const void *config)
{
    xy_log_d("GPIO channel %d initialized\n", channel);
    return XY_MUX_OK;
}

/**
 * @brief GPIO 反初始化回调
 */
static int32_t xy_mux_gpio_deinit(uint8_t channel)
{
    xy_log_d("GPIO channel %d deinitialized\n", channel);
    return XY_MUX_OK;
}

/**
 * @brief GPIO 读取回调
 */
static int32_t xy_mux_gpio_read(uint8_t channel, void *data, size_t len)
{
    if (!data || len < sizeof(uint8_t)) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    /* 读取操作需要通过 ioctl 实现，这里返回不支持 */
    xy_log_w("GPIO read not directly supported, use xy_mux_gpio_read()\n");
    return XY_MUX_ERROR_NOT_SUPPORTED;
}

/**
 * @brief GPIO 写入回调
 */
static int32_t xy_mux_gpio_write(uint8_t channel, const void *data, size_t len)
{
    if (!data || len < sizeof(uint8_t)) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    /* 写入操作需要通过 ioctl 实现，这里返回不支持 */
    xy_log_w("GPIO write not directly supported, use xy_mux_gpio_write()\n");
    return XY_MUX_ERROR_NOT_SUPPORTED;
}

/**
 * @brief GPIO 控制回调
 */
static int32_t xy_mux_gpio_ioctl(uint8_t channel, int cmd, void *arg)
{
    xy_log_d("GPIO channel %d ioctl: cmd=%d\n", channel, cmd);
    /* 具体操作由设备驱动实现 */
    return XY_MUX_OK;
}

/* GPIO 操作接口 */
static const xy_mux_ops_t g_gpio_ops = {
    .init = xy_mux_gpio_init,
    .deinit = xy_mux_gpio_deinit,
    .read = xy_mux_gpio_read,
    .write = xy_mux_gpio_write,
    .ioctl = xy_mux_gpio_ioctl,
};

/* ==================== GPIO API 实现 ==================== */

int32_t xy_mux_gpio_register(xy_mux_manager_t *mgr, uint8_t channel,
                             const xy_mux_ops_t *ops, void *user_data)
{
    if (!mgr) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    /* 合并操作接口 */
    static xy_mux_ops_t combined_ops;
    memcpy(&combined_ops, &g_gpio_ops, sizeof(xy_mux_ops_t));

    if (ops) {
        if (ops->init) combined_ops.init = ops->init;
        if (ops->deinit) combined_ops.deinit = ops->deinit;
        if (ops->read) combined_ops.read = ops->read;
        if (ops->write) combined_ops.write = ops->write;
        if (ops->ioctl) combined_ops.ioctl = ops->ioctl;
    }

    xy_log_i("Registering GPIO channel %d\n", channel);
    return xy_mux_register(mgr, XY_MUX_TYPE_GPIO, channel, &combined_ops, user_data);
}

int32_t xy_mux_gpio_config(xy_mux_manager_t *mgr, uint8_t channel,
                           const xy_mux_gpio_config_t *config)
{
    if (!mgr || !config) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    return xy_mux_ioctl(mgr, XY_MUX_TYPE_GPIO, channel,
                        XY_MUX_GPIO_CMD_SET_CONFIG, (void *)config);
}

int32_t xy_mux_gpio_read(xy_mux_manager_t *mgr, uint8_t channel)
{
    if (!mgr) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    uint8_t level = 0;
    int32_t ret = xy_mux_ioctl(mgr, XY_MUX_TYPE_GPIO, channel,
                               XY_MUX_GPIO_CMD_GET_LEVEL, &level);
    if (ret == XY_MUX_OK) {
        return level;
    }

    /* 尝试通过读接口获取 */
    ret = xy_mux_read(mgr, XY_MUX_TYPE_GPIO, channel, &level, sizeof(level));
    if (ret > 0) {
        return level;
    }

    return ret;
}

int32_t xy_mux_gpio_write(xy_mux_manager_t *mgr, uint8_t channel,
                          xy_mux_gpio_level_t level)
{
    if (!mgr) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    uint8_t level_val = (uint8_t)level;
    return xy_mux_ioctl(mgr, XY_MUX_TYPE_GPIO, channel,
                        XY_MUX_GPIO_CMD_SET_LEVEL, &level_val);
}

int32_t xy_mux_gpio_toggle(xy_mux_manager_t *mgr, uint8_t channel)
{
    if (!mgr) {
        return XY_MUX_ERROR_INVALID_PARAM;
    }

    return xy_mux_ioctl(mgr, XY_MUX_TYPE_GPIO, channel,
                        XY_MUX_GPIO_CMD_TOGGLE, NULL);
}
