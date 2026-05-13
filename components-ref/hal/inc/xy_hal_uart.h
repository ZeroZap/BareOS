/**
 * @file xy_hal_uart.h
 * @brief UART Hardware Abstraction Layer
 * @version 2.0
 * @date 2025-10-26
 */

#ifndef XY_HAL_UART_H
#define XY_HAL_UART_H
#include "xy_hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_hal.h"
#include <stddef.h>
#include <stdint.h>

/* UART Word Length */
typedef enum {
  XY_HAL_UART_WORDLEN_7B = 0, /**< 7-bit word length */
  XY_HAL_UART_WORDLEN_8B,     /**< 8-bit word length */
  XY_HAL_UART_WORDLEN_9B,     /**< 9-bit word length */
} xy_hal_uart_wordlen_t;

/* UART Stop Bits */
typedef enum {
  XY_HAL_UART_STOPBITS_1 = 0, /**< 1 stop bit */
  XY_HAL_UART_STOPBITS_1_5,   /**< 1.5 stop bits */
  XY_HAL_UART_STOPBITS_2,     /**< 2 stop bits */
} xy_hal_uart_stopbits_t;

/* UART Parity */
typedef enum {
  XY_HAL_UART_PARITY_NONE = 0, /**< No parity */
  XY_HAL_UART_PARITY_EVEN,     /**< Even parity */
  XY_HAL_UART_PARITY_ODD,      /**< Odd parity */
} xy_hal_uart_parity_t;

/* UART Flow Control */
typedef enum {
  XY_HAL_UART_FLOWCTRL_NONE = 0, /**< No flow control */
  XY_HAL_UART_FLOWCTRL_RTS,      /**< RTS flow control */
  XY_HAL_UART_FLOWCTRL_CTS,      /**< CTS flow control */
  XY_HAL_UART_FLOWCTRL_RTS_CTS,  /**< RTS/CTS flow control */
} xy_hal_uart_flowctrl_t;

/* UART Mode */
typedef enum {
  XY_HAL_UART_MODE_TX = 0x01,    /**< TX only */
  XY_HAL_UART_MODE_RX = 0x02,    /**< RX only */
  XY_HAL_UART_MODE_TX_RX = 0x03, /**< TX and RX */
} xy_hal_uart_mode_t;

/* UART Configuration Structure */
typedef struct {
  uint32_t baudrate;               /**< Baudrate (e.g., 115200) */
  xy_hal_uart_wordlen_t wordlen;   /**< Word length */
  xy_hal_uart_stopbits_t stopbits; /**< Stop bits */
  xy_hal_uart_parity_t parity;     /**< Parity */
  xy_hal_uart_flowctrl_t flowctrl; /**< Flow control */
  xy_hal_uart_mode_t mode;         /**< UART mode */
} xy_hal_uart_config_t;

/* UART Event Types */
typedef enum {
  XY_HAL_UART_EVENT_RX_DONE = 0, /**< RX complete */
  XY_HAL_UART_EVENT_TX_DONE,     /**< TX complete */
  XY_HAL_UART_EVENT_ERROR,       /**< Error occurred */
} xy_hal_uart_event_t;

/* UART callback */
typedef void (*xy_hal_uart_callback_t)(void *uart, xy_hal_uart_event_t event,
                                       void *arg);

/**
 * @brief Initialize UART
 * @param uart UART instance
 * @param config UART configuration
 * @return XY_HAL_OK on success, XY_HAL_ERROR_INVALID_PARAM if parameters
 * invalid, XY_HAL_ERROR_FAIL if initialization failed
 */
xy_hal_error_t xy_hal_uart_init(void *uart, const xy_hal_uart_config_t *config);

/**
 * @brief Deinitialize UART
 * @param uart UART instance
 * @return XY_HAL_OK on success, XY_HAL_ERROR_INVALID_PARAM if parameters
 * invalid
 */
xy_hal_error_t xy_hal_uart_deinit(void *uart);

/**
 * @brief Send data via UART (blocking)
 * @param uart UART instance
 * @param data Data buffer
 * @param len Data length
 * @param timeout Timeout in milliseconds
 * @return XY_HAL_OK on success, XY_HAL_ERROR_* on error
 */
xy_hal_error_t xy_hal_uart_send(void *uart, const uint8_t *data, size_t len,
                     uint32_t timeout);

/**
 * @brief Receive data via UART (blocking)
 * @param uart UART instance
 * @param data Data buffer
 * @param len Buffer length
 * @param timeout Timeout in milliseconds
 * @return XY_HAL_OK on success, XY_HAL_ERROR_* on error
 */
xy_hal_error_t xy_hal_uart_recv(void *uart, uint8_t *data, size_t len, uint32_t timeout);

/**
 * @brief Send data via UART (non-blocking with DMA)
 * @param uart UART instance
 * @param data Data buffer
 * @param len Data length
 * @return XY_HAL_OK on success, XY_HAL_ERROR_* on error
 */
xy_hal_error_t xy_hal_uart_send_dma(void *uart, const uint8_t *data,
                                    size_t len);

/**
 * @brief Receive data via UART (non-blocking with DMA)
 * @param uart UART instance
 * @param data Data buffer
 * @param len Buffer length
 * @return XY_HAL_OK on success, XY_HAL_ERROR_* on error
 */
xy_hal_error_t xy_hal_uart_recv_dma(void *uart, uint8_t *data, size_t len);

/**
 * @brief Register UART callback
 * @param uart UART instance
 * @param callback Callback function
 * @param arg User argument
 * @return XY_HAL_OK on success, XY_HAL_ERROR_INVALID_PARAM if parameters
 * invalid
 */
xy_hal_error_t xy_hal_uart_register_callback(void *uart,
                                             xy_hal_uart_callback_t callback,
                                             void *arg);

/**
 * @brief Get number of bytes available in RX buffer
 * @param uart UART instance
 * @return Number of bytes available, XY_HAL_ERROR_INVALID_PARAM on error
 */
int xy_hal_uart_available(void *uart);

/**
 * @brief Flush UART buffers
 * @param uart UART instance
 * @return XY_HAL_OK on success, XY_HAL_ERROR_INVALID_PARAM if parameters
 * invalid
 */
xy_hal_error_t xy_hal_uart_flush(void *uart);

/**
 * @brief Control UART error pin (if available)
 * @param uart UART instance
 * @return XY_HAL_OK on success, XY_HAL_ERROR_INVALID_PARAM if parameters
 * invalid
 */
xy_hal_error_t xy_hal_uart_error(void *uart);

/**
 * @brief Set SPI error callback
 * @param uart UART instance
 * @param callback Callback function
 * @param arg User argument
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_uart_set_error_cb(void *uart,
                                        xy_hal_uart_callback_t callback,
                                        void *arg);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_UART_H */