/**
 * @file xy_hal_n32l40x_internal.h
 * @brief Internal helpers shared by the N32L40x HAL port files.
 */

#ifndef XY_HAL_N32L40X_INTERNAL_H
#define XY_HAL_N32L40X_INTERNAL_H

#include "xy_hal_n32l40x.h"

#include <string.h>
#include "n32l40x_dma.h"
#include "n32l40x_exti.h"
#include "n32l40x_flash.h"
#include "n32l40x_gpio.h"
#include "n32l40x_i2c.h"
#include "n32l40x_iwdg.h"
#include "n32l40x_lptim.h"
#include "n32l40x_pwr.h"
#include "n32l40x_rcc.h"
#include "n32l40x_rtc.h"
#include "n32l40x_spi.h"
#include "n32l40x_tim.h"
#include "n32l40x_usart.h"
#include "xy_tick.h"

#ifndef XY_HAL_N32_FLASH_BASE
#define XY_HAL_N32_FLASH_BASE       0x08000000u
#endif

#ifndef XY_HAL_N32_FLASH_SIZE
#define XY_HAL_N32_FLASH_SIZE       (256u * 1024u)
#endif

#ifndef XY_HAL_N32_FLASH_PAGE_SIZE
#define XY_HAL_N32_FLASH_PAGE_SIZE  2048u
#endif

#ifndef XY_HAL_N32_SPI_DEFAULT_PRESCALER
#define XY_HAL_N32_SPI_DEFAULT_PRESCALER SPI_BR_PRESCALER_8
#endif

typedef struct {
    xy_hal_gpio_irq_cb_t cb;
    void *user;
    xy_hal_gpio_irq_trigger_t trigger;
    bool enabled;
} xy_hal_n32_gpio_irq_state_t;

typedef struct {
    void *dev;
    xy_hal_event_cb_t cb;
    void *user;
} xy_hal_n32_dev_cb_state_t;

typedef struct {
    LPTIM_Module *timer;
    uint32_t clock_hz;
    uint32_t timeout_ms;
    uint32_t programmed_ticks;
    uint32_t elapsed_ticks;
    bool active;
    xy_hal_timer_cb_t cb;
    void *user;
} xy_hal_n32_lptimer_state_t;

extern xy_hal_n32_gpio_irq_state_t g_xy_hal_n32_gpio_irq[4][16];
extern xy_hal_n32_dev_cb_state_t g_xy_hal_n32_uart_cb[5];
extern xy_hal_n32_dev_cb_state_t g_xy_hal_n32_dma_cb[8];
extern xy_hal_n32_lptimer_state_t g_xy_hal_n32_lptim;
extern xy_hal_power_policy_t g_xy_hal_n32_power_policy;
extern uint16_t g_xy_hal_n32_pm_locks[XY_HAL_PM_LOCK_COUNT];
extern bool g_xy_hal_n32_systick_running;

bool xy_hal_n32_expired(uint32_t start, uint32_t timeout_ms);

GPIO_Module *xy_hal_n32_gpio_port(uint32_t pin);
uint16_t xy_hal_n32_gpio_pin_mask(uint32_t pin);
uint32_t xy_hal_n32_gpio_clk(uint32_t port);
uint8_t xy_hal_n32_gpio_pin_source(uint32_t pin);
uint32_t xy_hal_n32_exti_line(uint32_t pin);

void xy_hal_n32_enable_uart_clock(USART_Module *uart);
void xy_hal_n32_enable_i2c_clock(I2C_Module *i2c);
void xy_hal_n32_enable_spi_clock(SPI_Module *spi);
void xy_hal_n32_enable_tim_clock(TIM_Module *tim);

xy_hal_n32_dev_cb_state_t *xy_hal_n32_uart_state(USART_Module *uart);
xy_hal_n32_dev_cb_state_t *xy_hal_n32_dma_state(DMA_ChannelType *dma);

#endif /* XY_HAL_N32L40X_INTERNAL_H */
