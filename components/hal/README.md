# BareOS HAL

`components/hal` is the stable boundary between BareOS components and MCU
vendor SDKs. It is intentionally smaller than a vendor HAL: it exposes only the
capabilities used by BareOS components and applications.

## Industry Patterns Used

- CMSIS-style handles: HAL APIs receive opaque `void *` peripheral handles so
  N32, STM32, and CH32 ports can pass their native peripheral instances without
  leaking vendor headers into components.
- STM32 HAL-style DMA linkage: UART/I2C/SPI configs carry optional `rx_dma` and
  `tx_dma` handles; DMA itself is configured separately and can be ignored for
  polling or IRQ-only ports.
- Zephyr-style capability separation: power, DMA, GPIO, bus, and flash are
  independent capabilities. Components use only the capability they need.
- ESP-IDF-style power locks: `xy_hal_power_acquire_lock()` prevents entry into
  deeper low-power modes while UART, DMA, flash, or radio work is active.
- nrfx-style per-instance configuration: each peripheral is initialized from a
  compact config struct, while MCU-specific channel mapping stays in the port.

## Porting Rules

- Vendor headers such as `n32l40x.h`, `stm32xxxx_hal.h`, or `ch32xxxx.h` belong
  in MCU port files, not in `components/` users.
- Return `XY_HAL_NOT_SUPPORTED` for features absent on a specific MCU.
- Keep DMA request mapping in the MCU port. `request_id` is a DMAMUX request on
  MCUs with DMAMUX and a fixed channel selector on simpler DMA controllers.
- Power policy is advisory. The port chooses the deepest safe mode allowed by
  active locks, wake sources, minimum residency, and maximum wake latency.
- ISR handlers in the MCU startup/BSP should call the relevant `xy_hal_*_irq`
  or component feed functions; the HAL does not install vector names.

## GPIO And AF

GPIO configuration includes electrical mode, pull, speed, output type, and
alternate function. Ports should map `alternate` to the MCU AF selector, for
example N32 GPIO AF numbers, STM32 `GPIO_AFx_*`, or CH32 remap/AFIO settings.

Use `xy_hal_gpio_config_af()` for simple peripheral pin setup, or
`xy_hal_gpio_init()` with `XY_HAL_GPIO_ALT` when the board needs all fields in a
single config object.

GPIO interrupt registration is explicit through `xy_hal_gpio_irq_configure()`.
The MCU EXTI/GPIO ISR should call `xy_hal_gpio_irq_handler(pin)` after clearing
the vendor interrupt flag. If `wakeup` is true, the port also configures the pin
as a low-power wake source when the MCU supports it.

## DMA Optionality

`rx_dma` and `tx_dma` are optional. If a config requests `XY_HAL_IO_DMA` but the
handle is `NULL`, the port must either return `XY_HAL_INVALID_PARAM` from init
or downgrade to IRQ/polling only if that behavior is documented by the port.

If `rx_dma` and `tx_dma` are `NULL` and the mode is `XY_HAL_IO_IRQ`, the port
uses peripheral interrupts. If the mode is `XY_HAL_IO_POLL`, blocking functions
such as `xy_hal_uart_read()` and `xy_hal_spi_transfer()` poll status flags until
completion or timeout.

## Low Power

System power policy and peripheral power policy are separate. System policy
chooses the deepest CPU mode. Peripheral PM config describes whether a UART,
I2C, SPI, DMA, RTC, LPTIMER, or GPIO instance keeps clocks/state in sleep or
stop and whether it can wake the MCU.

Use `xy_hal_power_acquire_lock()` before flash erase/write, active UART/DMA
transfers, radio transactions, and time-critical operations. Release the lock
when the operation finishes.

Low-power timers use `xy_hal_lptimer_*`. A port can back this with N32 LPTIM,
STM32 LPTIM/RTC wakeup timer, CH32 low-power timer, or a normal timer if stop
mode wake is not required. `run_in_stop` and `wakeup` make the requirement
explicit.

## Suggested MCU Layout

Keep vendor packages under `hal/Hal-<MCU>` and BareOS port glue under project or
port directories, for example:

```text
hal/Hal-N32L40x/                 vendor package
project/PLB -N32/USER/src/       board-level glue
ports/stm32/                     future STM32 BareOS HAL glue
ports/ch32/                      future CH32 BareOS HAL glue
```

The weak implementation in `xy_hal_stub.c` lets PC builds and partial ports link
without hardware support. Production MCU builds should provide strong
definitions for the required functions.
