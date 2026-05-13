# STM32 HAL Implementation

## Directory Structure

This directory contains STM32-specific implementations of the XY HAL interface.

```
stm32/
├── stm32f4/          # STM32F4 series (F401/F407/F411/F427/F429/F446/F469...)
├── stm32u0/          # STM32U0 series (Ultra-low-power)
├── stm32l4/          # STM32L4 series (Low-power)
├── stm32h7/          # STM32H7 series (High-performance)
├── stm32_hal.h       # Common STM32 HAL helper
├── example_usage.c   # Usage examples
├── CMakeLists.txt    # CMake build configuration
└── Makefile.template # Makefile template
```

## Supported Series

| Directory | Series | Status | Notes |
|-----------|--------|--------|-------|
| `stm32f4/` | STM32F4 | ✅ Complete | F401/F407/F411/F427/F429/F446/F469 |
| `stm32u0/` | STM32U0 | ⏳ Pending | Ultra-low-power series |
| `stm32l4/` | STM32L4 | ⏳ Pending | Low-power series |
| `stm32h7/` | STM32H7 | ⏳ Pending | High-performance series |

## Implemented Peripherals

Each series directory should implement:

- ✅ `xy_hal_pin.c` - GPIO/Pin control
- ✅ `xy_hal_uart.c` - UART communication
- ✅ `xy_hal_spi.c` - SPI bus
- ✅ `xy_hal_i2c.c` - I2C bus
- ✅ `xy_hal_timer.c` - Timer
- ✅ `xy_hal_pwm.c` - PWM output
- ✅ `xy_hal_rtc.c` - Real-time clock
- ✅ `xy_hal_dma.c` - DMA transfer
- ✅ `xy_hal_lp_timer.c` - Low-power timer

## Function Naming Convention

**IMPORTANT**: Implementation function names must match exactly with interface declarations in `../inc/`.

❌ **Wrong** (with suffix):
```c
int xy_hal_pin_init_stm32(void *port, uint8_t pin, const xy_hal_pin_config_t *config);
```

✅ **Correct** (exact match):
```c
int xy_hal_pin_init(void *port, uint8_t pin, const xy_hal_pin_config_t *config);
```

## Building

### With CMake

```bash
mkdir build && cd build
cmake .. -DSTM32_SERIES=stm32f4 -DSTM32_FAMILY=STM32F4 -DSTM32_DEVICE=STM32F407xx
make
```

### With Make

```bash
make STM32_SERIES=stm32f4 STM32_FAMILY=STM32F4 STM32_DEVICE=STM32F407xx
```

### Integration in Your Project

**Makefile**:
```makefile
STM32_SERIES ?= stm32f4
C_SOURCES += $(wildcard bsp/xy_hal/stm32/$(STM32_SERIES)/*.c)
C_INCLUDES += -Ibsp/xy_hal/inc -Ibsp/xy_hal/stm32
C_DEFS += -DSTM32_HAL_ENABLED
```

**CMakeLists.txt**:
```cmake
set(STM32_SERIES "stm32f4")
file(GLOB XY_HAL_SOURCES "bsp/xy_hal/stm32/${STM32_SERIES}/*.c")
target_sources(${PROJECT_NAME} PRIVATE ${XY_HAL_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE
    bsp/xy_hal/inc
    bsp/xy_hal/stm32
)
target_compile_definitions(${PROJECT_NAME} PRIVATE STM32_HAL_ENABLED)
```

## Adding New Series

To add support for a new STM32 series (e.g., STM32F1):

1. **Create directory**:
   ```bash
   mkdir stm32f1
   ```

2. **Copy reference implementation**:
   ```bash
   cp stm32f4/*.c stm32f1/
   ```

3. **Adapt to target series**:
   - Modify STM32 HAL API calls for the target series
   - Update register access if needed
   - Handle series-specific features

4. **Keep function names unchanged**:
   ```c
   // In stm32f1/xy_hal_pin.c
   int xy_hal_pin_init(void *port, uint8_t pin, const xy_hal_pin_config_t *config)
   {
       // STM32F1-specific implementation
   }
   ```

5. **Test build**:
   ```bash
   make STM32_SERIES=stm32f1
   ```

## Platform Detection

The `stm32_hal.h` header automatically detects and includes the correct STM32 HAL library:

```c
#ifdef STM32F4
    #include "stm32f4xx_hal.h"
#elif defined(STM32U0)
    #include "stm32u0xx_hal.h"
#elif defined(STM32L4)
    #include "stm32l4xx_hal.h"
// ...
#endif
```

## Examples

See `example_usage.c` for comprehensive usage examples covering:
- GPIO LED control
- UART communication
- SPI/I2C sensor reading
- PWM motor control
- Timer interrupts
- RTC date/time management

## Notes

- All implementations require the official STM32 HAL library
- Function names must exactly match interface declarations
- Use `#ifdef STM32_HAL_ENABLED` to enable/disable compilation
- Each series may have specific limitations or features

## References

- Main documentation: `../README.md`
- Integration guide: `../INTEGRATION_GUIDE.md`
- Restructuring notes: `../RESTRUCTURING_NOTES.md`

---

**Version**: 2.0
**Last Updated**: 2025-10-26
