# XY Broker - Quick Reference Card

## Initialization
```c
#include "xy_broker.h"

xy_broker_init();                    // Start broker
xy_broker_deinit();                  // Stop broker
```

## Register Server
```c
int my_handler(const xy_broker_msg_t *msg, void *ctx) {
    // Handle message
    return 0;
}

xy_broker_register_server(XY_BROKER_SERVER_SENSOR,
                          my_handler, NULL);
```

## Send Message
```c
uint8_t data[] = {1, 2, 3};
xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM,     // from
                   XY_BROKER_SERVER_SENSOR,     // to
                   XY_BROKER_MSG_SENSOR_DATA,   // what
                   data, sizeof(data),          // payload
                   XY_BROKER_PRIORITY_NORMAL);  // priority
```

## Process Messages
```c
xy_broker_process_msgs(XY_BROKER_SERVER_SENSOR, 0);  // Process all
xy_broker_process_msgs(XY_BROKER_SERVER_SENSOR, 5);  // Process max 5
```

## Pub/Sub
```c
// Create & subscribe
xy_broker_create_topic(XY_BROKER_TOPIC_SENSOR_DATA);
xy_broker_subscribe(XY_BROKER_TOPIC_SENSOR_DATA,
                   XY_BROKER_SERVER_STORAGE,
                   storage_handler, NULL);

// Publish
xy_broker_publish(XY_BROKER_SERVER_SENSOR,
                 XY_BROKER_TOPIC_SENSOR_DATA,
                 XY_BROKER_MSG_SENSOR_DATA,
                 data, len, XY_BROKER_PRIORITY_NORMAL);
```

## Request/Response
```c
xy_broker_msg_t response;
xy_broker_request(SRC_SERVER, DST_SERVER, MSG_ID,
                 req_data, req_len,
                 &response, 1000);  // 1 sec timeout
```

## Statistics
```c
xy_broker_stats_t stats;
xy_broker_get_stats(&stats);
printf("Messages sent: %lu\n", stats.total_msg_sent);

int pending = xy_broker_get_pending_count(SERVER_ID);
```

## Predefined IDs

### Servers
```c
XY_BROKER_SERVER_SYSTEM      // 0x0001
XY_BROKER_SERVER_POWER       // 0x0002
XY_BROKER_SERVER_SENSOR      // 0x0004
XY_BROKER_SERVER_STORAGE     // 0x0005
// ... more in xy_broker.h
```

### Messages
```c
XY_BROKER_MSG_SYSTEM_INIT    // 0x0001
XY_BROKER_MSG_POWER_ON       // 0x0101
XY_BROKER_MSG_SENSOR_DATA    // 0x0301
XY_BROKER_MSG_STORAGE_WRITE  // 0x0402
// ... more in xy_broker.h
```

### Topics
```c
XY_BROKER_TOPIC_SYSTEM_EVENT   // 0x0001
XY_BROKER_TOPIC_SENSOR_DATA    // 0x0003
XY_BROKER_TOPIC_ALARM_EVENT    // 0x0005
// ... more in xy_broker.h
```

## Custom IDs
```c
// Define your own
#define MY_SERVER_AUDIO  (XY_BROKER_SERVER_USER_BASE + 1)
#define MY_MSG_PLAY      (XY_BROKER_MSG_USER_BASE + 1)
#define MY_TOPIC_EVENT   (XY_BROKER_TOPIC_USER_BASE + 1)
```

## Priority Levels
```c
XY_BROKER_PRIORITY_CRITICAL  // 3 - Urgent
XY_BROKER_PRIORITY_HIGH      // 2 - Important
XY_BROKER_PRIORITY_NORMAL    // 1 - Default
XY_BROKER_PRIORITY_LOW       // 0 - Background
```

## Error Codes
```c
XY_BROKER_OK              //  0 - Success
XY_BROKER_ERROR           // -1 - General error
XY_BROKER_INVALID_PARAM   // -2 - Bad parameter
XY_BROKER_NO_MEMORY       // -3 - Out of memory
XY_BROKER_QUEUE_FULL      // -4 - Queue overflow
XY_BROKER_NOT_FOUND       // -5 - Not registered
XY_BROKER_TIMEOUT         // -6 - Request timeout
```

## Configuration
```c
// In xy_broker.h, adjust these:
#define XY_BROKER_MAX_SERVERS       16   // Max servers
#define XY_BROKER_MAX_SUBSCRIBERS   32   // Max subs/topic
#define XY_BROKER_MAX_TOPICS        64   // Max topics
#define XY_BROKER_MSG_QUEUE_SIZE    32   // Queue size
#define XY_BROKER_MAX_MSG_SIZE     256   // Max payload
```

## Message Structure
```c
typedef struct {
    uint16_t msg_id;         // Message ID
    uint16_t src_server;     // Source server
    uint16_t dst_server;     // Dest server
    uint16_t topic_id;       // Topic (for pub/sub)
    uint8_t priority;        // Priority level
    uint8_t flags;           // Flags
    uint16_t seq_num;        // Sequence number
    uint32_t timestamp;      // Timestamp
    uint16_t payload_len;    // Payload length
    uint8_t payload[...];    // Data
} xy_broker_msg_t;
```

## Complete Example
```c
#include "xy_broker.h"

int sensor_handler(const xy_broker_msg_t *msg, void *ctx) {
    if (msg->msg_id == XY_BROKER_MSG_SENSOR_DATA) {
        // Process sensor data
    }
    return 0;
}

int main(void) {
    // 1. Init
    xy_broker_init();

    // 2. Register
    xy_broker_register_server(XY_BROKER_SERVER_SENSOR,
                              sensor_handler, NULL);

    // 3. Send
    uint8_t data[] = {0x12, 0x34};
    xy_broker_send_msg(XY_BROKER_SERVER_SYSTEM,
                      XY_BROKER_SERVER_SENSOR,
                      XY_BROKER_MSG_SENSOR_DATA,
                      data, sizeof(data),
                      XY_BROKER_PRIORITY_NORMAL);

    // 4. Process
    xy_broker_process_msgs(XY_BROKER_SERVER_SENSOR, 0);

    // 5. Cleanup
    xy_broker_deinit();
    return 0;
}
```

## Build & Run
```bash
cd components/ipc/xy_broker
make clean && make
./xy_broker_example
```

## Memory Usage (Default)
- RAM: ~18 KB
- Code: ~4-5 KB

## See Also
- [README.md](README.md) - Full documentation
- [example.c](example.c) - Complete examples
- [xy_broker.h](xy_broker.h) - API reference
