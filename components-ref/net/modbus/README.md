# Modbus RTU Slave Implementation

## Overview

Lightweight Modbus RTU slave implementation optimized for resource-constrained microcontrollers. Provides complete support for standard Modbus RTU protocol with minimal memory footprint.

## Features

### ✅ Supported Function Codes

| Code | Function | Description |
|------|----------|-------------|
| 0x01 | Read Coils | Read 1-2000 coils (0x references) |
| 0x02 | Read Discrete Inputs | Read 1-2000 discrete inputs (1x references) |
| 0x03 | Read Holding Registers | Read 1-125 holding registers (4x references) |
| 0x04 | Read Input Registers | Read 1-125 input registers (3x references) |
| 0x05 | Write Single Coil | Write one coil |
| 0x06 | Write Single Register | Write one holding register |
| 0x0F | Write Multiple Coils | Write 1-1968 coils |
| 0x10 | Write Multiple Registers | Write 1-123 holding registers |

### ✅ Key Features

- **Compact**: <2KB code, configurable data size
- **Fast CRC**: Optimized CRC16 calculation
- **Frame Detection**: Automatic frame timeout based on baud rate
- **Error Handling**: Complete exception code support
- **Callbacks**: Optional write notifications
- **Statistics**: Request/exception/error counters
- **Thread-Safe**: Can be used with RTOS mutex
- **Broadcast Support**: Recognizes broadcast address (0)

## Memory Footprint

### Default Configuration
```c
#define MB_COIL_COUNT          64    // 8 bytes
#define MB_DISCRETE_COUNT      64    // 8 bytes
#define MB_INPUT_REG_COUNT     32    // 64 bytes
#define MB_HOLDING_REG_COUNT   32    // 64 bytes
#define MB_RX_BUFFER_SIZE      256   // 256 bytes
#define MB_TX_BUFFER_SIZE      256   // 256 bytes
```

**Total RAM**: ~660 bytes + mb_slave_t struct (~60 bytes) = **~720 bytes**

### Code Size
- **Core**: ~1.5 KB
- **CRC16**: ~100 bytes
- **Total**: **~1.6 KB**

## Quick Start

### 1. Configuration

Edit in `mb_slave.h` or define before including:

```c
#define MB_SLAVE_ADDRESS        1      // Your slave address
#define MB_UART_BAUDRATE        9600   // Baud rate
#define MB_COIL_COUNT           64     // Number of coils
#define MB_DISCRETE_COUNT       64     // Number of discrete inputs
#define MB_INPUT_REG_COUNT      32     // Number of input registers
#define MB_HOLDING_REG_COUNT    32     // Number of holding registers
```

### 2. Implement HAL Functions

You must implement these 4 functions:

```c
// Send single byte via UART
void mb_uart_send_byte(uint8_t data) {
    // Your UART send implementation
    UART1->DR = data;
    while (!(UART1->SR & UART_SR_TXE));
}

// Send buffer via UART
void mb_uart_send_buffer(const uint8_t *buffer, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        mb_uart_send_byte(buffer[i]);
    }
}

// Enable/disable UART RX (optional, can be empty)
void mb_uart_enable_rx(bool enable) {
    if (enable) {
        UART1->CR1 |= UART_CR1_RXNEIE;
    } else {
        UART1->CR1 &= ~UART_CR1_RXNEIE;
    }
}

// Get system time in milliseconds
uint32_t mb_get_time_ms(void) {
    return HAL_GetTick();  // Or your tick counter
}
```

### 3. Basic Usage

```c
#include "mb_slave.h"

static mb_slave_t modbus;

int main(void) {
    // Initialize
    mb_slave_init(&modbus, 1, 9600);  // Address 1, 9600 baud

    // Set initial values
    mb_slave_set_holding_register(&modbus, 0, 1000);
    mb_slave_set_input_register(&modbus, 0, 500);

    while (1) {
        // Poll for complete frames
        mb_slave_poll(&modbus, mb_get_time_ms());

        // Update inputs periodically
        uint16_t adc_value = read_adc();
        mb_slave_set_input_register(&modbus, 0, adc_value);
    }
}

// UART RX interrupt
void UART1_IRQHandler(void) {
    if (UART1->SR & UART_SR_RXNE) {
        uint8_t data = UART1->DR;
        mb_slave_receive_byte(&modbus, data);
    }
}
```

## Detailed Examples

### Example 1: Bare-Metal (STM32)

```c
#include "mb_slave.h"
#include "stm32f1xx.h"

static mb_slave_t g_modbus;
volatile uint32_t g_system_ticks = 0;

// SysTick handler (1ms)
void SysTick_Handler(void) {
    g_system_ticks++;
}

// HAL implementations
void mb_uart_send_byte(uint8_t data) {
    USART1->DR = data;
    while (!(USART1->SR & USART_SR_TXE));
}

void mb_uart_send_buffer(const uint8_t *buffer, uint16_t length) {
    for (uint16_t i = 0; i < length; i++) {
        mb_uart_send_byte(buffer[i]);
    }
}

void mb_uart_enable_rx(bool enable) {
    if (enable) {
        USART1->CR1 |= USART_CR1_RXNEIE;
    } else {
        USART1->CR1 &= ~USART_CR1_RXNEIE;
    }
}

uint32_t mb_get_time_ms(void) {
    return g_system_ticks;
}

// UART RX interrupt
void USART1_IRQHandler(void) {
    if (USART1->SR & USART_SR_RXNE) {
        uint8_t data = USART1->DR;
        mb_slave_receive_byte(&g_modbus, data);
    }
}

// Main
int main(void) {
    // Initialize system (72MHz, 1ms SysTick)
    SystemInit();
    SysTick_Config(72000);

    // Initialize UART1 (9600 baud, 8N1)
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN | RCC_APB2ENR_IOPAEN;
    // ... UART configuration ...

    // Initialize Modbus slave
    mb_slave_init(&g_modbus, 1, 9600);

    while (1) {
        mb_slave_poll(&g_modbus, g_system_ticks);

        // Update sensors every 100ms
        static uint32_t last_update = 0;
        if ((g_system_ticks - last_update) >= 100) {
            last_update = g_system_ticks;

            // Read ADC and update input register 0
            uint16_t adc = ADC1->DR;
            mb_slave_set_input_register(&g_modbus, 0, adc);
        }
    }
}
```

### Example 2: With Callbacks

```c
// Callback when coil is written
void on_coil_changed(uint16_t address, bool value) {
    if (address == 0) {
        // Coil 0 controls LED
        if (value) {
            GPIOC->BSRR = GPIO_Pin_13;  // LED ON
        } else {
            GPIOC->BRR = GPIO_Pin_13;   // LED OFF
        }
    }
}

// Callback when register is written
void on_register_changed(uint16_t address, uint16_t value) {
    if (address == 0) {
        // Register 0 controls PWM duty cycle
        TIM3->CCR1 = value;
    }
}

int main(void) {
    mb_slave_init(&g_modbus, 1, 9600);

    // Set callbacks
    mb_slave_set_coil_callback(&g_modbus, on_coil_changed);
    mb_slave_set_register_callback(&g_modbus, on_register_changed);

    // ... rest of code ...
}
```

### Example 3: With RTOS (FreeRTOS/RT-Thread)

```c
#include "mb_slave.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

static mb_slave_t g_modbus;
static SemaphoreHandle_t g_mb_mutex;

void modbus_task(void *param) {
    while (1) {
        xSemaphoreTake(g_mb_mutex, portMAX_DELAY);
        mb_slave_poll(&g_modbus, xTaskGetTickCount());
        xSemaphoreGive(g_mb_mutex);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void sensor_task(void *param) {
    while (1) {
        uint16_t temperature = read_sensor();

        xSemaphoreTake(g_mb_mutex, portMAX_DELAY);
        mb_slave_set_input_register(&g_modbus, 0, temperature);
        xSemaphoreGive(g_mb_mutex);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void) {
    mb_slave_init(&g_modbus, 1, 9600);

    g_mb_mutex = xSemaphoreCreateMutex();

    xTaskCreate(modbus_task, "Modbus", 256, NULL, 2, NULL);
    xTaskCreate(sensor_task, "Sensor", 128, NULL, 1, NULL);

    vTaskStartScheduler();
}
```

## Data Model

### Coils (0x, Read/Write)
- Digital outputs controlled by master
- Addresses: 0 to MB_COIL_COUNT-1
- Functions: 0x01 (read), 0x05 (write single), 0x0F (write multiple)

### Discrete Inputs (1x, Read Only)
- Digital inputs monitored by master
- Addresses: 0 to MB_DISCRETE_COUNT-1
- Function: 0x02 (read)

### Input Registers (3x, Read Only)
- Analog inputs, sensor values
- Addresses: 0 to MB_INPUT_REG_COUNT-1
- Function: 0x04 (read)
- Size: 16-bit per register

### Holding Registers (4x, Read/Write)
- Configuration, setpoints
- Addresses: 0 to MB_HOLDING_REG_COUNT-1
- Functions: 0x03 (read), 0x06 (write single), 0x10 (write multiple)
- Size: 16-bit per register

## Timing

### Frame Timeout Calculation

The slave automatically calculates frame timeout based on baud rate:

```
T3.5 = 3.5 × (11 bits / baudrate) × 1000 ms
```

| Baud Rate | Frame Timeout |
|-----------|--------------|
| 9600 | ~4 ms |
| 19200 | ~2 ms |
| 38400 | ~1 ms |
| 115200 | <1 ms (minimum 2ms used) |

### Recommended Polling Interval

- **Bare-metal**: Poll every 1-10ms in main loop
- **RTOS**: 10-20ms task period
- **High speed**: Poll more frequently for fast response

## Error Handling

### Exception Codes

| Code | Name | Cause |
|------|------|-------|
| 0x01 | Illegal Function | Unsupported function code |
| 0x02 | Illegal Data Address | Address out of range |
| 0x03 | Illegal Data Value | Invalid value or quantity |
| 0x04 | Slave Device Failure | Internal error |

### Statistics

```c
uint32_t requests, exceptions, crc_errors;
mb_slave_get_stats(&modbus, &requests, &exceptions, &crc_errors);

printf("Total Requests: %lu\n", requests);
printf("Exceptions: %lu\n", exceptions);
printf("CRC Errors: %lu\n", crc_errors);
```

## Testing

### Test with Modbus Poll (Windows)

1. Connect slave to PC via RS485/UART adapter
2. Open Modbus Poll
3. Configure: RTU, 9600 baud, 8N1
4. Set slave address (default: 1)
5. Read holding registers (function 0x03, address 0, count 10)

### Test with modpoll (Linux)

```bash
# Read 10 holding registers starting at address 0
modpoll -m rtu -b 9600 -a 1 -t 4 -r 1 -c 10 /dev/ttyUSB0

# Write single coil (address 0, value ON)
modpoll -m rtu -b 9600 -a 1 -t 0 -r 1 -c 1 /dev/ttyUSB0 1

# Write holding register (address 0, value 1234)
modpoll -m rtu -b 9600 -a 1 -t 4 -r 1 /dev/ttyUSB0 1234
```

## Troubleshooting

### Q: No response from slave
- Check UART configuration (baud rate, parity, stop bits)
- Verify slave address matches
- Check RS485 DE/RE pins (transmit enable)
- Measure timing with oscilloscope

### Q: CRC errors
- Check baud rate accuracy
- Verify UART timing (no dropped bytes)
- Check for electrical noise on RS485 bus
- Add termination resistors (120Ω) on RS485

### Q: Frame timeout too short
- Increase `MB_RX_BUFFER_SIZE`
- Lower baud rate
- Check `mb_get_time_ms()` accuracy

### Q: Missed bytes in RX interrupt
- Increase UART RX buffer
- Use DMA for UART reception
- Reduce interrupt latency

## Performance Optimization

### 1. Use DMA for UART

```c
void mb_uart_send_buffer(const uint8_t *buffer, uint16_t length) {
    HAL_UART_Transmit_DMA(&huart1, buffer, length);
    while (huart1.gState != HAL_UART_STATE_READY);
}
```

### 2. Optimize Polling

```c
// Only poll when data received
static bool frame_pending = false;

void mb_slave_receive_byte(mb_slave_t *slave, uint8_t data) {
    // ... receive byte ...
    frame_pending = true;
}

void main_loop(void) {
    if (frame_pending) {
        mb_slave_poll(&modbus, get_time());
        frame_pending = false;
    }
}
```

### 3. Reduce Buffer Size

For low data volume:
```c
#define MB_RX_BUFFER_SIZE 64
#define MB_TX_BUFFER_SIZE 64
```

## Porting to Other MCUs

1. **Implement 4 HAL functions** (see Quick Start)
2. **Configure UART** (baud rate, 8N1, RX interrupt)
3. **Setup SysTick** (1ms preferred)
4. **Call `mb_slave_receive_byte()`** from UART ISR
5. **Call `mb_slave_poll()`** periodically

## License

Same as XinYi project

## Version

- **Version**: 1.0.0
- **Date**: 2025-10-27
- **Status**: Production Ready
