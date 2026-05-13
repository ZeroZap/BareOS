# XY HAL Error Codes Reference

## Overview

All XY HAL functions now return standardized error codes defined in [`xy_hal.h`](file://e:\github_download\_ZeroZap\XinYi\bsp\xy_hal\inc\xy_hal.h). This provides consistent error handling across all platforms and peripherals.

## Error Code Enumeration

```c
typedef enum {
    XY_HAL_OK                = 0,    /**< Success */
    XY_HAL_ERROR             = -1,   /**< Generic error */
    XY_HAL_ERROR_INVALID_PARAM = -2, /**< Invalid parameter */
    XY_HAL_ERROR_NOT_SUPPORT = -3,   /**< Feature not supported */
    XY_HAL_ERROR_TIMEOUT     = -4,   /**< Operation timeout */
    XY_HAL_ERROR_BUSY        = -5,   /**< Resource busy */
    XY_HAL_ERROR_NO_MEM      = -6,   /**< Out of memory */
    XY_HAL_ERROR_IO          = -7,   /**< I/O error */
    XY_HAL_ERROR_NOT_INIT    = -8,   /**< Not initialized */
    XY_HAL_ERROR_ALREADY_INIT = -9,  /**< Already initialized */
    XY_HAL_ERROR_NO_RESOURCE = -10,  /**< Resource not available */
    XY_HAL_ERROR_FAIL        = -11,  /**< Operation failed */
} xy_hal_error_t;
```

## Error Code Details

### XY_HAL_OK (0)
**Meaning**: Operation completed successfully

**Usage**: Returned when function executes without any errors

**Example**:
```c
int ret = xy_hal_pin_init(GPIOA, 5, &config);
if (ret == XY_HAL_OK) {
    // Success
}
```

### XY_HAL_ERROR (-1)
**Meaning**: Generic/unspecified error

**Usage**: Used when error doesn't fit other categories

**When to use**:
- Unknown errors
- Multiple error conditions
- Fallback error code

### XY_HAL_ERROR_INVALID_PARAM (-2)
**Meaning**: One or more parameters are invalid

**Common causes**:
- NULL pointer passed
- Out-of-range values
- Invalid enum values
- Pin number > 15 for GPIO

**Example**:
```c
// NULL port parameter
int ret = xy_hal_pin_init(NULL, 5, &config);
// Returns: XY_HAL_ERROR_INVALID_PARAM

// Invalid pin number
ret = xy_hal_pin_init(GPIOA, 20, &config);
// Returns: XY_HAL_ERROR_INVALID_PARAM
```

### XY_HAL_ERROR_NOT_SUPPORT (-3)
**Meaning**: Feature or operation not supported on this platform/device

**Common causes**:
- Feature not available on MCU
- Peripheral not present
- Mode not supported

**Example**:
```c
// Low-power timer not available on STM32F1
int ret = xy_hal_lptimer_init(&hlptim, &config);
// Returns: XY_HAL_ERROR_NOT_SUPPORT (on F1 series)
```

### XY_HAL_ERROR_TIMEOUT (-4)
**Meaning**: Operation timed out

**Common causes**:
- UART/SPI/I2C transmission timeout
- Peripheral not responding
- DMA transfer timeout

**Example**:
```c
// UART transmission timeout
int ret = xy_hal_uart_send(&huart1, data, 100, 1000);
// Returns: XY_HAL_ERROR_TIMEOUT if not complete within 1000ms
```

### XY_HAL_ERROR_BUSY (-5)
**Meaning**: Resource is currently busy

**Common causes**:
- Peripheral in use
- Previous operation not complete
- Mutex locked

**Example**:
```c
// DMA channel busy with previous transfer
int ret = xy_hal_dma_start(&hdma, src, dst, len);
// Returns: XY_HAL_ERROR_BUSY if channel still active
```

### XY_HAL_ERROR_NO_MEM (-6)
**Meaning**: Out of memory or buffer space

**Common causes**:
- Heap exhaustion
- Buffer too small
- No free buffers

**Example**:
```c
// Insufficient buffer space
int ret = xy_hal_uart_recv(&huart1, small_buf, 10, 1000);
// Returns: XY_HAL_ERROR_NO_MEM if received data > 10 bytes
```

### XY_HAL_ERROR_IO (-7)
**Meaning**: Input/Output operation failed

**Common causes**:
- Hardware failure
- Bus error
- Communication error

**Example**:
```c
// I2C bus error (NAK, arbitration lost, etc.)
int ret = xy_hal_i2c_mem_read(&hi2c1, 0x50, 0x00, data, 8, 1000);
// Returns: XY_HAL_ERROR_IO on bus error
```

### XY_HAL_ERROR_NOT_INIT (-8)
**Meaning**: Peripheral/module not initialized

**Common causes**:
- Function called before initialization
- Init function not called

**Example**:
```c
// Attempt to use UART before initialization
int ret = xy_hal_uart_send(&huart1, data, 10, 1000);
// Returns: XY_HAL_ERROR_NOT_INIT if huart1 not initialized
```

### XY_HAL_ERROR_ALREADY_INIT (-9)
**Meaning**: Peripheral/module already initialized

**Common causes**:
- Init called multiple times
- Re-initialization attempt

**Example**:
```c
xy_hal_uart_init(&huart1, &config);  // First init OK
int ret = xy_hal_uart_init(&huart1, &config);  // Second init
// Returns: XY_HAL_ERROR_ALREADY_INIT
```

### XY_HAL_ERROR_NO_RESOURCE (-10)
**Meaning**: Required resource not available

**Common causes**:
- No free IRQ vectors
- No available DMA channels
- Hardware resource exhausted

**Example**:
```c
// All EXTI lines for this pin already used
int ret = xy_hal_pin_attach_irq(GPIOA, 5, mode, handler, arg);
// Returns: XY_HAL_ERROR_NO_RESOURCE if IRQ not available
```

### XY_HAL_ERROR_FAIL (-11)
**Meaning**: Operation failed for unspecified reason

**Common causes**:
- Hardware failure
- Configuration error
- Unexpected state

**Example**:
```c
// Hardware initialization failed
int ret = xy_hal_spi_init(&hspi1, &config);
// Returns: XY_HAL_ERROR_FAIL if HAL_SPI_Init() fails
```

## Usage Guidelines

### 1. Always Check Return Values

```c
int ret = xy_hal_pin_init(GPIOA, 5, &config);
if (ret != XY_HAL_OK) {
    // Handle error
    printf("GPIO init failed: %d\n", ret);
}
```

### 2. Handle Specific Errors

```c
int ret = xy_hal_i2c_mem_read(&hi2c1, addr, reg, data, len, 1000);
switch (ret) {
    case XY_HAL_OK:
        // Success - process data
        break;
    case XY_HAL_ERROR_TIMEOUT:
        // Retry or report timeout
        break;
    case XY_HAL_ERROR_IO:
        // Bus error - check connections
        break;
    case XY_HAL_ERROR_INVALID_PARAM:
        // Parameter error - fix code
        break;
    default:
        // Other errors
        break;
}
```

### 3. Error Propagation

```c
int my_sensor_read(uint8_t *data)
{
    int ret = xy_hal_i2c_mem_read(&hi2c1, SENSOR_ADDR, REG, data, 6, 100);
    if (ret != XY_HAL_OK) {
        return ret;  // Propagate HAL error
    }
    return XY_HAL_OK;
}
```

### 4. Error Logging

```c
#define HAL_CHECK(func) do { \
    int _ret = (func); \
    if (_ret != XY_HAL_OK) { \
        printf("[%s:%d] %s failed: %d\n", __FILE__, __LINE__, #func, _ret); \
        return _ret; \
    } \
} while(0)

// Usage
HAL_CHECK(xy_hal_pin_init(GPIOA, 5, &config));
HAL_CHECK(xy_hal_uart_init(&huart1, &uart_config));
```

## Return Value Conventions

### Success Operations
- **Always return**: `XY_HAL_OK` (0)

### Read Operations
- **On success**: Return read value (>= 0) OR `XY_HAL_OK`
- **On error**: Return negative error code

```c
// xy_hal_pin_read returns pin state or error
int state = xy_hal_pin_read(GPIOA, 5);
if (state < 0) {
    // Error occurred
} else {
    // state is 0 (LOW) or 1 (HIGH)
}
```

### Write/Control Operations
- **On success**: Return `XY_HAL_OK` (0)
- **On error**: Return negative error code

```c
int ret = xy_hal_pin_write(GPIOA, 5, XY_HAL_PIN_HIGH);
// ret == XY_HAL_OK (0) on success
// ret < 0 on error
```

## Migration from Old Code

### Before (v1.0)
```c
int ret = xy_hal_pin_init(GPIOA, 5, &config);
if (ret != 0) {  // Generic "not zero" check
    // Error
}
```

### After (v2.0)
```c
int ret = xy_hal_pin_init(GPIOA, 5, &config);
if (ret != XY_HAL_OK) {  // Explicit success check
    // Error - can now identify specific error
    if (ret == XY_HAL_ERROR_INVALID_PARAM) {
        // Handle invalid parameter
    }
}
```

## Platform-Specific Notes

### STM32
- Most errors map directly from STM32 HAL return values
- `HAL_OK` → `XY_HAL_OK`
- `HAL_ERROR` → `XY_HAL_ERROR`
- `HAL_TIMEOUT` → `XY_HAL_ERROR_TIMEOUT`
- `HAL_BUSY` → `XY_HAL_ERROR_BUSY`

### Future Platforms
- Each platform implementation should map native errors to XY HAL errors
- Maintain consistent error semantics across platforms

## Best Practices

1. **Always check return values** - Don't ignore errors
2. **Use specific error codes** - Return most appropriate error
3. **Document error conditions** - In function comments
4. **Consistent error handling** - Same error for same condition
5. **Propagate errors** - Don't mask errors from lower layers
6. **Log errors** - For debugging and diagnostics

## Error Code Summary Table

| Code | Value | Meaning | Common Use Cases |
|------|-------|---------|------------------|
| `XY_HAL_OK` | 0 | Success | Normal completion |
| `XY_HAL_ERROR` | -1 | Generic error | Unspecified errors |
| `XY_HAL_ERROR_INVALID_PARAM` | -2 | Invalid parameter | NULL ptr, out of range |
| `XY_HAL_ERROR_NOT_SUPPORT` | -3 | Not supported | Feature unavailable |
| `XY_HAL_ERROR_TIMEOUT` | -4 | Timeout | Comm timeout |
| `XY_HAL_ERROR_BUSY` | -5 | Resource busy | Peripheral in use |
| `XY_HAL_ERROR_NO_MEM` | -6 | Out of memory | Buffer exhausted |
| `XY_HAL_ERROR_IO` | -7 | I/O error | Bus/HW error |
| `XY_HAL_ERROR_NOT_INIT` | -8 | Not initialized | Use before init |
| `XY_HAL_ERROR_ALREADY_INIT` | -9 | Already initialized | Duplicate init |
| `XY_HAL_ERROR_NO_RESOURCE` | -10 | No resource | IRQ/DMA unavailable |
| `XY_HAL_ERROR_FAIL` | -11 | Operation failed | HW failure |

---

**Version**: 2.0
**Last Updated**: 2025-10-26
**Related**: [`xy_hal.h`](../inc/xy_hal.h)
