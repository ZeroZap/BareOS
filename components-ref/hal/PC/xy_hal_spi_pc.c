/**
 * @file xy_hal_spi_pc.c
 * @brief PC/Linux simulation layer for SPI HAL
 */

#include "xy_hal_spi.h"
#include <stdio.h>
#include <string.h>

xy_hal_error_t xy_hal_spi_transmit(void *hspi, const uint8_t *data, size_t len, uint32_t timeout)
{
    (void)hspi; (void)data; (void)len; (void)timeout;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_receive(void *hspi, uint8_t *data, size_t len, uint32_t timeout)
{
    (void)hspi; (void)timeout;
    if (data && len > 0) memset(data, 0, len);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_spi_transmit_receive(void *hspi, const uint8_t *tx_data,
                                            uint8_t *rx_data, size_t len, uint32_t timeout)
{
    (void)hspi; (void)tx_data; (void)timeout;
    if (rx_data && len > 0) memset(rx_data, 0, len);
    return XY_HAL_OK;
}
