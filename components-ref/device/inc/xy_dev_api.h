/**
 * @file xy_dev_api.h
 * @brief XinYi Device API Definitions
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_DEV_API_H
#define XY_DEV_API_H

#include "xy_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Common Device API ==================== */

/**
 * @brief Device API function pointers structure
 */
typedef struct xy_dev_api {
    xy_error_t (*init)(struct xy_device *dev, const void *config);
    xy_error_t (*deinit)(struct xy_device *dev);
    xy_error_t (*open)(struct xy_device *dev, uint32_t flags);
    xy_error_t (*close)(struct xy_device *dev);
    int32_t (*read)(struct xy_device *dev, uint32_t pos, void *buf, size_t size);
    int32_t (*write)(struct xy_device *dev, uint32_t pos, const void *buf, size_t size);
    xy_error_t (*control)(struct xy_device *dev, uint32_t cmd, void *args);
    xy_error_t (*async_read)(struct xy_device *dev, uint32_t pos, void *buf, 
                            size_t size, xy_async_callback_t cb, void *arg);
    xy_error_t (*async_write)(struct xy_device *dev, uint32_t pos, const void *buf,
                             size_t size, xy_async_callback_t cb, void *arg);
    xy_error_t (*ioctl)(struct xy_device *dev, uint32_t cmd, void *args);
} xy_dev_api_t;

/**
 * @brief Device control commands
 */
typedef enum {
    XY_DEV_CMD_CONFIG = 0,          /**< Configure device */
    XY_DEV_CMD_ENABLE,              /**< Enable device */
    XY_DEV_CMD_DISABLE,             /**< Disable device */
    XY_DEV_CMD_RESET,               /**< Reset device */
    XY_DEV_CMD_GET_INFO,            /**< Get device info */
    XY_DEV_CMD_SET_CALLBACK,        /**< Set callback */
    XY_DEV_CMD_GET_STATE,           /**< Get device state */
    XY_DEV_CMD_SET_POWER,           /**< Set power mode */
    XY_DEV_CMD_GET_POWER,           /**< Get power mode */
    XY_DEV_CMD_SUSPEND,             /**< Suspend device */
    XY_DEV_CMD_RESUME,              /**< Resume device */
} xy_dev_cmd_t;

/* ==================== UART Device API ==================== */

/**
 * @brief UART configuration structure
 */
typedef struct {
    uint32_t baudrate;              /**< Baud rate */
    uint8_t wordlen;                /**< Word length (8/9 bits) */
    uint8_t stopbits;               /**< Stop bits (1/2) */
    uint8_t parity;                 /**< Parity (0=none, 1=odd, 2=even) */
    uint8_t flowctrl;               /**< Flow control (0=none, 1=RTS, 2=CTS, 3=both) */
    uint8_t mode;                   /**< Mode (TX/RX/both) */
} xy_uart_config_t;

/**
 * @brief UART device API structure
 */
typedef struct {
    xy_error_t (*init)(struct xy_device *dev, const xy_uart_config_t *config);
    xy_error_t (*deinit)(struct xy_device *dev);
    int32_t (*send)(struct xy_device *dev, const uint8_t *data, size_t len, uint32_t timeout);
    int32_t (*recv)(struct xy_device *dev, uint8_t *data, size_t len, uint32_t timeout);
    xy_error_t (*flush)(struct xy_device *dev);
    xy_error_t (*set_baudrate)(struct xy_device *dev, uint32_t baudrate);
    uint32_t (*get_baudrate)(struct xy_device *dev);
    xy_error_t (*enable_irq)(struct xy_device *dev, uint8_t irq_type);
    xy_error_t (*disable_irq)(struct xy_device *dev, uint8_t irq_type);
    xy_error_t (*async_send)(struct xy_device *dev, const uint8_t *data, size_t len,
                            xy_async_callback_t cb, void *arg);
    xy_error_t (*async_recv)(struct xy_device *dev, uint8_t *data, size_t len,
                            xy_async_callback_t cb, void *arg);
} xy_uart_api_t;

/**
 * @brief UART control commands
 */
typedef enum {
    XY_UART_CMD_SET_CONFIG = 0,     /**< Set configuration */
    XY_UART_CMD_GET_CONFIG,         /**< Get configuration */
    XY_UART_CMD_FLUSH,              /**< Flush buffers */
    XY_UART_CMD_SET_BAUDRATE,       /**< Set baudrate */
    XY_UART_CMD_GET_BAUDRATE,       /**< Get baudrate */
    XY_UART_CMD_SET_CALLBACK,       /**< Set callback */
    XY_UART_CMD_GET_STATE,          /**< Get state */
    XY_UART_CMD_RESET,              /**< Reset UART */
    XY_UART_CMD_ENABLE_RX,          /**< Enable RX */
    XY_UART_CMD_DISABLE_RX,         /**< Disable RX */
    XY_UART_CMD_ENABLE_TX,          /**< Enable TX */
    XY_UART_CMD_DISABLE_TX,         /**< Disable TX */
} xy_uart_cmd_t;

/* ==================== SPI Device API ==================== */

/**
 * @brief SPI mode
 */
typedef enum {
    XY_SPI_MODE_0 = 0,              /**< CPOL=0, CPHA=0 */
    XY_SPI_MODE_1,                  /**< CPOL=0, CPHA=1 */
    XY_SPI_MODE_2,                  /**< CPOL=1, CPHA=0 */
    XY_SPI_MODE_3,                  /**< CPOL=1, CPHA=1 */
} xy_spi_mode_t;

/**
 * @brief SPI direction
 */
typedef enum {
    XY_SPI_DIR_2LINES = 0,          /**< Full duplex */
    XY_SPI_DIR_2LINES_RXONLY,       /**< 2 lines rx only */
    XY_SPI_DIR_1LINE,               /**< 1 line */
} xy_spi_direction_t;

/**
 * @brief SPI data size
 */
typedef enum {
    XY_SPI_DATASIZE_4B = 3,         /**< 4 bits */
    XY_SPI_DATASIZE_5B = 4,         /**< 5 bits */
    XY_SPI_DATASIZE_6B = 5,         /**< 6 bits */
    XY_SPI_DATASIZE_7B = 6,         /**< 7 bits */
    XY_SPI_DATASIZE_8B = 7,         /**< 8 bits */
    XY_SPI_DATASIZE_9B = 8,         /**< 9 bits */
    XY_SPI_DATASIZE_10B = 9,        /**< 10 bits */
    XY_SPI_DATASIZE_11B = 10,       /**< 11 bits */
    XY_SPI_DATASIZE_12B = 11,       /**< 12 bits */
    XY_SPI_DATASIZE_13B = 12,       /**< 13 bits */
    XY_SPI_DATASIZE_14B = 13,       /**< 14 bits */
    XY_SPI_DATASIZE_15B = 14,       /**< 15 bits */
    XY_SPI_DATASIZE_16B = 15,       /**< 16 bits */
} xy_spi_datasize_t;

/**
 * @brief SPI bit order
 */
typedef enum {
    XY_SPI_MSB_FIRST = 0,           /**< MSB first */
    XY_SPI_LSB_FIRST,               /**< LSB first */
} xy_spi_bitorder_t;

/**
 * @brief SPI configuration structure
 */
typedef struct {
    xy_spi_mode_t mode;              /**< SPI mode */
    xy_spi_direction_t direction;    /**< Direction */
    xy_spi_datasize_t datasize;      /**< Data size */
    xy_spi_bitorder_t bitorder;      /**< Bit order */
    uint8_t nss_mode;               /**< NSS mode */
    uint32_t baudrate;              /**< Baud rate */
    uint8_t is_master;              /**< Is master */
} xy_spi_config_t;

/**
 * @brief SPI device API structure
 */
typedef struct {
    xy_error_t (*init)(struct xy_device *dev, const xy_spi_config_t *config);
    xy_error_t (*deinit)(struct xy_device *dev);
    int32_t (*transfer)(struct xy_device *dev, const uint8_t *tx_data,
                       uint8_t *rx_data, size_t size, uint32_t timeout);
    xy_error_t (*set_speed)(struct xy_device *dev, uint32_t speed);
    uint32_t (*get_speed)(struct xy_device *dev);
    xy_error_t (*set_mode)(struct xy_device *dev, xy_spi_mode_t mode);
    xy_error_t (*async_transfer)(struct xy_device *dev, const uint8_t *tx_data,
                                uint8_t *rx_data, size_t size,
                                xy_async_callback_t cb, void *arg);
} xy_spi_api_t;

/* ==================== I2C Device API ==================== */

/**
 * @brief I2C addressing mode
 */
typedef enum {
    XY_I2C_ADDR_7BIT = 0,           /**< 7-bit addressing */
    XY_I2C_ADDR_10BIT,              /**< 10-bit addressing */
} xy_i2c_addr_mode_t;

/**
 * @brief I2C duty cycle
 */
typedef enum {
    XY_I2C_DUTY_2 = 0,              /**< Tlow/Thigh = 2 */
    XY_I2C_DUTY_16_9,               /**< Tlow/Thigh = 16/9 */
} xy_i2c_duty_t;

/**
 * @brief I2C configuration structure
 */
typedef struct {
    uint32_t clock_speed;           /**< Clock speed in Hz */
    xy_i2c_addr_mode_t addr_mode;   /**< Addressing mode */
    xy_i2c_duty_t duty_cycle;       /**< Duty cycle */
    uint16_t own_address;           /**< Own address (slave mode) */
    uint8_t general_call_mode;      /**< General call mode */
} xy_i2c_config_t;

/**
 * @brief I2C device API structure
 */
typedef struct {
    xy_error_t (*init)(struct xy_device *dev, const xy_i2c_config_t *config);
    xy_error_t (*deinit)(struct xy_device *dev);
    xy_error_t (*master_send)(struct xy_device *dev, uint16_t dev_addr,
                             const uint8_t *data, size_t len, uint32_t timeout);
    xy_error_t (*master_recv)(struct xy_device *dev, uint16_t dev_addr,
                             uint8_t *data, size_t len, uint32_t timeout);
    xy_error_t (*mem_write)(struct xy_device *dev, uint16_t dev_addr,
                           uint16_t mem_addr, const uint8_t *data, size_t len,
                           uint32_t timeout);
    xy_error_t (*mem_read)(struct xy_device *dev, uint16_t dev_addr,
                          uint16_t mem_addr, uint8_t *data, size_t len,
                          uint32_t timeout);
    xy_error_t (*is_device_ready)(struct xy_device *dev, uint16_t dev_addr,
                                  uint32_t trials, uint32_t timeout);
    xy_error_t (*async_transfer)(struct xy_device *dev, uint16_t dev_addr,
                                const uint8_t *tx_data, uint8_t *rx_data, size_t size,
                                xy_async_callback_t cb, void *arg);
} xy_i2c_api_t;

/* ==================== GPIO Device API ==================== */

/**
 * @brief GPIO mode
 */
typedef enum {
    XY_GPIO_MODE_INPUT = 0,         /**< Input mode */
    XY_GPIO_MODE_OUTPUT,            /**< Output mode */
    XY_GPIO_MODE_AF,                /**< Alternate function */
    XY_GPIO_MODE_ANALOG,            /**< Analog mode */
} xy_gpio_mode_t;

/**
 * @brief GPIO pull
 */
typedef enum {
    XY_GPIO_PULL_NONE = 0,          /**< No pull */
    XY_GPIO_PULL_UP,                /**< Pull up */
    XY_GPIO_PULL_DOWN,              /**< Pull down */
} xy_gpio_pull_t;

/**
 * @brief GPIO output type
 */
typedef enum {
    XY_GPIO_OTYPE_PP = 0,           /**< Push-pull */
    XY_GPIO_OTYPE_OD,               /**< Open-drain */
} xy_gpio_otype_t;

/**
 * @brief GPIO speed
 */
typedef enum {
    XY_GPIO_SPEED_LOW = 0,          /**< Low speed */
    XY_GPIO_SPEED_MEDIUM,           /**< Medium speed */
    XY_GPIO_SPEED_HIGH,             /**< High speed */
    XY_GPIO_SPEED_VERY_HIGH,        /**< Very high speed */
} xy_gpio_speed_t;

/**
 * @brief GPIO configuration structure
 */
typedef struct {
    xy_gpio_mode_t mode;             /**< Mode */
    xy_gpio_pull_t pull;             /**< Pull */
    xy_gpio_otype_t otype;           /**< Output type */
    xy_gpio_speed_t speed;           /**< Speed */
    uint8_t alternate;               /**< Alternate function */
} xy_gpio_config_t;

/**
 * @brief GPIO device API structure
 */
typedef struct {
    xy_error_t (*init)(struct xy_device *dev, const xy_gpio_config_t *config);
    xy_error_t (*deinit)(struct xy_device *dev);
    xy_error_t (*write)(struct xy_device *dev, uint8_t pin, uint8_t value);
    int32_t (*read)(struct xy_device *dev, uint8_t pin);
    xy_error_t (*toggle)(struct xy_device *dev, uint8_t pin);
    xy_error_t (*set_mode)(struct xy_device *dev, uint8_t pin, xy_gpio_mode_t mode);
    xy_error_t (*attach_irq)(struct xy_device *dev, uint8_t pin, 
                            xy_gpio_irq_mode_t mode, xy_gpio_irq_handler_t handler, void *arg);
    xy_error_t (*detach_irq)(struct xy_device *dev, uint8_t pin);
} xy_gpio_api_t;

/* ==================== ADC Device API ==================== */

/**
 * @brief ADC resolution
 */
typedef enum {
    XY_ADC_RESOLUTION_6B = 0,       /**< 6-bit resolution */
    XY_ADC_RESOLUTION_8B,           /**< 8-bit resolution */
    XY_ADC_RESOLUTION_10B,          /**< 10-bit resolution */
    XY_ADC_RESOLUTION_12B,          /**< 12-bit resolution */
    XY_ADC_RESOLUTION_14B,          /**< 14-bit resolution */
    XY_ADC_RESOLUTION_16B,          /**< 16-bit resolution */
} xy_adc_resolution_t;

/**
 * @brief ADC configuration structure
 */
typedef struct {
    xy_adc_resolution_t resolution;  /**< Resolution */
    uint8_t data_align;              /**< Data alignment */
    uint8_t scan_mode;               /**< Scan mode */
    uint8_t continuous;              /**< Continuous conversion */
    uint32_t clock_div;              /**< Clock divider */
    uint32_t sampling_time;          /**< Sampling time */
} xy_adc_config_t;

/**
 * @brief ADC channel configuration
 */
typedef struct {
    uint8_t channel;                 /**< Channel number */
    uint8_t rank;                    /**< Conversion rank */
    uint32_t sampling_time;          /**< Sampling time */
} xy_adc_channel_t;

/**
 * @brief ADC device API structure
 */
typedef struct {
    xy_error_t (*init)(struct xy_device *dev, const xy_adc_config_t *config);
    xy_error_t (*deinit)(struct xy_device *dev);
    xy_error_t (*config_channels)(struct xy_device *dev, 
                                  const xy_adc_channel_t *channels, size_t count);
    int32_t (*read)(struct xy_device *dev, uint8_t channel, uint32_t timeout);
    xy_error_t (*start_conversion)(struct xy_device *dev);
    xy_error_t (*stop_conversion)(struct xy_device *dev);
    xy_error_t (*async_read)(struct xy_device *dev, uint8_t channel,
                            xy_async_callback_t cb, void *arg);
} xy_adc_api_t;

/* ==================== Device Info Structures ==================== */

/**
 * @brief Device information
 */
typedef struct {
    const char *name;                /**< Device name */
    xy_dev_type_t type;              /**< Device type */
    uint32_t flags;                  /**< Device flags */
    xy_dev_state_t state;            /**< Device state */
    uint32_t max_data_size;          /**< Maximum data size */
    uint32_t buffer_size;            /**< Buffer size */
    uint32_t version;                /**< Driver version */
} xy_dev_info_t;

/**
 * @brief Device statistics
 */
typedef struct {
    uint32_t open_count;             /**< Open count */
    uint32_t read_count;             /**< Read count */
    uint32_t write_count;            /**< Write count */
    uint32_t error_count;            /**< Error count */
    uint32_t interrupt_count;        /**< Interrupt count */
    uint32_t dma_count;              /**< DMA count */
} xy_dev_stats_t;

#ifdef __cplusplus
}
#endif

#endif /* XY_DEV_API_H */
