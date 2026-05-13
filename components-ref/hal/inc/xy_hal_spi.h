/**
 * @file xy_hal_spi.h
 * @brief SPI Hardware Abstraction Layer
 * @version 2.0
 * @date 2025-10-26
 */

#ifndef XY_HAL_SPI_H
#define XY_HAL_SPI_H
#include "xy_hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_hal.h"
#include <stddef.h>
#include <stdint.h>

/* SPI Mode */
typedef enum {
  XY_HAL_SPI_MODE_0 = 0, /**< CPOL=0, CPHA=0 */
  XY_HAL_SPI_MODE_1,     /**< CPOL=0, CPHA=1 */
  XY_HAL_SPI_MODE_2,     /**< CPOL=1, CPHA=0 */
  XY_HAL_SPI_MODE_3,     /**< CPOL=1, CPHA=1 */
} xy_hal_spi_mode_t;

/* SPI Data Size */
typedef enum {
  XY_HAL_SPI_DATASIZE_8BIT = 0, /**< 8-bit data */
  XY_HAL_SPI_DATASIZE_16BIT,    /**< 16-bit data */
} xy_hal_spi_datasize_t;

/* SPI Bit Order */
typedef enum {
  XY_HAL_SPI_FIRSTBIT_MSB = 0, /**< MSB first */
  XY_HAL_SPI_FIRSTBIT_LSB,     /**< LSB first */
} xy_hal_spi_firstbit_t;

/* SPI NSS (Chip Select) Mode */
typedef enum {
  XY_HAL_SPI_NSS_SOFT = 0,    /**< Software NSS */
  XY_HAL_SPI_NSS_HARD_INPUT,  /**< Hardware NSS input */
  XY_HAL_SPI_NSS_HARD_OUTPUT, /**< Hardware NSS output */
} xy_hal_spi_nss_t;

/* SPI Direction */
typedef enum {
  XY_HAL_SPI_DIR_2LINES = 0,    /**< Full duplex */
  XY_HAL_SPI_DIR_2LINES_RXONLY, /**< Full duplex RX only */
  XY_HAL_SPI_DIR_1LINE,         /**< Half duplex */
} xy_hal_spi_direction_t;

/* SPI Configuration Structure */
typedef struct {
  xy_hal_spi_mode_t mode;           /**< SPI mode */
  xy_hal_spi_direction_t direction; /**< Direction */
  xy_hal_spi_datasize_t datasize;   /**< Data size */
  xy_hal_spi_firstbit_t firstbit;   /**< Bit order */
  xy_hal_spi_nss_t nss;             /**< NSS mode */
  uint32_t baudrate_prescaler;      /**< Baudrate prescaler */
  uint8_t is_master;                /**< 1=Master, 0=Slave */
} xy_hal_spi_config_t;

/* SPI Event Types */
typedef enum {
  XY_HAL_SPI_EVENT_TX_DONE = 0, /**< TX complete */
  XY_HAL_SPI_EVENT_RX_DONE,     /**< RX complete */
  XY_HAL_SPI_EVENT_TX_RX_DONE,  /**< TX/RX complete */
  XY_HAL_SPI_EVENT_ERROR,       /**< Error occurred */
} xy_hal_spi_event_t;

/* SPI callback */
typedef void (*xy_hal_spi_callback_t)(void *spi, xy_hal_spi_event_t event,
                                      void *arg);

/**
 * @brief Initialize SPI
 * @param spi SPI instance
 * @param config SPI configuration
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_spi_init(void *spi, const xy_hal_spi_config_t *config);

/**
 * @brief Deinitialize SPI
 * @param spi SPI instance
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_spi_deinit(void *spi);

/**
 * @brief Transmit data via SPI (blocking)
 * @param spi SPI instance
 * @param data Data buffer
 * @param len Data length
 * @param timeout Timeout in milliseconds
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_spi_transmit(void *spi, const uint8_t *data, size_t len,
                                   uint32_t timeout);

/**
 * @brief Receive data via SPI (blocking)
 * @param spi SPI instance
 * @param data Data buffer
 * @param len Buffer length
 * @param timeout Timeout in milliseconds
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_spi_receive(void *spi, uint8_t *data, size_t len,
                                  uint32_t timeout);

/**
 * @brief Transmit and receive data via SPI (blocking)
 * @param spi SPI instance
 * @param tx_data TX data buffer
 * @param rx_data RX data buffer
 * @param len Data length
 * @param timeout Timeout in milliseconds
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_spi_transmit_receive(void *spi, const uint8_t *tx_data,
                                           uint8_t *rx_data, size_t len,
                                           uint32_t timeout);

/**
 * @brief Transmit data via SPI (non-blocking with DMA)
 * @param spi SPI instance
 * @param data Data buffer
 * @param len Data length
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_spi_transmit_dma(void *spi, const uint8_t *data,
                                       size_t len);

/**
 * @brief Receive data via SPI (non-blocking with DMA)
 * @param spi SPI instance
 * @param data Data buffer
 * @param len Buffer length
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_spi_receive_dma(void *spi, uint8_t *data, size_t len);

/**
 * @brief Transmit and receive data via SPI (non-blocking with DMA)
 * @param spi SPI instance
 * @param tx_data TX data buffer
 * @param rx_data RX data buffer
 * @param len Data length
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_spi_transmit_receive_dma(void *spi,
                                               const uint8_t *tx_data,
                                               uint8_t *rx_data, size_t len);

/**
 * @brief Register SPI callback
 * @param spi SPI instance
 * @param callback Callback function
 * @param arg User argument
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_spi_register_callback(void *spi,
                                            xy_hal_spi_callback_t callback,
                                            void *arg);

/**
 * @brief Control CS pin (software NSS mode)
 * @param spi SPI instance
 * @param level 0=Low, 1=High
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_spi_set_cs(void *spi, uint8_t level);

/**
 * @brief Control SPI error pin (if available)
 * @param spi SPI instance
 * @param level 0=Low, 1=High
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_spi_error(void *spi, uint8_t level);

/**
 * @brief Set SPI error callback
 * @param spi SPI instance
 * @param callback Callback function
 * @param arg User argument
 * @return 0 on success, negative on error
 */
xy_hal_error_t
xy_hal_spi_set_error_cb(void *spi, xy_hal_spi_callback_t callback, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_SPI_H */