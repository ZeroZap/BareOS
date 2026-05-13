/**
 * @file xy_hal_uart_device.c
 * @brief STM32U5 UART Device Implementation - Unified HAL API
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 实现统一的 UART 设备 API，基于 STM32U5 HAL 库
 * @note 支持轮询/中断/DMA 三种传输模式
 */

#include "../inc/xy_hal_uart_dev.h"
#include "../inc/xy_hal_uart_types.h"
#include <string.h>

/* STM32U5 HAL 头文件 */
#include "stm32u5xx_hal.h"

/* ==================== Private Definitions ==================== */

#define STM32U5_UART_INSTANCE_COUNT  (8)  /* USART1-3, UART4-5, LPUART1, etc. */

/* ==================== Private Types ==================== */

/**
 * @brief STM32U5 UART 设备私有数据
 */
typedef struct {
    UART_HandleTypeDef *huart;     /* STM32 HAL UART 句柄 */
    uint32_t instance;             /* USART 实例编号 */
    xy_hal_uart_config_t config;   /* 当前配置 */
    xy_hal_uart_callback_t callback; /* 异步回调 */
    void *callback_arg;            /* 回调参数 */
    xy_hal_uart_stats_t stats;     /* 统计信息 */
    uint8_t tx_busy;               /* 发送忙标志 */
    uint8_t rx_busy;               /* 接收忙标志 */
} stm32u5_uart_data_t;

/* ==================== Private Variables ==================== */

/* UART 设备实例数组 */
static stm32u5_uart_data_t uart_devices[STM32U5_UART_INSTANCE_COUNT] = {0};

/* UART 实例名称映射 */
static const char *const uart_names[STM32U5_UART_INSTANCE_COUNT] = {
    "USART1", "USART2", "USART3", "UART4", "UART5", 
    "LPUART1", "LPUART2", "LPUART3"
};

/* UART 实例句柄 (外部定义) */
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart4;
extern UART_HandleTypeDef huart5;
extern UART_HandleTypeDef hlpuart1;
extern UART_HandleTypeDef hlpuart2;
extern UART_HandleTypeDef hlpuart3;

static UART_HandleTypeDef *const uart_handles[STM32U5_UART_INSTANCE_COUNT] = {
    &huart1, &huart2, &huart3, &huart4, &huart5,
    &hlpuart1, &hlpuart2, &hlpuart3
};

/* ==================== Private Functions ==================== */

/**
 * @brief 将统一字长转换为 STM32 HAL 字长
 */
static uint32_t uart_wordlen_to_hal(xy_hal_uart_wordlen_t wordlen)
{
    switch (wordlen) {
        case XY_HAL_UART_WORDLEN_7B:
            return UART_WORDLENGTH_7B;
        case XY_HAL_UART_WORDLEN_8B:
            return UART_WORDLENGTH_8B;
        case XY_HAL_UART_WORDLEN_9B:
            return UART_WORDLENGTH_9B;
        default:
            return UART_WORDLENGTH_8B;
    }
}

/**
 * @brief 将统一停止位转换为 STM32 HAL 停止位
 */
static uint32_t uart_stopbits_to_hal(xy_hal_uart_stopbits_t stopbits)
{
    switch (stopbits) {
        case XY_HAL_UART_STOPBITS_1:
            return UART_STOPBITS_1;
        case XY_HAL_UART_STOPBITS_1_5:
            return UART_STOPBITS_1_5;
        case XY_HAL_UART_STOPBITS_2:
            return UART_STOPBITS_2;
        default:
            return UART_STOPBITS_1;
    }
}

/**
 * @brief 将统一校验位转换为 STM32 HAL 校验位
 */
static uint32_t uart_parity_to_hal(xy_hal_uart_parity_t parity)
{
    switch (parity) {
        case XY_HAL_UART_PARITY_NONE:
            return UART_PARITY_NONE;
        case XY_HAL_UART_PARITY_EVEN:
            return UART_PARITY_EVEN;
        case XY_HAL_UART_PARITY_ODD:
            return UART_PARITY_ODD;
        default:
            return UART_PARITY_NONE;
    }
}

/**
 * @brief 将统一流控制转换为 STM32 HAL 流控制
 */
static uint32_t uart_flowctrl_to_hal(xy_hal_uart_flowctrl_t flowctrl)
{
    switch (flowctrl) {
        case XY_HAL_UART_FLOWCTRL_NONE:
            return UART_HWCONTROL_NONE;
        case XY_HAL_UART_FLOWCTRL_RTS:
            return UART_HWCONTROL_RTS;
        case XY_HAL_UART_FLOWCTRL_CTS:
            return UART_HWCONTROL_CTS;
        case XY_HAL_UART_FLOWCTRL_RTS_CTS:
            return UART_HWCONTROL_RTS_CTS;
        default:
            return UART_HWCONTROL_NONE;
    }
}

/**
 * @brief 查找 UART 设备索引
 */
static int uart_find_index(const char *name)
{
    for (int i = 0; i < STM32U5_UART_INSTANCE_COUNT; i++) {
        if (strcmp(name, uart_names[i]) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief STM32 HAL UART 回调转发
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    for (int i = 0; i < STM32U5_UART_INSTANCE_COUNT; i++) {
        if (huart == uart_devices[i].huart && uart_devices[i].callback) {
            uart_devices[i].tx_busy = 0;
            uart_devices[i].callback((xy_hal_uart_t)&uart_devices[i],
                                    XY_HAL_UART_EVENT_TX_DONE,
                                    uart_devices[i].callback_arg);
            break;
        }
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    for (int i = 0; i < STM32U5_UART_INSTANCE_COUNT; i++) {
        if (huart == uart_devices[i].huart && uart_devices[i].callback) {
            uart_devices[i].rx_busy = 0;
            uart_devices[i].callback((xy_hal_uart_t)&uart_devices[i],
                                    XY_HAL_UART_EVENT_RX_DONE,
                                    uart_devices[i].callback_arg);
            break;
        }
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    for (int i = 0; i < STM32U5_UART_INSTANCE_COUNT; i++) {
        if (huart == uart_devices[i].huart) {
            uart_devices[i].stats.rx_errors++;
            
            if (huart->ErrorCode & HAL_UART_ERROR_PE) {
                uart_devices[i].stats.parity_errors++;
            }
            if (huart->ErrorCode & HAL_UART_ERROR_FE) {
                uart_devices[i].stats.frame_errors++;
            }
            
            if (uart_devices[i].callback) {
                uart_devices[i].callback((xy_hal_uart_t)&uart_devices[i],
                                        XY_HAL_UART_EVENT_ERROR,
                                        uart_devices[i].callback_arg);
            }
            break;
        }
    }
}

/* ==================== Device Model API Implementation ==================== */

xy_hal_uart_t xy_hal_uart_bind(const char *name)
{
    int index = uart_find_index(name);
    if (index < 0) {
        return NULL;
    }
    
    stm32u5_uart_data_t *dev = &uart_devices[index];
    
    dev->huart = (UART_HandleTypeDef *)uart_handles[index];
    dev->instance = (uint32_t)index;
    dev->callback = NULL;
    dev->callback_arg = NULL;
    dev->tx_busy = 0;
    dev->rx_busy = 0;
    memset(&dev->stats, 0, sizeof(dev->stats));
    
    return (xy_hal_uart_t)dev;
}

xy_hal_error_t xy_hal_uart_unbind(xy_hal_uart_t uart)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    
    /* 停止任何正在进行的传输 */
    HAL_UART_AbortTransmit(dev->huart);
    HAL_UART_AbortReceive(dev->huart);
    
    /* 反初始化 UART */
    HAL_UART_DeInit(dev->huart);
    
    memset(dev, 0, sizeof(*dev));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_configure(xy_hal_uart_t uart,
                                     const xy_hal_uart_config_t *config)
{
    if (!uart || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    
    /* 配置 UART */
    dev->huart->Init.BaudRate = config->baudrate;
    dev->huart->Init.WordLength = uart_wordlen_to_hal(config->wordlen);
    dev->huart->Init.StopBits = uart_stopbits_to_hal(config->stopbits);
    dev->huart->Init.Parity = uart_parity_to_hal(config->parity);
    dev->huart->Init.HwFlowCtl = uart_flowctrl_to_hal(config->flowctrl);
    dev->huart->Init.Mode = (config->mode == XY_HAL_UART_MODE_TX) ? 
                            UART_MODE_TX : 
                            (config->mode == XY_HAL_UART_MODE_RX) ?
                            UART_MODE_RX : UART_MODE_TX_RX;
    
    /* 过采样配置 */
    if (config->oversampling == 8) {
        dev->huart->Init.OverSampling = UART_OVERSAMPLING_8;
    } else {
        dev->huart->Init.OverSampling = UART_OVERSAMPLING_16;
    }
    
    if (HAL_UART_Init(dev->huart) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    
    dev->config = *config;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_get_config(xy_hal_uart_t uart,
                                      xy_hal_uart_config_t *config)
{
    if (!uart || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    *config = dev->config;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_set_baudrate(xy_hal_uart_t uart, uint32_t baudrate)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    dev->huart->Init.BaudRate = baudrate;
    dev->config.baudrate = baudrate;
    
    if (HAL_UART_Init(dev->huart) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }
    
    return XY_HAL_OK;
}

uint32_t xy_hal_uart_get_baudrate(xy_hal_uart_t uart)
{
    if (!uart) {
        return 0;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    return dev->huart->Init.BaudRate;
}

int32_t xy_hal_uart_write(xy_hal_uart_t uart, const uint8_t *data,
                          size_t length, uint32_t timeout)
{
    if (!uart || !data || length == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    
    HAL_StatusTypeDef status = HAL_UART_Transmit(dev->huart, (uint8_t *)data,
                                                  length, timeout);
    
    if (status == HAL_OK) {
        dev->stats.tx_bytes += length;
        return (int32_t)length;
    } else if (status == HAL_TIMEOUT) {
        return XY_HAL_ERROR_TIMEOUT;
    } else {
        dev->stats.tx_errors++;
        return XY_HAL_ERROR_FAIL;
    }
}

int32_t xy_hal_uart_read(xy_hal_uart_t uart, uint8_t *data,
                         size_t length, uint32_t timeout)
{
    if (!uart || !data || length == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    
    HAL_StatusTypeDef status = HAL_UART_Receive(dev->huart, data, length, timeout);
    
    if (status == HAL_OK) {
        dev->stats.rx_bytes += length;
        return (int32_t)length;
    } else if (status == HAL_TIMEOUT) {
        return 0; /* 超时返回 0 */
    } else {
        dev->stats.rx_errors++;
        return XY_HAL_ERROR_FAIL;
    }
}

int32_t xy_hal_uart_puts(xy_hal_uart_t uart, const char *str, uint32_t timeout)
{
    if (!str) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return xy_hal_uart_write(uart, (const uint8_t *)str, strlen(str), timeout);
}

int32_t xy_hal_uart_putchar(xy_hal_uart_t uart, uint8_t ch, uint32_t timeout)
{
    return xy_hal_uart_write(uart, &ch, 1, timeout);
}

int32_t xy_hal_uart_getchar(xy_hal_uart_t uart, uint32_t timeout)
{
    uint8_t ch;
    int32_t ret = xy_hal_uart_read(uart, &ch, 1, timeout);
    return (ret > 0) ? (int32_t)ch : ret;
}

xy_hal_error_t xy_hal_uart_wait_tx_complete(xy_hal_uart_t uart, uint32_t timeout)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    
    uint32_t start = HAL_GetTick();
    while (__HAL_UART_GET_FLAG(dev->huart, UART_FLAG_TC) == RESET) {
        if (HAL_GetTick() - start >= timeout) {
            return XY_HAL_ERROR_TIMEOUT;
        }
    }
    
    return XY_HAL_OK;
}

int32_t xy_hal_uart_write_nb(xy_hal_uart_t uart, const uint8_t *data,
                             size_t length)
{
    if (!uart || !data || length == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    
    /* 检查发送寄存器是否为空 */
    if (__HAL_UART_GET_FLAG(dev->huart, UART_FLAG_TXE) == RESET) {
        return 0; /* 发送缓冲区满 */
    }
    
    /* 发送一个字节 */
    dev->huart->Instance->TDR = (*data & 0xFF);
    dev->stats.tx_bytes++;
    
    return 1;
}

int32_t xy_hal_uart_read_nb(xy_hal_uart_t uart, uint8_t *data,
                            size_t length)
{
    if (!uart || !data || length == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    
    /* 检查是否有数据 */
    if (__HAL_UART_GET_FLAG(dev->huart, UART_FLAG_RXNE) == RESET) {
        return 0; /* 无数据 */
    }
    
    /* 接收一个字节 */
    *data = (uint8_t)(dev->huart->Instance->RDR & 0xFF);
    dev->stats.rx_bytes++;
    
    return 1;
}

int32_t xy_hal_uart_data_available(xy_hal_uart_t uart)
{
    if (!uart) {
        return 0;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    return (__HAL_UART_GET_FLAG(dev->huart, UART_FLAG_RXNE) != RESET) ? 1 : 0;
}

int32_t xy_hal_uart_tx_empty(xy_hal_uart_t uart)
{
    if (!uart) {
        return 0;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    return (__HAL_UART_GET_FLAG(dev->huart, UART_FLAG_TXE) != RESET) ? 1 : 0;
}

xy_hal_error_t xy_hal_uart_write_async(xy_hal_uart_t uart, const uint8_t *data,
                                       size_t length, xy_hal_uart_callback_t callback,
                                       void *arg)
{
    if (!uart || !data || length == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    
    if (dev->tx_busy) {
        return XY_HAL_ERROR_BUSY;
    }
    
    dev->callback = callback;
    dev->callback_arg = arg;
    dev->tx_busy = 1;
    
    HAL_StatusTypeDef status;
    if (dev->config.transfer_mode == XY_HAL_UART_TRANSFER_DMA) {
        status = HAL_UART_Transmit_DMA(dev->huart, (uint8_t *)data, length);
    } else {
        status = HAL_UART_Transmit_IT(dev->huart, (uint8_t *)data, length);
    }
    
    if (status != HAL_OK) {
        dev->tx_busy = 0;
        dev->stats.tx_errors++;
        return XY_HAL_ERROR_FAIL;
    }
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_read_async(xy_hal_uart_t uart, uint8_t *data,
                                      size_t length, xy_hal_uart_callback_t callback,
                                      void *arg)
{
    if (!uart || !data || length == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    
    if (dev->rx_busy) {
        return XY_HAL_ERROR_BUSY;
    }
    
    dev->callback = callback;
    dev->callback_arg = arg;
    dev->rx_busy = 1;
    
    HAL_StatusTypeDef status;
    if (dev->config.transfer_mode == XY_HAL_UART_TRANSFER_DMA) {
        status = HAL_UART_Receive_DMA(dev->huart, data, length);
    } else {
        status = HAL_UART_Receive_IT(dev->huart, data, length);
    }
    
    if (status != HAL_OK) {
        dev->rx_busy = 0;
        dev->stats.rx_errors++;
        return XY_HAL_ERROR_FAIL;
    }
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_stop_async(xy_hal_uart_t uart)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    
    HAL_UART_AbortTransmit(dev->huart);
    HAL_UART_AbortReceive(dev->huart);
    
    dev->tx_busy = 0;
    dev->rx_busy = 0;
    dev->callback = NULL;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_get_status(xy_hal_uart_t uart,
                                      xy_hal_uart_status_t *status)
{
    if (!uart || !status) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    
    status->tx_busy = dev->tx_busy;
    status->rx_busy = dev->rx_busy;
    status->tx_complete = !dev->tx_busy;
    status->rx_available = xy_hal_uart_data_available(uart);
    status->errors = dev->huart->ErrorCode;
    
    return XY_HAL_OK;
}

xy_hal_uart_error_t xy_hal_uart_get_error(xy_hal_uart_t uart)
{
    if (!uart) {
        return XY_HAL_UART_ERROR_NONE;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    return (xy_hal_uart_error_t)dev->huart->ErrorCode;
}

xy_hal_error_t xy_hal_uart_clear_error(xy_hal_uart_t uart)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    dev->huart->ErrorCode = HAL_UART_ERROR_NONE;
    __HAL_UART_CLEAR_FLAG(dev->huart, UART_CLEAR_PEF | UART_CLEAR_FEF | 
                          UART_CLEAR_NEF | UART_CLEAR_OREF);
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_get_stats(xy_hal_uart_t uart,
                                     xy_hal_uart_stats_t *stats)
{
    if (!uart || !stats) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    *stats = dev->stats;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_reset_stats(xy_hal_uart_t uart)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    memset(&dev->stats, 0, sizeof(dev->stats));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_flush_tx(xy_hal_uart_t uart)
{
    /* STM32U5 没有硬件 TX FIFO 清空，软件方式 */
    return xy_hal_uart_wait_tx_complete(uart, 100);
}

xy_hal_error_t xy_hal_uart_flush_rx(xy_hal_uart_t uart)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    
    /* 清空接收寄存器 */
    while (__HAL_UART_GET_FLAG(dev->huart, UART_FLAG_RXNE) != RESET) {
        (void)dev->huart->Instance->RDR;
    }
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_send_break(xy_hal_uart_t uart, uint32_t duration)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    
    /* 发送 Break 信号 */
    HAL_LIN_SendBreak(dev->huart);
    
    if (duration > 0) {
        HAL_Delay(duration);
    }
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_enable_receiver(xy_hal_uart_t uart, int enable)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    
    if (enable) {
        SET_BIT(dev->huart->Instance->CR1, USART_CR1_RE);
    } else {
        CLEAR_BIT(dev->huart->Instance->CR1, USART_CR1_RE);
    }
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_enable_transmitter(xy_hal_uart_t uart, int enable)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    stm32u5_uart_data_t *dev = (stm32u5_uart_data_t *)uart;
    
    if (enable) {
        SET_BIT(dev->huart->Instance->CR1, USART_CR1_TE);
    } else {
        CLEAR_BIT(dev->huart->Instance->CR1, USART_CR1_TE);
    }
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_control(xy_hal_uart_t uart, int cmd, void *arg)
{
    switch (cmd) {
        case XY_HAL_UART_CMD_FLUSH_TX:
            return xy_hal_uart_flush_tx(uart);
        case XY_HAL_UART_CMD_FLUSH_RX:
            return xy_hal_uart_flush_rx(uart);
        case XY_HAL_UART_CMD_BREAK:
            return xy_hal_uart_send_break(uart, (uint32_t)(uintptr_t)arg);
        default:
            return XY_HAL_ERROR_NOT_SUPPORTED;
    }
}

/* ==================== Legacy API Compatibility ==================== */

xy_hal_error_t xy_hal_uart_init(void *uart, const xy_hal_uart_config_t *config)
{
    /* 查找对应的设备实例 */
    for (int i = 0; i < STM32U5_UART_INSTANCE_COUNT; i++) {
        if (uart_handles[i] == uart) {
            xy_hal_uart_t hal_uart = (xy_hal_uart_t)&uart_devices[i];
            return xy_hal_uart_configure(hal_uart, config);
        }
    }
    
    return XY_HAL_ERROR_INVALID_PARAM;
}

int32_t xy_hal_uart_transmit(void *uart, const uint8_t *data,
                             size_t len, uint32_t timeout)
{
    for (int i = 0; i < STM32U5_UART_INSTANCE_COUNT; i++) {
        if (uart_handles[i] == uart) {
            return xy_hal_uart_write((xy_hal_uart_t)&uart_devices[i], data, len, timeout);
        }
    }
    return XY_HAL_ERROR_INVALID_PARAM;
}

int32_t xy_hal_uart_receive(void *uart, uint8_t *data,
                            size_t len, uint32_t timeout)
{
    for (int i = 0; i < STM32U5_UART_INSTANCE_COUNT; i++) {
        if (uart_handles[i] == uart) {
            return xy_hal_uart_read((xy_hal_uart_t)&uart_devices[i], data, len, timeout);
        }
    }
    return XY_HAL_ERROR_INVALID_PARAM;
}

/* ==================== End of File ==================== */
