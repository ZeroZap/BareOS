#ifndef _XY_STRING_H_
#define _XY_STRING_H_
#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XY_CATTR_NONE     0x00
#define XY_CATTR_ALPHA    0x01
#define XY_CATTR_LOWER    0x02
#define XY_CATTR_UPPER    0x04
#define XY_CATTR_DIGIT    0x08
#define XY_CATTR_XDIGIT   0x10
#define XY_CATTR_WHSPACE  0x20
#define XY_CATTR_FILENM83 0x80

//
// Set MINIMIZE_CATTR_TABLE to 1 to configure for minimal CATTR table size,
//  (256 instead of 512 bytes) but at a cost of slightly larger code size.
//  However, setting this option also provides an additional level of checking
//  of the argument; if the argument is not a uint8_t, the functions are
//  guaranteed to return 0.
//
#define MINIMIZE_CATTR_TABLE 0

/**
 * memset
 * strcat
 * strncat
 * strchr
 * strcmp
 * strool
 * strcpy
 * strlen
 * strp..
 * https://www.runoob.com/cprogramming/c-standard-library-string-h.html
 */

void *xy_memset(void *dst, uint8_t val, uint32_t len);
int32_t xy_memcmp(const void *s1, const void *s2, uint32_t n);
void *xy_memcpy(void *dst, const void *src, uint32_t n);

uint32_t xy_strlen(const char *str);
int32_t xy_strncmp(const char *str1, const char *str2, uint32_t num);
int32_t xy_stricmp(const char *str1, const char *str2);
int32_t xy_strcmp(const char *str1, const char *str2);

char *xy_strncpy(char *dest, const char *src, uint32_t n);
char *xy_strcpy(char *dest, const char *src);

char *xy_strncat(char *dest, const char *src, uint32_t n);
char *xy_strcat(char *dest, const char *src);

char *xy_strchr(const char *str, uint8_t c);
char *xy_strrchr(const char *str, uint8_t c);

size_t xy_strcspn(const char *str1, const char *str2);
int32_t xy_strpbrk(const char *str1, const char *str2);
char *xy_strstr(const char *str1, const char *str2);
char *xy_strtok(char *str, const char *delim);

/** Convert a string of characters representing
 * a hex buffer into a series of bytes of that real value*/
uint8_t *hexstr2bytes(char *hexstr);

/* ========================================================================
 * Additional string functions (补充函数)
 * ======================================================================== */

/**
 * @brief Move memory area (handles overlapping regions)
 * @param dest Destination buffer
 * @param src Source buffer
 * @param n Number of bytes to copy
 * @return Pointer to dest
 */
void *xy_memmove(void *dest, const void *src, size_t n);

/**
 * @brief Scan memory for a character
 * @param s Memory area to search
 * @param c Character to find (converted to unsigned char)
 * @param n Number of bytes to search
 * @return Pointer to matching byte, or NULL if not found
 */
void *xy_memchr(const void *s, int c, size_t n);

/**
 * @brief Compare strings ignoring case
 * @param s1 First string
 * @param s2 Second string
 * @return 0 if equal, <0 if s1<s2, >0 if s1>s2
 */
int xy_strcasecmp(const char *s1, const char *s2);

/**
 * @brief Compare strings ignoring case (limited length)
 * @param s1 First string
 * @param s2 Second string
 * @param n Maximum number of characters to compare
 * @return 0 if equal, <0 if s1<s2, >0 if s1>s2
 */
int xy_strncasecmp(const char *s1, const char *s2, size_t n);

/**
 * @brief Get string length (with maximum limit)
 * @param s String to measure
 * @param maxlen Maximum length to check
 * @return Length of string, or maxlen if no null terminator found
 */
size_t xy_strnlen(const char *s, size_t maxlen);

/**
 * @brief Duplicate a string (allocates memory)
 * @param s String to duplicate
 * @return Pointer to duplicated string, or NULL on failure
 * @note Caller must free the returned pointer
 */
char *xy_strdup(const char *s);

/**
 * @brief Duplicate a string with length limit (allocates memory)
 * @param s String to duplicate
 * @param n Maximum number of characters to copy
 * @return Pointer to duplicated string, or NULL on failure
 * @note Caller must free the returned pointer
 */
char *xy_strndup(const char *s, size_t n);

/**
 * @brief Get span of character set in string
 * @param s String to search
 * @param accept Set of characters to match
 * @return Length of initial segment containing only characters from accept
 */
size_t xy_strspn(const char *s, const char *accept);

/**
 * @brief Reverse memory search for a character
 * @param s Memory area to search
 * @param c Character to find
 * @param n Number of bytes to search
 * @return Pointer to last matching byte, or NULL if not found
 */
void *xy_memrchr(const void *s, int c, size_t n);

#ifdef __cplusplus
}
#endif

#endif
