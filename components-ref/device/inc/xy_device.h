#include <stdbool.h>
/**
 * @file xy_device.h
 * @brief XinYi Device Framework - Unified Device Management
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_DEVICE_H
#define XY_DEVICE_H

#include "xy_hal.h"
#include "xy_device_error.h"
#include <stdint.h>
#include <stddef.h>

/* ==================== Configuration ==================== */

#ifndef XY_DEVICE_MAX_COUNT
#define XY_DEVICE_MAX_COUNT 32
#endif

#ifndef XY_DEVICE_NAME_MAX_LEN
#define XY_DEVICE_NAME_MAX_LEN 16
#endif

/* PC platform: include delay functions */
#ifdef HAL_PLATFORM_PC
#include "../hal/PC/xy_hal_pc.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Device Types ==================== */

/**
 * @brief Device type enumeration
 */
typedef enum {
    XY_DEV_TYPE_ADC = 0,        /**< ADC device */
    XY_DEV_TYPE_DAC,            /**< DAC device */
    XY_DEV_TYPE_UART,           /**< UART device */
    XY_DEV_TYPE_SPI,            /**< SPI device */
    XY_DEV_TYPE_I2C,            /**< I2C device */
    XY_DEV_TYPE_GPIO,           /**< GPIO device */
    XY_DEV_TYPE_PWM,            /**< PWM device */
    XY_DEV_TYPE_TIMER,          /**< Timer device */
    XY_DEV_TYPE_RTC,            /**< RTC device */
    XY_DEV_TYPE_WDG,            /**< Watchdog device */
    XY_DEV_TYPE_FLASH,          /**< Flash device */
    XY_DEV_TYPE_SENSOR,         /**< Sensor device */
    XY_DEV_TYPE_STORAGE,        /**< Storage device */
    XY_DEV_TYPE_BUS,            /**< Bus device */
    XY_DEV_TYPE_MISC,           /**< Miscellaneous device */
    XY_DEV_TYPE_I2C_DEVICE,     /**< I2C device wrapper */
    XY_DEV_TYPE_SPI_DEVICE,     /**< SPI device wrapper */
    XY_DEV_TYPE_MAX
} xy_dev_type_t;

/* Type alias for compatibility */
typedef xy_dev_type_t xy_device_type_t;

/* Legacy aliases for compatibility */
#define XY_DEVICE_TYPE_I2C      XY_DEV_TYPE_I2C_DEVICE
#define XY_DEVICE_TYPE_SPI      XY_DEV_TYPE_SPI_DEVICE
#define XY_DEVICE_TYPE_UART     XY_DEV_TYPE_UART
#define XY_DEVICE_TYPE_GPIO     XY_DEV_TYPE_GPIO
#define XY_DEVICE_TYPE_SENSOR   XY_DEV_TYPE_SENSOR
#define XY_DEVICE_TYPE_DISPLAY  XY_DEV_TYPE_MISC
#define XY_DEVICE_TYPE_MEMORY   XY_DEV_TYPE_STORAGE

/**
 * @brief Device state enumeration
 */
typedef enum {
    XY_DEV_STATE_INIT = 0,      /**< Initialized */
    XY_DEV_STATE_READY,         /**< Ready */
    XY_DEV_STATE_OPENED,        /**< Opened */
    XY_DEV_STATE_BUSY,          /**< Busy */
    XY_DEV_STATE_ERROR,         /**< Error */
    XY_DEV_STATE_SUSPENDED,     /**< Suspended */
    XY_DEV_STATE_CLOSED,        /**< Closed */
} xy_dev_state_t;

/**
 * @brief Device flags
 */
typedef enum {
    XY_DEV_FLAG_RDWR      = 0x0001, /**< Read-write */
    XY_DEV_FLAG_RDONLY    = 0x0002, /**< Read-only */
    XY_DEV_FLAG_WRONLY    = 0x0004, /**< Write-only */
    XY_DEV_FLAG_STREAM    = 0x0008, /**< Stream device */
    XY_DEV_FLAG_BLOCK     = 0x0010, /**< Block device */
    XY_DEV_FLAG_INT       = 0x0020, /**< Supports interrupt */
    XY_DEV_FLAG_DMA       = 0x0040, /**< Supports DMA */
    XY_DEV_FLAG_ASYNC     = 0x0080, /**< Supports async operations */
    XY_DEV_FLAG_POLL      = 0x0100, /**< Supports polling */
    XY_DEV_FLAG_EVENT     = 0x0200, /**< Supports events */
    XY_DEV_FLAG_POWER_MGMT = 0x0400, /**< Supports power management */
    XY_DEV_FLAG_REENTRANT  = 0x0800, /**< Reentrant */
    XY_DEV_FLAG_THREAD_SAFE = 0x1000, /**< Thread-safe */
    XY_DEV_FLAG_LOW_POWER  = 0x2000, /**< Low power device */
    XY_DEV_FLAG_MULTI_INST = 0x4000, /**< Multi-instance */
    XY_DEV_FLAG_VIRTUAL    = 0x8000, /**< Virtual device */
} xy_dev_flag_t;

/**
 * @brief Device control commands
 */
typedef enum {
    XY_DEV_CMD_CONFIG = 0,      /**< Configure device */
    XY_DEV_CMD_ENABLE,          /**< Enable device */
    XY_DEV_CMD_DISABLE,         /**< Disable device */
    XY_DEV_CMD_RESET,           /**< Reset device */
    XY_DEV_CMD_GET_INFO,        /**< Get device info */
    XY_DEV_CMD_SET_CALLBACK,    /**< Set callback */
    XY_DEV_CMD_GET_STATE,       /**< Get device state */
    XY_DEV_CMD_SET_POWER,       /**< Set power mode */
    XY_DEV_CMD_GET_POWER,       /**< Get power mode */
    XY_DEV_CMD_SUSPEND,         /**< Suspend device */
    XY_DEV_CMD_RESUME,          /**< Resume device */
    XY_DEV_CMD_LOCK,            /**< Lock device */
    XY_DEV_CMD_UNLOCK,          /**< Unlock device */
    XY_DEV_CMD_FLUSH,           /**< Flush device */
    XY_DEV_CMD_SET_TIMEOUT,     /**< Set timeout */
    XY_DEV_CMD_GET_TIMEOUT,     /**< Get timeout */
    XY_DEV_CMD_SET_BAUDRATE,    /**< Set baudrate */
    XY_DEV_CMD_GET_BAUDRATE,    /**< Get baudrate */
    XY_DEV_CMD_SET_FREQUENCY,   /**< Set frequency */
    XY_DEV_CMD_GET_FREQUENCY,   /**< Get frequency */
    XY_DEV_CMD_SET_DUTY_CYCLE,  /**< Set duty cycle */
    XY_DEV_CMD_GET_DUTY_CYCLE,  /**< Get duty cycle */
    XY_DEV_CMD_SET_MODE,        /**< Set mode */
    XY_DEV_CMD_GET_MODE,        /**< Get mode */
    XY_DEV_CMD_SET_SPEED,       /**< Set speed */
    XY_DEV_CMD_GET_SPEED,       /**< Get speed */
    XY_DEV_CMD_SET_POLARITY,    /**< Set polarity */
    XY_DEV_CMD_GET_POLARITY,    /**< Get polarity */
} xy_dev_cmd_t;

/* ==================== Callback Types ==================== */

/**
 * @brief Async operation callback
 */
typedef void (*xy_async_callback_t)(xy_error_t result, size_t transferred, void *arg);

/**
 * @brief Sensor data callback
 */
typedef void (*xy_sensor_callback_t)(void *data, size_t size, void *arg);

/* ==================== I2C/SPI Device Types ==================== */

/**
 * @brief I2C device handle
 */
/* ==================== Device Structures ==================== */

/**
 * @brief Device API structure (function pointers)
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
    xy_error_t (*power_control)(struct xy_device *dev, uint8_t power_mode);
    xy_error_t (*register_callback)(struct xy_device *dev, 
                                   xy_async_callback_t callback, void *arg);
} xy_dev_api_t;

/**
 * @brief Device structure
 */
typedef struct xy_device {
    const char *name;                 /**< Device name */
    xy_dev_type_t type;               /**< Device type */
    uint32_t flags;                   /**< Device flags */
    xy_dev_state_t state;             /**< Device state */
    const xy_dev_api_t *api;          /**< Driver API structure */
    const void *config;               /**< Device configuration (compile-time) */
    void *data;                       /**< Device private data (runtime) */
    uint8_t ref_count;                /**< Reference count */
    uint8_t power_mode;               /**< Power mode */
    uint8_t initialized;              /**< Initialization flag */
    uint16_t reserved;                /**< Reserved for future use */
    struct xy_device *next;           /**< Linked list pointer */
} xy_device_t;

/* ==================== I2C/SPI Device Types ==================== */

/**
 * @brief I2C device handle
 */
typedef struct xy_i2c_device {
    xy_device_t base;           /**< Base device */
    void *i2c_handle;           /**< I2C handle */
    uint16_t dev_addr;          /**< Device address */
    uint32_t timeout;           /**< Timeout in ms */
} xy_i2c_device_t;

/**
 * @brief SPI device handle
 */
typedef struct xy_spi_device {
    xy_device_t base;           /**< Base device */
    void *spi_handle;           /**< SPI handle */
    void *cs_pin;               /**< Chip select pin (GPIO handle) */
    uint32_t speed;             /**< Speed in Hz */
    uint8_t mode;               /**< SPI mode */
} xy_spi_device_t;

/**
 * @brief UART device handle
 */
typedef struct xy_uart_device {
    xy_device_t base;           /**< Base device */
    void *uart_handle;          /**< UART handle */
    uint32_t baudrate;          /**< Baudrate */
    uint32_t timeout;           /**< Timeout in ms */
} xy_uart_device_t;

/**
 * @brief GPIO device handle
 */
typedef struct xy_gpio_device {
    xy_device_t base;           /**< Base device */
    void *gpio_port;            /**< GPIO port handle */
    uint8_t gpio_pin;           /**< GPIO pin number */
    uint8_t mode;               /**< GPIO mode */
    uint8_t pull;               /**< Pull up/down */
} xy_gpio_device_t;

/* GPIO type aliases */
typedef xy_hal_gpio_mode_t xy_gpio_mode_t;
typedef xy_hal_gpio_pull_t xy_gpio_pull_t;

/* ==================== I2C/SPI/UART/GPIO Device Helpers ==================== */

/* I2C Device */
xy_error_t xy_i2c_device_init(xy_i2c_device_t *dev, void *i2c_handle, uint16_t addr, uint32_t timeout);
xy_error_t xy_i2c_device_read_reg(xy_i2c_device_t *dev, uint8_t reg, uint8_t *data, size_t len);
xy_error_t xy_i2c_device_write_reg(xy_i2c_device_t *dev, uint8_t reg, const uint8_t *data, size_t len);
xy_error_t xy_i2c_device_read(xy_i2c_device_t *dev, uint8_t *data, size_t len);
xy_error_t xy_i2c_device_write(xy_i2c_device_t *dev, const uint8_t *data, size_t len);

/* SPI Device */
xy_error_t xy_spi_device_init(xy_spi_device_t *dev, void *spi_handle, void *cs_pin, uint32_t speed, uint8_t mode);
void xy_spi_device_cs(xy_spi_device_t *dev, bool select);
xy_error_t xy_spi_device_transfer(xy_spi_device_t *dev, const uint8_t *tx, uint8_t *rx, size_t len);

/* UART Device */
xy_error_t xy_uart_device_init(xy_uart_device_t *dev, void *uart_handle, uint32_t baudrate);
xy_error_t xy_uart_device_send(xy_uart_device_t *dev, const uint8_t *data, size_t len);
xy_error_t xy_uart_device_recv(xy_uart_device_t *dev, uint8_t *data, size_t len);
int xy_uart_device_printf(xy_uart_device_t *dev, const char *fmt, ...);

/* GPIO Device */
xy_error_t xy_gpio_device_init(xy_gpio_device_t *dev, void *gpio_port, uint8_t pin, xy_gpio_mode_t mode, xy_gpio_pull_t pull);
xy_error_t xy_gpio_device_write(xy_gpio_device_t *dev, uint8_t value);
xy_error_t xy_gpio_device_read(xy_gpio_device_t *dev, uint8_t *value);
xy_error_t xy_gpio_device_toggle(xy_gpio_device_t *dev);

/**
 * @brief Device information structure
 */
typedef struct {
    const char *name;                 /**< Device name */
    xy_dev_type_t type;               /**< Device type */
    uint32_t flags;                   /**< Device flags */
    xy_dev_state_t state;             /**< Device state */
    uint32_t max_data_size;           /**< Maximum data size */
    uint32_t buffer_size;             /**< Buffer size */
    uint32_t version;                 /**< Driver version */
    uint32_t timestamp;               /**< Last access timestamp */
} xy_dev_info_t;

/**
 * @brief Device statistics structure
 */
typedef struct {
    uint32_t open_count;              /**< Open count */
    uint32_t read_count;              /**< Read count */
    uint32_t write_count;             /**< Write count */
    uint32_t error_count;             /**< Error count */
    uint32_t interrupt_count;         /**< Interrupt count */
    uint32_t dma_count;               /**< DMA count */
    uint32_t async_count;             /**< Async operation count */
    uint32_t timeout_count;           /**< Timeout count */
} xy_dev_stats_t;

/* ==================== Device Framework API ==================== */

/**
 * @brief Initialize device framework
 * @return XY_OK on success, error code on failure
 */
xy_error_t xy_device_init(void);

/**
 * @brief Register a device
 * @param dev Device structure pointer
 * @return XY_OK on success, error code on failure
 */
xy_error_t xy_device_register(xy_device_t *dev);

/**
 * @brief Unregister a device
 * @param dev Device structure pointer
 * @return XY_OK on success, error code on failure
 */
xy_error_t xy_device_unregister(xy_device_t *dev);

/**
 * @brief Find a device by name
 * @param name Device name
 * @return Device pointer, NULL if not found
 */
xy_device_t *xy_device_find(const char *name);

/**
 * @brief Open a device
 * @param name Device name
 * @param flags Open flags
 * @return Device pointer, NULL on failure
 */
xy_device_t *xy_device_open(const char *name, uint32_t flags);

/**
 * @brief Close a device
 * @param dev Device pointer
 * @return XY_OK on success, error code on failure
 */
xy_error_t xy_device_close(xy_device_t *dev);

/**
 * @brief Read from device
 * @param dev Device pointer
 * @param pos Read position
 * @param buf Data buffer
 * @param size Buffer size
 * @return Number of bytes read, negative on error
 */
int32_t xy_device_read(xy_device_t *dev, uint32_t pos, void *buf, size_t size);

/**
 * @brief Write to device
 * @param dev Device pointer
 * @param pos Write position
 * @param buf Data buffer
 * @param size Data size
 * @return Number of bytes written, negative on error
 */
int32_t xy_device_write(xy_device_t *dev, uint32_t pos, const void *buf, size_t size);

/**
 * @brief Control device
 * @param dev Device pointer
 * @param cmd Control command
 * @param args Command arguments
 * @return XY_OK on success, error code on failure
 */
xy_error_t xy_device_control(xy_device_t *dev, uint32_t cmd, void *args);

/**
 * @brief Get device information
 * @param dev Device pointer
 * @param info Device information output
 * @return XY_OK on success, error code on failure
 */
xy_error_t xy_device_get_info(xy_device_t *dev, xy_dev_info_t *info);

/**
 * @brief Get device state
 * @param dev Device pointer
 * @return Device state, negative on error
 */
int32_t xy_device_get_state(xy_device_t *dev);

/**
 * @brief Enumerate devices
 * @param type Device type (XY_DEV_TYPE_MAX for all types)
 * @param names Device name array output
 * @param max_count Maximum number of names to return
 * @return Actual number of devices found
 */
uint32_t xy_device_enumerate(xy_dev_type_t type, const char **names, uint32_t max_count);

/**
 * @brief Get device count
 * @param type Device type (XY_DEV_TYPE_MAX for all types)
 * @return Number of devices
 */
uint32_t xy_device_get_count(xy_dev_type_t type);

/**
 * @brief Set device power mode
 * @param dev Device pointer
 * @param power_mode Power mode
 * @return XY_OK on success, error code on failure
 */
xy_error_t xy_device_set_power_mode(xy_device_t *dev, uint8_t power_mode);

/**
 * @brief Get device power mode
 * @param dev Device pointer
 * @param power_mode Power mode output
 * @return XY_OK on success, error code on failure
 */
xy_error_t xy_device_get_power_mode(xy_device_t *dev, uint8_t *power_mode);

/**
 * @brief Async read from device
 * @param dev Device pointer
 * @param pos Read position
 * @param buf Data buffer
 * @param size Buffer size
 * @param cb Callback function
 * @param arg Callback argument
 * @return XY_OK on success, error code on failure
 */
xy_error_t xy_device_async_read(xy_device_t *dev, uint32_t pos, void *buf, 
                                size_t size, xy_async_callback_t cb, void *arg);

/**
 * @brief Async write to device
 * @param dev Device pointer
 * @param pos Write position
 * @param buf Data buffer
 * @param size Data size
 * @param cb Callback function
 * @param arg Callback argument
 * @return XY_OK on success, error code on failure
 */
xy_error_t xy_device_async_write(xy_device_t *dev, uint32_t pos, const void *buf,
                                 size_t size, xy_async_callback_t cb, void *arg);

/**
 * @brief Check if device exists
 * @param name Device name
 * @return 1 if exists, 0 if not
 */
int xy_device_exists(const char *name);

/**
 * @brief Get device type
 * @param name Device name
 * @return Device type, XY_DEV_TYPE_MAX if not found
 */
xy_dev_type_t xy_device_get_type(const char *name);

/**
 * @brief Get device flags
 * @param name Device name
 * @return Device flags, 0 if not found
 */
uint32_t xy_device_get_flags(const char *name);

/**
 * @brief Check if device is opened
 * @param name Device name
 * @return 1 if opened, 0 if not
 */
int xy_device_is_opened(const char *name);

/**
 * @brief Get reference count
 * @param name Device name
 * @return Reference count, 0 if not found
 */
int xy_device_get_ref_count(const char *name);

/* ==================== Utility Macros ==================== */

/**
 * @brief Device ready check macro
 */
#define XY_DEVICE_READY(dev) \
    ((dev) && (dev)->state == XY_DEV_STATE_OPENED)

/**
 * @brief Device read convenience macro
 */
#define XY_DEVICE_READ(dev, buf, size) \
    xy_device_read(dev, 0, buf, size)

/**
 * @brief Device write convenience macro
 */
#define XY_DEVICE_WRITE(dev, buf, size) \
    xy_device_write(dev, 0, buf, size)

/**
 * @brief Device control convenience macro
 */
#define XY_DEVICE_CONTROL(dev, cmd, args) \
    xy_device_control(dev, cmd, args)

/**
 * @brief Device register convenience macro
 */
#define XY_DEVICE_REGISTER(name, type, init_func, api_ptr, config_ptr) \
    static xy_device_t name##_device = { \
        .name = #name, \
        .type = type, \
        .flags = XY_DEV_FLAG_RDWR, \
        .state = XY_DEV_STATE_INIT, \
        .api = api_ptr, \
        .config = config_ptr, \
        .data = NULL, \
        .ref_count = 0, \
        .power_mode = 0, \
        .reserved = 0, \
        .next = NULL, \
    }; \
    XY_INITIALIZER(xy_register_##name##_device, \
                   XY_INIT_LEVEL_DRIVER, \
                   xy_device_register, &name##_device)

/* ==================== Bus Device Framework ==================== */

/**
 * @brief Bus operation API structure
 */
typedef struct xy_bus_api {
    xy_error_t (*take_bus)(struct xy_device *bus);
    xy_error_t (*release_bus)(struct xy_device *bus);
    xy_error_t (*transfer)(struct xy_device *bus, struct xy_device *node,
                          const void *send_buf, void *recv_buf, size_t length);
    xy_error_t (*configure)(struct xy_device *bus, struct xy_device *node,
                           const void *config);
    xy_error_t (*set_speed)(struct xy_device *bus, struct xy_device *node, uint32_t speed);
    uint32_t (*get_speed)(struct xy_device *bus, struct xy_device *node);
} xy_bus_api_t;

/**
 * @brief Bus node structure (device attached to bus)
 */
typedef struct xy_bus_node {
    xy_device_t parent;              /**< Parent device */
    xy_device_t *bus;                /**< Associated bus */
    uint32_t addr;                   /**< Device address on bus */
    void *node_data;                 /**< Node-specific data */
    uint8_t bus_type;                /**< Bus type identifier */
    uint8_t reserved[3];             /**< Padding */
} xy_bus_node_t;

/**
 * @brief Take bus ownership
 * @param bus Bus device pointer
 * @return XY_OK on success, error code on failure
 */
xy_error_t xy_bus_take(xy_device_t *bus);

/**
 * @brief Release bus ownership
 * @param bus Bus device pointer
 * @return XY_OK on success, error code on failure
 */
xy_error_t xy_bus_release(xy_device_t *bus);

/**
 * @brief Transfer data on bus
 * @param bus Bus device pointer
 * @param node Bus node device pointer
 * @param send_buf Send buffer
 * @param recv_buf Receive buffer
 * @param length Data length
 * @return XY_OK on success, error code on failure
 */
xy_error_t xy_bus_transfer(xy_device_t *bus, xy_bus_node_t *node,
                          const void *send_buf, void *recv_buf, size_t length);

/**
 * @brief Configure bus node
 * @param bus Bus device pointer
 * @param node Bus node device pointer
 * @param config Configuration data
 * @return XY_OK on success, error code on failure
 */
xy_error_t xy_bus_configure(xy_device_t *bus, xy_bus_node_t *node, 
                           const void *config);

/* ==================== Sensor Device Framework ==================== */

/**
 * @brief Sensor types
 */
typedef enum {
    XY_SENSOR_TYPE_TEMP = 0,         /**< Temperature sensor */
    XY_SENSOR_TYPE_HUMIDITY,         /**< Humidity sensor */
    XY_SENSOR_TYPE_PRESSURE,         /**< Pressure sensor */
    XY_SENSOR_TYPE_ACCELEROMETER,    /**< Accelerometer */
    XY_SENSOR_TYPE_GYROSCOPE,        /**< Gyroscope */
    XY_SENSOR_TYPE_MAGNETOMETER,     /**< Magnetometer */
    XY_SENSOR_TYPE_LIGHT,            /**< Light sensor */
    XY_SENSOR_TYPE_PROXIMITY,        /**< Proximity sensor */
    XY_SENSOR_TYPE_GAS,              /**< Gas sensor */
    XY_SENSOR_TYPE_COLOR,            /**< Color sensor */
    XY_SENSOR_TYPE_HALL,             /**< Hall sensor */
    XY_SENSOR_TYPE_IR,               /**< IR sensor */
    XY_SENSOR_TYPE_UV,               /**< UV sensor */
    XY_SENSOR_TYPE_PH,               /**< pH sensor */
    XY_SENSOR_TYPE_EC,               /**< EC sensor */
    XY_SENSOR_TYPE_MAX
} xy_sensor_type_t;

/**
 * @brief Sensor value structure
 */
typedef struct {
    int32_t val1;                    /**< Integer part */
    int32_t val2;                    /**< Fractional part (micro-units) */
} xy_sensor_value_t;

/**
 * @brief Sensor event types
 */
typedef enum {
    XY_SENSOR_EVT_DATA_READY = 0,    /**< Data ready */
    XY_SENSOR_EVT_THRESHOLD,         /**< Threshold event */
    XY_SENSOR_EVT_ERROR,             /**< Error event */
    XY_SENSOR_EVT_CONFIG,            /**< Configuration event */
} xy_sensor_evt_t;

/**
 * @brief Sensor configuration structure
 */
typedef struct {
    xy_sensor_type_t sensor_type;    /**< Sensor type */
    uint32_t sample_rate;            /**< Sample rate (Hz) */
    uint8_t power_mode;              /**< Power mode */
    uint32_t scale;                  /**< Measurement scale */
    uint8_t resolution;              /**< Resolution (bits) */
    uint8_t oversampling;            /**< Oversampling ratio */
    uint8_t filter_enabled;          /**< Filter enabled */
    uint8_t calibration_enabled;     /**< Calibration enabled */
} xy_sensor_config_t;

/**
 * @brief Sensor device API structure
 */
typedef struct xy_sensor_api {
    xy_error_t (*sample_fetch)(struct xy_device *sensor, xy_sensor_type_t channel);
    xy_error_t (*channel_get)(struct xy_device *sensor, xy_sensor_type_t channel, 
                             xy_sensor_value_t *val);
    xy_error_t (*configure)(struct xy_device *sensor, const xy_sensor_config_t *config);
    xy_error_t (*set_threshold)(struct xy_device *sensor, xy_sensor_type_t channel,
                                const xy_sensor_value_t *threshold);
    xy_error_t (*register_callback)(struct xy_device *sensor, 
                                   xy_sensor_callback_t callback, void *arg);
    xy_error_t (*enable_irq)(struct xy_device *sensor, uint8_t irq_type);
    xy_error_t (*disable_irq)(struct xy_device *sensor, uint8_t irq_type);
    xy_error_t (*set_power_mode)(struct xy_device *sensor, uint8_t power_mode);
    xy_error_t (*get_power_mode)(struct xy_device *sensor, uint8_t *power_mode);
} xy_sensor_api_t;

/**
 * @brief Fetch sensor sample
 * @param sensor Sensor device pointer
 * @param channel Channel type
 * @return XY_OK on success, error code on failure
 */
xy_error_t xy_sensor_sample_fetch(xy_device_t *sensor, xy_sensor_type_t channel);

/**
 * @brief Get sensor channel value
 * @param sensor Sensor device pointer
 * @param channel Channel type
 * @param val Value output
 * @return XY_OK on success, error code on failure
 */
xy_error_t xy_sensor_channel_get(xy_device_t *sensor, xy_sensor_type_t channel,
                                 xy_sensor_value_t *val);

/**
 * @brief Configure sensor
 * @param sensor Sensor device pointer
 * @param config Configuration
 * @return XY_OK on success, error code on failure
 */
xy_error_t xy_sensor_configure(xy_device_t *sensor, const xy_sensor_config_t *config);

/* ==================== Common Device Types ==================== */

/**
 * @brief UART device configuration
 */
typedef struct {
    uint32_t baudrate;               /**< Baud rate */
    uint8_t data_bits;               /**< Data bits (5-9) */
    uint8_t stop_bits;               /**< Stop bits (1-2) */
    uint8_t parity;                  /**< Parity (0=none, 1=odd, 2=even) */
    uint8_t flow_ctrl;               /**< Flow control */
    uint8_t mode;                    /**< Mode (TX/RX/both) */
    uint32_t timeout;                /**< Timeout (ms) */
    uint8_t enable_irq;              /**< Enable interrupt */
    uint8_t enable_dma;              /**< Enable DMA */
} xy_uart_dev_config_t;

/**
 * @brief SPI device configuration
 */
typedef struct {
    uint8_t mode;                    /**< SPI mode (0-3) */
    uint8_t direction;               /**< Direction */
    uint8_t datasize;                /**< Data size (8/16 bits) */
    uint8_t bitorder;                /**< Bit order (MSB/LSB first) */
    uint8_t nss_mode;                /**< NSS mode */
    uint32_t baudrate;               /**< Baud rate */
    uint8_t is_master;               /**< Master/slave mode */
    uint8_t enable_irq;              /**< Enable interrupt */
    uint8_t enable_dma;              /**< Enable DMA */
} xy_spi_dev_config_t;

/**
 * @brief I2C device configuration
 */
typedef struct {
    uint32_t clock_speed;            /**< Clock speed (Hz) */
    uint8_t addr_mode;               /**< Address mode (7/10 bit) */
    uint8_t duty_cycle;              /**< Duty cycle */
    uint16_t own_address;            /**< Own address (slave mode) */
    uint8_t general_call_mode;       /**< General call mode */
    uint32_t timeout;                /**< Timeout (ms) */
    uint8_t enable_irq;              /**< Enable interrupt */
    uint8_t enable_dma;              /**< Enable DMA */
} xy_i2c_dev_config_t;

/**
 * @brief GPIO device configuration
 */
typedef struct {
    uint8_t mode;                    /**< GPIO mode */
    uint8_t pull;                    /**< Pull mode */
    uint8_t otype;                   /**< Output type */
    uint8_t speed;                   /**< Speed */
    uint8_t alternate;               /**< Alternate function */
    uint8_t enable_irq;              /**< Enable interrupt */
} xy_gpio_dev_config_t;

/**
 * @brief ADC device configuration
 */
typedef struct {
    uint8_t resolution;              /**< Resolution (8/10/12/16 bits) */
    uint8_t align;                   /**< Data alignment */
    uint8_t scan_mode;               /**< Scan mode */
    uint8_t continuous;              /**< Continuous mode */
    uint8_t trigger_src;             /**< Trigger source */
    uint32_t clock_div;              /**< Clock divider */
    uint32_t sampling_time;          /**< Sampling time */
    uint8_t enable_irq;              /**< Enable interrupt */
    uint8_t enable_dma;              /**< Enable DMA */
} xy_adc_dev_config_t;

/* ==================== Device Manager ==================== */

/**
 * @brief Device manager structure
 */
typedef struct {
    xy_device_t **devices;        /**< Device array (dynamically allocated) */
    uint32_t count;
    uint32_t max_count;
    void *lock;
} xy_device_manager_t;

/**
 * @brief Initialize device manager
 * @return XY_OK on success, error code on failure
 */
xy_error_t xy_device_manager_init(xy_device_manager_t *mgr, size_t max_count);

/**
 * @brief Register device with group
 * @param group Device group
 * @param dev Device pointer
 * @return XY_OK on success, error code on failure
 */
xy_error_t xy_device_manager_register(xy_device_manager_t *mgr, xy_device_t *dev);

/**
 * @brief Find device by group and index
 * @param group Device group
 * @param index Device index in group
 * @return Device pointer, NULL if not found
 */
xy_device_t *xy_device_manager_find_by_group(const char *group, int index);

/**
 * @brief Get device count in group
 * @param group Device group
 * @return Device count in group
 */
uint32_t xy_device_manager_get_group_count(const char *group);

/**
 * @brief Enumerate devices in group
 * @param group Device group
 * @param names Device name array output
 * @param max_count Maximum number of names to return
 * @return Actual number of devices found
 */
uint32_t xy_device_manager_enumerate_group(const char *group, const char **names, 
                                          uint32_t max_count);

/* ==================== Error Code Definitions ==================== */

/**
 * @brief Device-specific error codes
 */
typedef enum {
    XY_DEVICE_ERROR_BASE = -100,
    XY_DEVICE_ERROR_INVALID_HANDLE = -101,  /**< Invalid device handle */
    XY_DEVICE_ERROR_INVALID_MODE = -102,    /**< Invalid device mode */
    XY_DEVICE_ERROR_INVALID_CONFIG = -103,  /**< Invalid configuration */
    XY_DEVICE_ERROR_PIN_CONFLICT = -104,    /**< Pin conflict */
    XY_DEVICE_ERROR_CLOCK_ERROR = -105,     /**< Clock error */
    XY_DEVICE_ERROR_PERIPH_ERROR = -106,    /**< Peripheral error */
    XY_DEVICE_ERROR_DMA_ERROR = -107,       /**< DMA error */
    XY_DEVICE_ERROR_IRQ_ERROR = -108,       /**< IRQ error */
    XY_DEVICE_ERROR_GPIO_ERROR = -109,      /**< GPIO error */
    XY_DEVICE_ERROR_UART_ERROR = -110,      /**< UART error */
    XY_DEVICE_ERROR_SPI_ERROR = -111,       /**< SPI error */
    XY_DEVICE_ERROR_I2C_ERROR = -112,       /**< I2C error */
    XY_DEVICE_ERROR_TIMER_ERROR = -113,     /**< Timer error */
    XY_DEVICE_ERROR_ADC_ERROR = -114,       /**< ADC error */
    XY_DEVICE_ERROR_DAC_ERROR = -115,       /**< DAC error */
    XY_DEVICE_ERROR_RTC_ERROR = -116,       /**< RTC error */
    XY_DEVICE_ERROR_WDG_ERROR = -117,       /**< Watchdog error */
    XY_DEVICE_ERROR_FLASH_ERROR = -118,     /**< Flash error */
    XY_DEVICE_ERROR_CRC_ERROR = -119,       /**< CRC error */
    XY_DEVICE_ERROR_RNG_ERROR = -120,       /**< RNG error */
    XY_DEVICE_ERROR_I2S_ERROR = -121,       /**< I2S error */
    XY_DEVICE_ERROR_CAN_ERROR = -122,       /**< CAN error */
    XY_DEVICE_ERROR_SDMMC_ERROR = -123,     /**< SDMMC error */
    XY_DEVICE_ERROR_ETH_ERROR = -124,       /**< Ethernet error */
    XY_DEVICE_ERROR_USB_ERROR = -125,       /**< USB error */
    XY_DEVICE_ERROR_LCD_ERROR = -126,       /**< LCD error */
    XY_DEVICE_ERROR_TOUCH_ERROR = -127,     /**< Touch error */
    XY_DEVICE_ERROR_SENSOR_ERROR = -128,    /**< Sensor error */
    XY_DEVICE_ERROR_END = -199,
} xy_device_error_codes_t;

#ifdef __cplusplus
}
#endif

#endif /* XY_DEVICE_H */
