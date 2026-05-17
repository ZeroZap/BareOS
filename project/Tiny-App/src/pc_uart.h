/**
 * @file pc_uart.h
 * @brief PC UART simulation — stdin/stdout.
 *
 * Provides non-blocking stdin reads and stdout writes so component code
 * that calls pc_uart_write_str() / pc_uart_read_byte() works without a
 * real serial port.
 *
 * Call pc_uart_init() once at startup, then pc_uart_poll() each main-loop
 * iteration.  Each received byte is forwarded to the registered rx callback.
 */

#ifndef PC_UART_H
#define PC_UART_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Callback type: called for each received byte from stdin. */
typedef void (*pc_uart_rx_fn)(uint8_t byte, void *arg);

/* Initialise stdin for non-blocking reads.  Pass NULL to skip rx. */
void pc_uart_init(pc_uart_rx_fn rx_cb, void *rx_arg);

/* Poll stdin for available bytes; fires rx_cb for each one. */
void pc_uart_poll(void);

/* Write a NUL-terminated string to stdout. */
void pc_uart_write_str(const char *s);

/* Write raw bytes to stdout. */
void pc_uart_write(const uint8_t *buf, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* PC_UART_H */
