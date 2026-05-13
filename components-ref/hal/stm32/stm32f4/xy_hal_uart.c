/**
 * @file xy_hal_uart_stm32.c
 * @brief UART HAL STM32 Implementation
 * @version 1.0
 * @date 2025-10-26
 */

#include "../inc/xy_hal_uart.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"

/* UART context structure */
typedef struct {
    UART_HandleTypeDef *huart;
    xy_hal_uart_callback_t callback;
    void *arg;
} uart_ctx_t;

static uint32_t xy_to_stm32_wordlen(xy_hal_uart_wordlen_t wordlen)
{
    switch (wordlen) {
#ifdef UART_WORDLENGTH_7B
    case XY_HAL_UART_WORDLEN_7B:
        return UART_WORDLENGTH_7B;
#endif
    case XY_HAL_UART_WORDLEN_8B:
        return UART_WORDLENGTH_8B;
    case XY_HAL_UART_WORDLEN_9B:
        return UART_WORDLENGTH_9B;
    default:
        return UART_WORDLENGTH_8B;
    }

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

static uint32_t xy_to_stm32_stopbits(xy_hal_uart_stopbits_t stopbits)
{
    switch (stopbits) {
    case XY_HAL_UART_STOPBITS_1:
        return UART_STOPBITS_1;
#ifdef UART_STOPBITS_1_5
    case XY_HAL_UART_STOPBITS_1_5:
        return UART_STOPBITS_1_5;
#endif
    case XY_HAL_UART_STOPBITS_2:
        return UART_STOPBITS_2;
    default:
        return UART_STOPBITS_1;
    }

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

static uint32_t xy_to_stm32_parity(xy_hal_uart_parity_t parity)
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

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

static uint32_t xy_to_stm32_flowctrl(xy_hal_uart_flowctrl_t flowctrl)
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

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

static uint32_t xy_to_stm32_mode(xy_hal_uart_mode_t mode)
{
    switch (mode) {
    case XY_HAL_UART_MODE_TX:
        return UART_MODE_TX;
    case XY_HAL_UART_MODE_RX:
        return UART_MODE_RX;
    case XY_HAL_UART_MODE_TX_RX:
        return UART_MODE_TX_RX;
    default:
        return UART_MODE_TX_RX;
    }

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_uart_init(void *uart, const xy_hal_uart_config_t *config)
{
    if (!uart || !config) {
        return -1;
    }

    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)uart;

    huart->Init.BaudRate     = config->baudrate;
    huart->Init.WordLength   = xy_to_stm32_wordlen(config->wordlen);
    huart->Init.StopBits     = xy_to_stm32_stopbits(config->stopbits);
    huart->Init.Parity       = xy_to_stm32_parity(config->parity);
    huart->Init.Mode         = xy_to_stm32_mode(config->mode);
    huart->Init.HwFlowCtl    = xy_to_stm32_flowctrl(config->flowctrl);
    huart->Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(huart) != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_uart_deinit(void *uart)
{
    if (!uart) {
        return -1;
    }

    if (HAL_UART_DeInit((UART_HandleTypeDef *)uart) != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_uart_send(void *uart, const uint8_t *data, size_t len,
                     uint32_t timeout)
{
    if (!uart || !data || len == 0) {
        return -1;
    }

    if (HAL_UART_Transmit(
            (UART_HandleTypeDef *)uart, (uint8_t *)data, len, timeout)
        != HAL_OK) {
        return -1;
    }

    return (int)len;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_uart_recv(void *uart, uint8_t *data, size_t len, uint32_t timeout)
{
    if (!uart || !data || len == 0) {
        return -1;
    }

    if (HAL_UART_Receive((UART_HandleTypeDef *)uart, data, len, timeout)
        != HAL_OK) {
        return -1;
    }

    return (int)len;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_uart_send_dma(void *uart, const uint8_t *data, size_t len)
{
    if (!uart || !data || len == 0) {
        return -1;
    }

    if (HAL_UART_Transmit_DMA((UART_HandleTypeDef *)uart, (uint8_t *)data, len)
        != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_uart_recv_dma(void *uart, uint8_t *data, size_t len)
{
    if (!uart || !data || len == 0) {
        return -1;
    }

    if (HAL_UART_Receive_DMA((UART_HandleTypeDef *)uart, data, len) != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_uart_register_callback(void *uart, xy_hal_uart_callback_t callback,
                                  void *arg)
{
    if (!uart) {
        return -1;
    }

    /* Store callback in user data field or separate context */
    /* Implementation depends on how you manage UART contexts */

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_uart_available(void *uart)
{
    /* Implementation depends on buffer management */
    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_uart_flush(void *uart)
{
    if (!uart) {
        return -1;
    }

    /* Wait for transmission to complete */
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)uart;
    while (huart->gState != HAL_UART_STATE_READY)
        ;

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

/* HAL callbacks */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    /* Call user callback if registered */

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    /* Call user callback if registered */

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    /* Call user callback if registered */

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

#endif /* STM32_HAL_ENABLED */
