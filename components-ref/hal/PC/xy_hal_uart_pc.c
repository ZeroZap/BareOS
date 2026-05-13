/**
 * @file xy_hal_uart_pc.c
 * @brief PC/Linux simulation layer for UART HAL
 */

#include "xy_hal_uart.h"
#include <stdio.h>
#include <string.h>

xy_hal_error_t xy_hal_uart_send(void *huart, const uint8_t *data, size_t len, uint32_t timeout)
{
    (void)huart; (void)data; (void)len; (void)timeout;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_recv(void *huart, uint8_t *data, size_t len, uint32_t timeout)
{
    (void)huart; (void)timeout;
    if (data && len > 0) memset(data, 0, len);
    return XY_HAL_OK;
}
