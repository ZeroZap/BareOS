/**
 * @file xy_tlv.h
 * @brief XY TLV (Type-Length-Value) Management System
 *
 * This module provides efficient TLV encoding and decoding for embedded
 * systems. Features:
 * - Zero dynamic allocation (uses provided buffers)
 * - Type-safe API with predefined common types
 * - Support for nested TLV structures
 * - Iterator-based traversal
 * - Validation and boundary checking
 * - Compact binary format for efficient storage/transmission
 *
 * TLV Format:
 * - Type: 2 bytes (uint16_t) - identifies data type
 * - Length: 2 bytes (uint16_t) - value payload length
 * - Value: variable length - actual data payload
 *
 * @author XY Team
 * @date 2025
 */

#ifndef XY_TLV_H
#define XY_TLV_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Configuration ==================== */

#ifndef XY_TLV_MAX_NESTING_LEVEL
#define XY_TLV_MAX_NESTING_LEVEL 4 /**< Maximum nesting depth for containers \
                                    */
#endif

#ifndef XY_TLV_ENABLE_VALIDATION
#define XY_TLV_ENABLE_VALIDATION 1 /**< Enable strict validation checks */
#endif

/* ==================== Return Codes ==================== */

#define XY_TLV_OK               0  /**< Success */
#define XY_TLV_ERROR            -1 /**< General error */
#define XY_TLV_INVALID_PARAM    -2 /**< Invalid parameter */
#define XY_TLV_BUFFER_OVERFLOW  -3 /**< Buffer too small */
#define XY_TLV_BUFFER_UNDERFLOW -4 /**< Insufficient data to decode */
#define XY_TLV_TYPE_MISMATCH    -5 /**< Type does not match expected */
#define XY_TLV_NOT_FOUND        -6 /**< TLV type not found */
#define XY_TLV_INVALID_LENGTH   -7 /**< Invalid length field */
#define XY_TLV_NESTING_OVERFLOW -8 /**< Exceeded max nesting level */
#define XY_TLV_CHECKSUM_ERROR   -9 /**< Checksum validation failed */

/* ==================== TLV Header Constants ==================== */

#define XY_TLV_HEADER_SIZE 4 /**< Size of T+L fields (2+2 bytes) */

/* ==================== Predefined TLV Types ==================== */

/**
 * @brief Common TLV type definitions
 *
 * Users can define custom types starting from XY_TLV_TYPE_USER_BASE.
 * These are provided as reference values.
 */

/* Basic types (0x0001 - 0x00FF) */
#define XY_TLV_TYPE_UINT8  0x0001
#define XY_TLV_TYPE_UINT16 0x0002
#define XY_TLV_TYPE_UINT32 0x0003
#define XY_TLV_TYPE_UINT64 0x0004
#define XY_TLV_TYPE_INT8   0x0005
#define XY_TLV_TYPE_INT16  0x0006
#define XY_TLV_TYPE_INT32  0x0007
#define XY_TLV_TYPE_INT64  0x0008
#define XY_TLV_TYPE_FLOAT  0x0009
#define XY_TLV_TYPE_DOUBLE 0x000A
#define XY_TLV_TYPE_BOOL   0x000B

/* String and binary types (0x0100 - 0x01FF) */
#define XY_TLV_TYPE_STRING 0x0101 /**< NULL-terminated string */
#define XY_TLV_TYPE_BYTES  0x0102 /**< Raw binary data */
#define XY_TLV_TYPE_BLOB   0x0103 /**< Binary large object */

/* Container types (0x0200 - 0x02FF) */
#define XY_TLV_TYPE_CONTAINER 0x0201 /**< Nested TLV container */
#define XY_TLV_TYPE_ARRAY     0x0202 /**< Array of TLVs */
#define XY_TLV_TYPE_LIST      0x0203 /**< Linked list of TLVs */

/* Special types (0x0300 - 0x03FF) */
#define XY_TLV_TYPE_TIMESTAMP 0x0301 /**< Unix timestamp (uint32_t) */
#define XY_TLV_TYPE_UUID      0x0302 /**< UUID (16 bytes) */
#define XY_TLV_TYPE_MAC_ADDR  0x0303 /**< MAC address (6 bytes) */
#define XY_TLV_TYPE_IPV4_ADDR 0x0304 /**< IPv4 address (4 bytes) */
#define XY_TLV_TYPE_IPV6_ADDR 0x0305 /**< IPv6 address (16 bytes) */
#define XY_TLV_TYPE_CHECKSUM  0x0306 /**< Checksum/CRC value */

/* User-defined types */
#define XY_TLV_TYPE_USER_BASE 0x1000 /**< User-defined types start */
#define XY_TLV_TYPE_USER_END  0xFFFF /**< User-defined types end */

/* ==================== Data Structures ==================== */

/**
 * @brief TLV header structure
 */
typedef struct {
    uint16_t type;   /**< TLV type identifier */
    uint16_t length; /**< Value length in bytes */
} xy_tlv_header_t;

/**
 * @brief TLV element structure
 */
typedef struct {
    uint16_t type;        /**< TLV type identifier */
    uint16_t length;      /**< Value length in bytes */
    const uint8_t *value; /**< Pointer to value data */
} xy_tlv_t;

/**
 * @brief TLV buffer context for encoding
 */
typedef struct {
    uint8_t *buffer;   /**< Buffer for TLV data */
    uint16_t capacity; /**< Total buffer capacity */
    uint16_t offset;   /**< Current write offset */
    uint8_t nesting;   /**< Current nesting level */
} xy_tlv_buffer_t;

/**
 * @brief TLV iterator for decoding/traversal
 */
typedef struct {
    const uint8_t *buffer; /**< Source buffer */
    uint16_t buffer_len;   /**< Total buffer length */
    uint16_t offset;       /**< Current read offset */
    uint16_t remaining;    /**< Remaining bytes to parse */
    uint8_t nesting;       /**< Current nesting level */
} xy_tlv_iterator_t;

/**
 * @brief TLV statistics
 */
typedef struct {
    uint32_t total_encoded;   /**< Total TLVs encoded */
    uint32_t total_decoded;   /**< Total TLVs decoded */
    uint32_t encoding_errors; /**< Encoding error count */
    uint32_t decoding_errors; /**< Decoding error count */
    uint32_t bytes_encoded;   /**< Total bytes encoded */
    uint32_t bytes_decoded;   /**< Total bytes decoded */
} xy_tlv_stats_t;

/* ==================== Core API - Buffer Management ==================== */

/**
 * @brief Initialize TLV buffer for encoding
 *
 * @param tlv_buf TLV buffer context
 * @param buffer Destination buffer for TLV data
 * @param capacity Buffer capacity in bytes
 * @return XY_TLV_OK on success, error code otherwise
 */
int xy_tlv_buffer_init(xy_tlv_buffer_t *tlv_buf, uint8_t *buffer,
                       uint16_t capacity);

/**
 * @brief Reset TLV buffer to initial state
 *
 * @param tlv_buf TLV buffer context
 * @return XY_TLV_OK on success, error code otherwise
 */
int xy_tlv_buffer_reset(xy_tlv_buffer_t *tlv_buf);

/**
 * @brief Get current buffer usage
 *
 * @param tlv_buf TLV buffer context
 * @return Number of bytes currently used
 */
uint16_t xy_tlv_buffer_get_used(const xy_tlv_buffer_t *tlv_buf);

/**
 * @brief Get remaining buffer space
 *
 * @param tlv_buf TLV buffer context
 * @return Number of bytes available
 */
uint16_t xy_tlv_buffer_get_free(const xy_tlv_buffer_t *tlv_buf);

/* ==================== Core API - Encoding ==================== */

/**
 * @brief Encode a TLV element with raw bytes
 *
 * @param tlv_buf TLV buffer context
 * @param type TLV type
 * @param value Pointer to value data
 * @param length Value length in bytes
 * @return XY_TLV_OK on success, error code otherwise
 */
int xy_tlv_encode(xy_tlv_buffer_t *tlv_buf, uint16_t type, const void *value,
                  uint16_t length);

/**
 * @brief Encode uint8_t value
 */
int xy_tlv_encode_uint8(xy_tlv_buffer_t *tlv_buf, uint16_t type, uint8_t value);

/**
 * @brief Encode uint16_t value
 */
int xy_tlv_encode_uint16(xy_tlv_buffer_t *tlv_buf, uint16_t type,
                         uint16_t value);

/**
 * @brief Encode uint32_t value
 */
int xy_tlv_encode_uint32(xy_tlv_buffer_t *tlv_buf, uint16_t type,
                         uint32_t value);

/**
 * @brief Encode uint64_t value
 */
int xy_tlv_encode_uint64(xy_tlv_buffer_t *tlv_buf, uint16_t type,
                         uint64_t value);

/**
 * @brief Encode int8_t value
 */
int xy_tlv_encode_int8(xy_tlv_buffer_t *tlv_buf, uint16_t type, int8_t value);

/**
 * @brief Encode int16_t value
 */
int xy_tlv_encode_int16(xy_tlv_buffer_t *tlv_buf, uint16_t type, int16_t value);

/**
 * @brief Encode int32_t value
 */
int xy_tlv_encode_int32(xy_tlv_buffer_t *tlv_buf, uint16_t type, int32_t value);

/**
 * @brief Encode int64_t value
 */
int xy_tlv_encode_int64(xy_tlv_buffer_t *tlv_buf, uint16_t type, int64_t value);

/**
 * @brief Encode boolean value
 */
int xy_tlv_encode_bool(xy_tlv_buffer_t *tlv_buf, uint16_t type, bool value);

/**
 * @brief Encode NULL-terminated string
 */
int xy_tlv_encode_string(xy_tlv_buffer_t *tlv_buf, uint16_t type,
                         const char *str);

/**
 * @brief Encode binary bytes
 */
int xy_tlv_encode_bytes(xy_tlv_buffer_t *tlv_buf, uint16_t type,
                        const uint8_t *bytes, uint16_t length);

/* ==================== Core API - Decoding ==================== */

/**
 * @brief Initialize TLV iterator for decoding
 *
 * @param iter TLV iterator
 * @param buffer Source buffer containing TLV data
 * @param buffer_len Buffer length in bytes
 * @return XY_TLV_OK on success, error code otherwise
 */
int xy_tlv_iterator_init(xy_tlv_iterator_t *iter, const uint8_t *buffer,
                         uint16_t buffer_len);

/**
 * @brief Get next TLV element from iterator
 *
 * @param iter TLV iterator
 * @param tlv Output TLV element (pointer to data in buffer)
 * @return XY_TLV_OK on success, error code otherwise
 */
int xy_tlv_iterator_next(xy_tlv_iterator_t *iter, xy_tlv_t *tlv);

/**
 * @brief Check if iterator has more elements
 *
 * @param iter TLV iterator
 * @return true if more elements available, false otherwise
 */
bool xy_tlv_iterator_has_next(const xy_tlv_iterator_t *iter);

/**
 * @brief Reset iterator to beginning
 *
 * @param iter TLV iterator
 * @return XY_TLV_OK on success, error code otherwise
 */
int xy_tlv_iterator_reset(xy_tlv_iterator_t *iter);

/**
 * @brief Decode generic TLV value
 *
 * @param tlv TLV element
 * @param value Output buffer for value
 * @param value_len Buffer size (input), actual size (output)
 * @return XY_TLV_OK on success, error code otherwise
 */
int xy_tlv_decode(const xy_tlv_t *tlv, void *value, uint16_t *value_len);

/**
 * @brief Decode uint8_t value
 */
int xy_tlv_decode_uint8(const xy_tlv_t *tlv, uint8_t *value);

/**
 * @brief Decode uint16_t value
 */
int xy_tlv_decode_uint16(const xy_tlv_t *tlv, uint16_t *value);

/**
 * @brief Decode uint32_t value
 */
int xy_tlv_decode_uint32(const xy_tlv_t *tlv, uint32_t *value);

/**
 * @brief Decode uint64_t value
 */
int xy_tlv_decode_uint64(const xy_tlv_t *tlv, uint64_t *value);

/**
 * @brief Decode int8_t value
 */
int xy_tlv_decode_int8(const xy_tlv_t *tlv, int8_t *value);

/**
 * @brief Decode int16_t value
 */
int xy_tlv_decode_int16(const xy_tlv_t *tlv, int16_t *value);

/**
 * @brief Decode int32_t value
 */
int xy_tlv_decode_int32(const xy_tlv_t *tlv, int32_t *value);

/**
 * @brief Decode int64_t value
 */
int xy_tlv_decode_int64(const xy_tlv_t *tlv, int64_t *value);

/**
 * @brief Decode boolean value
 */
int xy_tlv_decode_bool(const xy_tlv_t *tlv, bool *value);

/**
 * @brief Decode string value
 *
 * @param tlv TLV element
 * @param str Output buffer for string
 * @param str_len Buffer size (must include space for NULL terminator)
 * @return XY_TLV_OK on success, error code otherwise
 */
int xy_tlv_decode_string(const xy_tlv_t *tlv, char *str, uint16_t str_len);

/**
 * @brief Decode binary bytes
 *
 * @param tlv TLV element
 * @param bytes Output buffer for bytes
 * @param bytes_len Buffer size (input), actual size (output)
 * @return XY_TLV_OK on success, error code otherwise
 */
int xy_tlv_decode_bytes(const xy_tlv_t *tlv, uint8_t *bytes,
                        uint16_t *bytes_len);

/* ==================== Advanced API - Searching ==================== */

/**
 * @brief Find TLV by type in buffer
 *
 * @param buffer Source buffer
 * @param buffer_len Buffer length
 * @param type TLV type to search for
 * @param tlv Output TLV element if found
 * @return XY_TLV_OK if found, XY_TLV_NOT_FOUND otherwise
 */
int xy_tlv_find(const uint8_t *buffer, uint16_t buffer_len, uint16_t type,
                xy_tlv_t *tlv);

/**
 * @brief Find all TLVs of specific type
 *
 * @param buffer Source buffer
 * @param buffer_len Buffer length
 * @param type TLV type to search for
 * @param tlv_array Output array for found TLVs
 * @param array_size Array capacity (input), found count (output)
 * @return Number of TLVs found, or error code if negative
 */
int xy_tlv_find_all(const uint8_t *buffer, uint16_t buffer_len, uint16_t type,
                    xy_tlv_t *tlv_array, uint16_t *array_size);

/**
 * @brief Count TLVs in buffer
 *
 * @param buffer Source buffer
 * @param buffer_len Buffer length
 * @return Number of TLVs, or error code if negative
 */
int xy_tlv_count(const uint8_t *buffer, uint16_t buffer_len);

/* ==================== Advanced API - Containers ==================== */

/**
 * @brief Begin encoding a container (nested TLV)
 *
 * @param tlv_buf TLV buffer context
 * @param type Container type
 * @return XY_TLV_OK on success, error code otherwise
 */
int xy_tlv_container_begin(xy_tlv_buffer_t *tlv_buf, uint16_t type);

/**
 * @brief End encoding a container
 *
 * @param tlv_buf TLV buffer context
 * @return XY_TLV_OK on success, error code otherwise
 */
int xy_tlv_container_end(xy_tlv_buffer_t *tlv_buf);

/**
 * @brief Enter a container for iteration
 *
 * @param iter Current iterator
 * @param tlv Container TLV element
 * @param child_iter Child iterator for container contents
 * @return XY_TLV_OK on success, error code otherwise
 */
int xy_tlv_container_enter(const xy_tlv_iterator_t *iter, const xy_tlv_t *tlv,
                           xy_tlv_iterator_t *child_iter);

/* ==================== Utility API ==================== */

/**
 * @brief Validate TLV buffer structure
 *
 * @param buffer Source buffer
 * @param buffer_len Buffer length
 * @return XY_TLV_OK if valid, error code otherwise
 */
int xy_tlv_validate(const uint8_t *buffer, uint16_t buffer_len);

/**
 * @brief Calculate checksum for TLV buffer
 *
 * @param buffer Source buffer
 * @param buffer_len Buffer length
 * @return Checksum value (CRC16)
 */
uint16_t xy_tlv_checksum(const uint8_t *buffer, uint16_t buffer_len);

/**
 * @brief Get TLV type name for debugging
 *
 * @param type TLV type
 * @return Type name string
 */
const char *xy_tlv_get_type_name(uint16_t type);

/**
 * @brief Get error message string
 *
 * @param error_code Error code
 * @return Error message string
 */
const char *xy_tlv_get_error_string(int error_code);

/**
 * @brief Get TLV statistics
 *
 * @param stats Output statistics structure
 * @return XY_TLV_OK on success, error code otherwise
 */
int xy_tlv_get_stats(xy_tlv_stats_t *stats);

/**
 * @brief Reset TLV statistics
 */
void xy_tlv_reset_stats(void);

/**
 * @brief Print TLV buffer in human-readable format (for debugging)
 *
 * @param buffer Source buffer
 * @param buffer_len Buffer length
 * @param indent Indentation level
 */
void xy_tlv_print(const uint8_t *buffer, uint16_t buffer_len, uint8_t indent);

#ifdef __cplusplus
}
#endif

#endif /* XY_TLV_H */
