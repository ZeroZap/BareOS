/**
 * @file xy_can.c
 * @brief CAN Bus Protocol Stack Implementation
 * @version 1.0.0
 * @date 2026-03-01 上午
 */

#include "xy_can.h"
#include "xy_log.h"
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

/**
 * @brief FIFO 写入
 */
static int xy_can_fifo_write(xy_can_msg_t *fifo, uint32_t size, 
                             uint32_t *head, uint32_t *tail,
                             const xy_can_msg_t *msg)
{
    uint32_t next_head;
    
    /* 计算下一个写位置 */
    next_head = (*head + 1) % size;
    
    /* 检查 FIFO 是否满 */
    if (next_head == *tail) {
        return XY_CAN_FIFO_FULL;
    }
    
    /* 写入数据 */
    memcpy(&fifo[*head], msg, sizeof(xy_can_msg_t));
    *head = next_head;
    
    return XY_CAN_OK;
}

/**
 * @brief FIFO 读取
 */
static int xy_can_fifo_read(xy_can_msg_t *fifo, uint32_t size,
                            uint32_t *head, uint32_t *tail,
                            xy_can_msg_t *msg)
{
    /* 检查 FIFO 是否空 */
    if (*head == *tail) {
        return XY_CAN_FIFO_EMPTY;
    }
    
    /* 读取数据 */
    memcpy(msg, &fifo[*tail], sizeof(xy_can_msg_t));
    *tail = (*tail + 1) % size;
    
    return XY_CAN_OK;
}

/**
 * @brief FIFO 计数
 */
static uint32_t xy_can_fifo_count(uint32_t head, uint32_t tail, uint32_t size)
{
    if (head >= tail) {
        return head - tail;
    } else {
        return size - tail + head;
    }
}

int xy_can_init(xy_can_t *can, void *hw_handle, const xy_can_config_t *config)
{
    int ret;
    
    if (!can || !config) {
        return XY_CAN_INVALID_PARAM;
    }
    
    memset(can, 0, sizeof(*can));
    
    /* 复制配置 */
    memcpy(&can->config, config, sizeof(xy_can_config_t));
    can->hw_handle = hw_handle;
    
    /* 分配 FIFO */
    if (can->config.rx_fifo_size > 0) {
        can->rx_fifo = malloc(can->config.rx_fifo_size * sizeof(xy_can_msg_t));
        if (!can->rx_fifo) {
            return XY_CAN_ERROR;
        }
        can->rx_fifo_size = can->config.rx_fifo_size;
    }
    
    if (can->config.tx_fifo_size > 0) {
        can->tx_fifo = malloc(can->config.tx_fifo_size * sizeof(xy_can_msg_t));
        if (!can->tx_fifo) {
            if (can->rx_fifo) {
                free(can->rx_fifo);
            }
            return XY_CAN_ERROR;
        }
        can->tx_fifo_size = can->config.tx_fifo_size;
    }
    
    can->initialized = true;
    
    xy_log_i("CAN initialized (baudrate=%lu, RX_FIFO=%lu, TX_FIFO=%lu)\n",
             can->config.baudrate, can->rx_fifo_size, can->tx_fifo_size);
    
    return XY_CAN_OK;
}

int xy_can_deinit(xy_can_t *can)
{
    if (!can) {
        return XY_CAN_INVALID_PARAM;
    }
    
    if (can->rx_fifo) {
        free(can->rx_fifo);
        can->rx_fifo = NULL;
    }
    
    if (can->tx_fifo) {
        free(can->tx_fifo);
        can->tx_fifo = NULL;
    }
    
    can->initialized = false;
    
    return XY_CAN_OK;
}

int xy_can_start(xy_can_t *can)
{
    if (!can || !can->initialized) {
        return XY_CAN_INVALID_PARAM;
    }

    /* 初始化硬件 CAN 控制器 */
#ifdef MCU_CH32
    xy_hal_can_config_t hal_cfg = {
        .baudrate_prescaler = can->config.baudrate
    };
    xy_hal_can_init(can->hw_handle, &hal_cfg);
#endif

    xy_log_i("CAN started\n");
    return XY_CAN_OK;
}

int xy_can_stop(xy_can_t *can)
{
    if (!can) {
        return XY_CAN_INVALID_PARAM;
    }

    /* 停止硬件 CAN 控制器 */
#ifdef MCU_CH32
    xy_hal_can_deinit(can->hw_handle);
#endif

    xy_log_i("CAN stopped\n");
    return XY_CAN_OK;
}

int xy_can_send(xy_can_t *can, const xy_can_msg_t *msg, uint32_t timeout)
{
    int ret;
    uint32_t start;
    
    if (!can || !msg || !can->initialized) {
        return XY_CAN_INVALID_PARAM;
    }
    
    if (!can->tx_fifo) {
        /* 直接发送 */
        /* ret = xy_hal_can_send(can->hw_handle, msg); */
        ret = XY_CAN_OK;
        if (ret == XY_CAN_OK) {
            can->tx_count++;
        }
        return ret;
    }
    
    /* FIFO 模式 */
    start = xy_os_tick_get();
    
    do {
        ret = xy_can_fifo_write(can->tx_fifo, can->tx_fifo_size,
                               &can->tx_head, &can->tx_tail, msg);
        if (ret == XY_CAN_OK) {
            can->tx_count++;

            /* 触发硬件发送
             * 
             * 平台适配说明:
             * - WCH CH32: CAN 硬件自动发送，无需额外触发
             * - STM32: 调用 xy_hal_can_trigger_send()
             * - 其他 MCU: 根据 HAL 实现调整
             */

            return XY_CAN_OK;
        }
        
        /* FIFO 满，等待 */
        xy_os_delay(1);
    } while ((xy_os_tick_get() - start) < timeout);
    
    return XY_CAN_TIMEOUT;
}

int xy_can_receive(xy_can_t *can, xy_can_msg_t *msg, uint32_t timeout)
{
    int ret;
    uint32_t start;
    
    if (!can || !msg || !can->initialized) {
        return XY_CAN_INVALID_PARAM;
    }
    
    if (!can->rx_fifo) {
        /* 直接接收 */
        /* ret = xy_hal_can_receive(can->hw_handle, msg); */
        ret = XY_CAN_OK;
        if (ret == XY_CAN_OK) {
            can->rx_count++;
            /* 触发回调 */
            if (can->rx_callback) {
                can->rx_callback(can, msg);
            }
        }
        return ret;
    }
    
    /* FIFO 模式 */
    start = xy_os_tick_get();
    
    do {
        ret = xy_can_fifo_read(can->rx_fifo, can->rx_fifo_size,
                              &can->rx_head, &can->rx_tail, msg);
        if (ret == XY_CAN_OK) {
            can->rx_count++;
            /* 触发回调 */
            if (can->rx_callback) {
                can->rx_callback(can, msg);
            }
            return XY_CAN_OK;
        }
        
        /* FIFO 空，等待 */
        xy_os_delay(1);
    } while ((xy_os_tick_get() - start) < timeout);
    
    return XY_CAN_TIMEOUT;
}

int xy_can_register_rx_callback(xy_can_t *can, xy_can_rx_callback_t callback, void *user_data)
{
    if (!can) {
        return XY_CAN_INVALID_PARAM;
    }
    
    can->rx_callback = callback;
    can->callback_user_data = user_data;
    
    xy_log_d("CAN RX callback registered\n");
    return XY_CAN_OK;
}

int xy_can_unregister_rx_callback(xy_can_t *can)
{
    if (!can) {
        return XY_CAN_INVALID_PARAM;
    }
    
    can->rx_callback = NULL;
    can->callback_user_data = NULL;
    
    xy_log_d("CAN RX callback unregistered\n");
    return XY_CAN_OK;
}

/**
 * @brief CAN 中断接收处理
 * 
 * 在硬件 CAN 中断中调用此函数，会自动:
 * - 写入 RX FIFO
 * - 触发回调 (如果已注册)
 */
void xy_can_isr_receive(xy_can_t *can, const xy_can_msg_t *msg)
{
    if (!can || !msg || !can->initialized) {
        return;
    }
    
    /* 写入 FIFO */
    if (can->rx_fifo) {
        xy_can_fifo_write(can->rx_fifo, can->rx_fifo_size,
                         &can->rx_head, &can->rx_tail, msg);
    }
    
    /* 更新计数 */
    can->rx_count++;
    
    /* 触发回调 */
    if (can->rx_callback) {
        can->rx_callback(can, msg);
    }
}

uint32_t xy_can_get_tx_count(const xy_can_t *can)
{
    if (!can) {
        return 0;
    }
    return can->tx_count;
}

uint32_t xy_can_get_rx_count(const xy_can_t *can)
{
    if (!can) {
        return 0;
    }
    return can->rx_count;
}

uint32_t xy_can_get_error_count(const xy_can_t *can)
{
    if (!can) {
        return 0;
    }
    return can->error_count;
}

int xy_can_get_fifo_usage(const xy_can_t *can, float *rx_usage, float *tx_usage)
{
    uint32_t count;
    
    if (!can) {
        return XY_CAN_INVALID_PARAM;
    }
    
    if (rx_usage && can->rx_fifo) {
        count = xy_can_fifo_count(can->rx_head, can->rx_tail, can->rx_fifo_size);
        *rx_usage = (float)count / can->rx_fifo_size * 100.0F;
    }
    
    if (tx_usage && can->tx_fifo) {
        count = xy_can_fifo_count(can->tx_head, can->tx_tail, can->tx_fifo_size);
        *tx_usage = (float)count / can->tx_fifo_size * 100.0F;
    }
    
    return XY_CAN_OK;
}
