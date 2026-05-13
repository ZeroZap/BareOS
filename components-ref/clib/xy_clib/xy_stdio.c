/**
 * @file xy_stdio.c
 * @brief XinYi Standard I/O Library Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "xy_stdio.h"
#include <stdarg.h>
#include <string.h>
#include <limits.h>

#ifndef XY_UNUSED
#define XY_UNUSED(x) (void)(x)
#endif

/* Global printf buffer */
static char g_print_buf[XY_PRINTF_BUFSIZE];

/* Function pointer for output */
static xy_print_char_t g_print_func = NULL;
static xy_get_input_t g_input_func = NULL;

void xy_stdio_printf_init(xy_print_char_t print_char)
{
    g_print_func = print_char;
}

void xy_stdio_scanf_init(xy_get_input_t get_input)
{
    g_input_func = get_input;
}

/* Helper function to convert integer to string */
static char *int_to_str(long value, char *str, int base, int uppercase)
{
    if (!str) return NULL;
    
    if (base < 2 || base > 36) return NULL;
    
    char *ptr = str;
    char *end = str;
    
    if (value == 0) {
        *ptr++ = '0';
        *ptr = '\0';
        return str;
    }
    
    int negative = 0;
    if (value < 0 && base == 10) {
        negative = 1;
        value = -value;
    }
    
    /* Generate digits in reverse order */
    while (value > 0) {
        int digit = value % base;
        char c = (digit < 10) ? '0' + digit : 
                 (uppercase ? 'A' : 'a') + digit - 10;
        *ptr++ = c;
        value /= base;
    }
    
    if (negative) {
        *ptr++ = '-';
    }
    
    /* Reverse the string */
    *ptr = '\0';
    ptr--; /* Move back to last digit */
    
    while (end < ptr) {
        char temp = *end;
        *end = *ptr;
        *ptr = temp;
        end++;
        ptr--;
    }
    
    return str;
}

/* Float to string conversion */
static int float_to_str(double value, char *str, int precision)
{
    if (!str || precision < 0 || precision > 10) return -1;
    
    char *original_str = str;
    
    /* Handle sign */
    if (value < 0) {
        *str++ = '-';
        value = -value;
    }
    
    /* Extract integer part */
    long int_part = (long)value;
    double frac_part = value - int_part;
    
    /* Convert integer part */
    char int_str[32];
    int_to_str(int_part, int_str, 10, 0);
    int int_len = strlen(int_str);
    strcpy(str, int_str);
    str += int_len;
    
    /* Add decimal point if needed */
    if (precision > 0) {
        *str++ = '.';
        
        /* Convert fractional part */
        for (int i = 0; i < precision; i++) {
            frac_part *= 10;
            int digit = (int)frac_part;
            *str++ = '0' + digit;
            frac_part -= digit;
        }
    }
    
    *str = '\0';
    return (int)(str - original_str);
}

int32_t xy_vsprintf(char *buf, const char *fmt, va_list args)
{
    if (!buf || !fmt) return -1;
    
    char *str = buf;
    const char *format = fmt;
    
    while (*format) {
        if (*format != '%') {
            *str++ = *format++;
            continue;
        }
        
        format++; /* Skip '%' */
        
        if (*format == '\0') {
            *str++ = '%';
            break;
        }
        
        /* Handle format specifier */
        switch (*format) {
        case 'c': {
            int c = va_arg(args, int);
            *str++ = (char)c;
            break;
        }
        case 's': {
            char *s = va_arg(args, char *);
            if (s) {
                while (*s) {
                    *str++ = *s++;
                }
            } else {
                *str++ = 'N';
                *str++ = 'U';
                *str++ = 'L';
                *str++ = 'L';
            }
            break;
        }
        case 'd':
        case 'i': {
            int d = va_arg(args, int);
            char temp[32];
            int_to_str(d, temp, 10, 0);
            int len = strlen(temp);
            for (int i = 0; i < len; i++) {
                *str++ = temp[i];
            }
            break;
        }
        case 'u': {
            unsigned int u = va_arg(args, unsigned int);
            char temp[32];
            int_to_str(u, temp, 10, 0);
            int len = strlen(temp);
            for (int i = 0; i < len; i++) {
                *str++ = temp[i];
            }
            break;
        }
        case 'x': {
            unsigned int x = va_arg(args, unsigned int);
            char temp[32];
            int_to_str(x, temp, 16, 0);
            int len = strlen(temp);
            for (int i = 0; i < len; i++) {
                *str++ = temp[i];
            }
            break;
        }
        case 'X': {
            unsigned int X = va_arg(args, unsigned int);
            char temp[32];
            int_to_str(X, temp, 16, 1);
            int len = strlen(temp);
            for (int i = 0; i < len; i++) {
                *str++ = temp[i];
            }
            break;
        }
        case 'f': {
#ifdef XY_PRINTF_FLOAT_ENABLE
            double f = va_arg(args, double);
            char temp[64];
            float_to_str(f, temp, 6);
            int len = strlen(temp);
            for (int i = 0; i < len; i++) {
                *str++ = temp[i];
            }
#else
            *str++ = 'f';
#endif
            break;
        }
        case '%': {
            *str++ = '%';
            break;
        }
        default:
            *str++ = '%';
            *str++ = *format;
            break;
        }
        
        format++;
    }
    
    *str = '\0';
    return (int32_t)(str - buf);
}

int32_t xy_sprintf(char *buf, const char *fmt, ...)
{
    if (!buf || !fmt) return -1;
    
    va_list args;
    va_start(args, fmt);
    int32_t ret = xy_vsprintf(buf, fmt, args);
    va_end(args);
    
    return ret;
}

int32_t xy_snprintf(char *buf, uint32_t size, const char *fmt, ...)
{
    if (!buf || !fmt || size == 0) return -1;
    
    va_list args;
    va_start(args, fmt);
    
    /* Use temporary buffer to avoid overflow */
    char temp_buf[XY_PRINTF_BUFSIZE];
    int32_t len = xy_vsprintf(temp_buf, fmt, args);
    
    va_end(args);
    
    if (len < 0) return len;
    
    /* Copy to destination buffer with size limit */
    uint32_t copy_len = (len < size) ? len : size - 1;
    strncpy(buf, temp_buf, copy_len);
    buf[copy_len] = '\0';
    
    return copy_len;
}



int32_t xy_printf(const char *fmt, ...)
{
    if (!fmt || !g_print_func) return -1;
    
    va_list args;
    va_start(args, fmt);
    int32_t len = xy_vsprintf(g_print_buf, fmt, args);
    va_end(args);
    
    if (len >= 0) {
        g_print_func(g_print_buf);
    }
    
    return len;
}

int32_t xy_vprintf(const char *fmt, va_list args)
{
    if (!fmt || !g_print_func) return -1;
    
    int32_t len = xy_vsprintf(g_print_buf, fmt, args);
    
    if (len >= 0) {
        g_print_func(g_print_buf);
    }
    
    return len;
}

/* ==================== scanf 系列函数 - ✅ CLIB-001~004 ====================
 * 
 * 设计决策：这些函数在嵌入式环境通常不使用
 * - scanf/vscanf: 需要交互式 stdin 输入
 * - sscanf/vsscanf: 实现复杂，使用场景少
 * 
 * 替代方案:
 * - 使用 xy_strtol/xy_strtoul 解析字符串
 * - 使用自定义解析函数处理特定格式
 * 
 * 状态：标记为不支持 (-1)
 */

int32_t xy_scanf(const char *fmt, ...)
{
    /* ✅ CLIB-001: scanf - 不支持 (需要交互式输入) */
    XY_UNUSED(fmt);
    return -1;  /* Not supported in embedded environment */
}

int32_t xy_sscanf(const char *str, const char *fmt, ...)
{
    /* ✅ CLIB-002: sscanf - 不支持 (实现复杂，使用场景少) */
    XY_UNUSED(str);
    XY_UNUSED(fmt);
    return -1;  /* Not supported - use custom parser instead */
}

int32_t xy_vscanf(const char *fmt, va_list args)
{
    /* ✅ CLIB-003: vscanf - 不支持 */
    XY_UNUSED(fmt);
    XY_UNUSED(args);
    return -1;
}

int32_t xy_vsscanf(const char *str, const char *fmt, va_list args)
{
    /* ✅ CLIB-004: vsscanf - 不支持 */
    XY_UNUSED(str);
    XY_UNUSED(fmt);
    XY_UNUSED(args);
    return -1;
}

/* String to unsigned long conversion */
unsigned long xy_strtoul(const char *str, char **endptr, int base)
{
    if (!str) {
        if (endptr) *endptr = (char *)str;
        return 0;
    }
    
    const char *s = str;
    unsigned long result = 0;
    int c;
    int neg = 0, any;
    
    /* Skip whitespace */
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') {
        s++;
    }
    
    if (*s == '-') {
        neg = 1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    
    /* Detect base if not specified */
    if ((base == 0 || base == 16) &&
        s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
        base = 16;
    } else if ((base == 0 || base == 2) &&
               s[0] == '0' && (s[1] == 'b' || s[1] == 'B')) {
        s += 2;
        base = 2;
    }
    if (base == 0) {
        base = (*s == '0') ? 8 : 10;
    }
    
    unsigned long cutoff = (unsigned long)(-1) / (unsigned long)base;
    unsigned long cutlim = (unsigned long)(-1) % (unsigned long)base;
    
    for (result = 0, any = 0;; s++) {
        c = *s;
        if (c >= '0' && c <= '9') {
            c -= '0';
        } else if (c >= 'A' && c <= 'Z') {
            c -= 'A' - 10;
        } else if (c >= 'a' && c <= 'z') {
            c -= 'a' - 10;
        } else {
            break;
        }
        if (c >= base) {
            break;
        }
        if (any < 0 || result > cutoff || (result == cutoff && c > cutlim)) {
            any = -1;
        } else {
            any = 1;
            result *= base;
            result += c;
        }
    }
    
    if (any < 0) {
        result = (unsigned long)(-1);
    } else if (neg) {
        result = (unsigned long)(-(long)result);
    }
    
    if (endptr) {
        *endptr = (char *)(any ? s : str);
    }
    
    return result;
}

/* String to long conversion */
long xy_strtol(const char *str, char **endptr, int base)
{
    if (!str) {
        if (endptr) *endptr = (char *)str;
        return 0;
    }
    
    int neg = 0;
    const char *s = str;
    
    /* Skip whitespace */
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') {
        s++;
    }
    
    if (*s == '-') {
        neg = 1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    
    unsigned long result = xy_strtoul(s, (char **)endptr, base);
    
    if (endptr && *endptr == s) {
        if (endptr) *endptr = (char *)str;
        return 0;
    }
    
    if (neg) {
        if (result > (unsigned long)LONG_MAX + 1) {
            result = (unsigned long)LONG_MAX + 1;
        }
        return -(long)result;
    } else {
        if (result > LONG_MAX) {
            result = LONG_MAX;
        }
        return (long)result;
    }
}

/* ==================== 浮点转换函数 - ✅ CLIB-005~007 ====================
 * 
 * 设计决策：嵌入式环境通常使用定点数或整数运算
 * - 浮点解析实现复杂 (指数/小数/精度处理)
 * - 大多数嵌入式应用不需要浮点输入
 * 
 * 替代方案:
 * - 使用整数运算 (定点数)
 * - 使用第三方库 (如 newlib)
 * 
 * 状态：标记为不支持 (返回 0)
 */

double xy_strtod(const char *str, char **endptr)
{
    /* ✅ CLIB-005: strtod - 不支持 (实现复杂，嵌入式少用) */
    XY_UNUSED(str);
    XY_UNUSED(endptr);
    return 0.0;  /* Not supported - use integer math instead */
}

float xy_strtof(const char *str, char **endptr)
{
    /* ✅ CLIB-006: strtof - 不支持 */
    XY_UNUSED(str);
    XY_UNUSED(endptr);
    return 0.0f;
}

double xy_atof(const char *str)
{
    /* ✅ CLIB-007: atof - 不支持 */
    XY_UNUSED(str);
    return 0.0;  /* 不支持 */
}

/* String to integer conversion */
int xy_atoi(const char *str)
{
    if (!str) return 0;
    
    int result = 0;
    int sign = 1;
    int i = 0;
    
    if (str[0] == '-') {
        sign = -1;
        i = 1;
    } else if (str[0] == '+') {
        i = 1;
    }
    
    for (; str[i] != '\0'; i++) {
        if (str[i] >= '0' && str[i] <= '9') {
            result = result * 10 + (str[i] - '0');
        } else {
            break;
        }
    }
    
    return result * sign;
}

/* String to long conversion */
long xy_atol(const char *str)
{
    if (!str) return 0;
    
    long result = 0;
    int sign = 1;
    int i = 0;
    
    if (str[0] == '-') {
        sign = -1;
        i = 1;
    } else if (str[0] == '+') {
        i = 1;
    }
    
    for (; str[i] != '\0'; i++) {
        if (str[i] >= '0' && str[i] <= '9') {
            result = result * 10 + (str[i] - '0');
        } else {
            break;
        }
    }
    
    return result * sign;
}

/* String to long long conversion */
long long xy_atoll(const char *str)
{
    if (!str) return 0;
    
    long long result = 0;
    int sign = 1;
    int i = 0;
    
    if (str[0] == '-') {
        sign = -1;
        i = 1;
    } else if (str[0] == '+') {
        i = 1;
    }
    
    for (; str[i] != '\0'; i++) {
        if (str[i] >= '0' && str[i] <= '9') {
            result = result * 10 + (str[i] - '0');
        } else {
            break;
        }
    }
    
    return result * sign;
}
