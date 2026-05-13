/**
 * @file xy_helper.h
 * @brief XinYi Helper Macros and Utility Functions
 * @version 2.0
 * @date 2026-02-28
 */

#ifndef XY_HELPER_H
#define XY_HELPER_H

#include "xy_typedef.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Helper Macros ==================== */

/**
 * @brief Get offset of member in struct
 * @param type Struct type
 * @param member Member name
 * @return Offset in bytes
 */
#define XY_OFFSET_OF(type, member) \
    ((xy_size_t)&((type *)0)->member)

/**
 * @brief Get container of struct from member pointer
 * @param ptr Pointer to member
 * @param type Container struct type
 * @param member Member name
 * @return Pointer to container struct
 */
#define XY_CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - XY_OFFSET_OF(type, member)))

/**
 * @brief Get array size
 * @param arr Array
 * @return Number of elements in array
 */
#define XY_ARRAY_SIZE(arr) \
    (sizeof(arr) / sizeof((arr)[0]))

/**
 * @brief Get minimum of two values
 * @param a First value
 * @param b Second value
 * @return Minimum value
 */
#define XY_MIN(a, b) \
    (((a) < (b)) ? (a) : (b))

/**
 * @brief Get maximum of two values
 * @param a First value
 * @param b Second value
 * @return Maximum value
 */
#define XY_MAX(a, b) \
    (((a) > (b)) ? (a) : (b))

/**
 * @brief Clamp value to range
 * @param x Value to clamp
 * @param min Minimum value
 * @param max Maximum value
 * @return Clamped value
 */
#define XY_CLAMP(x, min, max) \
    (((x) < (min)) ? (min) : (((x) > (max)) ? (max) : (x)))

/**
 * @brief Round up to nearest multiple
 * @param value Value to round
 * @param multiple Multiple to round to
 * @return Rounded value
 */
#define XY_ROUND_UP(value, multiple) \
    (((value) + (multiple) - 1) / (multiple) * (multiple))

/**
 * @brief Align value to boundary
 * @param value Value to align
 * @param boundary Alignment boundary (must be power of 2)
 * @return Aligned value
 */
#define XY_ALIGN_UP(value, boundary) \
    (((value) + (boundary) - 1) & ~((boundary) - 1))

/**
 * @brief Check if value is aligned to boundary
 * @param value Value to check
 * @param boundary Alignment boundary (must be power of 2)
 * @return 1 if aligned, 0 otherwise
 */
#define XY_IS_ALIGNED(value, boundary) \
    (((value) & ((boundary) - 1)) == 0)

/**
 * @brief Swap two values
 * @param a First value
 * @param b Second value
 * @param type Type of values
 */
#define XY_SWAP(a, b, type) \
    do { \
        type temp = (a); \
        (a) = (b); \
        (b) = temp; \
    } while(0)

/**
 * @brief Swap two pointers
 * @param a First pointer
 * @param b Second pointer
 */
#define XY_SWAP_PTR(a, b) \
    do { \
        void *temp = (a); \
        (a) = (b); \
        (b) = temp; \
    } while(0)

/**
 * @brief Check if value is power of 2
 * @param value Value to check
 * @return 1 if power of 2, 0 otherwise
 */
#define XY_IS_POW2(value) \
    ((value) != 0 && (((value) & ((value) - 1)) == 0))

/**
 * @brief Get next power of 2
 * @param value Input value
 * @return Next power of 2 >= value
 */
static inline uint32_t xy_next_pow2(uint32_t value)
{
    if (value == 0) return 1;
    if (value == 1) return 1;
    
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;
    
    return value;
}

/**
 * @brief Get number of bits required to represent value
 * @param value Input value
 * @return Number of bits required
 */
static inline int xy_bits_required(uint32_t value)
{
    int bits = 0;
    while (value) {
        bits++;
        value >>= 1;
    }
    return bits;
}

/**
 * @brief Get number of leading zeros
 * @param value Input value
 * @return Number of leading zeros
 */


/* ==================== Bit Manipulation Macros ==================== */

/**
 * @brief Set bit at position
 * @param value Value to modify
 * @param pos Bit position
 */
#define XY_SET_BIT(value, pos) \
    ((value) |= (1U << (pos)))

/**
 * @brief Clear bit at position
 * @param value Value to modify
 * @param pos Bit position
 */
#define XY_CLR_BIT(value, pos) \
    ((value) &= ~(1U << (pos)))

/**
 * @brief Toggle bit at position
 * @param value Value to modify
 * @param pos Bit position
 */
#define XY_TOGGLE_BIT(value, pos) \
    ((value) ^= (1U << (pos)))

/**
 * @brief Get bit at position
 * @param value Value to read
 * @param pos Bit position
 * @return Bit value (0 or 1)
 */
#define XY_GET_BIT(value, pos) \
    (((value) >> (pos)) & 1U)

/**
 * @brief Set multiple bits
 * @param value Value to modify
 * @param mask Bit mask
 */
#define XY_SET_BITS(value, mask) \
    ((value) |= (mask))

/**
 * @brief Clear multiple bits
 * @param value Value to modify
 * @param mask Bit mask
 */
#define XY_CLR_BITS(value, mask) \
    ((value) &= ~(mask))

/**
 * @brief Get multiple bits
 * @param value Value to read
 * @param mask Bit mask
 * @return Selected bits
 */
#define XY_GET_BITS(value, mask) \
    ((value) & (mask))

/**
 * @brief Extract bits from position with length
 * @param value Value to extract from
 * @param pos Starting position
 * @param len Number of bits
 * @return Extracted bits
 */
#define XY_EXTRACT_BITS(value, pos, len) \
    (((value) >> (pos)) & ((1U << (len)) - 1))

/**
 * @brief Insert bits at position
 * @param value Value to modify
 * @param pos Position to insert at
 * @param len Number of bits
 * @param bits Bits to insert
 */
#define XY_INSERT_BITS(value, pos, len, bits) \
    do { \
        uint32_t mask = ((1U << (len)) - 1) << (pos); \
        (value) = ((value) & ~mask) | (((bits) << (pos)) & mask); \
    } while(0)

/* ==================== Endian Conversion Macros ==================== */

/**
 * @brief Byte swap 16-bit value
 * @param value Input value
 * @return Byte swapped value
 */
#define XY_SWAP16(value) \
    ((((value) & 0xFF00) >> 8) | (((value) & 0x00FF) << 8))

/**
 * @brief Byte swap 32-bit value
 * @param value Input value
 * @return Byte swapped value
 */
#define XY_SWAP32(value) \
    ((((value) & 0xFF000000) >> 24) | \
     (((value) & 0x00FF0000) >> 8)  | \
     (((value) & 0x0000FF00) << 8)  | \
     (((value) & 0x000000FF) << 24))

/**
 * @brief Byte swap 64-bit value
 * @param value Input value
 * @return Byte swapped value
 */
#define XY_SWAP64(value) \
    ((((value) & 0xFF00000000000000ULL) >> 56) | \
     (((value) & 0x00FF000000000000ULL) >> 40) | \
     (((value) & 0x0000FF0000000000ULL) >> 24) | \
     (((value) & 0x000000FF00000000ULL) >> 8)  | \
     (((value) & 0x00000000FF000000ULL) << 8)  | \
     (((value) & 0x0000000000FF0000ULL) << 24) | \
     (((value) & 0x000000000000FF00ULL) << 40) | \
     (((value) & 0x00000000000000FFULL) << 56))

/**
 * @brief Host to little endian 16-bit
 * @param value Input value
 * @return Little endian value
 */
#define XY_HTOLE16(value) (value)

/**
 * @brief Host to big endian 16-bit
 * @param value Input value
 * @return Big endian value
 */
#define XY_HTOBE16(value) XY_SWAP16(value)

/**
 * @brief Little endian to host 16-bit
 * @param value Input value
 * @return Host endian value
 */
#define XY_LETOH16(value) (value)

/**
 * @brief Big endian to host 16-bit
 * @param value Input value
 * @return Host endian value
 */
#define XY_BETOH16(value) XY_SWAP16(value)

/* ==================== Utility Functions ==================== */



/* ==================== Safe Operations ==================== */

/**
 * @brief Safe string copy with size limit
 * @param dest Destination buffer
 * @param src Source string
 * @param size Destination buffer size
 * @return Number of characters copied
 */
static inline xy_size_t xy_strlcpy(char *dest, const char *src, xy_size_t size)
{
    if (!dest || !src || size == 0) return 0;
    
    xy_size_t src_len = xy_strlen(src);
    xy_size_t copy_len = (src_len >= size) ? size - 1 : src_len;
    
    for (xy_size_t i = 0; i < copy_len; i++) {
        dest[i] = src[i];
    }
    dest[copy_len] = '\0';
    
    return src_len;
}

/**
 * @brief Safe string concatenation with size limit
 * @param dest Destination buffer
 * @param src Source string
 * @param size Destination buffer size
 * @return Total length of string it tried to create
 */
static inline xy_size_t xy_strlcat(char *dest, const char *src, xy_size_t size)
{
    if (!dest || !src || size == 0) return xy_strlen(src);
    
    xy_size_t dest_len = xy_strlen(dest);
    xy_size_t src_len = xy_strlen(src);
    
    if (dest_len >= size) return size + src_len;
    
    xy_size_t copy_len = size - dest_len - 1;
    if (copy_len > src_len) copy_len = src_len;
    
    for (xy_size_t i = 0; i < copy_len; i++) {
        dest[dest_len + i] = src[i];
    }
    dest[dest_len + copy_len] = '\0';
    
    return dest_len + src_len;
}

/**
 * @brief Safe memory copy with size limit
 * @param dest Destination buffer
 * @param src Source buffer
 * @param size Copy size limit
 * @return Number of bytes copied
 */
static inline xy_size_t xy_memlcpy(void *dest, const void *src, xy_size_t size)
{
    if (!dest || !src || size == 0) return 0;
    
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    
    for (xy_size_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
    
    return size;
}

/* ==================== Debug Helpers ==================== */

/**
 * @brief Print debug message (if enabled)
 * @param fmt Format string
 * @param ... Arguments
 */
#ifdef XY_DEBUG
#define XY_DBG_PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define XY_DBG_PRINT(fmt, ...) ((void)0)
#endif

/**
 * @brief Breakpoint macro
 */
#ifdef __GNUC__
#define XY_BREAKPOINT() __builtin_trap()
#elif defined(_MSC_VER)
#define XY_BREAKPOINT() __debugbreak()
#else
#define XY_BREAKPOINT() while(1)
#endif

/**
 * @brief Static assertion
 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define XY_STATIC_ASSERT(condition, message) _Static_assert(condition, message)
#else
#define XY_STATIC_ASSERT(condition, message) \
    extern int xy_static_assert_##__LINE__[(condition) ? 1 : -1]
#endif

#ifdef __cplusplus
}
#endif

#endif /* XY_HELPER_H */
