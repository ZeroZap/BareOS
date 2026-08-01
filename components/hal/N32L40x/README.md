# N32L40x HAL Port

This directory implements the generic BareOS HAL on top of the Nations
`N32L40x_Library.2.2.0` standard peripheral driver.

## Files

```text
xy_hal_n32l40x.h              public N32 pin helpers and BSP hooks
xy_hal_n32l40x_internal.h     private shared helpers/state
xy_hal_n32l40x_common.c       shared clock/pin/callback helpers
xy_hal_n32l40x_time_irq.c     time, delay, SysTick, critical sections
xy_hal_n32l40x_gpio.c         GPIO, AF, EXTI registration
xy_hal_n32l40x_uart.c         USART/UART polling and IRQ callback bridge
xy_hal_n32l40x_i2c.c          I2C blocking master transfers
xy_hal_n32l40x_spi.c          SPI polling transfers
xy_hal_n32l40x_pwm.c          TIM PWM output
xy_hal_n32l40x_dma.c          DMA channel setup/start/stop
xy_hal_n32l40x_flash.c        internal Flash read/write/erase
xy_hal_n32l40x_lptimer.c      LPTIM wrapper
xy_hal_n32l40x_rtc.c          RTC wall clock and wakeup timer
xy_hal_n32l40x_power.c        PWR, PM locks, tickless entry
xy_hal_n32l40x_sys.c          reset, watchdog, chip UID
```

## Handles

Pass Nations peripheral pointers directly as HAL handles:

```c
xy_hal_uart_init(USART1, &uart_cfg);
xy_hal_i2c_read(I2C1, 0x68, 0x00, buf, len);
xy_hal_spi_transfer(SPI1, tx, rx, len, 100);
xy_hal_pwm_init(TIM3, &pwm_cfg);
xy_hal_dma_init(DMA_CH3, &dma_cfg);
xy_hal_lptimer_init(LPTIM, &lp_cfg);
xy_hal_rtc_init(NULL, &rtc_cfg);
```

## GPIO Pin Encoding

Use `XY_HAL_N32_PIN(port, index)` from `xy_hal_n32l40x.h`:

```c
uint32_t pa9 = XY_HAL_N32_PIN(XY_HAL_N32_PORT_A, 9);
xy_hal_gpio_config_af(pa9, GPIO_AF4_USART1,
                      XY_HAL_GPIO_SPEED_HIGH,
                      XY_HAL_GPIO_PUSH_PULL,
                      XY_HAL_GPIO_PULLUP);
```

The board code is responsible for choosing the correct AF value from
`n32l40x_gpio.h` and enabling NVIC IRQ channels for GPIO/USART/DMA/LPTIM.

## Tickless

`xy_hal_tickless_enter()` provides a first-stage implementation. It suspends the
SysTick interrupt, enters the requested power mode, resumes SysTick, and returns
an elapsed time. For production STOP/STOP2 tickless, the BSP should configure
LPTIM or RTC wakeup before calling tickless and override:

```c
void xy_hal_n32l40x_before_sleep(void);
void xy_hal_n32l40x_after_stop_restore_clock(void);
void xy_hal_n32l40x_after_wake(void);
```

`xy_hal_n32l40x_after_stop_restore_clock()` must restore the board clock tree
after STOP/STOP2, because N32 clock setup is application-specific.

## LPTIM synchronization note

The tested N32L406 revision does not assert `INTSTS.ARRUPD` after an ARR write,
although the standard peripheral driver exposes it as the `ARROK` flag. Waiting
for that flag on every tickless entry previously caused a fixed busy-wait before
STOP2; a post-decrement timeout also underflowed and incorrectly reported the
operation as successful. A 100 ms LPTIM self-test consequently took 193 ms.

The N32 port now follows the hardware clock-domain timing requirement instead:

- wait at least three LPTIM source-clock periods after enabling LPTIM;
- write ARR and verify its APB readback;
- wait another three source-clock periods before starting one-shot mode;
- never spin on `ARRUPD` as a completion condition on this MCU revision.

Validation on PLB-N32 with LSE at 32768 Hz measured a 100 ms request as
`start=2 ms`, `count=100 ms`, `total=102 ms`. Tickless heartbeat intervals then
returned to approximately 1001-1002 ms. LSI remains a fallback only because its
measured frequency varies from the nominal 40 kHz.

STOP2 wakeup latency and system-clock restoration are separate concerns. The
MCU can resume instruction execution within the documented wakeup latency, but
the BSP must still wait for HSI/PLL readiness and switch SYSCLK back to the board
run clock in `xy_hal_n32l40x_after_stop_restore_clock()`.

## Current Limitations

- UART/SPI polling paths are implemented.
- I2C register access uses blocking master transfers and a small 32-byte write
  staging buffer for `xy_hal_i2c_write()`.
- DMA init/start/stop is implemented, but DMA IRQ decoding is intentionally
  minimal; project IRQ handlers can call `xy_hal_dma_irq_handler(DMA_CHx)` after
  clearing the Nations DMA flags.
- Peripheral PM configuration stores no per-device state yet; it returns OK so
  board code can adopt the API incrementally.
- RTC and LPTIMER are optional HAL capabilities. This N32L40x port implements
  both; MCU ports without the hardware should leave the weak defaults or return
  `XY_HAL_NOT_SUPPORTED` from their port implementation.
