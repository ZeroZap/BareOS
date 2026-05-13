# XY Broker System

## Overview

XY Broker is a lightweight message broker system designed for embedded systems with inter-domain communication needs. It uses **fixed integer IDs** instead of dynamic strings for optimal performance and predictability.

## Key Features

✅ **Fixed ID Design** - No dynamic string allocation or comparison
✅ **Multiple Patterns** - Point-to-point, pub/sub, and request/response
✅ **Priority Support** - Critical, high, normal, and low priority messages
✅ **Lightweight** - Minimal memory footprint, suitable for MCU
✅ **Type-Safe** - All IDs are compile-time constants
✅ **Statistics** - Built-in message tracking and diagnostics

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                     XY Broker Core                      │
│                                                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │   Servers    │  │    Topics    │  │   Messages   │ │
│  │  (Fixed IDs) │  │  (Fixed IDs) │  │  (Fixed IDs) │ │
│  └──────────────┘  └──────────────┘  └──────────────┘ │
│                                                         │
└─────────────────────────────────────────────────────────┘
         │              │              │
         ▼              ▼              ▼
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│   System     │ │    Power     │ │   Sensor     │
│   Server     │ │   Server     │ │   Server     │
└──────────────┘ └──────────────┘ └──────────────┘
```

## Quick Start

### 1. Initialize the Broker

```c
#include "xy_broker.h"

// Initialize broker system
xy_broker_init();
```

### 2. Register a Server

```c
// Message handler callback
int power_msg_handler(const xy_broker_msg_t *msg, void *user_data) {
    switch (msg->msg_id) {
    case XY_BROKER_MSG_POWER_ON:
        // Handle power on
        break;
    case XY_BROKER_MSG_POWER_OFF:
        // Handle power off
        break;
    }
    return 0;
}

// Register power server
xy_broker_register_server(XY_BROKER_SERVER_POWER,
                          power_msg_handler,
                          NULL);
```

### 3. Send a Message

```c
// Send power on command
uint8_t data[] = {0x01, 0x02, 0x03};
xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM,    // from
                   XY_BROKER_SERVER_POWER,     // to
                   XY_BROKER_MSG_POWER_ON,     // message ID
                   data,                        // payload
                   sizeof(data),               // length
                   XY_BROKER_PRIORITY_HIGH);   // priority
```

### 4. Process Messages

```c
// In your main loop or task
xy_broker_process_msgs(XY_BROKER_SERVER_POWER, 0);  // Process all pending
```

## Usage Patterns

### Pattern 1: Point-to-Point Messaging

Direct message between two servers:

```c
// Sensor server sends data to storage server
typedef struct {
    uint16_t sensor_id;
    float temperature;
    float humidity;
} sensor_data_t;

sensor_data_t data = {
    .sensor_id = 1,
    .temperature = 25.5,
    .humidity = 60.0
};

xy_broker_send_msg(XY_BROKER_SERVER_SENSOR,
                   XY_BROKER_SERVER_STORAGE,
                   XY_BROKER_MSG_SENSOR_DATA,
                   &data,
                   sizeof(data),
                   XY_BROKER_PRIORITY_NORMAL);
```

### Pattern 2: Publish/Subscribe

One-to-many message distribution:

```c
// 1. Create topic
xy_broker_create_topic(XY_BROKER_TOPIC_SENSOR_DATA);

// 2. Subscribe to topic
int storage_topic_handler(const xy_broker_msg_t *msg, void *user_data) {
    // Handle sensor data
    sensor_data_t *data = (sensor_data_t *)msg->payload;
    // Store data...
    return 0;
}

xy_broker_subscribe(XY_BROKER_TOPIC_SENSOR_DATA,
                    XY_BROKER_SERVER_STORAGE,
                    storage_topic_handler,
                    NULL);

// 3. Publish to topic
xy_broker_publish(XY_BROKER_SERVER_SENSOR,
                  XY_BROKER_TOPIC_SENSOR_DATA,
                  XY_BROKER_MSG_SENSOR_DATA,
                  &data,
                  sizeof(data),
                  XY_BROKER_PRIORITY_NORMAL);
```

### Pattern 3: Request/Response

Synchronous request-response pattern:

```c
// Send request and wait for response
typedef struct {
    uint8_t reg_addr;
} config_request_t;

typedef struct {
    uint8_t reg_addr;
    uint32_t value;
} config_response_t;

config_request_t req = { .reg_addr = 0x10 };
xy_broker_msg_t response;

int ret = xy_broker_request(XY_BROKER_SERVER_SYSTEM,
                            XY_BROKER_SERVER_SENSOR,
                            XY_BROKER_MSG_SENSOR_CONFIG,
                            &req,
                            sizeof(req),
                            &response,
                            1000);  // 1 second timeout

if (ret == XY_BROKER_OK) {
    config_response_t *resp = (config_response_t *)response.payload;
    // Use response data...
}
```

## Defining Custom IDs

### Custom Server IDs

```c
// In your application header
typedef enum {
    MY_SERVER_AUDIO     = XY_BROKER_SERVER_USER_BASE + 1,
    MY_SERVER_VIDEO     = XY_BROKER_SERVER_USER_BASE + 2,
    MY_SERVER_BLUETOOTH = XY_BROKER_SERVER_USER_BASE + 3,
} my_server_ids_t;
```

### Custom Message IDs

```c
typedef enum {
    MY_MSG_AUDIO_PLAY   = XY_BROKER_MSG_USER_BASE + 1,
    MY_MSG_AUDIO_STOP   = XY_BROKER_MSG_USER_BASE + 2,
    MY_MSG_AUDIO_VOLUME = XY_BROKER_MSG_USER_BASE + 3,
} my_message_ids_t;
```

### Custom Topic IDs

```c
typedef enum {
    MY_TOPIC_AUDIO_EVENT = XY_BROKER_TOPIC_USER_BASE + 1,
    MY_TOPIC_VIDEO_EVENT = XY_BROKER_TOPIC_USER_BASE + 2,
} my_topic_ids_t;
```

## Complete Example

```c
#include "xy_broker.h"
#include <stdio.h>

// Custom message structure
typedef struct {
    float temperature;
    float humidity;
    uint32_t timestamp;
} sensor_reading_t;

// Sensor server handler
int sensor_handler(const xy_broker_msg_t *msg, void *user_data) {
    printf("Sensor received msg: %s\n",
           xy_broker_get_msg_name(msg->msg_id));
    return 0;
}

// Storage server handler
int storage_handler(const xy_broker_msg_t *msg, void *user_data) {
    if (msg->msg_id == XY_BROKER_MSG_SENSOR_DATA) {
        sensor_reading_t *reading = (sensor_reading_t *)msg->payload;
        printf("Storing: temp=%.1f, humidity=%.1f\n",
               reading->temperature, reading->humidity);
    }
    return 0;
}

// Topic subscriber handler
int topic_handler(const xy_broker_msg_t *msg, void *user_data) {
    sensor_reading_t *reading = (sensor_reading_t *)msg->payload;
    printf("Topic received: temp=%.1f\n", reading->temperature);
    return 0;
}

int main(void) {
    // 1. Initialize broker
    xy_broker_init();

    // 2. Register servers
    xy_broker_register_server(XY_BROKER_SERVER_SENSOR,
                              sensor_handler, NULL);
    xy_broker_register_server(XY_BROKER_SERVER_STORAGE,
                              storage_handler, NULL);
    xy_broker_register_server(XY_BROKER_SERVER_LOG,
                              storage_handler, NULL);

    // 3. Setup pub/sub
    xy_broker_create_topic(XY_BROKER_TOPIC_SENSOR_DATA);
    xy_broker_subscribe(XY_BROKER_TOPIC_SENSOR_DATA,
                       XY_BROKER_SERVER_LOG,
                       topic_handler,
                       NULL);

    // 4. Send point-to-point message
    sensor_reading_t reading = {
        .temperature = 25.5,
        .humidity = 60.0,
        .timestamp = 12345
    };

    xy_broker_send_msg(XY_BROKER_SERVER_SENSOR,
                      XY_BROKER_SERVER_STORAGE,
                      XY_BROKER_MSG_SENSOR_DATA,
                      &reading,
                      sizeof(reading),
                      XY_BROKER_PRIORITY_NORMAL);

    // 5. Publish to topic
    reading.temperature = 26.0;
    xy_broker_publish(XY_BROKER_SERVER_SENSOR,
                     XY_BROKER_TOPIC_SENSOR_DATA,
                     XY_BROKER_MSG_SENSOR_DATA,
                     &reading,
                     sizeof(reading),
                     XY_BROKER_PRIORITY_NORMAL);

    // 6. Process messages
    xy_broker_process_msgs(XY_BROKER_SERVER_STORAGE, 0);

    // 7. Get statistics
    xy_broker_stats_t stats;
    xy_broker_get_stats(&stats);
    printf("Total messages sent: %lu\n", stats.total_msg_sent);
    printf("Active servers: %lu\n", stats.active_servers);

    // 8. Cleanup
    xy_broker_deinit();

    return 0;
}
```

## Configuration

Adjust these values in `xy_broker.h`:

```c
#define XY_BROKER_MAX_SERVERS        16   // Max number of servers
#define XY_BROKER_MAX_SUBSCRIBERS    32   // Max subscribers per topic
#define XY_BROKER_MAX_TOPICS         64   // Max number of topics
#define XY_BROKER_MSG_QUEUE_SIZE     32   // Queue size per server
#define XY_BROKER_MAX_MSG_SIZE      256   // Max message payload
```

## API Reference

### Core Functions

| Function | Description |
|----------|-------------|
| `xy_broker_init()` | Initialize broker system |
| `xy_broker_deinit()` | Cleanup broker system |
| `xy_broker_register_server()` | Register a server |
| `xy_broker_unregister_server()` | Unregister a server |
| `xy_broker_send_msg()` | Send point-to-point message |
| `xy_broker_process_msgs()` | Process pending messages |

### Pub/Sub Functions

| Function | Description |
|----------|-------------|
| `xy_broker_create_topic()` | Create a topic |
| `xy_broker_subscribe()` | Subscribe to topic |
| `xy_broker_unsubscribe()` | Unsubscribe from topic |
| `xy_broker_publish()` | Publish to topic |

### Request/Response Functions

| Function | Description |
|----------|-------------|
| `xy_broker_request()` | Send request, wait for response |
| `xy_broker_respond()` | Send response to request |

### Utility Functions

| Function | Description |
|----------|-------------|
| `xy_broker_get_stats()` | Get broker statistics |
| `xy_broker_is_server_registered()` | Check if server exists |
| `xy_broker_get_pending_count()` | Get pending message count |
| `xy_broker_clear_queue()` | Clear message queue |

## Performance

### Memory Usage (Default Configuration)

- Server slots: 16 × ~300 bytes = ~4.8 KB
- Topic slots: 64 × ~200 bytes = ~12.8 KB
- Total: ~18 KB RAM

### Throughput

- Message delivery: O(1) for point-to-point
- Topic publish: O(N) where N = subscribers
- Queue operations: O(1)

## Best Practices

1. **Use Fixed IDs** - Define all IDs at compile time
2. **Size Messages** - Keep payloads small, use pointers for large data
3. **Handle Errors** - Always check return codes
4. **Process Regularly** - Call `process_msgs()` frequently
5. **Set Priorities** - Use priority for time-critical messages
6. **Monitor Queues** - Check for queue overflow

## Thread Safety

⚠️ **Not thread-safe by default**. If using in multi-threaded environment:

1. Add mutex protection around broker calls
2. Use separate servers per thread
3. Process messages in a dedicated task

## Integration with XY Framework

```c
// Integrate with xy_tick for timestamp
uint32_t broker_get_timestamp(void) {
    return xy_tick_get_ms();
}

// Integrate with xy_log
#define BROKER_LOG_INFO(...)  xy_log_info("BROKER", __VA_ARGS__)
#define BROKER_LOG_ERROR(...) xy_log_error("BROKER", __VA_ARGS__)
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Queue full | Increase `XY_BROKER_MSG_QUEUE_SIZE` or process faster |
| Out of memory | Reduce `MAX_SERVERS/TOPICS/SUBSCRIBERS` |
| Message not received | Check server is registered and processing |
| Topic not working | Ensure topic created before subscribe |

## License

Part of the XY embedded framework. See project LICENSE.

## See Also

- [xy_config.h](../../xy_config.h) - System configuration
- [ipc/completion](../completion/) - Completion mechanism
- [trace/xy_log](../../trace/xy_log/) - Logging system
