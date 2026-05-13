# XinYi Net Component

## Overview

The `net` component provides network communication protocols for embedded systems, including AT commands, Modbus, MQTT, CAN, and LTE modules.

## Component Structure

```
components/net/
├── at/                          # AT Command implementations
│   ├── AT-Command-V2/          # Version 2 AT command framework
│   ├── FreeRTOS-Cellular-Interface/  # FreeRTOS cellular support
│   ├── atc/                    # Basic AT command parser
│   ├── ats/                    # AT command server
│   └── rtthread-at/           # RT-Thread AT package
├── xy_mqtt/                    # MQTT protocol (INCOMPLETE)
├── modbus/                     # Modbus RTU Slave (Production Ready)
├── modbus_tiny/               # Lightweight Modbus RTU
├── modbus_full/               # Full-featured Modbus (RTU + TCP + ASCII)
├── smbus/                      # SMBus protocol
├── xy_iso7816/                # ISO7816 smart card
├── src/                        # Source implementations
│   ├── nano_modbus.c          # Unified Modbus API
│   ├── xy_can.c              # CAN bus implementation
│   ├── xy_lte.c              # LTE/4G module driver (STUB)
│   ├── xy_net.c              # Network core
│   └── xy_net_platform.c     # Platform abstraction
├── inc/                        # Public headers
│   ├── nano_modbus.h
│   ├── xy_can.h
│   ├── xy_lte.h
│   ├── xy_net.h
│   └── xy_net_config.h
├── CMakeLists.txt
├── Kconfig
└── Makefile
```

## Module Status

| Module | Status | Description |
|--------|--------|-------------|
| **Modbus** | ✅ Production Ready | Full RTU slave implementation with examples |
| **Modbus Tiny** | ✅ Production Ready | Lightweight version for constrained MCUs |
| **Modbus Full** | ✅ Production Ready | RTU + TCP + ASCII support |
| **CAN** | ✅ Implemented | Full CAN bus with FIFO and callbacks |
| **LTE** | ⚠️ Stub | Skeleton implementation, needs completion |
| **MQTT** | ❌ Incomplete | Stub only, not functional |
| **AT Client** | ⚠️ varies | Multiple implementations available |

## AT Client/Server Architecture

### Available Implementations

1. **AT-Command-V2** - Modern AT command framework with command registration
2. **ats** - Lightweight AT command server
3. **atc** - Basic AT command client
4. **FreeRTOS-Cellular-Interface** - FreeRTOS integration for cellular modules
5. **rtthread-at** - RT-Thread OS AT package

### Quick Start (AT Command V2)

```c
#include "at_command_v2.h"

// Define command handler
at_cmd_status_t handle_test(at_context_t *ctx, char *resp) {
    at_response(ctx, "OK");
    return AT_OK;
}

// Register and run
at_init();
at_register("AT+TEST", handle_test);
at_process();
```

## Modbus RTU Slave

### Features
- **Function Codes**: 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0F, 0x10
- **Memory Footprint**: ~720 bytes RAM, ~1.6 KB flash
- **Configurable**: Coil, discrete, holding, input register counts

### Configuration

```c
#define MB_SLAVE_ADDRESS      1
#define MB_UART_BAUDRATE      9600
#define MB_COIL_COUNT         64
#define MB_HOLDING_REG_COUNT  32
```

### Basic Usage

```c
#include "mb_slave.h"

static mb_slave_t modbus;

int main(void) {
    mb_slave_init(&modbus, 1, 9600);

    while (1) {
        mb_slave_poll(&modbus, mb_get_time_ms());
    }
}

// UART RX interrupt
void UART1_IRQHandler(void) {
    if (UART1->SR & UART_SR_RXNE) {
        mb_slave_receive_byte(&modbus, UART1->DR);
    }
}
```

### Data Access

```c
// Set holding register
mb_slave_set_holding_register(&modbus, 0, 1234);

// Get input register
uint16_t value = mb_slave_get_input_register(&modbus, 0);

// Set callbacks
mb_slave_set_register_callback(&modbus, on_register_changed);
```

### See Also
- [Modbus README](modbus/README.md) - Complete documentation with examples

## CAN Bus

### Features
- FIFO-based TX/RX buffers
- Configurable baudrate
- Interrupt-driven reception
- Callback support

### Usage

```c
#include "xy_can.h"

xy_can_t can;
xy_can_config_t config = {
    .baudrate = 500000,
    .rx_fifo_size = 16,
    .tx_fifo_size = 16,
};

xy_can_init(&can, hw_handle, &config);
xy_can_start(&can);

// Send message
xy_can_msg_t msg = {.id = 0x100, .len = 8};
xy_can_send(&can, &msg, 1000);

// Register callback
xy_can_register_rx_callback(&can, on_can_receive, user_data);
```

## LTE/4G Module

⚠️ **This module is a stub implementation**

### Status
- Header complete with full API definition
- Implementation has `(void)` stubs for all functions
- Needs UART HAL integration and actual AT command handling

### Supported Modules (Planned)
- 移远 EC100Y/EC200Y
- 广和通 L610/L630
- 合宙 Air780E

### Planned Features
- [ ] Network attachment
- [ ] PDP context management
- [ ] TCP/UDP connections
- [ ] Data transmission
- [ ] Signal quality monitoring

## MQTT Protocol

⚠️ **NOT FUNCTIONAL - STUB ONLY**

The MQTT implementation is incomplete and should not be used.

### Current State
- Basic packet structure defined in header
- `xy_mqtt_parse()` function has syntax errors
- No packet encoding/decoding implemented
- No PING, CONNACK, PUBLISH handling

### Missing Implementation
- [ ] CONNACK packet handling
- [ ] PINGREQ/PINGRESP (keep-alive)
- [ ] PUBLISH QoS 0/1/2
- [ ] SUBSCRIBE/SUBACK
- [ ] UNSUBSCRIBE/UNSUBACK
- [ ] PUBACK/PUBREC/PUBREL/PUBCOMP
- [ ] Connection state machine
- [ ] Topic filtering
- [ ] Will message support

### See Also
- [MQTT Implementation Plan](xy_mqtt/MQTT_IMPLEMENTATION_PLAN.md)

## Build Instructions

### Using CMake

```bash
# In your project CMakeLists.txt
add_subdirectory(components/net)
target_link_libraries(your_app PRIVATE xy_net)

# Or using Kconfig
# Run `make menuconfig` and enable modules under Components → Net
```

### Using Make

```bash
# Set NET_DIR in your Makefile
NET_DIR = components/net
include $(NET_DIR)/Makefile
```

### Configuration Options

Via Kconfig or defines:

```c
// Modbus configuration
#define MB_COIL_COUNT         64
#define MB_DISCRETE_COUNT     64
#define MB_INPUT_REG_COUNT    32
#define MB_HOLDING_REG_COUNT  32

// CAN configuration
#define XY_CAN_RX_FIFO_SIZE  16
#define XY_CAN_TX_FIFO_SIZE  16
```

## API Reference

### Modbus API

| Function | Description |
|----------|-------------|
| `mb_slave_init()` | Initialize Modbus slave |
| `mb_slave_poll()` | Poll for complete frames |
| `mb_slave_receive_byte()` | Feed received byte |
| `mb_slave_set_holding_register()` | Set holding register |
| `mb_slave_get_holding_register()` | Get holding register |
| `mb_slave_set_coil()` | Set coil state |
| `mb_slave_get_coil()` | Get coil state |
| `mb_crc16()` | Calculate CRC16 |

### CAN API

| Function | Description |
|----------|-------------|
| `xy_can_init()` | Initialize CAN |
| `xy_can_start()` | Start CAN |
| `xy_can_stop()` | Stop CAN |
| `xy_can_send()` | Send CAN message |
| `xy_can_receive()` | Receive CAN message |
| `xy_can_register_rx_callback()` | Register RX callback |

### LTE API

| Function | Description |
|----------|-------------|
| `xy_lte_init()` | Initialize LTE module |
| `xy_lte_attach()` | Attach to network |
| `xy_lte_connect()` | TCP/UDP connection |
| `xy_lte_send()` | Send data |
| `xy_lte_recv()` | Receive data |
| `xy_lte_get_signal()` | Get signal quality |

## Troubleshooting

### Modbus
- **No response**: Check UART config (baud rate, parity)
- **CRC errors**: Verify electrical connections, add termination resistor
- **Missed bytes**: Use DMA for UART RX

### CAN
- **No TX interrupts**: Check hardware initialization
- **FIFO overflow**: Increase buffer size or poll more frequently

### LTE
- **AT commands fail**: Verify UART wiring and AT command syntax
- **Network attach fails**: Check SIM card and antenna

## Version

- **Version**: 1.0.0
- **Date**: 2025-10-27
- **Status**: Mixed (Modbus production-ready, others incomplete)
