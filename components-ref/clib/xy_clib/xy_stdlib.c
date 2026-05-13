/**
 * @file xy_stdlib.c
 * @brief XinYi Standard Library Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "xy_stdlib.h"
#include <string.h>
#include <math.h>

#ifndef XY_UNUSED
#define XY_UNUSED(x) (void)(x)
#endif

/* Memory Management */

void *xy_malloc(size_t size)
{
    /* In embedded systems, this might use a memory pool */
    /* For now, delegate to system malloc if available */
    XY_UNUSED(size);
    return NULL; /* Placeholder - implement with memory pool in real system */
}

void *xy_calloc(size_t nmemb, size_t size)
{
    size_t total_size = nmemb * size;
    void *ptr = xy_malloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void *xy_realloc(void *ptr, size_t size)
{
    XY_UNUSED(ptr);
    XY_UNUSED(size);
    /* Placeholder - implement with memory pool in real system */
    return NULL;
}

void xy_free(void *ptr)
{
    XY_UNUSED(ptr);
    /* Placeholder - implement with memory pool in real system */
}

void xy_safe_free(void **ptr)
{
    if (ptr && *ptr) {
        xy_free(*ptr);
        *ptr = NULL;
    }
}

/* String to Number Conversion */

double xy_atof(const char *str)
{
    if (!str) return 0.0;
    
    double result = 0.0;
    double factor = 1.0;
    int sign = 1;
    int i = 0;
    
    if (str[0] == '-') {
        sign = -1;
        i = 1;
    } else if (str[0] == '+') {
        i = 1;
    }
    
    for (; str[i] != '\0' && str[i] != '.'; i++) {
        if (str[i] >= '0' && str[i] <= '9') {
            result = result * 10.0 + (str[i] - '0');
        } else {
            break;
        }
    }
    
    if (str[i] == '.') {
        i++;
        for (; str[i] != '\0'; i++) {
            if (str[i] >= '0' && str[i] <= '9') {
                factor /= 10.0;
                result += (str[i] - '0') * factor;
            } else {
                break;
            }
        }
    }
    
    return result * sign;
}

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

/* Number to String Conversion */

char *xy_itoa(int value, char *str, int base)
{
    if (!str || base < 2 || base > 36) return NULL;
    
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return str;
    }
    
    int negative = 0;
    if (value < 0 && base == 10) {
        negative = 1;
        value = -value;
    }
    
    char *start = str;
    char *end = str;
    
    /* Generate digits in reverse order */
    while (value > 0) {
        int digit = value % base;
        *end++ = (digit < 10) ? '0' + digit : 'A' + digit - 10;
        value /= base;
    }
    
    if (negative) {
        *end++ = '-';
    }
    
    /* Reverse the string */
    *end = '\0';
    end--; /* Move back to last digit */
    
    while (start < end) {
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
    
    return str;
}

char *xy_ltoa(long value, char *str, int base)
{
    if (!str || base < 2 || base > 36) return NULL;
    
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return str;
    }
    
    int negative = 0;
    if (value < 0 && base == 10) {
        negative = 1;
        value = -value;
    }
    
    char *start = str;
    char *end = str;
    
    /* Generate digits in reverse order */
    while (value > 0) {
        long digit = value % base;
        *end++ = (digit < 10) ? '0' + digit : 'A' + digit - 10;
        value /= base;
    }
    
    if (negative) {
        *end++ = '-';
    }
    
    /* Reverse the string */
    *end = '\0';
    end--; /* Move back to last digit */
    
    while (start < end) {
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
    
    return str;
}

char *xy_ultoa(unsigned long value, char *str, int base)
{
    if (!str || base < 2 || base > 36) return NULL;
    
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return str;
    }
    
    char *start = str;
    char *end = str;
    
    /* Generate digits in reverse order */
    while (value > 0) {
        unsigned long digit = value % base;
        *end++ = (digit < 10) ? '0' + digit : 'A' + digit - 10;
        value /= base;
    }
    
    /* Reverse the string */
    *end = '\0';
    end--; /* Move back to last digit */
    
    while (start < end) {
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
    
    return str;
}

char *xy_utoa(unsigned int value, char *str, int base)
{
    if (!str || base < 2 || base > 36) return NULL;
    
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return str;
    }
    
    char *start = str;
    char *end = str;
    
    /* Generate digits in reverse order */
    while (value > 0) {
        unsigned int digit = value % base;
        *end++ = (digit < 10) ? '0' + digit : 'A' + digit - 10;
        value /= base;
    }
    
    /* Reverse the string */
    *end = '\0';
    end--; /* Move back to last digit */
    
    while (start < end) {
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
    
    return str;
}

/* Sorting and Searching */

void xy_qsort(void *base, size_t num, size_t size,
              int (*compar)(const void *, const void *))
{
    if (!base || !compar || num <= 1) return;
    
    char *arr = (char *)base;
    for (size_t i = 0; i < num - 1; i++) {
        for (size_t j = 0; j < num - i - 1; j++) {
            char *a = arr + j * size;
            char *b = arr + (j + 1) * size;
            
            if (compar(a, b) > 0) {
                /* Swap elements */
                for (size_t k = 0; k < size; k++) {
                    char temp = a[k];
                    a[k] = b[k];
                    b[k] = temp;
                }
            }
        }
    }
}

void *xy_bsearch(const void *key, const void *base, size_t num, size_t size,
                 int (*compar)(const void *, const void *))
{
    if (!key || !base || !compar || num == 0) return NULL;
    
    const char *arr = (const char *)base;
    size_t left = 0, right = num - 1;
    
    while (left <= right) {
        size_t mid = left + (right - left) / 2;
        const char *mid_elem = arr + mid * size;
        int cmp = compar(key, mid_elem);
        
        if (cmp == 0) {
            return (void *)mid_elem;
        } else if (cmp < 0) {
            if (mid == 0) break; /* Prevent underflow */
            right = mid - 1;
        } else {
            left = mid + 1;
        }
    }
    
    return NULL;
}

/* Math Functions */

int xy_abs(int x)
{
    return (x < 0) ? -x : x;
}

long xy_labs(long x)
{
    return (x < 0) ? -x : x;
}

long long xy_llabs(long long x)
{
    return (x < 0) ? -x : x;
}

xy_div_t xy_div(int numer, int denom)
{
    xy_div_t result;
    result.quot = numer / denom;
    result.rem = numer % denom;
    return result;
}

xy_ldiv_t xy_ldiv(long numer, long denom)
{
    xy_ldiv_t result;
    result.quot = numer / denom;
    result.rem = numer % denom;
    return result;
}

xy_lldiv_t xy_lldiv(long long numer, long long denom)
{
    xy_lldiv_t result;
    result.quot = numer / denom;
    result.rem = numer % denom;
    return result;
}

/* Random Number Generation */

static uint32_t g_rand_state = 1;



void xy_srand(unsigned int seed)
{
    g_rand_state = seed ? seed : 1;
}



/* Utility Functions */

int xy_pow(int base, unsigned int exp)
{
    if (exp == 0) return 1;
    
    int result = 1;
    while (exp > 0) {
        if (exp & 1) {
            result *= base;
        }
        base *= base;
        exp >>= 1;
    }
    return result;
}

int xy_sqrt(int x)
{
    if (x <= 1) return x;
    
    int low = 0, high = x, result = 0;
    
    while (low <= high) {
        int mid = low + (high - low) / 2;
        long long sq = (long long)mid * mid;
        
        if (sq == x) {
            return mid;
        } else if (sq < x) {
            low = mid + 1;
            result = mid;
        } else {
            high = mid - 1;
        }
    }
    
    return result;
}

int xy_cbrt(int x)
{
    if (x == 0) return 0;
    
    int sign = 1;
    if (x < 0) {
        sign = -1;
        x = -x;
    }
    
    int low = 0, high = x;
    
    while (low <= high) {
        int mid = low + (high - low) / 2;
        long long cube = (long long)mid * mid * mid;
        
        if (cube == x) {
            return mid * sign;
        } else if (cube < x) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    
    return (high * sign);
}



int xy_map(int x, int in_min, int in_max, int out_min, int out_max)
{
    if (in_min == in_max) return out_min;
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int xy_constrain(int x, int min, int max)
{
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

int xy_gcd(int a, int b)
{
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

int xy_lcm(int a, int b)
{
    if (a == 0 || b == 0) return 0;
    return (a / xy_gcd(a, b)) * b;
}

int xy_factorial(int n)
{
    if (n < 0) return 0;
    if (n <= 1) return 1;
    
    int result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

/* Fixed-point Math */

int32_t xy_fixed_mul(int32_t a, int32_t b)
{
    // For Q16.16, multiply and shift right by 16
    return ((int64_t)a * b) >> 16;
}

int32_t xy_fixed_div(int32_t a, int32_t b)
{
    // For Q16.16, shift left by 16 and divide
    if (b == 0) return 0; // Avoid division by zero
    return ((int64_t)a << 16) / b;
}

/* Trigonometric Functions (Fixed-point) */

static const int32_t g_sin_table[91] = {
    0, 174, 348, 523, 697, 871, 1045, 1218, 1391, 1564, 
    1736, 1908, 2079, 2249, 2419, 2588, 2756, 2923, 3090, 3255, 
    3420, 3583, 3746, 3907, 4067, 4226, 4383, 4539, 4694, 4848, 
    5000, 5150, 5299, 5446, 5591, 5735, 5877, 6018, 6156, 6293, 
    6427, 6560, 6691, 6819, 6946, 7071, 7193, 7313, 7431, 7547, 
    7660, 7771, 7880, 7986, 8090, 8191, 8290, 8386, 8480, 8571, 
    8660, 8746, 8829, 8910, 8987, 9063, 9135, 9205, 9271, 9335, 
    9396, 9455, 9510, 9563, 9612, 9659, 9702, 9743, 9781, 9816, 
    9848, 9876, 9902, 9925, 9945, 9961, 9975, 9986, 9993, 9998, 
    10000
};

int32_t xy_sin_fixed(int32_t angle_degrees_x100)
{
    // Normalize angle to 0-36000 range
    int32_t normalized = angle_degrees_x100 % 36000;
    if (normalized < 0) normalized += 36000;
    
    // Determine quadrant
    int32_t degrees = normalized / 100;
    int32_t quadrant = degrees / 90;
    int32_t angle_in_quadrant = degrees % 90;
    
    // Adjust angle_in_quadrant to be within 0-90 range
    if (angle_in_quadrant < 0) angle_in_quadrant = -angle_in_quadrant;
    if (angle_in_quadrant > 90) angle_in_quadrant = 180 - angle_in_quadrant;
    
    int32_t result;
    if (angle_in_quadrant > 90) {
        angle_in_quadrant = 90;
    }
    
    if (quadrant == 0) {
        // 0-89 degrees
        result = g_sin_table[angle_in_quadrant];
    } else if (quadrant == 1) {
        // 90-179 degrees: sin(180-x) = sin(x)
        result = g_sin_table[90 - angle_in_quadrant];
    } else if (quadrant == 2) {
        // 180-269 degrees: sin(180+x) = -sin(x)
        result = -g_sin_table[angle_in_quadrant];
    } else {
        // 270-359 degrees: sin(360-x) = -sin(x)
        result = -g_sin_table[90 - angle_in_quadrant];
    }
    
    return result;
}

int32_t xy_cos_fixed(int32_t angle_degrees_x100)
{
    // cos(x) = sin(90 - x)
    return xy_sin_fixed((9000 - angle_degrees_x100) % 36000);
}

int32_t xy_tan_fixed(int32_t angle_degrees_x100)
{
    int32_t sin_val = xy_sin_fixed(angle_degrees_x100);
    int32_t cos_val = xy_cos_fixed(angle_degrees_x100);
    
    if (cos_val == 0) {
        // Return large value for undefined tangent
        return (sin_val >= 0) ? 1000000 : -1000000;
    }
    
    // Perform fixed-point division: (sin * 10000) / cos
    return (sin_val * 10000) / cos_val;
}

/* Bit Manipulation */



uint32_t xy_round_up_pow2(uint32_t value)
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

int xy_is_pow2(uint32_t value)
{
    return (value != 0) && ((value & (value - 1)) == 0);
}

/* Floating Point Approximation */

float xy_fast_inv_sqrt(float x)
{
    // Quake's fast inverse square root algorithm
    union { float f; uint32_t i; } u;
    u.f = x;
    u.i = 0x5F3759DF - (u.i >> 1);
    return u.f * (1.5f - 0.5f * x * u.f * u.f);
}

float xy_fast_sin(float x)
{
    // Fast sine approximation using Taylor series
    // Reduce to [-π, π] range
    while (x > 3.14159265f) x -= 6.28318531f;
    while (x < -3.14159265f) x += 6.28318531f;
    
    // For small angles, use x - x^3/6
    float x2 = x * x;
    return x * (1.0f - x2 * (1.0f/6.0f - x2 * (1.0f/120.0f)));
}

float xy_fast_cos(float x)
{
    // cos(x) = sin(x + π/2)
    return xy_fast_sin(x + 1.57079632f);
}

/* Environment Functions */

void xy_abort(void)
{
    while (1) {
        // Halt execution
    }
}

void xy_exit(int status)
{
    XY_UNUSED(status);
    xy_abort();
}

char *xy_getenv(const char *name)
{
    XY_UNUSED(name);
    // In embedded systems, environment variables may not be available
    return NULL;
}

int xy_system(const char *command)
{
    XY_UNUSED(command);
    // In embedded systems, system commands may not be available
    return -1;
}

/* String Conversion */

char *xy_itoa_append(int value, char *str)
{
    if (!str) return NULL;
    
    // Find end of string
    char *end = str;
    while (*end != '\0') end++;
    
    // Convert value to string
    char temp[32];
    xy_itoa(value, temp, 10);
    
    // Append to existing string
    int i = 0;
    while (temp[i] != '\0') {
        *end++ = temp[i++];
    }
    *end = '\0';
    
    return str;
}
