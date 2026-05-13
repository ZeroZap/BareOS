/**
 * @file xy_hal_spi.c
 * @brief SPI HAL STM32U5 Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal_spi.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

/* SPI context structure */
typedef struct {
    SPI_HandleTypeDef *hspi;
    xy_hal_spi_callback_t callback;
    void *arg;
    uint8_t initialized;
} spi_ctx_t;

/* Maximum SPI instances */
#define MAX_SPI_INSTANCES 4
static spi_ctx_t g_spi_ctx[MAX_SPI_INSTANCES];

/**
 * @brief Find SPI context by handle
 */
static spi_ctx_t *find_spi_ctx(void *spi)
{
    for (size_t i = 0; i < MAX_SPI_INSTANCES; i++) {
        if (g_spi_ctx[i].hspi == spi) {
            return &g_spi_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Allocate new SPI context
 */
static spi_ctx_t *alloc_spi_ctx(void)
{
    for (size_t i = 0; i < MAX_SPI_INSTANCES; i++) {
        if (g_spi_ctx[i].hspi == NULL) {
            return &g_spi_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Convert XY SPI mode to STM32
 */
static uint32_t xy_to_stm32_mode(xy_hal_spi_mode_t mode)
{
    switch (mode) {
    case XY_HAL_SPI_MODE_0:
        return SPI_MODE_0;
    case XY_HAL_SPI_MODE_1:
        return SPI_MODE_1;
    case XY_HAL_SPI_MODE_2:
        return SPI_MODE_2;
    case XY_HAL_SPI_MODE_3:
        return SPI_MODE_3;
    default:
        return SPI_MODE_0;
    }
}

/**
 * @brief Convert XY data size to STM32
 */
static uint32_t xy_to_stm32_datasize(xy_hal_spi_datasize_t datasize)
{
    switch (datasize) {
    case XY_HAL_SPI_DATASIZE_8BIT:
        return SPI_DATASIZE_8BIT;
    case XY_HAL_SPI_DATASIZE_16BIT:
        return SPI_DATASIZE_16BIT;
    default:
        return SPI_DATASIZE_8BIT;
    }
}

/**
 * @brief Convert XY first bit to STM32
 */
static uint32_t xy_to_stm32_firstbit(xy_hal_spi_firstbit_t firstbit)
{
    switch (firstbit) {
    case XY_HAL_SPI_FIRSTBIT_MSB:
        return SPI_FIRSTBIT_MSB;
    case XY_HAL_SPI_FIRSTBIT_LSB:
        return SPI_FIRSTBIT_LSB;
    default:
        return SPI_FIRSTBIT_MSB;
    }
}

/**
 * @brief Convert XY NSS mode to STM32
 */
static uint32_t xy_to_stm32_nss(xy_hal_spi_nss_t nss)
{
    switch (nss) {
    case XY_HAL_SPI_NSS_SOFT:
        return SPI_NSS_SOFT;
    case XY_HAL_SPI_NSS_HARD_INPUT:
        return SPI_NSS_HARD_INPUT;
    case XY_HAL_SPI_NSS_HARD_OUTPUT:
        return SPI_NSS_HARD_OUTPUT;
    default:
        return SPI_NSS_SOFT;
    }
}

/**
 * @brief Convert XY direction to STM32
 */
static uint32_t xy_to_stm32_direction(xy_hal_spi_direction_t direction)
{
    switch (direction) {
    case XY_HAL_SPI_DIR_2LINES:
        return SPI_DIRECTION_2LINES;
    case XY_HAL_SPI_DIR_2LINES_RXONLY:
        return SPI_DIRECTION_2LINES_RXONLY;
    case XY_HAL_SPI_DIR_1LINE:
        return SPI_DIRECTION_1LINE;
    default:
        return SPI_DIRECTION_2LINES;
    }
}

xy_hal_error_t xy_hal_spi_init(void *spi, const xy_hal_spi_config_t *config)
{
    if (!spi || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)spi;

    /* Check if already initialized */
    spi_ctx_t *ctx = find_spi_ctx(spi);
    if (ctx != NULL && ctx->initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    /* Allocate context if not exists */
    if (ctx == NULL) {
        ctx = alloc_spi_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    hspi->Init.Mode           = config->is_master ? SPI_MODE_MASTER : SPI_MODE_SLAVE;
    hspi->Init.Direction      = xy_to_stm32_direction(config->direction);
    hspi->Init.CLKPhase       = xy_to_stm32_mode(config->mode) & 0x01;
    hspi->Init.CLKPolarity    = xy_to_stm32_mode(config->mode) >> 1;
    hspi->Init.DataSize       = xy_to_stm32_datasize(config->datasize);
    hspi->Init.FirstBit       = xy_to_stm32_firstbit(config->firstbit);
    hspi->Init.NSS            = xy_to_stm32_nss(config->nss);
    hspi->Init.BaudRatePrescaler = config->baudrate_prescaler;
    hspi->Init.TIMode         = SPI_TIMODE_DISABLE;
    hspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi->Init.CRCPolynomial  = 7;

    if (HAL_SPI_Init(hspi) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    /* Initialize context */
    ctx->hspi        = hspi;
    ctx->callback    = NULL;
    ctx->arg         = NULL;
    ctx->initialized = 1;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_deinit(void *spi)
{
    if (!spi) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    spi_ctx_t *ctx = find_spi_ctx(spi);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_SPI_DeInit((SPI_HandleTypeDef *)spi) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->initialized = 0;
    ctx->hspi        = NULL;
    ctx->callback    = NULL;
    ctx->arg         = NULL;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_transmit(void *spi, const uint8_t *data, size_t len,
                                   uint32_t timeout)
{
    if (!spi || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    spi_ctx_t *ctx = find_spi_ctx(spi);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_SPI_Transmit(
        (SPI_HandleTypeDef *)spi, (uint8_t *)data, (uint16_t)len, timeout);

    if (status != HAL_OK) {
        if (status == HAL_TIMEOUT) {
            return XY_HAL_ERROR_TIMEOUT;
        }
        return XY_HAL_ERROR_IO;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_receive(void *spi, uint8_t *data, size_t len,
                                  uint32_t timeout)
{
    if (!spi || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    spi_ctx_t *ctx = find_spi_ctx(spi);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_SPI_Receive(
        (SPI_HandleTypeDef *)spi, data, (uint16_t)len, timeout);

    if (status != HAL_OK) {
        if (status == HAL_TIMEOUT) {
            return XY_HAL_ERROR_TIMEOUT;
        }
        return XY_HAL_ERROR_IO;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_transmit_receive(void *spi, const uint8_t *tx_data,
                                           uint8_t *rx_data, size_t len,
                                           uint32_t timeout)
{
    if (!spi || !tx_data || !rx_data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    spi_ctx_t *ctx = find_spi_ctx(spi);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(
        (SPI_HandleTypeDef *)spi, (uint8_t *)tx_data, rx_data,
        (uint16_t)len, timeout);

    if (status != HAL_OK) {
        if (status == HAL_TIMEOUT) {
            return XY_HAL_ERROR_TIMEOUT;
        }
        return XY_HAL_ERROR_IO;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_transmit_dma(void *spi, const uint8_t *data,
                                       size_t len)
{
    if (!spi || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    spi_ctx_t *ctx = find_spi_ctx(spi);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_SPI_Transmit_DMA(
        (SPI_HandleTypeDef *)spi, (uint8_t *)data, (uint16_t)len);

    if (status != HAL_OK) {
        return XY_HAL_ERROR_BUSY;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_receive_dma(void *spi, uint8_t *data, size_t len)
{
    if (!spi || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    spi_ctx_t *ctx = find_spi_ctx(spi);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_SPI_Receive_DMA(
        (SPI_HandleTypeDef *)spi, data, (uint16_t)len);

    if (status != HAL_OK) {
        return XY_HAL_ERROR_BUSY;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_transmit_receive_dma(void *spi,
                                               const uint8_t *tx_data,
                                               uint8_t *rx_data, size_t len)
{
    if (!spi || !tx_data || !rx_data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    spi_ctx_t *ctx = find_spi_ctx(spi);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive_DMA(
        (SPI_HandleTypeDef *)spi, (uint8_t *)tx_data, rx_data, (uint16_t)len);

    if (status != HAL_OK) {
        return XY_HAL_ERROR_BUSY;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_register_callback(void *spi,
                                            xy_hal_spi_callback_t callback,
                                            void *arg)
{
    if (!spi) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    spi_ctx_t *ctx = find_spi_ctx(spi);
    if (ctx == NULL) {
        ctx = alloc_spi_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->hspi = (SPI_HandleTypeDef *)spi;
    }

    ctx->callback = callback;
    ctx->arg      = arg;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_set_cs(void *spi, uint8_t level)
{
    XY_UNUSED(spi);
    XY_UNUSED(level);
    /* CS pin control depends on hardware implementation */
    /* User should control CS pin manually if using software NSS */
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_error(void *spi, uint8_t level)
{
    XY_UNUSED(spi);
    XY_UNUSED(level);
    /* Placeholder for SPI error pin control if needed */
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_set_error_cb(void *spi, xy_hal_spi_callback_t callback,
                                       void *arg)
{
    return xy_hal_spi_register_callback(spi, callback, arg);
}

/* ==================== HAL Callbacks ==================== */

/**
 * @brief SPI TX complete callback
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    spi_ctx_t *ctx = find_spi_ctx(hspi);
    if (ctx && ctx->callback) {
        ctx->callback(hspi, XY_HAL_SPI_EVENT_TX_DONE, ctx->arg);
    }
}

/**
 * @brief SPI RX complete callback
 */
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    spi_ctx_t *ctx = find_spi_ctx(hspi);
    if (ctx && ctx->callback) {
        ctx->callback(hspi, XY_HAL_SPI_EVENT_RX_DONE, ctx->arg);
    }
}

/**
 * @brief SPI TX/RX complete callback
 */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    spi_ctx_t *ctx = find_spi_ctx(hspi);
    if (ctx && ctx->callback) {
        ctx->callback(hspi, XY_HAL_SPI_EVENT_TX_RX_DONE, ctx->arg);
    }
}

/**
 * @brief SPI error callback
 */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    spi_ctx_t *ctx = find_spi_ctx(hspi);
    if (ctx && ctx->callback) {
        ctx->callback(hspi, XY_HAL_SPI_EVENT_ERROR, ctx->arg);
    }
}

#endif /* STM32U5 || STM32U5xx */
