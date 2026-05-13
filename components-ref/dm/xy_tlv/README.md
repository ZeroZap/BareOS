# XY TLV Management System

## Overview

The XY TLV (Type-Length-Value) management system provides efficient binary data encoding and decoding for embedded systems. It's designed for resource-constrained environments with zero dynamic allocation and comprehensive error checking.

## Features

- ✅ **Zero Dynamic Allocation** - Uses provided buffers only
- ✅ **Type-Safe API** - Dedicated functions for each data type
- ✅ **Network Byte Order** - Big-endian encoding for cross-platform compatibility
- ✅ **Nested Containers** - Support for hierarchical data structures
- ✅ **Iterator Pattern** - Efficient traversal without copying
- ✅ **Validation** - Comprehensive boundary and integrity checking
- ✅ **Statistics** - Built-in performance monitoring
- ✅ **Extensible** - Easy to add custom types

## TLV Format

Each TLV element consists of:

```
+--------+--------+--------+--------+------------------+
|  Type  |  Type  | Length | Length |      Value       |
| (MSB)  | (LSB)  | (MSB)  | (LSB)  |   (0-65535 bytes)|
+--------+--------+--------+--------+------------------+
   0        1        2        3         4 ... N
```

- **Type**: 2 bytes (uint16_t) - identifies data type
- **Length**: 2 bytes (uint16_t) - value payload length (0-65535)
- **Value**: variable length - actual data payload

## Quick Start

### Basic Encoding

```c
#include "xy_tlv.h"

uint8_t buffer[256];
xy_tlv_buffer_t tlv_buf;

/* Initialize buffer */
xy_tlv_buffer_init(&tlv_buf, buffer, sizeof(buffer));

/* Encode different types */
xy_tlv_encode_uint32(&tlv_buf, 0x1001, 12345);
xy_tlv_encode_string(&tlv_buf, 0x1002, "Hello");
xy_tlv_encode_bool(&tlv_buf, 0x1003, true);

/* Get encoded size */
uint16_t size = xy_tlv_buffer_get_used(&tlv_buf);
```

### Basic Decoding

```c
xy_tlv_iterator_t iter;
xy_tlv_t tlv;

/* Initialize iterator */
xy_tlv_iterator_init(&iter, buffer, size);

/* Iterate through TLVs */
while (xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_OK) {
    switch (tlv.type) {
        case 0x1001: {
            uint32_t value;
            xy_tlv_decode_uint32(&tlv, &value);
            printf("UINT32: %u\n", value);
            break;
        }
        case 0x1002: {
            char str[64];
            xy_tlv_decode_string(&tlv, str, sizeof(str));
            printf("STRING: %s\n", str);
            break;
        }
        case 0x1003: {
            bool value;
            xy_tlv_decode_bool(&tlv, &value);
            printf("BOOL: %d\n", value);
            break;
        }
    }
}
```

## API Reference

### Buffer Management

#### `xy_tlv_buffer_init`
Initialize TLV buffer for encoding.

```c
int xy_tlv_buffer_init(xy_tlv_buffer_t *tlv_buf, uint8_t *buffer, uint16_t capacity);
```

#### `xy_tlv_buffer_reset`
Reset buffer to initial state (clears all data).

```c
int xy_tlv_buffer_reset(xy_tlv_buffer_t *tlv_buf);
```

#### `xy_tlv_buffer_get_used`
Get number of bytes currently used in buffer.

```c
uint16_t xy_tlv_buffer_get_used(const xy_tlv_buffer_t *tlv_buf);
```

#### `xy_tlv_buffer_get_free`
Get remaining free space in buffer.

```c
uint16_t xy_tlv_buffer_get_free(const xy_tlv_buffer_t *tlv_buf);
```

### Encoding Functions

#### Generic Encoding
```c
int xy_tlv_encode(xy_tlv_buffer_t *tlv_buf, uint16_t type,
                  const void *value, uint16_t length);
```

#### Type-Specific Encoding
```c
int xy_tlv_encode_uint8(xy_tlv_buffer_t *tlv_buf, uint16_t type, uint8_t value);
int xy_tlv_encode_uint16(xy_tlv_buffer_t *tlv_buf, uint16_t type, uint16_t value);
int xy_tlv_encode_uint32(xy_tlv_buffer_t *tlv_buf, uint16_t type, uint32_t value);
int xy_tlv_encode_uint64(xy_tlv_buffer_t *tlv_buf, uint16_t type, uint64_t value);
int xy_tlv_encode_int8(xy_tlv_buffer_t *tlv_buf, uint16_t type, int8_t value);
int xy_tlv_encode_int16(xy_tlv_buffer_t *tlv_buf, uint16_t type, int16_t value);
int xy_tlv_encode_int32(xy_tlv_buffer_t *tlv_buf, uint16_t type, int32_t value);
int xy_tlv_encode_int64(xy_tlv_buffer_t *tlv_buf, uint16_t type, int64_t value);
int xy_tlv_encode_bool(xy_tlv_buffer_t *tlv_buf, uint16_t type, bool value);
int xy_tlv_encode_string(xy_tlv_buffer_t *tlv_buf, uint16_t type, const char *str);
int xy_tlv_encode_bytes(xy_tlv_buffer_t *tlv_buf, uint16_t type,
                        const uint8_t *bytes, uint16_t length);
```

### Decoding Functions

#### Iterator Management
```c
int xy_tlv_iterator_init(xy_tlv_iterator_t *iter, const uint8_t *buffer,
                         uint16_t buffer_len);
int xy_tlv_iterator_next(xy_tlv_iterator_t *iter, xy_tlv_t *tlv);
bool xy_tlv_iterator_has_next(const xy_tlv_iterator_t *iter);
int xy_tlv_iterator_reset(xy_tlv_iterator_t *iter);
```

#### Type-Specific Decoding
```c
int xy_tlv_decode_uint8(const xy_tlv_t *tlv, uint8_t *value);
int xy_tlv_decode_uint16(const xy_tlv_t *tlv, uint16_t *value);
int xy_tlv_decode_uint32(const xy_tlv_t *tlv, uint32_t *value);
int xy_tlv_decode_uint64(const xy_tlv_t *tlv, uint64_t *value);
int xy_tlv_decode_int8(const xy_tlv_t *tlv, int8_t *value);
int xy_tlv_decode_int16(const xy_tlv_t *tlv, int16_t *value);
int xy_tlv_decode_int32(const xy_tlv_t *tlv, int32_t *value);
int xy_tlv_decode_int64(const xy_tlv_t *tlv, int64_t *value);
int xy_tlv_decode_bool(const xy_tlv_t *tlv, bool *value);
int xy_tlv_decode_string(const xy_tlv_t *tlv, char *str, uint16_t str_len);
int xy_tlv_decode_bytes(const xy_tlv_t *tlv, uint8_t *bytes, uint16_t *bytes_len);
```

### Advanced Features

#### Searching
```c
/* Find first TLV of specific type */
int xy_tlv_find(const uint8_t *buffer, uint16_t buffer_len,
                uint16_t type, xy_tlv_t *tlv);

/* Find all TLVs of specific type */
int xy_tlv_find_all(const uint8_t *buffer, uint16_t buffer_len,
                    uint16_t type, xy_tlv_t *tlv_array, uint16_t *array_size);

/* Count total TLVs in buffer */
int xy_tlv_count(const uint8_t *buffer, uint16_t buffer_len);
```

#### Containers (Nested TLV)
```c
/* Begin encoding a container */
int xy_tlv_container_begin(xy_tlv_buffer_t *tlv_buf, uint16_t type);

/* End encoding a container */
int xy_tlv_container_end(xy_tlv_buffer_t *tlv_buf);

/* Enter a container for iteration */
int xy_tlv_container_enter(const xy_tlv_iterator_t *iter,
                           const xy_tlv_t *tlv,
                           xy_tlv_iterator_t *child_iter);
```

#### Validation & Utilities
```c
/* Validate TLV buffer structure */
int xy_tlv_validate(const uint8_t *buffer, uint16_t buffer_len);

/* Calculate checksum (CRC16) */
uint16_t xy_tlv_checksum(const uint8_t *buffer, uint16_t buffer_len);

/* Get type name for debugging */
const char *xy_tlv_get_type_name(uint16_t type);

/* Get error message string */
const char *xy_tlv_get_error_string(int error_code);

/* Get statistics */
int xy_tlv_get_stats(xy_tlv_stats_t *stats);
void xy_tlv_reset_stats(void);
```

## Predefined Types

### Basic Types (0x0001-0x00FF)
- `XY_TLV_TYPE_UINT8` (0x0001)
- `XY_TLV_TYPE_UINT16` (0x0002)
- `XY_TLV_TYPE_UINT32` (0x0003)
- `XY_TLV_TYPE_UINT64` (0x0004)
- `XY_TLV_TYPE_INT8` (0x0005)
- `XY_TLV_TYPE_INT16` (0x0006)
- `XY_TLV_TYPE_INT32` (0x0007)
- `XY_TLV_TYPE_INT64` (0x0008)
- `XY_TLV_TYPE_FLOAT` (0x0009)
- `XY_TLV_TYPE_DOUBLE` (0x000A)
- `XY_TLV_TYPE_BOOL` (0x000B)

### String & Binary Types (0x0100-0x01FF)
- `XY_TLV_TYPE_STRING` (0x0101) - NULL-terminated string
- `XY_TLV_TYPE_BYTES` (0x0102) - Raw binary data
- `XY_TLV_TYPE_BLOB` (0x0103) - Binary large object

### Container Types (0x0200-0x02FF)
- `XY_TLV_TYPE_CONTAINER` (0x0201) - Nested TLV container
- `XY_TLV_TYPE_ARRAY` (0x0202) - Array of TLVs
- `XY_TLV_TYPE_LIST` (0x0203) - Linked list

### Special Types (0x0300-0x03FF)
- `XY_TLV_TYPE_TIMESTAMP` (0x0301) - Unix timestamp
- `XY_TLV_TYPE_UUID` (0x0302) - UUID (16 bytes)
- `XY_TLV_TYPE_MAC_ADDR` (0x0303) - MAC address (6 bytes)
- `XY_TLV_TYPE_IPV4_ADDR` (0x0304) - IPv4 address (4 bytes)
- `XY_TLV_TYPE_IPV6_ADDR` (0x0305) - IPv6 address (16 bytes)
- `XY_TLV_TYPE_CHECKSUM` (0x0306) - Checksum/CRC value

### User-Defined Types (0x1000-0xFFFF)
Use `XY_TLV_TYPE_USER_BASE` (0x1000) and above for custom types.

## Return Codes

- `XY_TLV_OK` (0) - Success
- `XY_TLV_ERROR` (-1) - General error
- `XY_TLV_INVALID_PARAM` (-2) - Invalid parameter
- `XY_TLV_BUFFER_OVERFLOW` (-3) - Buffer too small
- `XY_TLV_BUFFER_UNDERFLOW` (-4) - Insufficient data
- `XY_TLV_TYPE_MISMATCH` (-5) - Type mismatch
- `XY_TLV_NOT_FOUND` (-6) - TLV not found
- `XY_TLV_INVALID_LENGTH` (-7) - Invalid length
- `XY_TLV_NESTING_OVERFLOW` (-8) - Max nesting exceeded
- `XY_TLV_CHECKSUM_ERROR` (-9) - Checksum failed

## Examples

### Example 1: Device Configuration

```c
/* Define custom types */
#define CFG_DEVICE_ID    0x1001
#define CFG_DEVICE_NAME  0x1002
#define CFG_WIFI_SSID    0x1003
#define CFG_WIFI_PASS    0x1004
#define CFG_ENABLED      0x1005

/* Encode configuration */
uint8_t config_buf[512];
xy_tlv_buffer_t cfg;

xy_tlv_buffer_init(&cfg, config_buf, sizeof(config_buf));
xy_tlv_encode_uint32(&cfg, CFG_DEVICE_ID, 0xDEADBEEF);
xy_tlv_encode_string(&cfg, CFG_DEVICE_NAME, "MyDevice");
xy_tlv_encode_string(&cfg, CFG_WIFI_SSID, "MyNetwork");
xy_tlv_encode_string(&cfg, CFG_WIFI_PASS, "password123");
xy_tlv_encode_bool(&cfg, CFG_ENABLED, true);

uint16_t cfg_size = xy_tlv_buffer_get_used(&cfg);

/* Decode configuration */
xy_tlv_iterator_t iter;
xy_tlv_t tlv;

xy_tlv_iterator_init(&iter, config_buf, cfg_size);

while (xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_OK) {
    switch (tlv.type) {
        case CFG_DEVICE_ID: {
            uint32_t id;
            xy_tlv_decode_uint32(&tlv, &id);
            break;
        }
        case CFG_DEVICE_NAME: {
            char name[32];
            xy_tlv_decode_string(&tlv, name, sizeof(name));
            break;
        }
        /* ... handle other fields ... */
    }
}
```

### Example 2: Sensor Data Packet

```c
#define SENSOR_TEMP      0x2001
#define SENSOR_HUMIDITY  0x2002
#define SENSOR_PRESSURE  0x2003
#define SENSOR_TIMESTAMP 0x2004

/* Encode sensor data */
uint8_t sensor_buf[128];
xy_tlv_buffer_t sensor;

xy_tlv_buffer_init(&sensor, sensor_buf, sizeof(sensor_buf));
xy_tlv_encode_int16(&sensor, SENSOR_TEMP, 2350);        // 23.50°C
xy_tlv_encode_uint16(&sensor, SENSOR_HUMIDITY, 6520);   // 65.20%
xy_tlv_encode_uint32(&sensor, SENSOR_PRESSURE, 101325); // Pa
xy_tlv_encode_uint32(&sensor, SENSOR_TIMESTAMP, time(NULL));

/* Add checksum */
uint16_t size = xy_tlv_buffer_get_used(&sensor);
uint16_t crc = xy_tlv_checksum(sensor_buf, size);
xy_tlv_encode_uint16(&sensor, XY_TLV_TYPE_CHECKSUM, crc);
```

### Example 3: Searching for Specific TLV

```c
/* Find specific configuration value */
xy_tlv_t found;

if (xy_tlv_find(config_buf, cfg_size, CFG_WIFI_SSID, &found) == XY_TLV_OK) {
    char ssid[33];
    xy_tlv_decode_string(&found, ssid, sizeof(ssid));
    printf("WiFi SSID: %s\n", ssid);
}
```

### Example 4: Nested Containers

```c
#define CONFIG_NETWORK   0x3001
#define CONFIG_SECURITY  0x3002

xy_tlv_buffer_t cfg;
xy_tlv_buffer_init(&cfg, buffer, sizeof(buffer));

/* Begin network container */
xy_tlv_container_begin(&cfg, CONFIG_NETWORK);
xy_tlv_encode_string(&cfg, CFG_WIFI_SSID, "MyNetwork");
xy_tlv_encode_string(&cfg, CFG_WIFI_PASS, "password");
xy_tlv_container_end(&cfg);

/* Begin security container */
xy_tlv_container_begin(&cfg, CONFIG_SECURITY);
xy_tlv_encode_bool(&cfg, 0x3101, true);
xy_tlv_container_end(&cfg);
```

### Example 5: Validation

```c
/* Validate received TLV buffer */
if (xy_tlv_validate(rx_buffer, rx_len) != XY_TLV_OK) {
    printf("Invalid TLV structure!\n");
    return;
}

/* Verify checksum */
xy_tlv_t crc_tlv;
if (xy_tlv_find(rx_buffer, rx_len, XY_TLV_TYPE_CHECKSUM, &crc_tlv) == XY_TLV_OK) {
    uint16_t received_crc;
    xy_tlv_decode_uint16(&crc_tlv, &received_crc);

    /* Calculate CRC of data (excluding checksum TLV) */
    uint16_t data_len = rx_len - (XY_TLV_HEADER_SIZE + 2);
    uint16_t calc_crc = xy_tlv_checksum(rx_buffer, data_len);

    if (received_crc != calc_crc) {
        printf("Checksum mismatch!\n");
    }
}
```

## Configuration

Customize behavior by defining these macros before including `xy_tlv.h`:

```c
#define XY_TLV_MAX_NESTING_LEVEL 8  /* Default: 4 */
#define XY_TLV_ENABLE_VALIDATION 1  /* Default: 1 */
```

## Best Practices

1. **Always check return codes** - All functions return error codes
2. **Validate external data** - Use `xy_tlv_validate()` on received buffers
3. **Use checksums** - Add integrity checking for critical data
4. **Buffer sizing** - Allocate sufficient buffer space (estimate with worst-case)
5. **Type definitions** - Define custom types as constants for maintainability
6. **Error handling** - Use `xy_tlv_get_error_string()` for debugging

## Memory Usage

- **Per TLV Element**: 4 bytes header + value length
- **Buffer Context**: ~12 bytes
- **Iterator**: ~16 bytes
- **Statistics**: ~24 bytes (global)

**Total RAM for basic usage**: ~52 bytes + buffer size

## Performance

- **Encoding**: O(1) per element
- **Decoding**: O(1) per element
- **Finding**: O(n) linear search
- **Validation**: O(n) single pass

## Thread Safety

The TLV library is **not thread-safe** by default. For multi-threaded use:
- Use separate buffers/iterators per thread
- Add external synchronization for shared buffers
- Statistics are global and may need mutex protection

## Integration

Add to your project:

1. Copy `xy_tlv.h` and `xy_tlv.c` to your source tree
2. Include the header: `#include "xy_tlv.h"`
3. Link `xy_tlv.c` in your build system
4. No external dependencies (only standard C library)

## License

Part of the XY embedded components framework.

## Support

For issues or questions, refer to the main XY framework documentation.
