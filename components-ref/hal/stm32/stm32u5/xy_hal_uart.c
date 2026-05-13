/**
 * @file xy_hal_uart.c
 * @brief UART HAL STM32U5 Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal_uart.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

/* UART context structure */
typedef struct {
    UART_HandleTypeDef *huart;
    xy_hal_uart_callback_t callback;
    void *arg;
    uint8_t initialized;
} uart_ctx_t;

/* Maximum UART instances - adjust based on MCU */
#define MAX_UART_INSTANCES 8
static uart_ctx_t g_uart_ctx[MAX_UART_INSTANCES];

/**
 * @brief Find UART context by handle
 */
static uart_ctx_t *find_uart_ctx(void *uart)
{
    for (size_t i = 0; i < MAX_UART_INSTANCES; i++) {
        if (g_uart_ctx[i].huart == uart) {
            return &g_uart_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Allocate new UART context
 */
static uart_ctx_t *alloc_uart_ctx(void)
{
    for (size_t i = 0; i < MAX_UART_INSTANCES; i++) {
        if (g_uart_ctx[i].huart == NULL) {
            return &g_uart_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Convert XY UART word length to STM32
 */
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
}

/**
 * @brief Convert XY UART stop bits to STM32
 */
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
}

/**
 * @brief Convert XY UART parity to STM32
 */
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
}

/**
 * @brief Convert XY UART flow control to STM32
 */
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
}

/**
 * @brief Convert XY UART mode to STM32
 */
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
}

xy_hal_error_t xy_hal_uart_init(void *uart, const xy_hal_uart_config_t *config)
{
    if (!uart || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)uart;

    /* Check if already initialized */
    uart_ctx_t *ctx = find_uart_ctx(uart);
    if (ctx != NULL && ctx->initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    /* Allocate context if not exists */
    if (ctx == NULL) {
        ctx = alloc_uart_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    huart->Init.BaudRate   = config->baudrate;
    huart->Init.WordLength = xy_to_stm32_wordlen(config->wordlen);
    huart->Init.StopBits   = xy_to_stm32_stopbits(config->stopbits);
    huart->Init.Parity     = xy_to_stm32_parity(config->parity);
    huart->Init.Mode       = xy_to_stm32_mode(config->mode);
    huart->Init.HwFlowCtl  = xy_to_stm32_flowctrl(config->flowctrl);
    huart->Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(huart) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    /* Initialize context */
    ctx->huart      = huart;
    ctx->callback   = NULL;
    ctx->arg        = NULL;
    ctx->initialized = 1;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_deinit(void *uart)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uart_ctx_t *ctx = find_uart_ctx(uart);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_UART_DeInit((UART_HandleTypeDef *)uart) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->initialized = 0;
    ctx->huart       = NULL;
    ctx->callback    = NULL;
    ctx->arg         = NULL;

    return XY_HAL_OK;
}

int xy_hal_uart_send(void *uart, const uint8_t *data, size_t len,
                     uint32_t timeout)
{
    if (!uart || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uart_ctx_t *ctx = find_uart_ctx(uart);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_UART_Transmit(
        (UART_HandleTypeDef *)uart, (uint8_t *)data, (uint16_t)len, timeout);

    if (status != HAL_OK) {
        if (status == HAL_TIMEOUT) {
            return XY_HAL_ERROR_TIMEOUT;
        }
        return XY_HAL_ERROR_IO;
    }

    return (int)len;
}

int xy_hal_uart_recv(void *uart, uint8_t *data, size_t len, uint32_t timeout)
{
    if (!uart || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uart_ctx_t *ctx = find_uart_ctx(uart);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_UART_Receive(
        (UART_HandleTypeDef *)uart, data, (uint16_t)len, timeout);

    if (status != HAL_OK) {
        if (status == HAL_TIMEOUT) {
            return XY_HAL_ERROR_TIMEOUT;
        }
        return XY_HAL_ERROR_IO;
    }

    return (int)len;
}

xy_hal_error_t xy_hal_uart_send_dma(void *uart, const uint8_t *data,
                                    size_t len)
{
    if (!uart || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uart_ctx_t *ctx = find_uart_ctx(uart);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_UART_Transmit_DMA(
        (UART_HandleTypeDef *)uart, (uint8_t *)data, (uint16_t)len);

    if (status != HAL_OK) {
        return XY_HAL_ERROR_BUSY;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_recv_dma(void *uart, uint8_t *data, size_t len)
{
    if (!uart || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uart_ctx_t *ctx = find_uart_ctx(uart);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_UART_Receive_DMA(
        (UART_HandleTypeDef *)uart, data, (uint16_t)len);

    if (status != HAL_OK) {
        return XY_HAL_ERROR_BUSY;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_register_callback(void *uart,
                                             xy_hal_uart_callback_t callback,
                                             void *arg)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uart_ctx_t *ctx = find_uart_ctx(uart);
    if (ctx == NULL) {
        ctx = alloc_uart_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->huart = (UART_HandleTypeDef *)uart;
    }

    ctx->callback = callback;
    ctx->arg      = arg;

    return XY_HAL_OK;
}

int xy_hal_uart_available(void *uart)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)uart;

    /* Return number of bytes in RX buffer */
    if (huart->RxState == HAL_UART_STATE_BUSY_RX) {
        return (int)(huart->RxXferSize - huart->RxXferCount);
    }
    return 0;
}

xy_hal_error_t xy_hal_uart_flush(void *uart)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)uart;

    /* Wait for transmission to complete */
    uint32_t timeout = HAL_GetTick() + 1000;
    while (huart->gState != HAL_UART_STATE_READY) {
        if (HAL_GetTick() >= timeout) {
            return XY_HAL_ERROR_TIMEOUT;
        }
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_error(void *uart)
{
    if (!uart) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    /* Placeholder for UART error pin control if needed */
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_set_error_cb(void *uart,
                                        xy_hal_uart_callback_t callback,
                                        void *arg)
{
    /* Use same callback as main callback for simplicity */
    return xy_hal_uart_register_callback(uart, callback, arg);
}

/* ==================== HAL Callbacks ==================== */

/**
 * @brief UART TX complete callback
 */
void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    uart_ctx_t *ctx = find_uart_ctx(huart);
    if (ctx && ctx->callback) {
        ctx->callback(huart, XY_HAL_UART_EVENT_TX_DONE, ctx->arg);
    }
}

/**
 * @brief UART RX complete callback
 */
void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    uart_ctx_t *ctx = find_uart_ctx(huart);
    if (ctx && ctx->callback) {
        ctx->callback(huart, XY_HAL_UART_EVENT_RX_DONE, ctx->arg);
    }
}

/**
 * @brief UART error callback
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    uart_ctx_t *ctx = find_uart_ctx(huart);
    if (ctx && ctx->callback) {
        ctx->callback(huart, XY_HAL_UART_EVENT_ERROR, ctx->arg);
    }
}

#endif /* STM32U5 || STM32U5xx */
