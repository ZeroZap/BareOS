/**
 * @file xy_string.c
 * @brief XinYi String Library Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "xy_string.h"
#include "xy_ctype.h"
#include "xy_stdlib.h"
#include "xy_mem.h"

/* Memory Functions */

void *xy_memset(void *dst, uint8_t val, uint32_t len)
{
    if (!dst) return NULL;
    
    uint8_t *ptr = (uint8_t *)dst;
    for (uint32_t i = 0; i < len; i++) {
        ptr[i] = val;
    }
    return dst;
}

void *xy_memcpy(void *dst, const void *src, uint32_t n)
{
    if (!dst || !src) return NULL;
    
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    
    for (uint32_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dst;
}

int32_t xy_memcmp(const void *s1, const void *s2, uint32_t n)
{
    if (!s1 || !s2) return -1;
    
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    
    for (uint32_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return (int32_t)p1[i] - (int32_t)p2[i];
        }
    }
    return 0;
}

/* String Functions */

uint32_t xy_strlen(const char *str)
{
    if (!str) return 0;
    
    uint32_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

int32_t xy_strcmp(const char *str1, const char *str2)
{
    if (!str1 || !str2) {
        return (!str1 && !str2) ? 0 : (!str1 ? -1 : 1);
    }
    
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}

int32_t xy_strncmp(const char *str1, const char *str2, uint32_t num)
{
    if (!str1 || !str2) {
        return (!str1 && !str2) ? 0 : (!str1 ? -1 : 1);
    }
    
    for (uint32_t i = 0; i < num; i++) {
        if (str1[i] != str2[i]) {
            return (int32_t)(unsigned char)str1[i] - (int32_t)(unsigned char)str2[i];
        }
        if (str1[i] == '\0') {
            break;
        }
    }
    return 0;
}

int32_t xy_stricmp(const char *str1, const char *str2)
{
    if (!str1 || !str2) {
        return (!str1 && !str2) ? 0 : (!str1 ? -1 : 1);
    }
    
    while (*str1 && xy_tolower(*str1) == xy_tolower(*str2)) {
        str1++;
        str2++;
    }
    
    return xy_tolower(*(unsigned char *)str1) - xy_tolower(*(unsigned char *)str2);
}

char *xy_strcpy(char *dest, const char *src)
{
    if (!dest || !src) return NULL;
    
    char *ret = dest;
    while ((*dest++ = *src++) != '\0')
        ;
    return ret;
}

char *xy_strncpy(char *dest, const char *src, uint32_t n)
{
    if (!dest || !src) return NULL;
    
    char *ret = dest;
    uint32_t i;
    
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    
    /* Pad with null bytes if src is shorter than n */
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    
    return ret;
}

char *xy_strcat(char *dest, const char *src)
{
    if (!dest || !src) return NULL;
    
    char *ret = dest;
    /* Find end of dest */
    while (*dest) dest++;
    /* Append src */
    while ((*dest++ = *src++) != '\0')
        ;
    return ret;
}

char *xy_strncat(char *dest, const char *src, uint32_t n)
{
    if (!dest || !src) return NULL;
    
    char *ret = dest;
    /* Find end of dest */
    while (*dest) dest++;
    /* Append up to n chars from src */
    uint32_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        *dest++ = src[i];
    }
    *dest = '\0';
    return ret;
}

char *xy_strchr(const char *str, uint8_t c)
{
    if (!str) return NULL;
    
    char ch = (char)c;
    while (*str) {
        if (*str == ch) return (char *)str;
        str++;
    }
    
    if (ch == '\0') return (char *)str;
    return NULL;
}

char *xy_strrchr(const char *str, uint8_t c)
{
    if (!str) return NULL;
    
    const char *last = NULL;
    char ch = (char)c;
    
    while (*str) {
        if (*str == ch) last = str;
        str++;
    }
    
    if (ch == '\0') return (char *)str;
    return (char *)last;
}

size_t xy_strcspn(const char *str1, const char *str2)
{
    if (!str1 || !str2) return 0;
    
    const char *s1 = str1;
    while (*s1) {
        const char *s2 = str2;
        while (*s2) {
            if (*s1 == *s2) {
                return s1 - str1;
            }
            s2++;
        }
        s1++;
    }
    return s1 - str1;
}

int32_t xy_strpbrk(const char *str1, const char *str2)
{
    if (!str1 || !str2) return -1;
    
    const char *s1 = str1;
    while (*s1) {
        const char *s2 = str2;
        while (*s2) {
            if (*s1 == *s2) {
                return (int32_t)(s1 - str1);
            }
            s2++;
        }
        s1++;
    }
    return -1;
}

char *xy_strstr(const char *str1, const char *str2)
{
    if (!str1 || !str2) return NULL;
    if (*str2 == '\0') return (char *)str1;
    
    const char *s1 = str1;
    while (*s1) {
        const char *s1p = s1;
        const char *s2p = str2;
        
        while (*s1p && *s2p && *s1p == *s2p) {
            s1p++;
            s2p++;
        }
        
        if (*s2p == '\0') {
            return (char *)s1;
        }
        
        s1++;
    }
    return NULL;
}

char *xy_strtok(char *str, const char *delim)
{
    static char *last_token = NULL;
    
    if (str != NULL) {
        last_token = str;
    }
    
    if (!last_token) return NULL;
    
    /* Skip leading delimiters */
    while (*last_token && xy_strchr(delim, *last_token)) {
        last_token++;
    }
    
    if (*last_token == '\0') {
        last_token = NULL;
        return NULL;
    }
    
    char *token_start = last_token;
    
    /* Find end of token */
    while (*last_token && !xy_strchr(delim, *last_token)) {
        last_token++;
    }
    
    if (*last_token) {
        *last_token = '\0';
        last_token++;
    } else {
        last_token = NULL;
    }
    
    return token_start;
}

uint8_t *hexstr2bytes(char *hexstr)
{
    if (!hexstr) return NULL;
    
    size_t len = xy_strlen(hexstr);
    if (len % 2 != 0) return NULL; /* Hex string must have even length */
    
    size_t byte_len = len / 2;
    uint8_t *bytes = (uint8_t *)xy_malloc(byte_len);
    if (!bytes) return NULL;
    
    for (size_t i = 0; i < byte_len; i++) {
        char hex_byte[3] = { hexstr[i * 2], hexstr[i * 2 + 1], '\0' };
        
        char *endptr;
        bytes[i] = (uint8_t)xy_strtoul(hex_byte, &endptr, 16);
    }
    
    return bytes;
}

/* Additional string functions */

void *xy_memmove(void *dest, const void *src, size_t n)
{
    if (!dest || !src) return NULL;
    if (dest == src) return dest;
    
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    
    if (d < s) {
        /* Forward copy */
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        /* Backward copy to handle overlapping */
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    
    return dest;
}

void *xy_memchr(const void *s, int c, size_t n)
{
    if (!s) return NULL;
    
    const uint8_t *ptr = (const uint8_t *)s;
    uint8_t val = (uint8_t)c;
    
    for (size_t i = 0; i < n; i++) {
        if (ptr[i] == val) {
            return (void *)(ptr + i);
        }
    }
    
    return NULL;
}

int xy_strcasecmp(const char *s1, const char *s2)
{
    if (!s1 || !s2) {
        return (!s1 && !s2) ? 0 : (!s1 ? -1 : 1);
    }
    
    while (*s1 && xy_tolower(*s1) == xy_tolower(*s2)) {
        s1++;
        s2++;
    }
    
    return xy_tolower(*(unsigned char *)s1) - xy_tolower(*(unsigned char *)s2);
}

int xy_strncasecmp(const char *s1, const char *s2, size_t n)
{
    if (!s1 || !s2) {
        return (!s1 && !s2) ? 0 : (!s1 ? -1 : 1);
    }
    
    for (size_t i = 0; i < n; i++) {
        if (xy_tolower(s1[i]) != xy_tolower(s2[i])) {
            return xy_tolower((unsigned char)s1[i]) - xy_tolower((unsigned char)s2[i]);
        }
        if (s1[i] == '\0') {
            break;
        }
    }
    
    return 0;
}

size_t xy_strnlen(const char *s, size_t maxlen)
{
    if (!s) return 0;
    
    size_t i;
    for (i = 0; i < maxlen && s[i] != '\0'; i++)
        ;
    return i;
}

char *xy_strdup(const char *s)
{
    if (!s) return NULL;
    
    size_t len = xy_strlen(s);
    char *dup = (char *)xy_malloc(len + 1);
    if (!dup) return NULL;
    
    xy_strcpy(dup, s);
    return dup;
}

char *xy_strndup(const char *s, size_t n)
{
    if (!s) return NULL;
    
    size_t len = xy_strnlen(s, n);
    char *dup = (char *)xy_malloc(len + 1);
    if (!dup) return NULL;
    
    xy_strncpy(dup, s, len);
    dup[len] = '\0';
    return dup;
}

size_t xy_strspn(const char *s, const char *accept)
{
    if (!s || !accept) return 0;
    
    size_t count = 0;
    const char *s_ptr = s;
    
    while (*s_ptr) {
        const char *a_ptr = accept;
        int found = 0;
        
        while (*a_ptr) {
            if (*s_ptr == *a_ptr) {
                found = 1;
                break;
            }
            a_ptr++;
        }
        
        if (!found) break;
        count++;
        s_ptr++;
    }
    
    return count;
}

void *xy_memrchr(const void *s, int c, size_t n)
{
    if (!s) return NULL;
    
    const uint8_t *ptr = (const uint8_t *)s;
    uint8_t val = (uint8_t)c;
    
    for (size_t i = n; i > 0; i--) {
        if (ptr[i - 1] == val) {
            return (void *)(ptr + i - 1);
        }
    }
    
    return NULL;
}
