# XY OSAL Bare-metal Backend

## Overview

The bare-metal backend provides minimal OSAL functionality for systems without an RTOS. It implements basic kernel control and delay functions, while threading and synchronization primitives return errors or stubs.

## Features

### ✅ Supported
- Kernel initialization and state management
- Kernel lock/unlock (simple counter)
- Tick count access via `xy_tick` module
- Delay functions (busy-wait implementation)
- Basic kernel info query

### ❌ Not Supported
- Thread creation/management
- Mutexes
- Semaphores
- Event flags
- Message queues
- Memory pools
- Timers

All unsupported functions return appropriate error codes (`XY_OS_ERROR`, `NULL`, etc.)

## Implementation Details

### Kernel Control

- **State**: Tracks kernel state (INACTIVE → READY → RUNNING)
- **Lock**: Simple lock counter, increments on lock, decrements on unlock
- **Tick**: Delegates to `xy_tick_get()` from `components/kernel/xy_tick.h`

### Delay

```c
xy_os_status_t xy_os_delay(uint32_t ticks)
```

Implements busy-wait delay:
- Reads start tick
- Loops until elapsed ticks >= requested ticks
- **Warning**: Blocks CPU, no power saving

### Thread Stubs

- `xy_os_thread_get_id()` returns dummy ID `0x1`
- `xy_os_thread_get_name()` returns `"main"`
- `xy_os_thread_get_count()` returns `1`
- All thread creation/management functions return error

## Usage Example

```c
#include "components/osal/xy_os.h"

int main(void)
{
    // Initialize kernel
    xy_os_kernel_init();
    
    // Start kernel (moves to RUNNING state)
    xy_os_kernel_start();
    
    while (1) {
        // Main loop
        
        // Delay 1000 ticks (1 second at 1kHz)
        xy_os_delay(1000);
        
        // Get current tick
        uint32_t tick = xy_os_kernel_get_tick_count();
    }
}
```

## Critical Sections

```c
// Disable interrupts or lock
int32_t lock = xy_os_kernel_lock();

// Critical section
// ...

// Restore
xy_os_kernel_restore_lock(lock);
```

**Note**: In bare-metal, lock/unlock only maintains a counter. You must implement actual interrupt disable/enable based on your MCU.

## Integration

### Requirements

- `components/kernel/xy_tick.h` must be available
- `xy_tick_get()` must return current tick count
- Tick frequency should be 1kHz (1ms per tick)

### Build

Add to your makefile:

```makefile
SRC_FILES += components/osal/baremetal/xy_os_baremetal.c
INCLUDE_DIRS += components/osal
```

### Linker

No special linker requirements.

## Limitations

1. **No Multitasking**: Only single-threaded execution
2. **Busy-Wait Delays**: CPU runs at full speed during delays
3. **No Synchronization**: Cannot share resources between ISR and main
4. **No Power Management**: Cannot enter low-power modes during delays

## When to Use

✅ **Good for:**
- Simple bootloaders
- Hardware initialization code
- Single-task applications
- Testing/debugging without RTOS

❌ **Not suitable for:**
- Multi-threaded applications
- Complex state machines requiring task switching
- Power-sensitive applications
- Applications requiring inter-task communication

## Migration Path

To migrate from bare-metal to RTOS:

1. Replace `components/osal/baremetal/xy_os_baremetal.c` with RTOS backend
2. No application code changes needed (same API)
3. Add RTOS-specific initialization
4. Convert main loop to RTOS task
5. Add threading/synchronization as needed

## Performance

- **Kernel overhead**: Negligible (just counter and state variable)
- **Delay accuracy**: Depends on `xy_tick_get()` accuracy
- **Memory footprint**: ~100 bytes (code + data)
- **Interrupt latency**: Not affected by OSAL

## API Coverage

| Function Category | Support Level | Notes |
|-------------------|--------------|-------|
| Kernel control | Full | Init, start, lock, unlock, tick |
| Thread mgmt | Stub only | Returns errors |
| Thread flags | Stub only | Returns errors |
| Delay | Full | Busy-wait implementation |
| Timer | None | Returns NULL/error |
| Event flags | None | Returns NULL/error |
| Mutex | None | Returns NULL/error |
| Semaphore | None | Returns NULL/error |
| Memory pool | None | Returns NULL/error |
| Message queue | None | Returns NULL/error |

## Future Enhancements

Potential improvements:
- Software timer list using tick interrupts
- Simple cooperative scheduler
- Basic semaphore using interrupt disable
- Memory pool using static allocation

## Troubleshooting

**Q: Delay doesn't work**
- Check that `xy_tick_get()` is incrementing
- Verify tick ISR is running
- Ensure tick frequency is configured correctly

**Q: Kernel lock has no effect**
- Bare-metal lock is just a counter
- You must add platform-specific interrupt disable
- See your MCU's reference manual for critical section macros

**Q: Thread functions return errors**
- Expected behavior in bare-metal
- Indicates feature not supported
- Migrate to RTOS backend if needed

## Example Integration

```c
// main.c
#include "components/osal/xy_os.h"
#include "bsp/xy_hal/inc/xy_hal.h"

void system_init(void)
{
    // HAL init
    xy_hal_init();
    
    // OSAL init
    xy_os_kernel_init();
    xy_os_kernel_start();
}

int main(void)
{
    system_init();
    
    while (1) {
        // Application logic
        process_tasks();
        
        // Delay
        xy_os_delay(10);
    }
}
```

## Version

- **Version**: 1.0.0
- **Date**: 2025-10-27
- **Status**: Stable
