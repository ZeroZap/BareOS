/**
 * @file xy_hal_stub.c
 * @brief Weak default HAL implementation.
 *
 * MCU ports should provide strong definitions for the functions they support.
 * Defaults are intentionally conservative: no hardware access and
 * XY_HAL_NOT_SUPPORTED for optional features.
 */

#include "xy_hal.h"
#include "xy_tick.h"

uint32_t XY_HAL_WEAK xy_hal_time_ms(void)
{
    return xy_tick_now_ms();
}

uint32_t XY_HAL_WEAK xy_hal_time_s(void)
{
    return xy_tick_now_s();
}

void XY_HAL_WEAK xy_hal_delay_ms(uint32_t ms)
{
    uint32_t start = xy_hal_time_ms();
    while ((uint32_t)(xy_hal_time_ms() - start) < ms) {
    }
}

int XY_HAL_WEAK xy_hal_systick_suspend(void)
{
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_systick_resume(void)
{
    return XY_HAL_NOT_SUPPORTED;
}

bool XY_HAL_WEAK xy_hal_systick_is_running(void)
{
    return true;
}

uint32_t XY_HAL_WEAK xy_hal_irq_save(void)
{
    return 0u;
}

void XY_HAL_WEAK xy_hal_irq_restore(uint32_t key)
{
    (void)key;
}

void XY_HAL_WEAK xy_hal_irq_enable(void)
{
}

void XY_HAL_WEAK xy_hal_irq_disable(void)
{
}

int XY_HAL_WEAK xy_hal_gpio_init(uint32_t pin, const xy_hal_gpio_config_t *config)
{
    (void)pin;
    (void)config;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_gpio_config_af(uint32_t pin, uint32_t alternate,
                                      xy_hal_gpio_speed_t speed,
                                      xy_hal_gpio_output_t output_type,
                                      xy_hal_gpio_pull_t pull)
{
    (void)pin;
    (void)alternate;
    (void)speed;
    (void)output_type;
    (void)pull;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_gpio_write(uint32_t pin, int level)
{
    (void)pin;
    (void)level;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_gpio_read(uint32_t pin)
{
    (void)pin;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_gpio_toggle(uint32_t pin)
{
    (void)pin;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_gpio_irq_configure(uint32_t pin,
                                          const xy_hal_gpio_irq_config_t *config)
{
    (void)pin;
    (void)config;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_gpio_irq_enable(uint32_t pin)
{
    (void)pin;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_gpio_irq_disable(uint32_t pin)
{
    (void)pin;
    return XY_HAL_NOT_SUPPORTED;
}

void XY_HAL_WEAK xy_hal_gpio_irq_handler(uint32_t pin)
{
    (void)pin;
}

int XY_HAL_WEAK xy_hal_uart_init(void *uart, const xy_hal_uart_config_t *config)
{
    (void)uart;
    (void)config;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_uart_deinit(void *uart)
{
    (void)uart;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_uart_read(void *uart, uint8_t *data, size_t len,
                                 uint32_t timeout_ms)
{
    (void)uart;
    (void)data;
    (void)len;
    (void)timeout_ms;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_uart_write(void *uart, const uint8_t *data, size_t len,
                                  uint32_t timeout_ms)
{
    (void)uart;
    (void)data;
    (void)len;
    (void)timeout_ms;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_uart_start_rx(void *uart, uint8_t *buffer, size_t len)
{
    (void)uart;
    (void)buffer;
    (void)len;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_uart_stop_rx(void *uart)
{
    (void)uart;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_uart_enable_irq(void *uart, uint32_t irq_mask)
{
    (void)uart;
    (void)irq_mask;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_uart_disable_irq(void *uart, uint32_t irq_mask)
{
    (void)uart;
    (void)irq_mask;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_uart_wait_tx_done(void *uart, uint32_t timeout_ms)
{
    (void)uart;
    (void)timeout_ms;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_uart_set_callback(void *uart, xy_hal_event_cb_t cb,
                                         void *user)
{
    (void)uart;
    (void)cb;
    (void)user;
    return XY_HAL_NOT_SUPPORTED;
}

void XY_HAL_WEAK xy_hal_uart_irq_handler(void *uart)
{
    (void)uart;
}

int XY_HAL_WEAK xy_hal_i2c_init(void *i2c, const xy_hal_i2c_config_t *config)
{
    (void)i2c;
    (void)config;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_i2c_deinit(void *i2c)
{
    (void)i2c;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_i2c_read(void *i2c, uint8_t dev_addr, uint8_t reg,
                                uint8_t *data, uint16_t len)
{
    (void)i2c;
    (void)dev_addr;
    (void)reg;
    (void)data;
    (void)len;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_i2c_write(void *i2c, uint8_t dev_addr, uint8_t reg,
                                 const uint8_t *data, uint16_t len)
{
    (void)i2c;
    (void)dev_addr;
    (void)reg;
    (void)data;
    (void)len;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_i2c_transfer(void *i2c, uint8_t dev_addr,
                                    const uint8_t *tx, uint16_t tx_len,
                                    uint8_t *rx, uint16_t rx_len,
                                    uint32_t timeout_ms)
{
    (void)i2c;
    (void)dev_addr;
    (void)tx;
    (void)tx_len;
    (void)rx;
    (void)rx_len;
    (void)timeout_ms;
    return XY_HAL_NOT_SUPPORTED;
}

void XY_HAL_WEAK xy_hal_i2c_irq_handler(void *i2c)
{
    (void)i2c;
}

int XY_HAL_WEAK xy_hal_i2c_set_recovery(void *i2c,
                                        const xy_hal_i2c_recovery_config_t *config)
{
    (void)i2c;
    (void)config;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_i2c_get_stats(void *i2c, xy_hal_i2c_stats_t *stats)
{
    (void)i2c;
    (void)stats;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_spi_init(void *spi, const xy_hal_spi_config_t *config)
{
    (void)spi;
    (void)config;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_spi_deinit(void *spi)
{
    (void)spi;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_spi_read(void *spi, uint8_t *data, uint16_t len)
{
    (void)spi;
    (void)data;
    (void)len;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_spi_write(void *spi, const uint8_t *data, uint16_t len)
{
    (void)spi;
    (void)data;
    (void)len;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_spi_transfer(void *spi, const uint8_t *tx, uint8_t *rx,
                                    uint16_t len, uint32_t timeout_ms)
{
    (void)spi;
    (void)tx;
    (void)rx;
    (void)len;
    (void)timeout_ms;
    return XY_HAL_NOT_SUPPORTED;
}

void XY_HAL_WEAK xy_hal_spi_irq_handler(void *spi)
{
    (void)spi;
}

int XY_HAL_WEAK xy_hal_pwm_init(void *pwm, const xy_hal_pwm_config_t *config)
{
    (void)pwm;
    (void)config;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_pwm_deinit(void *pwm, uint8_t channel)
{
    (void)pwm;
    (void)channel;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_pwm_start(void *pwm, uint8_t channel)
{
    (void)pwm;
    (void)channel;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_pwm_stop(void *pwm, uint8_t channel)
{
    (void)pwm;
    (void)channel;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_pwm_set(void *pwm, uint8_t channel,
                               uint32_t frequency_hz,
                               uint16_t duty_permille)
{
    (void)pwm;
    (void)channel;
    (void)frequency_hz;
    (void)duty_permille;
    return XY_HAL_NOT_SUPPORTED;
}

static const xy_hal_flash_t *flash_ops(void *flash)
{
    return (const xy_hal_flash_t *)flash;
}

int XY_HAL_WEAK xy_hal_flash_read(void *flash, uint32_t address, void *data,
                                  size_t len)
{
    const xy_hal_flash_t *ops = flash_ops(flash);
    if (!ops || !ops->read) {
        return XY_HAL_NOT_SUPPORTED;
    }
    return ops->read(ops->ctx, address, data, len);
}

int XY_HAL_WEAK xy_hal_flash_write(void *flash, uint32_t address,
                                   const void *data, size_t len)
{
    const xy_hal_flash_t *ops = flash_ops(flash);
    if (!ops || !ops->write) {
        return XY_HAL_NOT_SUPPORTED;
    }
    return ops->write(ops->ctx, address, data, len);
}

int XY_HAL_WEAK xy_hal_flash_erase(void *flash, uint32_t address, size_t len)
{
    const xy_hal_flash_t *ops = flash_ops(flash);
    if (!ops || !ops->erase) {
        return XY_HAL_NOT_SUPPORTED;
    }
    return ops->erase(ops->ctx, address, len);
}

int XY_HAL_WEAK xy_hal_flash_get_info(void *flash, xy_hal_flash_info_t *info)
{
    const xy_hal_flash_t *ops = flash_ops(flash);
    if (!ops || !ops->get_info) {
        return XY_HAL_NOT_SUPPORTED;
    }
    return ops->get_info(ops->ctx, info);
}

int XY_HAL_WEAK xy_hal_dma_init(void *dma, const xy_hal_dma_config_t *config)
{
    (void)dma;
    (void)config;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_dma_deinit(void *dma)
{
    (void)dma;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_dma_start(void *dma, const xy_hal_dma_transfer_t *transfer)
{
    (void)dma;
    (void)transfer;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_dma_stop(void *dma)
{
    (void)dma;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_dma_get_remaining(void *dma, size_t *remaining)
{
    (void)dma;
    if (remaining) {
        *remaining = 0u;
    }
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_dma_set_callback(void *dma, xy_hal_event_cb_t cb,
                                        void *user)
{
    (void)dma;
    (void)cb;
    (void)user;
    return XY_HAL_NOT_SUPPORTED;
}

void XY_HAL_WEAK xy_hal_dma_irq_handler(void *dma)
{
    (void)dma;
}

int XY_HAL_WEAK xy_hal_lptimer_init(void *timer,
                                    const xy_hal_lptimer_config_t *config)
{
    (void)timer;
    (void)config;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_lptimer_deinit(void *timer)
{
    (void)timer;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_lptimer_start(void *timer, uint32_t timeout_ms,
                                     xy_hal_timer_cb_t cb, void *user)
{
    (void)timer;
    (void)timeout_ms;
    (void)cb;
    (void)user;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_lptimer_stop(void *timer)
{
    (void)timer;
    return XY_HAL_NOT_SUPPORTED;
}

uint32_t XY_HAL_WEAK xy_hal_lptimer_now_ms(void *timer)
{
    (void)timer;
    return xy_hal_time_ms();
}

void XY_HAL_WEAK xy_hal_lptimer_irq_handler(void *timer)
{
    (void)timer;
}

int XY_HAL_WEAK xy_hal_rtc_init(void *rtc, const xy_hal_rtc_config_t *config)
{
    (void)rtc;
    (void)config;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_rtc_deinit(void *rtc)
{
    (void)rtc;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_rtc_get_time(void *rtc, xy_hal_rtc_time_t *time)
{
    (void)rtc;
    (void)time;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_rtc_set_time(void *rtc, const xy_hal_rtc_time_t *time)
{
    (void)rtc;
    (void)time;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_rtc_set_wakeup(void *rtc, uint32_t timeout_s)
{
    (void)rtc;
    (void)timeout_s;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_rtc_cancel_wakeup(void *rtc)
{
    (void)rtc;
    return XY_HAL_NOT_SUPPORTED;
}

void XY_HAL_WEAK xy_hal_rtc_irq_handler(void *rtc)
{
    (void)rtc;
}

int XY_HAL_WEAK xy_hal_power_configure(const xy_hal_power_policy_t *policy)
{
    (void)policy;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_power_enter(xy_hal_power_mode_t mode,
                                   uint32_t wake_sources,
                                   uint32_t timeout_ms)
{
    (void)mode;
    (void)wake_sources;
    (void)timeout_ms;
    return XY_HAL_NOT_SUPPORTED;
}

static uint16_t s_pm_lock_count[XY_HAL_PM_LOCK_COUNT];

int XY_HAL_WEAK xy_hal_power_acquire_lock(xy_hal_pm_lock_t lock)
{
    if ((uint32_t)lock >= (uint32_t)XY_HAL_PM_LOCK_COUNT ||
        s_pm_lock_count[lock] == UINT16_MAX) {
        return XY_HAL_INVALID_PARAM;
    }
    s_pm_lock_count[lock]++;
    return XY_HAL_OK;
}

int XY_HAL_WEAK xy_hal_power_release_lock(xy_hal_pm_lock_t lock)
{
    if ((uint32_t)lock >= (uint32_t)XY_HAL_PM_LOCK_COUNT ||
        s_pm_lock_count[lock] == 0u) {
        return XY_HAL_INVALID_PARAM;
    }
    s_pm_lock_count[lock]--;
    return XY_HAL_OK;
}

uint16_t XY_HAL_WEAK xy_hal_power_get_lock_count(xy_hal_pm_lock_t lock)
{
    if ((uint32_t)lock >= (uint32_t)XY_HAL_PM_LOCK_COUNT) {
        return 0u;
    }
    return s_pm_lock_count[lock];
}

xy_hal_power_mode_t XY_HAL_WEAK xy_hal_power_get_allowed_mode(void)
{
    return XY_HAL_POWER_RUN;
}

int XY_HAL_WEAK xy_hal_periph_pm_configure(const xy_hal_periph_pm_config_t *config)
{
    (void)config;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_periph_suspend(xy_hal_periph_type_t type, void *instance)
{
    (void)type;
    (void)instance;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_periph_resume(xy_hal_periph_type_t type, void *instance)
{
    (void)type;
    (void)instance;
    return XY_HAL_NOT_SUPPORTED;
}

int XY_HAL_WEAK xy_hal_tickless_enter(const xy_hal_tickless_request_t *req,
                                      xy_hal_tickless_result_t *res)
{
    (void)req;
    if (res) {
        res->elapsed_ms = 0u;
        res->wake_sources = 0u;
        res->reason = XY_HAL_WAKE_REASON_NONE;
    }
    return XY_HAL_NOT_SUPPORTED;
}

void XY_HAL_WEAK xy_hal_system_reset(void)
{
    for (;;) {
    }
}

void XY_HAL_WEAK xy_hal_watchdog_kick(void)
{
}

int XY_HAL_WEAK xy_hal_get_chip_id(uint8_t *buf, size_t len)
{
    (void)buf;
    (void)len;
    return XY_HAL_NOT_SUPPORTED;
}
