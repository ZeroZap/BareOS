/**
 * @file xy_hal_spi_device.h
 * @brief HC32L021 SPI Device Driver Header
 * @version 1.0.0
 * @date 2026-03-22
 */

#ifndef XY_HAL_SPI_DEVICE_H
#define XY_HAL_SPI_DEVICE_H

#include "xy_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

// SPI modes
#define XY_SPI_MODE_0           (0x00)  // CPOL=0, CPHA=0
#define XY_SPI_MODE_1           (0x01)  // CPOL=0, CPHA=1
#define XY_SPI_MODE_2           (0x02)  // CPOL=1, CPHA=0
#define XY_SPI_MODE_3           (0x03)  // CPOL=1, CPHA=1

// SPI data bits
#define XY_SPI_DATA_BITS_8      (8)
#define XY_SPI_DATA_BITS_16     (16)

// SPI master/slave
#define XY_SPI_MASTER           (1)
#define XY_SPI_SLAVE            (0)

// SPI events
#define XY_SPI_EVENT_TRANSFER_COMPLETE  (1)
#define XY_SPI_EVENT_ERROR              (2)

// SPI device configuration structure
typedef struct {
    uint8_t mode;               // SPI mode (0-3)
    uint8_t data_bits;          // Data bits (8 or 16)
    uint32_t baud_rate;         // Baud rate in Hz
    uint8_t master_slave;       // Master (1) or Slave (0)
    uint8_t cs_active_low;      // Chip select active low (1) or high (0)
} xy_spi_config_t;

// SPI callback function type
typedef void (*xy_spi_callback_t)(struct xy_spi_device *device, uint32_t event);

// SPI device structure
typedef struct xy_spi_device {
    uint8_t device_id;          // SPI0, SPI1, etc.
    xy_spi_config_t config;     // SPI configuration
    xy_spi_callback_t callback; // Optional callback function
    void *user_data;           // User data for callback
} xy_spi_device_t;

// Function prototypes
int xy_hal_spi_init(xy_spi_device_t *device);
int xy_hal_spi_deinit(xy_spi_device_t *device);
int xy_hal_spi_send(xy_spi_device_t *device, const void *data, size_t length);
int xy_hal_spi_receive(xy_spi_device_t *device, void *buffer, size_t length);
int xy_hal_spi_transfer(xy_spi_device_t *device, const void *tx_data, void *rx_data, size_t length);

#ifdef __cplusplus
}
#endif

#endif // XY_HAL_SPI_DEVICE_H