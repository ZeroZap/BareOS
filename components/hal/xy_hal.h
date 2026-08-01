/**
 * @file xy_hal.h
 * @brief BareOS hardware abstraction boundary.
 *
 * The HAL keeps BareOS components independent from vendor SDKs. MCU ports
 * implement these functions by wrapping N32, STM32, CH32, or other vendor
 * peripheral libraries.
 */

#ifndef XY_BAREOS_HAL_H
#define XY_BAREOS_HAL_H

#include <stddef.h>
#include <stdint.h>
#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) && !defined(_WIN32) && !defined(PC_BUILD)
#define XY_HAL_WEAK __attribute__((weak))
#else
#define XY_HAL_WEAK
#endif

typedef enum {
    XY_HAL_OK            = 0,
    XY_HAL_ERROR         = -1,
    XY_HAL_BUSY          = -2,
    XY_HAL_TIMEOUT       = -3,
    XY_HAL_INVALID_PARAM = -4,
    XY_HAL_NOT_SUPPORTED = -5,
} xy_hal_status_t;

typedef enum {
    XY_HAL_IO_POLL = 0,
    XY_HAL_IO_IRQ,
    XY_HAL_IO_DMA,
} xy_hal_io_mode_t;

typedef enum {
    XY_HAL_EVENT_RX_READY = 1,
    XY_HAL_EVENT_RX_IDLE,
    XY_HAL_EVENT_TX_DONE,
    XY_HAL_EVENT_DMA_HALF,
    XY_HAL_EVENT_DMA_DONE,
    XY_HAL_EVENT_ERROR,
} xy_hal_event_t;

typedef void (*xy_hal_event_cb_t)(void *dev, xy_hal_event_t event, void *user);
typedef void (*xy_hal_gpio_irq_cb_t)(uint32_t pin, void *user);
typedef void (*xy_hal_timer_cb_t)(void *timer, void *user);

/* Time / tick ----------------------------------------------------------- */

uint32_t xy_hal_time_ms(void);
uint32_t xy_hal_time_s(void);
void xy_hal_delay_ms(uint32_t ms);
int xy_hal_systick_suspend(void);
int xy_hal_systick_resume(void);
bool xy_hal_systick_is_running(void);

/* IRQ / critical section ------------------------------------------------ */

uint32_t xy_hal_irq_save(void);
void xy_hal_irq_restore(uint32_t key);
void xy_hal_irq_enable(void);
void xy_hal_irq_disable(void);

/* GPIO ------------------------------------------------------------------ */

typedef enum {
    XY_HAL_GPIO_INPUT = 0,
    XY_HAL_GPIO_OUTPUT,
    XY_HAL_GPIO_ANALOG,
    XY_HAL_GPIO_ALT,
} xy_hal_gpio_mode_t;

typedef enum {
    XY_HAL_GPIO_NOPULL = 0,
    XY_HAL_GPIO_PULLUP,
    XY_HAL_GPIO_PULLDOWN,
} xy_hal_gpio_pull_t;

typedef enum {
    XY_HAL_GPIO_PUSH_PULL = 0,
    XY_HAL_GPIO_OPEN_DRAIN,
} xy_hal_gpio_output_t;

typedef enum {
    XY_HAL_GPIO_SPEED_LOW = 0,
    XY_HAL_GPIO_SPEED_MEDIUM,
    XY_HAL_GPIO_SPEED_HIGH,
    XY_HAL_GPIO_SPEED_VERY_HIGH,
} xy_hal_gpio_speed_t;

typedef enum {
    XY_HAL_GPIO_IRQ_NONE = 0,
    XY_HAL_GPIO_IRQ_RISING,
    XY_HAL_GPIO_IRQ_FALLING,
    XY_HAL_GPIO_IRQ_BOTH,
    XY_HAL_GPIO_IRQ_LOW_LEVEL,
    XY_HAL_GPIO_IRQ_HIGH_LEVEL,
} xy_hal_gpio_irq_trigger_t;

typedef struct {
    uint32_t pin;              /* MCU-port-defined pin ID. */
    xy_hal_gpio_mode_t mode;
    xy_hal_gpio_pull_t pull;
    xy_hal_gpio_output_t output_type;
    xy_hal_gpio_speed_t speed;
    uint32_t alternate;        /* AF index/function. 0 if unused. */
} xy_hal_gpio_config_t;

typedef struct {
    xy_hal_gpio_irq_trigger_t trigger;
    uint8_t priority;
    bool wakeup;
    xy_hal_gpio_irq_cb_t callback;
    void *user;
} xy_hal_gpio_irq_config_t;

int xy_hal_gpio_init(uint32_t pin, const xy_hal_gpio_config_t *config);
int xy_hal_gpio_config_af(uint32_t pin, uint32_t alternate,
                          xy_hal_gpio_speed_t speed,
                          xy_hal_gpio_output_t output_type,
                          xy_hal_gpio_pull_t pull);
int xy_hal_gpio_write(uint32_t pin, int level);
int xy_hal_gpio_read(uint32_t pin);
int xy_hal_gpio_toggle(uint32_t pin);
int xy_hal_gpio_irq_configure(uint32_t pin,
                              const xy_hal_gpio_irq_config_t *config);
int xy_hal_gpio_irq_enable(uint32_t pin);
int xy_hal_gpio_irq_disable(uint32_t pin);
void xy_hal_gpio_irq_handler(uint32_t pin);

/* UART ------------------------------------------------------------------ */

typedef enum {
    XY_HAL_UART_PARITY_NONE = 0,
    XY_HAL_UART_PARITY_EVEN,
    XY_HAL_UART_PARITY_ODD,
} xy_hal_uart_parity_t;

typedef struct {
    uint32_t baudrate;
    uint8_t data_bits;
    uint8_t stop_bits;
    xy_hal_uart_parity_t parity;
    xy_hal_io_mode_t rx_mode;
    xy_hal_io_mode_t tx_mode;
    void *rx_dma;              /* Optional. NULL selects polling/IRQ path. */
    void *tx_dma;              /* Optional. NULL selects polling/IRQ path. */
} xy_hal_uart_config_t;

int xy_hal_uart_init(void *uart, const xy_hal_uart_config_t *config);
int xy_hal_uart_deinit(void *uart);
int xy_hal_uart_read(void *uart, uint8_t *data, size_t len, uint32_t timeout_ms);
int xy_hal_uart_write(void *uart, const uint8_t *data, size_t len, uint32_t timeout_ms);
int xy_hal_uart_start_rx(void *uart, uint8_t *buffer, size_t len);
int xy_hal_uart_stop_rx(void *uart);
int xy_hal_uart_enable_irq(void *uart, uint32_t irq_mask);
int xy_hal_uart_disable_irq(void *uart, uint32_t irq_mask);
int xy_hal_uart_wait_tx_done(void *uart, uint32_t timeout_ms);
int xy_hal_uart_set_callback(void *uart, xy_hal_event_cb_t cb, void *user);
void xy_hal_uart_irq_handler(void *uart);

/* I2C ------------------------------------------------------------------- */

typedef struct {
    uint32_t speed_hz;
    uint16_t own_address;
    xy_hal_io_mode_t mode;
    void *rx_dma;              /* Optional. NULL selects polling/IRQ path. */
    void *tx_dma;              /* Optional. NULL selects polling/IRQ path. */
} xy_hal_i2c_config_t;

int xy_hal_i2c_init(void *i2c, const xy_hal_i2c_config_t *config);
int xy_hal_i2c_deinit(void *i2c);
int xy_hal_i2c_read(void *i2c, uint8_t dev_addr, uint8_t reg,
                    uint8_t *data, uint16_t len);
int xy_hal_i2c_write(void *i2c, uint8_t dev_addr, uint8_t reg,
                     const uint8_t *data, uint16_t len);
int xy_hal_i2c_transfer(void *i2c, uint8_t dev_addr, const uint8_t *tx,
                        uint16_t tx_len, uint8_t *rx, uint16_t rx_len,
                        uint32_t timeout_ms);
void xy_hal_i2c_irq_handler(void *i2c);

typedef enum {
    XY_HAL_I2C_STAGE_NONE = 0,
    XY_HAL_I2C_STAGE_BUSY,
    XY_HAL_I2C_STAGE_START,
    XY_HAL_I2C_STAGE_ADDR,
    XY_HAL_I2C_STAGE_TX,
    XY_HAL_I2C_STAGE_RX,
    XY_HAL_I2C_STAGE_STOP,
    XY_HAL_I2C_STAGE_RECOVERY,
} xy_hal_i2c_stage_t;

typedef enum {
    XY_HAL_I2C_RECOVER_OK = 0,
    XY_HAL_I2C_RECOVER_INVALID_ARG = -1,
    XY_HAL_I2C_RECOVER_PREPARE_FAIL = -2,
    XY_HAL_I2C_RECOVER_GPIO_FAIL = -3,
    XY_HAL_I2C_RECOVER_SCL_STUCK = -4,
    XY_HAL_I2C_RECOVER_SDA_STUCK = -5,
    XY_HAL_I2C_RECOVER_STOP_FAIL = -6,
    XY_HAL_I2C_RECOVER_UNPREPARE_FAIL = -7,
} xy_hal_i2c_recover_result_t;

typedef struct {
    bool scl_high;
    bool sda_high;
} xy_hal_i2c_line_state_t;

typedef struct {
    int  (*prepare)(void *ctx);
    int  (*unprepare)(void *ctx);
    int  (*gpio_od_init)(void *ctx);
    void (*release_scl)(void *ctx);
    void (*drive_scl_low)(void *ctx);
    void (*release_sda)(void *ctx);
    void (*drive_sda_low)(void *ctx);
    bool (*read_scl)(void *ctx);
    bool (*read_sda)(void *ctx);
    void (*delay_us)(void *ctx, uint32_t us);
    int  (*reset_target)(void *ctx);
} xy_hal_i2c_recovery_ops_t;

typedef struct {
    const xy_hal_i2c_recovery_ops_t *ops;
    void *ctx;
    uint8_t max_retries;        /* Per transaction. 0 disables auto recovery. */
    uint8_t max_pulses;         /* 9..18 typical, 16 default. */
    uint16_t pulse_low_us;
    uint16_t pulse_high_us;
    uint16_t scl_wait_us;
} xy_hal_i2c_recovery_config_t;

typedef struct {
    uint32_t xfer_total;
    uint32_t timeout_count;
    uint32_t busy_count;
    uint32_t recover_attempt;
    uint32_t recover_success;
    uint32_t recover_fail_scl;
    uint32_t recover_fail_sda;
    xy_hal_i2c_stage_t last_stage;
    uint8_t last_addr;
    uint8_t last_pulses;
    xy_hal_i2c_line_state_t before;
    xy_hal_i2c_line_state_t after;
    xy_hal_i2c_recover_result_t last_recover;
} xy_hal_i2c_stats_t;

int xy_hal_i2c_recover_bus(const xy_hal_i2c_recovery_config_t *config,
                           xy_hal_i2c_stats_t *stats);
int xy_hal_i2c_set_recovery(void *i2c,
                            const xy_hal_i2c_recovery_config_t *config);
int xy_hal_i2c_get_stats(void *i2c, xy_hal_i2c_stats_t *stats);

/* SPI ------------------------------------------------------------------- */

typedef struct {
    uint32_t speed_hz;
    uint8_t mode;              /* CPOL/CPHA encoded as 0..3. */
    uint8_t data_bits;
    xy_hal_io_mode_t io_mode;
    void *rx_dma;              /* Optional. NULL selects polling/IRQ path. */
    void *tx_dma;              /* Optional. NULL selects polling/IRQ path. */
} xy_hal_spi_config_t;

int xy_hal_spi_init(void *spi, const xy_hal_spi_config_t *config);
int xy_hal_spi_deinit(void *spi);
int xy_hal_spi_read(void *spi, uint8_t *data, uint16_t len);
int xy_hal_spi_write(void *spi, const uint8_t *data, uint16_t len);
int xy_hal_spi_transfer(void *spi, const uint8_t *tx, uint8_t *rx, uint16_t len,
                        uint32_t timeout_ms);
void xy_hal_spi_irq_handler(void *spi);

/* PWM ------------------------------------------------------------------- */

typedef enum {
    XY_HAL_PWM_POLARITY_HIGH = 0,
    XY_HAL_PWM_POLARITY_LOW,
} xy_hal_pwm_polarity_t;

typedef struct {
    uint32_t clock_hz;          /* Timer input clock. 0 lets the port choose. */
    uint32_t frequency_hz;
    uint16_t duty_permille;     /* 0..1000 */
    uint8_t channel;            /* 1..N */
    xy_hal_pwm_polarity_t polarity;
} xy_hal_pwm_config_t;

int xy_hal_pwm_init(void *pwm, const xy_hal_pwm_config_t *config);
int xy_hal_pwm_deinit(void *pwm, uint8_t channel);
int xy_hal_pwm_start(void *pwm, uint8_t channel);
int xy_hal_pwm_stop(void *pwm, uint8_t channel);
int xy_hal_pwm_set(void *pwm, uint8_t channel, uint32_t frequency_hz,
                   uint16_t duty_permille);

/* Flash ----------------------------------------------------------------- */

typedef struct {
    uint32_t base_addr;
    uint32_t total_size;
    uint32_t erase_size;
    uint32_t write_unit;
    uint8_t erased_value;
} xy_hal_flash_info_t;

typedef struct {
    int (*read)(void *ctx, uint32_t address, void *data, size_t len);
    int (*write)(void *ctx, uint32_t address, const void *data, size_t len);
    int (*erase)(void *ctx, uint32_t address, size_t len);
    int (*get_info)(void *ctx, xy_hal_flash_info_t *info);
    void *ctx;
} xy_hal_flash_t;

int xy_hal_flash_read(void *flash, uint32_t address, void *data, size_t len);
int xy_hal_flash_write(void *flash, uint32_t address, const void *data, size_t len);
int xy_hal_flash_erase(void *flash, uint32_t address, size_t len);
int xy_hal_flash_get_info(void *flash, xy_hal_flash_info_t *info);

/* DMA ------------------------------------------------------------------- */

typedef enum {
    XY_HAL_DMA_MEM_TO_MEM = 0,
    XY_HAL_DMA_MEM_TO_PERIPH,
    XY_HAL_DMA_PERIPH_TO_MEM,
    XY_HAL_DMA_PERIPH_TO_PERIPH,
} xy_hal_dma_direction_t;

typedef enum {
    XY_HAL_DMA_WIDTH_8BIT = 1,
    XY_HAL_DMA_WIDTH_16BIT = 2,
    XY_HAL_DMA_WIDTH_32BIT = 4,
} xy_hal_dma_width_t;

typedef enum {
    XY_HAL_DMA_PRIORITY_LOW = 0,
    XY_HAL_DMA_PRIORITY_MEDIUM,
    XY_HAL_DMA_PRIORITY_HIGH,
    XY_HAL_DMA_PRIORITY_VERY_HIGH,
} xy_hal_dma_priority_t;

typedef struct {
    uint32_t request_id;        /* DMAMUX request or fixed-channel selector. */
    xy_hal_dma_direction_t direction;
    xy_hal_dma_width_t periph_width;
    xy_hal_dma_width_t mem_width;
    bool periph_inc;
    bool mem_inc;
    bool circular;
    xy_hal_dma_priority_t priority;
} xy_hal_dma_config_t;

typedef struct {
    uintptr_t periph_addr;
    uintptr_t mem_addr;
    size_t len;
} xy_hal_dma_transfer_t;

int xy_hal_dma_init(void *dma, const xy_hal_dma_config_t *config);
int xy_hal_dma_deinit(void *dma);
int xy_hal_dma_start(void *dma, const xy_hal_dma_transfer_t *transfer);
int xy_hal_dma_stop(void *dma);
int xy_hal_dma_get_remaining(void *dma, size_t *remaining);
int xy_hal_dma_set_callback(void *dma, xy_hal_event_cb_t cb, void *user);
void xy_hal_dma_irq_handler(void *dma);

/* Low-power timer ------------------------------------------------------- */

typedef enum {
    XY_HAL_LPTIMER_ONESHOT = 0,
    XY_HAL_LPTIMER_PERIODIC,
} xy_hal_lptimer_mode_t;

typedef struct {
    uint32_t clock_hz;
    xy_hal_lptimer_mode_t mode;
    bool run_in_stop;
    bool wakeup;
    uint8_t priority;
} xy_hal_lptimer_config_t;

int xy_hal_lptimer_init(void *timer, const xy_hal_lptimer_config_t *config);
int xy_hal_lptimer_deinit(void *timer);
int xy_hal_lptimer_start(void *timer, uint32_t timeout_ms,
                         xy_hal_timer_cb_t cb, void *user);
int xy_hal_lptimer_stop(void *timer);
uint32_t xy_hal_lptimer_now_ms(void *timer);
void xy_hal_lptimer_irq_handler(void *timer);

/* RTC ------------------------------------------------------------------- */

typedef struct {
    uint16_t year;             /* Full year, e.g. 2026. */
    uint8_t month;             /* 1..12 */
    uint8_t day;               /* 1..31 */
    uint8_t weekday;           /* 1..7, port-defined; 0 if unknown. */
    uint8_t hour;              /* 0..23 */
    uint8_t minute;            /* 0..59 */
    uint8_t second;            /* 0..59 */
} xy_hal_rtc_time_t;

typedef struct {
    uint32_t clock_hz;
    bool use_lse;
    bool wakeup;
} xy_hal_rtc_config_t;

int xy_hal_rtc_init(void *rtc, const xy_hal_rtc_config_t *config);
int xy_hal_rtc_deinit(void *rtc);
int xy_hal_rtc_get_time(void *rtc, xy_hal_rtc_time_t *time);
int xy_hal_rtc_set_time(void *rtc, const xy_hal_rtc_time_t *time);
int xy_hal_rtc_set_wakeup(void *rtc, uint32_t timeout_s);
int xy_hal_rtc_cancel_wakeup(void *rtc);
void xy_hal_rtc_irq_handler(void *rtc);

/* Power management ------------------------------------------------------ */

typedef enum {
    XY_HAL_POWER_RUN = 0,
    XY_HAL_POWER_SLEEP,
    XY_HAL_POWER_STOP,
    XY_HAL_POWER_STANDBY,
    XY_HAL_POWER_SHUTDOWN,
} xy_hal_power_mode_t;

#define XY_HAL_WAKE_GPIO     (1u << 0)
#define XY_HAL_WAKE_RTC      (1u << 1)
#define XY_HAL_WAKE_LPTIMER  (1u << 2)
#define XY_HAL_WAKE_UART     (1u << 3)
#define XY_HAL_WAKE_WDG      (1u << 4)

typedef struct {
    xy_hal_power_mode_t deepest_mode;
    uint32_t wake_sources;
    uint32_t min_residency_ms;
    uint32_t max_latency_ms;
    bool keep_sram;
    bool keep_rtc;
} xy_hal_power_policy_t;

typedef enum {
    XY_HAL_PM_LOCK_CPU = 0,
    XY_HAL_PM_LOCK_SLEEP,
    XY_HAL_PM_LOCK_STOP,
    XY_HAL_PM_LOCK_STANDBY,
    XY_HAL_PM_LOCK_COUNT,
} xy_hal_pm_lock_t;

typedef enum {
    XY_HAL_PERIPH_GPIO = 0,
    XY_HAL_PERIPH_UART,
    XY_HAL_PERIPH_I2C,
    XY_HAL_PERIPH_SPI,
    XY_HAL_PERIPH_DMA,
    XY_HAL_PERIPH_ADC,
    XY_HAL_PERIPH_RTC,
    XY_HAL_PERIPH_LPTIMER,
    XY_HAL_PERIPH_FLASH,
    XY_HAL_PERIPH_USB,
    XY_HAL_PERIPH_CUSTOM,
} xy_hal_periph_type_t;

typedef struct {
    xy_hal_periph_type_t type;
    void *instance;
    xy_hal_power_mode_t max_active_mode;
    xy_hal_power_mode_t wake_mode;
    uint32_t wake_sources;
    bool clock_in_sleep;
    bool clock_in_stop;
    bool retain_state;
} xy_hal_periph_pm_config_t;

int xy_hal_power_configure(const xy_hal_power_policy_t *policy);
int xy_hal_power_enter(xy_hal_power_mode_t mode, uint32_t wake_sources,
                       uint32_t timeout_ms);
int xy_hal_power_acquire_lock(xy_hal_pm_lock_t lock);
int xy_hal_power_release_lock(xy_hal_pm_lock_t lock);
/* Returns zero for an invalid lock so diagnostics can safely scan all types. */
uint16_t xy_hal_power_get_lock_count(xy_hal_pm_lock_t lock);
xy_hal_power_mode_t xy_hal_power_get_allowed_mode(void);
int xy_hal_periph_pm_configure(const xy_hal_periph_pm_config_t *config);
int xy_hal_periph_suspend(xy_hal_periph_type_t type, void *instance);
int xy_hal_periph_resume(xy_hal_periph_type_t type, void *instance);

typedef enum {
    XY_HAL_WAKE_REASON_NONE = 0,
    XY_HAL_WAKE_REASON_TIMEOUT,
    XY_HAL_WAKE_REASON_GPIO,
    XY_HAL_WAKE_REASON_UART,
    XY_HAL_WAKE_REASON_RTC,
    XY_HAL_WAKE_REASON_LPTIMER,
    XY_HAL_WAKE_REASON_WDG,
    XY_HAL_WAKE_REASON_RESET,
    XY_HAL_WAKE_REASON_UNKNOWN,
} xy_hal_wake_reason_t;

typedef struct {
    xy_hal_power_mode_t mode;
    uint32_t sleep_ms;
    uint32_t wake_sources;
} xy_hal_tickless_request_t;

typedef struct {
    uint32_t elapsed_ms;
    uint32_t wake_sources;
    xy_hal_wake_reason_t reason;
} xy_hal_tickless_result_t;

int xy_hal_tickless_enter(const xy_hal_tickless_request_t *req,
                          xy_hal_tickless_result_t *res);

/* System control -------------------------------------------------------- */

void xy_hal_system_reset(void);
void xy_hal_watchdog_kick(void);
int xy_hal_get_chip_id(uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* XY_BAREOS_HAL_H */
