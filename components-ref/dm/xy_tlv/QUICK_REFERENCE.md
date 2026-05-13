# XY TLV Quick Reference

## Common Operations

### Initialize Buffer for Encoding
```c
uint8_t buffer[256];
xy_tlv_buffer_t tlv_buf;
xy_tlv_buffer_init(&tlv_buf, buffer, sizeof(buffer));
```

### Encode Values
```c
xy_tlv_encode_uint8(&tlv_buf, type, value);
xy_tlv_encode_uint16(&tlv_buf, type, value);
xy_tlv_encode_uint32(&tlv_buf, type, value);
xy_tlv_encode_uint64(&tlv_buf, type, value);
xy_tlv_encode_int8(&tlv_buf, type, value);
xy_tlv_encode_int16(&tlv_buf, type, value);
xy_tlv_encode_int32(&tlv_buf, type, value);
xy_tlv_encode_int64(&tlv_buf, type, value);
xy_tlv_encode_bool(&tlv_buf, type, value);
xy_tlv_encode_string(&tlv_buf, type, "string");
xy_tlv_encode_bytes(&tlv_buf, type, bytes, length);
```

### Get Encoded Size
```c
uint16_t size = xy_tlv_buffer_get_used(&tlv_buf);
```

### Initialize Iterator for Decoding
```c
xy_tlv_iterator_t iter;
xy_tlv_iterator_init(&iter, buffer, buffer_len);
```

### Iterate and Decode
```c
xy_tlv_t tlv;
while (xy_tlv_iterator_next(&iter, &tlv) == XY_TLV_OK) {
    switch (tlv.type) {
        case MY_TYPE:
            uint32_t value;
            xy_tlv_decode_uint32(&tlv, &value);
            break;
    }
}
```

### Decode Values
```c
xy_tlv_decode_uint8(&tlv, &value);
xy_tlv_decode_uint16(&tlv, &value);
xy_tlv_decode_uint32(&tlv, &value);
xy_tlv_decode_uint64(&tlv, &value);
xy_tlv_decode_int8(&tlv, &value);
xy_tlv_decode_int16(&tlv, &value);
xy_tlv_decode_int32(&tlv, &value);
xy_tlv_decode_int64(&tlv, &value);
xy_tlv_decode_bool(&tlv, &value);
xy_tlv_decode_string(&tlv, str, str_len);
xy_tlv_decode_bytes(&tlv, bytes, &bytes_len);
```

### Find Specific TLV
```c
xy_tlv_t found;
if (xy_tlv_find(buffer, len, type, &found) == XY_TLV_OK) {
    /* Process found TLV */
}
```

### Validate Buffer
```c
if (xy_tlv_validate(buffer, len) == XY_TLV_OK) {
    /* Buffer is valid */
}
```

### Calculate Checksum
```c
uint16_t crc = xy_tlv_checksum(buffer, len);
xy_tlv_encode_uint16(&tlv_buf, XY_TLV_TYPE_CHECKSUM, crc);
```

## Return Codes

| Code | Value | Meaning |
|------|-------|---------|
| `XY_TLV_OK` | 0 | Success |
| `XY_TLV_ERROR` | -1 | General error |
| `XY_TLV_INVALID_PARAM` | -2 | Invalid parameter |
| `XY_TLV_BUFFER_OVERFLOW` | -3 | Buffer too small |
| `XY_TLV_BUFFER_UNDERFLOW` | -4 | Insufficient data |
| `XY_TLV_TYPE_MISMATCH` | -5 | Type mismatch |
| `XY_TLV_NOT_FOUND` | -6 | TLV not found |
| `XY_TLV_INVALID_LENGTH` | -7 | Invalid length |
| `XY_TLV_NESTING_OVERFLOW` | -8 | Max nesting exceeded |
| `XY_TLV_CHECKSUM_ERROR` | -9 | Checksum failed |

## Common Patterns

### Sensor Data Packet
```c
xy_tlv_encode_int16(&buf, TEMP, 235);      // 23.5°C
xy_tlv_encode_uint16(&buf, HUMIDITY, 652); // 65.2%
xy_tlv_encode_uint32(&buf, TIMESTAMP, time(NULL));
```

### Configuration Storage
```c
xy_tlv_encode_uint32(&buf, DEVICE_ID, id);
xy_tlv_encode_string(&buf, DEVICE_NAME, name);
xy_tlv_encode_bool(&buf, ENABLED, true);
```

### With Checksum
```c
uint16_t size = xy_tlv_buffer_get_used(&buf);
uint16_t crc = xy_tlv_checksum(buffer, size);
xy_tlv_encode_uint16(&buf, XY_TLV_TYPE_CHECKSUM, crc);
```

## Predefined Types (Common)

| Type | ID | Size |
|------|-----|------|
| `XY_TLV_TYPE_UINT8` | 0x0001 | 1 byte |
| `XY_TLV_TYPE_UINT16` | 0x0002 | 2 bytes |
| `XY_TLV_TYPE_UINT32` | 0x0003 | 4 bytes |
| `XY_TLV_TYPE_UINT64` | 0x0004 | 8 bytes |
| `XY_TLV_TYPE_BOOL` | 0x000B | 1 byte |
| `XY_TLV_TYPE_STRING` | 0x0101 | Variable |
| `XY_TLV_TYPE_BYTES` | 0x0102 | Variable |
| `XY_TLV_TYPE_TIMESTAMP` | 0x0301 | 4 bytes |
| `XY_TLV_TYPE_MAC_ADDR` | 0x0303 | 6 bytes |
| `XY_TLV_TYPE_IPV4_ADDR` | 0x0304 | 4 bytes |
| `XY_TLV_TYPE_CHECKSUM` | 0x0306 | 2 bytes |

**User types**: Start from `XY_TLV_TYPE_USER_BASE` (0x1000)

## Memory Usage

- TLV Header: **4 bytes** (2 type + 2 length)
- Buffer Context: **~12 bytes**
- Iterator: **~16 bytes**
- Statistics: **~24 bytes** (global)

## Tips

1. ✅ Always check return codes
2. ✅ Validate external data
3. ✅ Use checksums for critical data
4. ✅ Define custom types as constants
5. ✅ Estimate buffer size: `(header + value) * count`
6. ✅ Network byte order (big-endian) is automatic
