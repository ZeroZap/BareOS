# MQTT Implementation Plan

## Current State Analysis

### File: `xy_mqtt/xy_mqtt.h`

**What's defined:**
- `xy_mqtt_fix_header` union with bit fields for:
  - `retain`, `qos`, `dup`, `type` (for CONNECT)
  - Generic bit access
- `xy_mqtt_connect_packet` structure with:
  - Fix header
  - Protocol name (4 bytes "MQTT")
  - Protocol level (4 = 3.1.1)
  - Connection flags (clean_session, will, will_qos, will_retain)
- `xy_mqtt` context struct with status and packet pointer

**Issues:**
- Missing many packet types (CONNACK, PUBLISH, etc.)
- No remaining length encoding
- No packet identifier fields
- Incomplete flag definitions

### File: `xy_mqtt/xy_mqtt.c`

**Content:**
```c
int xy_mqtt_parse(unsigned char *pdata)
{
    switch (pdata[0] && 0xf0)  {
        case
    }
}
```

**Issues:**
- Syntax error: `&&` should be `&`
- Empty switch statement
- No actual parsing logic
- No packet type handling

### File: `xy_mqtt/xy_mqtt.md`

**Content:**
```
## 协议
- Fixed Header
  - 0 Reserved
  - 1 CONNACK
- Variable Header
- Payload
```

Only a placeholder outline, not actual documentation.

---

## Missing MQTT Packet Types

### MQTT Packet Types (per MQTT 3.1.1 spec)

| Type | Name | Direction | Implemented |
|------|------|-----------|-------------|
| 0 | Reserved | - | N/A |
| 1 | CONNECT | Client→Server | ❌ Incomplete |
| 2 | CONNACK | Server→Client | ❌ Missing |
| 3 | PUBLISH | Both | ❌ Missing |
| 4 | PUBACK | Both | ❌ Missing |
| 5 | PUBREC | Both | ❌ Missing |
| 6 | PUBREL | Both | ❌ Missing |
| 7 | PUBCOMP | Both | ❌ Missing |
| 8 | SUBSCRIBE | Client→Server | ❌ Missing |
| 9 | SUBACK | Server→Client | ❌ Missing |
| 10 | UNSUBSCRIBE | Client→Server | ❌ Missing |
| 11 | UNSUBACK | Server→Client | ❌ Missing |
| 12 | PINGREQ | Client→Server | ❌ Missing |
| 13 | PINGRESP | Server→Client | ❌ Missing |
| 14 | DISCONNECT | Client→Server | ❌ Missing |
| 15 | AUTH | Both | ❌ Missing |

---

## Implementation Plan

### Phase 1: Core Infrastructure

#### 1.1 Fix Header Parsing

```c
// xy_mqtt_types.h - Add complete packet type enum
typedef enum {
    XY_MQTT_TYPE_RESERVED    = 0,
    XY_MQTT_TYPE_CONNECT     = 1,
    XY_MQTT_TYPE_CONNACK     = 2,
    XY_MQTT_TYPE_PUBLISH     = 3,
    XY_MQTT_TYPE_PUBACK      = 4,
    XY_MQTT_TYPE_PUBREC      = 5,
    XY_MQTT_TYPE_PUBREL      = 6,
    XY_MQTT_TYPE_PUBCOMP     = 7,
    XY_MQTT_TYPE_SUBSCRIBE   = 8,
    XY_MQTT_TYPE_SUBACK      = 9,
    XY_MQTT_TYPE_UNSUBSCRIBE = 10,
    XY_MQTT_TYPE_UNSUBACK    = 11,
    XY_MQTT_TYPE_PINGREQ     = 12,
    XY_MQTT_TYPE_PINGRESP    = 13,
    XY_MQTT_TYPE_DISCONNECT  = 14,
    XY_MQTT_TYPE_AUTH       = 15,
} xy_mqtt_type_t;

// Remaining length encoding/decoding
int xy_mqtt_encode_remaining_length(uint8_t *buf, size_t len);
int xy_mqtt_decode_remaining_length(const uint8_t *buf, uint32_t *len);
```

#### 1.2 Connection State Machine

```c
typedef enum {
    XY_MQTT_STATE_DISCONNECTED = 0,
    XY_MQTT_STATE_TCP_CONNECTING,
    XY_MQTT_STATE_MQTT_CONNECTING,
    XY_MQTT_STATE_CONNECTED,
    XY_MQTT_STATE_DISCONNECTING,
} xy_mqtt_state_t;

struct xy_mqtt {
    xy_mqtt_state_t state;
    uint16_t keepalive_interval;    // seconds
    uint32_t last_activity_time;
    uint8_t  clean_session;
    uint8_t  connected;
    uint8_t  connack_rc;
    // ... more fields
};
```

#### 1.3 Timer/Keep-alive

```c
// Keep-alive handling
int xy_mqtt_handle_timeout(xy_mqtt_t *mqtt);
int xy_mqtt_send_pingreq(xy_mqtt_t *mqtt);
```

---

### Phase 2: CONNECT/CONNACK

#### 2.1 CONNECT Packet Structure

```c
// Variable header
struct xy_mqtt_connect_varheader {
    uint16_t protocol_name_len;     // 4 ("MQTT")
    char protocol_name[4];
    uint8_t protocol_level;          // 4 (3.1.1) or 3 (3.1)
    union {
        uint8_t flags;
        struct {
            uint8_t reserved: 1;
            uint8_t clean_session: 1;
            uint8_t will_flag: 1;
            uint8_t will_qos: 2;
            uint8_t will_retain: 1;
            uint8_t password_flag: 1;
            uint8_t username_flag: 1;
        } bits;
    } flags;
    uint16_t keepalive;              // seconds
};

// Payload
struct xy_mqtt_connect_payload {
    char *client_id;
    char *will_topic;       // if will_flag
    char *will_message;     // if will_flag
    char *username;         // if username_flag
    char *password;         // if password_flag
};
```

#### 2.2 CONNACK Packet

```c
struct xy_mqtt_connack_packet {
    uint8_t session_present;    // bit 0
    uint8_t return_code;        // 0=Success, 1-5=Error
};
```

---

### Phase 3: PUBLISH and QoS

#### 3.1 PUBLISH Packet

```c
struct xy_mqtt_publish_packet {
    uint8_t dup;
    uint8_t qos;            // 0, 1, or 2
    uint8_t retain;
    uint16_t topic_length;
    char *topic_name;
    uint16_t packet_id;     // if QoS > 0
    uint8_t *payload;
    size_t payload_len;
};
```

#### 3.2 QoS Flow

| QoS | PUBLISH | PUBACK | PUBREC | PUBREL | PUBCOMP |
|-----|---------|--------|--------|--------|---------|
| 0 | → | - | - | - | - |
| 1 | → | ← | - | - | - |
| 2 | → | ← | ← | → | ← |

---

### Phase 4: SUBSCRIBE/SUBACK

#### 4.1 SUBSCRIBE Packet

```c
struct xy_mqtt_subscribe_packet {
    uint16_t packet_id;
    // Topic filters (variable length)
    struct {
        char *topic;
        uint8_t qos;
    } topics[MAX_SUBSCRIPTIONS];
    uint8_t topic_count;
};
```

#### 4.2 SUBACK Packet

```c
struct xy_mqtt_suback_packet {
    uint16_t packet_id;
    uint8_t return_codes[MAX_SUBSCRIPTIONS];  // 0-2 or 0x80 (failure)
};
```

---

### Phase 5: Message Handler Framework

```c
// Message handlers
typedef int (*xy_mqtt_packet_handler)(xy_mqtt_t *mqtt, const uint8_t *data, size_t len);

// Dispatch table
static const xy_mqtt_packet_handler g_mqtt_handlers[16] = {
    [XY_MQTT_TYPE_CONNACK]     = xy_mqtt_handle_connack,
    [XY_MQTT_TYPE_PUBLISH]    = xy_mqtt_handle_publish,
    [XY_MQTT_TYPE_PUBACK]     = xy_mqtt_handle_puback,
    [XY_MQTT_TYPE_PUBREC]     = xy_mqtt_handle_pubrec,
    [XY_MQTT_TYPE_PUBREL]     = xy_mqtt_handle_pubrel,
    [XY_MQTT_TYPE_PUBCOMP]    = xy_mqtt_handle_pubcomp,
    [XY_MQTT_TYPE_SUBACK]     = xy_mqtt_handle_suback,
    [XY_MQTT_TYPE_UNSUBACK]   = xy_mqtt_handle_unsuback,
    [XY_MQTT_TYPE_PINGRESP]   = xy_mqtt_handle_pingresp,
    [XY_MQTT_TYPE_DISCONNECT] = xy_mqtt_handle_disconnect,
};

int xy_mqtt_parse_packet(xy_mqtt_t *mqtt, const uint8_t *data, size_t len) {
    if (len < 2) return XY_MQTT_ERROR_INVALID_PACKET;

    uint8_t type = (data[0] >> 4) & 0x0F;
    uint32_t remaining_len;

    if (xy_mqtt_decode_remaining_length(data + 1, &remaining_len) < 0)
        return XY_MQTT_ERROR_INVALID_REMAINING_LEN;

    if (len < 1 + xy_mqtt_encode_remaining_length(remaining_len, NULL) + remaining_len)
        return XY_MQTT_ERROR_INCOMPLETE_PACKET;

    if (g_mqtt_handlers[type])
        return g_mqtt_handlers[type](mqtt, data, len);

    return XY_MQTT_ERROR_UNSUPPORTED_PACKET;
}
```

---

## Priority Order

1. **High Priority**
   - [ ] Fix header parsing (extract type, flags, remaining length)
   - [ ] CONNECT packet encoding
   - [ ] CONNACK packet decoding
   - [ ] Connection state machine
   - [ ] PINGREQ/PINGRESP (keep-alive)

2. **Medium Priority**
   - [ ] PUBLISH QoS 0 send/receive
   - [ ] PUBACK handling
   - [ ] SUBSCRIBE/SUBACK
   - [ ] Topic subscription matching

3. **Lower Priority**
   - [ ] QoS 1 full flow
   - [ ] QoS 2 flow (PUBREC/PUBREL/PUBCOMP)
   - [ ] Will message handling
   - [ ] Authentication (AUTH)
   - [ ] TLS/SSL support

---

## API Design

### User API

```c
// Initialization
int xy_mqtt_init(xy_mqtt_t *mqtt, const xy_mqtt_config_t *config);
void xy_mqtt_deinit(xy_mqtt_t *mqtt);

// Connection
int xy_mqtt_connect(xy_mqtt_t *mqtt, const xy_mqtt_connect_info_t *info);
int xy_mqtt_disconnect(xy_mqtt_t *mqtt);
int xy_mqtt_is_connected(xy_mqtt_t *mqtt);

// Publishing
int xy_mqtt_publish(xy_mqtt_t *mqtt, const char *topic, const uint8_t *data,
                   size_t len, uint8_t qos, bool retain);
int xy_mqtt_publish_callback_set(xy_mqtt_t *mqtt,
                                 xy_mqtt_publish_cb_t callback, void *user_data);

// Subscribing
int xy_mqtt_subscribe(xy_mqtt_t *mqtt, const char *topic, uint8_t qos);
int xy_mqtt_unsubscribe(xy_mqtt_t *mqtt, const char *topic);

// Polling (for non-blocking operation)
int xy_mqtt_process(xy_mqtt_t *mqtt, uint32_t timeout_ms);
```

---

## Configuration

```c
typedef struct {
    uint16_t keepalive;           // seconds (0 = disabled)
    uint8_t clean_session;
    uint8_t qos_default;
    size_t tx_buffer_size;
    size_t rx_buffer_size;
    void *transport_context;      // e.g., TCP socket, TLS context
    xy_mqtt_transport_send_t send;
    xy_mqtt_transport_recv_t recv;
} xy_mqtt_config_t;
```

---

## Testing Approach

### Unit Tests
- Remaining length encoding/decoding
- Packet header parsing
- Topic matching algorithm
- QoS state machines

### Integration Tests
- CONNECT → CONNACK flow
- PUBLISH → PUBACK (QoS 1)
- SUBSCRIBE → SUBACK → PUBLISH to subscriber
- Keep-alive timeout

---

## Estimated Effort

| Phase | Complexity | Estimated Lines |
|-------|------------|-----------------|
| Phase 1: Infrastructure | Medium | ~300 |
| Phase 2: CONNECT/CONNACK | Low | ~200 |
| Phase 3: PUBLISH/QoS | High | ~400 |
| Phase 4: SUBSCRIBE | Medium | ~300 |
| Phase 5: Handler Framework | Medium | ~200 |
| **Total** | | **~1400** |

---

## References

- [MQTT 3.1.1 Specification](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html)
- [MQTT 5.0 Specification](https://docs.oasis-open.org/mqtt/mqtt/v5.0/mqtt-v5.0.html)
- Reference implementation: `MCU/STM32F1/Middlewares/Third_Party/LwIP/src/apps/mqtt/mqtt.c`
