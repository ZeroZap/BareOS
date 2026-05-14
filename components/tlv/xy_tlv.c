/**
 * @file xy_tlv.c
 * @brief XY TLV Management System Implementation
 */

#include "xy_tlv.h"
#include <string.h>

/* ==================== Internal Helper Macros ==================== */

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Endianness conversion macros */
#define HTONS(x) ((uint16_t)((((x) & 0xFF00) >> 8) | (((x) & 0x00FF) << 8)))
#define NTOHS(x) HTONS(x)
#define HTONL(x)                                                       \
    ((uint32_t)((((x) & 0xFF000000) >> 24) | (((x) & 0x00FF0000) >> 8) \
                | (((x) & 0x0000FF00) << 8) | (((x) & 0x000000FF) << 24)))
#define NTOHL(x) HTONL(x)

/* ==================== Global Statistics ==================== */

static xy_tlv_stats_t g_tlv_stats = { 0 };

/* ==================== Internal Helper Functions ==================== */

/**
 * @brief Write uint16_t to buffer in network byte order
 */
static inline void write_uint16(uint8_t *buf, uint16_t value)
{
    buf[0] = (value >> 8) & 0xFF;
    buf[1] = value & 0xFF;
}

/**
 * @brief Read uint16_t from buffer in network byte order
 */
static inline uint16_t read_uint16(const uint8_t *buf)
{
    return (uint16_t)((buf[0] << 8) | buf[1]);
}

/**
 * @brief Write uint32_t to buffer in network byte order
 */
static inline void write_uint32(uint8_t *buf, uint32_t value)
{
    buf[0] = (value >> 24) & 0xFF;
    buf[1] = (value >> 16) & 0xFF;
    buf[2] = (value >> 8) & 0xFF;
    buf[3] = value & 0xFF;
}

/**
 * @brief Read uint32_t from buffer in network byte order
 */
static inline uint32_t read_uint32(const uint8_t *buf)
{
    return (uint32_t)((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]);
}

/**
 * @brief Write uint64_t to buffer in network byte order
 */
static inline void write_uint64(uint8_t *buf, uint64_t value)
{
    buf[0] = (value >> 56) & 0xFF;
    buf[1] = (value >> 48) & 0xFF;
    buf[2] = (value >> 40) & 0xFF;
    buf[3] = (value >> 32) & 0xFF;
    buf[4] = (value >> 24) & 0xFF;
    buf[5] = (value >> 16) & 0xFF;
    buf[6] = (value >> 8) & 0xFF;
    buf[7] = value & 0xFF;
}

/**
 * @brief Read uint64_t from buffer in network byte order
 */
static inline uint64_t read_uint64(const uint8_t *buf)
{
    return ((uint64_t)buf[0] << 56) | ((uint64_t)buf[1] << 48)
           | ((uint64_t)buf[2] << 40) | ((uint64_t)buf[3] << 32)
           | ((uint64_t)buf[4] << 24) | ((uint64_t)buf[5] << 16)
           | ((uint64_t)buf[6] << 8) | (uint64_t)buf[7];
}

/* ==================== Core API - Buffer Management ==================== */

int xy_tlv_buffer_init(xy_tlv_buffer_t *tlv_buf, uint8_t *buffer,
                       uint16_t capacity)
{
    if (!tlv_buf || !buffer || capacity == 0) {
        return XY_TLV_INVALID_PARAM;
    }

    tlv_buf->buffer   = buffer;
    tlv_buf->capacity = capacity;
    tlv_buf->offset   = 0;
    tlv_buf->nesting  = 0;

    return XY_TLV_OK;
}

int xy_tlv_buffer_reset(xy_tlv_buffer_t *tlv_buf)
{
    if (!tlv_buf) {
        return XY_TLV_INVALID_PARAM;
    }

    tlv_buf->offset  = 0;
    tlv_buf->nesting = 0;

    return XY_TLV_OK;
}

uint16_t xy_tlv_buffer_get_used(const xy_tlv_buffer_t *tlv_buf)
{
    if (!tlv_buf) {
        return 0;
    }
    return tlv_buf->offset;
}

uint16_t xy_tlv_buffer_get_free(const xy_tlv_buffer_t *tlv_buf)
{
    if (!tlv_buf) {
        return 0;
    }
    return tlv_buf->capacity - tlv_buf->offset;
}

/* ==================== Core API - Encoding ==================== */

int xy_tlv_encode(xy_tlv_buffer_t *tlv_buf, uint16_t type, const void *value,
                  uint16_t length)
{
    if (!tlv_buf || !tlv_buf->buffer) {
        g_tlv_stats.encoding_errors++;
        return XY_TLV_INVALID_PARAM;
    }

    if (length > 0 && !value) {
        g_tlv_stats.encoding_errors++;
        return XY_TLV_INVALID_PARAM;
    }

    /* Check buffer space */
    uint16_t required = XY_TLV_HEADER_SIZE + length;
    if (tlv_buf->offset + required > tlv_buf->capacity) {
        g_tlv_stats.encoding_errors++;
        return XY_TLV_BUFFER_OVERFLOW;
    }

    /* Write type and length */
    write_uint16(&tlv_buf->buffer[tlv_buf->offset], type);
    tlv_buf->offset += 2;
    write_uint16(&tlv_buf->buffer[tlv_buf->offset], length);
    tlv_buf->offset += 2;

    /* Write value */
    if (length > 0) {
        memcpy(&tlv_buf->buffer[tlv_buf->offset], value, length);
        tlv_buf->offset += length;
    }

    g_tlv_stats.total_encoded++;
    g_tlv_stats.bytes_encoded += required;

    return XY_TLV_OK;
}

int xy_tlv_encode_uint8(xy_tlv_buffer_t *tlv_buf, uint16_t type, uint8_t value)
{
    return xy_tlv_encode(tlv_buf, type, &value, sizeof(uint8_t));
}

int xy_tlv_encode_uint16(xy_tlv_buffer_t *tlv_buf, uint16_t type,
                         uint16_t value)
{
    uint8_t buf[2];
    write_uint16(buf, value);
    return xy_tlv_encode(tlv_buf, type, buf, sizeof(uint16_t));
}

int xy_tlv_encode_uint32(xy_tlv_buffer_t *tlv_buf, uint16_t type,
                         uint32_t value)
{
    uint8_t buf[4];
    write_uint32(buf, value);
    return xy_tlv_encode(tlv_buf, type, buf, sizeof(uint32_t));
}

int xy_tlv_encode_uint64(xy_tlv_buffer_t *tlv_buf, uint16_t type,
                         uint64_t value)
{
    uint8_t buf[8];
    write_uint64(buf, value);
    return xy_tlv_encode(tlv_buf, type, buf, sizeof(uint64_t));
}

int xy_tlv_encode_int8(xy_tlv_buffer_t *tlv_buf, uint16_t type, int8_t value)
{
    return xy_tlv_encode(tlv_buf, type, &value, sizeof(int8_t));
}

int xy_tlv_encode_int16(xy_tlv_buffer_t *tlv_buf, uint16_t type, int16_t value)
{
    uint8_t buf[2];
    write_uint16(buf, (uint16_t)value);
    return xy_tlv_encode(tlv_buf, type, buf, sizeof(int16_t));
}

int xy_tlv_encode_int32(xy_tlv_buffer_t *tlv_buf, uint16_t type, int32_t value)
{
    uint8_t buf[4];
    write_uint32(buf, (uint32_t)value);
    return xy_tlv_encode(tlv_buf, type, buf, sizeof(int32_t));
}

int xy_tlv_encode_int64(xy_tlv_buffer_t *tlv_buf, uint16_t type, int64_t value)
{
    uint8_t buf[8];
    write_uint64(buf, (uint64_t)value);
    return xy_tlv_encode(tlv_buf, type, buf, sizeof(int64_t));
}

int xy_tlv_encode_bool(xy_tlv_buffer_t *tlv_buf, uint16_t type, bool value)
{
    uint8_t val = value ? 1 : 0;
    return xy_tlv_encode(tlv_buf, type, &val, sizeof(uint8_t));
}

int xy_tlv_encode_string(xy_tlv_buffer_t *tlv_buf, uint16_t type,
                         const char *str)
{
    if (!str) {
        return XY_TLV_INVALID_PARAM;
    }
    uint16_t len = (uint16_t)strlen(str);
    return xy_tlv_encode(tlv_buf, type, str, len);
}

int xy_tlv_encode_bytes(xy_tlv_buffer_t *tlv_buf, uint16_t type,
                        const uint8_t *bytes, uint16_t length)
{
    return xy_tlv_encode(tlv_buf, type, bytes, length);
}

/* ==================== Core API - Decoding ==================== */

int xy_tlv_iterator_init(xy_tlv_iterator_t *iter, const uint8_t *buffer,
                         uint16_t buffer_len)
{
    if (!iter || !buffer || buffer_len == 0) {
        return XY_TLV_INVALID_PARAM;
    }

    iter->buffer     = buffer;
    iter->buffer_len = buffer_len;
    iter->offset     = 0;
    iter->remaining  = buffer_len;
    iter->nesting    = 0;

    return XY_TLV_OK;
}

int xy_tlv_iterator_next(xy_tlv_iterator_t *iter, xy_tlv_t *tlv)
{
    if (!iter || !tlv) {
        g_tlv_stats.decoding_errors++;
        return XY_TLV_INVALID_PARAM;
    }

    /* Check if enough data for header */
    if (iter->remaining < XY_TLV_HEADER_SIZE) {
        if (iter->remaining > 0) {
            g_tlv_stats.decoding_errors++;
            return XY_TLV_BUFFER_UNDERFLOW;
        }
        return XY_TLV_NOT_FOUND; /* End of buffer */
    }

    /* Read type and length */
    const uint8_t *ptr = &iter->buffer[iter->offset];
    tlv->type          = read_uint16(ptr);
    ptr += 2;
    tlv->length = read_uint16(ptr);
    ptr += 2;

    /* Validate length */
    if (tlv->length > iter->remaining - XY_TLV_HEADER_SIZE) {
        g_tlv_stats.decoding_errors++;
        return XY_TLV_INVALID_LENGTH;
    }

    /* Set value pointer */
    tlv->value = ptr;

    /* Update iterator state */
    uint16_t tlv_size = XY_TLV_HEADER_SIZE + tlv->length;
    iter->offset += tlv_size;
    iter->remaining -= tlv_size;

    g_tlv_stats.total_decoded++;
    g_tlv_stats.bytes_decoded += tlv_size;

    return XY_TLV_OK;
}

bool xy_tlv_iterator_has_next(const xy_tlv_iterator_t *iter)
{
    if (!iter) {
        return false;
    }
    return iter->remaining >= XY_TLV_HEADER_SIZE;
}

int xy_tlv_iterator_reset(xy_tlv_iterator_t *iter)
{
    if (!iter) {
        return XY_TLV_INVALID_PARAM;
    }

    iter->offset    = 0;
    iter->remaining = iter->buffer_len;

    return XY_TLV_OK;
}

int xy_tlv_decode(const xy_tlv_t *tlv, void *value, uint16_t *value_len)
{
    if (!tlv || !value || !value_len) {
        return XY_TLV_INVALID_PARAM;
    }

    if (*value_len < tlv->length) {
        return XY_TLV_BUFFER_OVERFLOW;
    }

    memcpy(value, tlv->value, tlv->length);
    *value_len = tlv->length;

    return XY_TLV_OK;
}

int xy_tlv_decode_uint8(const xy_tlv_t *tlv, uint8_t *value)
{
    if (!tlv || !value) {
        return XY_TLV_INVALID_PARAM;
    }

    if (tlv->length != sizeof(uint8_t)) {
        return XY_TLV_INVALID_LENGTH;
    }

    *value = tlv->value[0];
    return XY_TLV_OK;
}

int xy_tlv_decode_uint16(const xy_tlv_t *tlv, uint16_t *value)
{
    if (!tlv || !value) {
        return XY_TLV_INVALID_PARAM;
    }

    if (tlv->length != sizeof(uint16_t)) {
        return XY_TLV_INVALID_LENGTH;
    }

    *value = read_uint16(tlv->value);
    return XY_TLV_OK;
}

int xy_tlv_decode_uint32(const xy_tlv_t *tlv, uint32_t *value)
{
    if (!tlv || !value) {
        return XY_TLV_INVALID_PARAM;
    }

    if (tlv->length != sizeof(uint32_t)) {
        return XY_TLV_INVALID_LENGTH;
    }

    *value = read_uint32(tlv->value);
    return XY_TLV_OK;
}

int xy_tlv_decode_uint64(const xy_tlv_t *tlv, uint64_t *value)
{
    if (!tlv || !value) {
        return XY_TLV_INVALID_PARAM;
    }

    if (tlv->length != sizeof(uint64_t)) {
        return XY_TLV_INVALID_LENGTH;
    }

    *value = read_uint64(tlv->value);
    return XY_TLV_OK;
}

int xy_tlv_decode_int8(const xy_tlv_t *tlv, int8_t *value)
{
    if (!tlv || !value) {
        return XY_TLV_INVALID_PARAM;
    }

    if (tlv->length != sizeof(int8_t)) {
        return XY_TLV_INVALID_LENGTH;
    }

    *value = (int8_t)tlv->value[0];
    return XY_TLV_OK;
}

int xy_tlv_decode_int16(const xy_tlv_t *tlv, int16_t *value)
{
    if (!tlv || !value) {
        return XY_TLV_INVALID_PARAM;
    }

    if (tlv->length != sizeof(int16_t)) {
        return XY_TLV_INVALID_LENGTH;
    }

    *value = (int16_t)read_uint16(tlv->value);
    return XY_TLV_OK;
}

int xy_tlv_decode_int32(const xy_tlv_t *tlv, int32_t *value)
{
    if (!tlv || !value) {
        return XY_TLV_INVALID_PARAM;
    }

    if (tlv->length != sizeof(int32_t)) {
        return XY_TLV_INVALID_LENGTH;
    }

    *value = (int32_t)read_uint32(tlv->value);
    return XY_TLV_OK;
}

int xy_tlv_decode_int64(const xy_tlv_t *tlv, int64_t *value)
{
    if (!tlv || !value) {
        return XY_TLV_INVALID_PARAM;
    }

    if (tlv->length != sizeof(int64_t)) {
        return XY_TLV_INVALID_LENGTH;
    }

    *value = (int64_t)read_uint64(tlv->value);
    return XY_TLV_OK;
}

int xy_tlv_decode_bool(const xy_tlv_t *tlv, bool *value)
{
    if (!tlv || !value) {
        return XY_TLV_INVALID_PARAM;
    }

    if (tlv->length != sizeof(uint8_t)) {
        return XY_TLV_INVALID_LENGTH;
    }

    *value = (tlv->value[0] != 0);
    return XY_TLV_OK;
}

int xy_tlv_decode_string(const xy_tlv_t *tlv, char *str, uint16_t str_len)
{
    if (!tlv || !str || str_len == 0) {
        return XY_TLV_INVALID_PARAM;
    }

    /* Need space for NULL terminator */
    if (str_len < tlv->length + 1) {
        return XY_TLV_BUFFER_OVERFLOW;
    }

    memcpy(str, tlv->value, tlv->length);
    str[tlv->length] = '\0';

    return XY_TLV_OK;
}

int xy_tlv_decode_bytes(const xy_tlv_t *tlv, uint8_t *bytes,
                        uint16_t *bytes_len)
{
    if (!tlv || !bytes || !bytes_len) {
        return XY_TLV_INVALID_PARAM;
    }

    if (*bytes_len < tlv->length) {
        return XY_TLV_BUFFER_OVERFLOW;
    }

    memcpy(bytes, tlv->value, tlv->length);
    *bytes_len = tlv->length;

    return XY_TLV_OK;
}

/* ==================== Advanced API - Searching ==================== */

int xy_tlv_find(const uint8_t *buffer, uint16_t buffer_len, uint16_t type,
                xy_tlv_t *tlv)
{
    xy_tlv_iterator_t iter;
    xy_tlv_t current;
    int ret;

    ret = xy_tlv_iterator_init(&iter, buffer, buffer_len);
    if (ret != XY_TLV_OK) {
        return ret;
    }

    while ((ret = xy_tlv_iterator_next(&iter, &current)) == XY_TLV_OK) {
        if (current.type == type) {
            if (tlv) {
                *tlv = current;
            }
            return XY_TLV_OK;
        }
    }

    return XY_TLV_NOT_FOUND;
}

int xy_tlv_find_all(const uint8_t *buffer, uint16_t buffer_len, uint16_t type,
                    xy_tlv_t *tlv_array, uint16_t *array_size)
{
    xy_tlv_iterator_t iter;
    xy_tlv_t current;
    uint16_t count     = 0;
    uint16_t max_count = tlv_array ? *array_size : 0;
    int ret;

    ret = xy_tlv_iterator_init(&iter, buffer, buffer_len);
    if (ret != XY_TLV_OK) {
        return ret;
    }

    while ((ret = xy_tlv_iterator_next(&iter, &current)) == XY_TLV_OK) {
        if (current.type == type) {
            if (tlv_array && count < max_count) {
                tlv_array[count] = current;
            }
            count++;
        }
    }

    if (array_size) {
        *array_size = count;
    }

    return (int)count;
}

int xy_tlv_count(const uint8_t *buffer, uint16_t buffer_len)
{
    xy_tlv_iterator_t iter;
    xy_tlv_t tlv;
    int count = 0;
    int ret;

    ret = xy_tlv_iterator_init(&iter, buffer, buffer_len);
    if (ret != XY_TLV_OK) {
        return ret;
    }

    while ((ret = xy_tlv_iterator_next(&iter, &tlv)) == XY_TLV_OK) {
        count++;
    }

    return count;
}

/* ==================== Advanced API - Containers ==================== */

int xy_tlv_container_begin(xy_tlv_buffer_t *tlv_buf, uint16_t type)
{
    if (!tlv_buf) {
        return XY_TLV_INVALID_PARAM;
    }

    if (tlv_buf->nesting >= XY_TLV_MAX_NESTING_LEVEL) {
        return XY_TLV_NESTING_OVERFLOW;
    }

    /* Reserve space for container header (will update length at end) */
    if (tlv_buf->offset + XY_TLV_HEADER_SIZE > tlv_buf->capacity) {
        return XY_TLV_BUFFER_OVERFLOW;
    }

    /* Write type, length will be updated at container_end */
    write_uint16(&tlv_buf->buffer[tlv_buf->offset], type);
    tlv_buf->offset += 2;
    write_uint16(&tlv_buf->buffer[tlv_buf->offset], 0); /* Placeholder */
    tlv_buf->offset += 2;

    tlv_buf->nesting++;

    return XY_TLV_OK;
}

int xy_tlv_container_end(xy_tlv_buffer_t *tlv_buf)
{
    if (!tlv_buf) {
        return XY_TLV_INVALID_PARAM;
    }

    if (tlv_buf->nesting == 0) {
        return XY_TLV_ERROR;
    }

    tlv_buf->nesting--;

    /* Note: In a full implementation, we would need to track the container
     * start position and update the length field. For simplicity, this
     * basic version requires manual length management or post-processing. */

    return XY_TLV_OK;
}

int xy_tlv_container_enter(const xy_tlv_iterator_t *iter, const xy_tlv_t *tlv,
                           xy_tlv_iterator_t *child_iter)
{
    if (!iter || !tlv || !child_iter) {
        return XY_TLV_INVALID_PARAM;
    }

    if (iter->nesting >= XY_TLV_MAX_NESTING_LEVEL) {
        return XY_TLV_NESTING_OVERFLOW;
    }

    /* Initialize child iterator with container's value */
    child_iter->buffer     = tlv->value;
    child_iter->buffer_len = tlv->length;
    child_iter->offset     = 0;
    child_iter->remaining  = tlv->length;
    child_iter->nesting    = iter->nesting + 1;

    return XY_TLV_OK;
}

/* ==================== Utility API ==================== */

int xy_tlv_validate(const uint8_t *buffer, uint16_t buffer_len)
{
    xy_tlv_iterator_t iter;
    xy_tlv_t tlv;
    int ret;

    ret = xy_tlv_iterator_init(&iter, buffer, buffer_len);
    if (ret != XY_TLV_OK) {
        return ret;
    }

    /* Try to parse all TLVs */
    while ((ret = xy_tlv_iterator_next(&iter, &tlv)) == XY_TLV_OK) {
        /* Successfully parsed */
    }

    /* Should end with NOT_FOUND (end of buffer), not an error */
    if (ret == XY_TLV_NOT_FOUND && iter.remaining == 0) {
        return XY_TLV_OK;
    }

    return ret;
}

uint16_t xy_tlv_checksum(const uint8_t *buffer, uint16_t buffer_len)
{
    /* Simple CRC16 calculation */
    uint16_t crc = 0xFFFF;
    uint16_t i;

    for (i = 0; i < buffer_len; i++) {
        crc ^= buffer[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
        }
    }

    return crc;
}

const char *xy_tlv_get_type_name(uint16_t type)
{
    switch (type) {
    case XY_TLV_TYPE_UINT8:
        return "UINT8";
    case XY_TLV_TYPE_UINT16:
        return "UINT16";
    case XY_TLV_TYPE_UINT32:
        return "UINT32";
    case XY_TLV_TYPE_UINT64:
        return "UINT64";
    case XY_TLV_TYPE_INT8:
        return "INT8";
    case XY_TLV_TYPE_INT16:
        return "INT16";
    case XY_TLV_TYPE_INT32:
        return "INT32";
    case XY_TLV_TYPE_INT64:
        return "INT64";
    case XY_TLV_TYPE_FLOAT:
        return "FLOAT";
    case XY_TLV_TYPE_DOUBLE:
        return "DOUBLE";
    case XY_TLV_TYPE_BOOL:
        return "BOOL";
    case XY_TLV_TYPE_STRING:
        return "STRING";
    case XY_TLV_TYPE_BYTES:
        return "BYTES";
    case XY_TLV_TYPE_BLOB:
        return "BLOB";
    case XY_TLV_TYPE_CONTAINER:
        return "CONTAINER";
    case XY_TLV_TYPE_ARRAY:
        return "ARRAY";
    case XY_TLV_TYPE_LIST:
        return "LIST";
    case XY_TLV_TYPE_TIMESTAMP:
        return "TIMESTAMP";
    case XY_TLV_TYPE_UUID:
        return "UUID";
    case XY_TLV_TYPE_MAC_ADDR:
        return "MAC_ADDR";
    case XY_TLV_TYPE_IPV4_ADDR:
        return "IPV4_ADDR";
    case XY_TLV_TYPE_IPV6_ADDR:
        return "IPV6_ADDR";
    case XY_TLV_TYPE_CHECKSUM:
        return "CHECKSUM";
    default:
        return "UNKNOWN";
    }
}

const char *xy_tlv_get_error_string(int error_code)
{
    switch (error_code) {
    case XY_TLV_OK:
        return "Success";
    case XY_TLV_ERROR:
        return "General error";
    case XY_TLV_INVALID_PARAM:
        return "Invalid parameter";
    case XY_TLV_BUFFER_OVERFLOW:
        return "Buffer overflow";
    case XY_TLV_BUFFER_UNDERFLOW:
        return "Buffer underflow";
    case XY_TLV_TYPE_MISMATCH:
        return "Type mismatch";
    case XY_TLV_NOT_FOUND:
        return "Not found";
    case XY_TLV_INVALID_LENGTH:
        return "Invalid length";
    case XY_TLV_NESTING_OVERFLOW:
        return "Nesting overflow";
    case XY_TLV_CHECKSUM_ERROR:
        return "Checksum error";
    default:
        return "Unknown error";
    }
}

int xy_tlv_get_stats(xy_tlv_stats_t *stats)
{
    if (!stats) {
        return XY_TLV_INVALID_PARAM;
    }

    *stats = g_tlv_stats;
    return XY_TLV_OK;
}

void xy_tlv_reset_stats(void)
{
    memset(&g_tlv_stats, 0, sizeof(xy_tlv_stats_t));
}

void xy_tlv_print(const uint8_t *buffer, uint16_t buffer_len, uint8_t indent)
{
    /* This function would require printf or similar output facility.
     * For embedded systems, implementation depends on available logging. */
    (void)buffer;
    (void)buffer_len;
    (void)indent;
}
