#include "xy_hal_n32l40x_internal.h"

int xy_hal_uart_init(void *uart, const xy_hal_uart_config_t *config)
{
    USART_Module *u = (USART_Module *)uart;
    USART_InitType init;
    if (!u || !config) return XY_HAL_INVALID_PARAM;
    if ((config->rx_mode == XY_HAL_IO_DMA && !config->rx_dma) || (config->tx_mode == XY_HAL_IO_DMA && !config->tx_dma)) return XY_HAL_INVALID_PARAM;
    xy_hal_n32_enable_uart_clock(u);
    USART_StructInit(&init);
    init.BaudRate = config->baudrate ? config->baudrate : 115200u;
    init.WordLength = config->data_bits == 9u ? USART_WL_9B : USART_WL_8B;
    init.StopBits = config->stop_bits == 2u ? USART_STPB_2 : USART_STPB_1;
    init.Parity = config->parity == XY_HAL_UART_PARITY_EVEN ? USART_PE_EVEN : config->parity == XY_HAL_UART_PARITY_ODD ? USART_PE_ODD : USART_PE_NO;
    init.Mode = USART_MODE_RX | USART_MODE_TX;
    init.HardwareFlowControl = USART_HFCTRL_NONE;
    USART_Init(u, &init);
    USART_EnableDMA(u, USART_DMAREQ_RX, config->rx_dma ? ENABLE : DISABLE);
    USART_EnableDMA(u, USART_DMAREQ_TX, config->tx_dma ? ENABLE : DISABLE);
    if (config->rx_mode == XY_HAL_IO_IRQ) USART_ConfigInt(u, USART_INT_RXDNE, ENABLE);
    if (config->tx_mode == XY_HAL_IO_IRQ) USART_ConfigInt(u, USART_INT_TXDE, ENABLE);
    USART_Enable(u, ENABLE);
    return XY_HAL_OK;
}

int xy_hal_uart_deinit(void *uart)
{
    if (!uart) return XY_HAL_INVALID_PARAM;
    USART_Enable((USART_Module *)uart, DISABLE);
    USART_DeInit((USART_Module *)uart);
    return XY_HAL_OK;
}

int xy_hal_uart_read(void *uart, uint8_t *data, size_t len, uint32_t timeout_ms)
{
    USART_Module *u = (USART_Module *)uart;
    uint32_t start = xy_hal_time_ms();
    size_t count = 0u;
    if (!u || (!data && len)) return XY_HAL_INVALID_PARAM;
    while (count < len) {
        if (USART_GetFlagStatus(u, USART_FLAG_RXDNE) == SET) data[count++] = (uint8_t)USART_ReceiveData(u);
        else if (xy_hal_n32_expired(start, timeout_ms)) break;
    }
    return (int)count;
}

int xy_hal_uart_write(void *uart, const uint8_t *data, size_t len, uint32_t timeout_ms)
{
    USART_Module *u = (USART_Module *)uart;
    uint32_t start = xy_hal_time_ms();
    size_t count = 0u;
    if (!u || (!data && len)) return XY_HAL_INVALID_PARAM;
    while (count < len) {
        if (USART_GetFlagStatus(u, USART_FLAG_TXDE) == SET) {
            USART_SendData(u, data[count++]);
            start = xy_hal_time_ms();
        } else if (xy_hal_n32_expired(start, timeout_ms)) break;
    }
    return (int)count;
}

int xy_hal_uart_start_rx(void *uart, uint8_t *buffer, size_t len)
{
    (void)buffer;
    (void)len;
    if (!uart) return XY_HAL_INVALID_PARAM;
    USART_ConfigInt((USART_Module *)uart, USART_INT_RXDNE, ENABLE);
    return XY_HAL_OK;
}

int xy_hal_uart_stop_rx(void *uart)
{
    if (!uart) return XY_HAL_INVALID_PARAM;
    USART_ConfigInt((USART_Module *)uart, USART_INT_RXDNE, DISABLE);
    return XY_HAL_OK;
}

int xy_hal_uart_enable_irq(void *uart, uint32_t irq_mask)
{
    if (!uart) return XY_HAL_INVALID_PARAM;
    USART_ConfigInt((USART_Module *)uart, (uint16_t)irq_mask, ENABLE);
    return XY_HAL_OK;
}

int xy_hal_uart_disable_irq(void *uart, uint32_t irq_mask)
{
    if (!uart) return XY_HAL_INVALID_PARAM;
    USART_ConfigInt((USART_Module *)uart, (uint16_t)irq_mask, DISABLE);
    return XY_HAL_OK;
}

int xy_hal_uart_wait_tx_done(void *uart, uint32_t timeout_ms)
{
    uint32_t start = xy_hal_time_ms();
    if (!uart) return XY_HAL_INVALID_PARAM;
    while (USART_GetFlagStatus((USART_Module *)uart, USART_FLAG_TXC) == RESET) {
        if (xy_hal_n32_expired(start, timeout_ms)) return XY_HAL_TIMEOUT;
    }
    return XY_HAL_OK;
}

int xy_hal_uart_set_callback(void *uart, xy_hal_event_cb_t cb, void *user)
{
    xy_hal_n32_dev_cb_state_t *st = xy_hal_n32_uart_state((USART_Module *)uart);
    if (!st) return XY_HAL_INVALID_PARAM;
    st->dev = uart;
    st->cb = cb;
    st->user = user;
    return XY_HAL_OK;
}

void xy_hal_uart_irq_handler(void *uart)
{
    USART_Module *u = (USART_Module *)uart;
    xy_hal_n32_dev_cb_state_t *st = xy_hal_n32_uart_state(u);
    if (!u || !st || !st->cb) return;
    if (USART_GetIntStatus(u, USART_INT_RXDNE) != RESET) st->cb(u, XY_HAL_EVENT_RX_READY, st->user);
    if (USART_GetIntStatus(u, USART_INT_IDLEF) != RESET) {
        (void)USART_ReceiveData(u);
        st->cb(u, XY_HAL_EVENT_RX_IDLE, st->user);
    }
    if (USART_GetIntStatus(u, USART_INT_TXC) != RESET) {
        USART_ClrIntPendingBit(u, USART_INT_TXC);
        st->cb(u, XY_HAL_EVENT_TX_DONE, st->user);
    }
}
