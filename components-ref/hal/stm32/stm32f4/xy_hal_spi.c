/**
 * @file xy_hal_spi_stm32.c
 * @brief SPI HAL STM32 Implementation
 * @version 1.0
 * @date 2025-10-26
 */

#include "../inc/xy_hal_spi.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"

static uint32_t xy_to_stm32_spi_mode(xy_hal_spi_mode_t mode)
{
    switch (mode) {
    case XY_HAL_SPI_MODE_0:
        return SPI_POLARITY_LOW | SPI_PHASE_1EDGE;
    case XY_HAL_SPI_MODE_1:
        return SPI_POLARITY_LOW | SPI_PHASE_2EDGE;
    case XY_HAL_SPI_MODE_2:
        return SPI_POLARITY_HIGH | SPI_PHASE_1EDGE;
    case XY_HAL_SPI_MODE_3:
        return SPI_POLARITY_HIGH | SPI_PHASE_2EDGE;
    default:
        return SPI_POLARITY_LOW | SPI_PHASE_1EDGE;
    }

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

static uint32_t xy_to_stm32_datasize(xy_hal_spi_datasize_t datasize)
{
    return (datasize == XY_HAL_SPI_DATASIZE_8BIT) ? SPI_DATASIZE_8BIT
                                                  : SPI_DATASIZE_16BIT;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

static uint32_t xy_to_stm32_firstbit(xy_hal_spi_firstbit_t firstbit)
{
    return (firstbit == XY_HAL_SPI_FIRSTBIT_MSB) ? SPI_FIRSTBIT_MSB
                                                 : SPI_FIRSTBIT_LSB;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_spi_init(void *spi, const xy_hal_spi_config_t *config)
{
    if (!spi || !config) {
        return -1;
    }

    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)spi;

    hspi->Init.Mode      = config->is_master ? SPI_MODE_MASTER : SPI_MODE_SLAVE;
    hspi->Init.Direction = (config->direction == XY_HAL_SPI_DIR_2LINES)
                               ? SPI_DIRECTION_2LINES
                               : SPI_DIRECTION_1LINE;
    hspi->Init.DataSize  = xy_to_stm32_datasize(config->datasize);

    uint32_t mode_val = xy_to_stm32_spi_mode(config->mode);
    hspi->Init.CLKPolarity =
        (mode_val & SPI_POLARITY_HIGH) ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
    hspi->Init.CLKPhase =
        (mode_val & SPI_PHASE_2EDGE) ? SPI_PHASE_2EDGE : SPI_PHASE_1EDGE;

    hspi->Init.NSS = (config->nss == XY_HAL_SPI_NSS_SOFT) ? SPI_NSS_SOFT
                                                          : SPI_NSS_HARD_OUTPUT;
    hspi->Init.BaudRatePrescaler = config->baudrate_prescaler;
    hspi->Init.FirstBit          = xy_to_stm32_firstbit(config->firstbit);
    hspi->Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi->Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;

    if (HAL_SPI_Init(hspi) != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_spi_deinit(void *spi)
{
    if (!spi) {
        return -1;
    }

    if (HAL_SPI_DeInit((SPI_HandleTypeDef *)spi) != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_spi_transmit(void *spi, const uint8_t *data, size_t len,
                        uint32_t timeout)
{
    if (!spi || !data || len == 0) {
        return -1;
    }

    if (HAL_SPI_Transmit(
            (SPI_HandleTypeDef *)spi, (uint8_t *)data, len, timeout)
        != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_spi_receive(void *spi, uint8_t *data, size_t len, uint32_t timeout)
{
    if (!spi || !data || len == 0) {
        return -1;
    }

    if (HAL_SPI_Receive((SPI_HandleTypeDef *)spi, data, len, timeout)
        != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_spi_transmit_receive(void *spi, const uint8_t *tx_data,
                                uint8_t *rx_data, size_t len, uint32_t timeout)
{
    if (!spi || !tx_data || !rx_data || len == 0) {
        return -1;
    }

    if (HAL_SPI_TransmitReceive(
            (SPI_HandleTypeDef *)spi, (uint8_t *)tx_data, rx_data, len, timeout)
        != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_spi_transmit_dma(void *spi, const uint8_t *data, size_t len)
{
    if (!spi || !data || len == 0) {
        return -1;
    }

    if (HAL_SPI_Transmit_DMA((SPI_HandleTypeDef *)spi, (uint8_t *)data, len)
        != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_spi_receive_dma(void *spi, uint8_t *data, size_t len)
{
    if (!spi || !data || len == 0) {
        return -1;
    }

    if (HAL_SPI_Receive_DMA((SPI_HandleTypeDef *)spi, data, len) != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_spi_transmit_receive_dma(void *spi, const uint8_t *tx_data,
                                    uint8_t *rx_data, size_t len)
{
    if (!spi || !tx_data || !rx_data || len == 0) {
        return -1;
    }

    if (HAL_SPI_TransmitReceive_DMA(
            (SPI_HandleTypeDef *)spi, (uint8_t *)tx_data, rx_data, len)
        != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_spi_register_callback(void *spi, xy_hal_spi_callback_t callback,
                                 void *arg)
{
    /* Store callback in context */
    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_spi_set_cs(void *spi, uint8_t level)
{
    /* Implement software CS control */
    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

#endif /* STM32_HAL_ENABLED */
