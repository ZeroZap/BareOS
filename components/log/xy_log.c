#include "xy_log.h"
#include "xy_stdio.h"

extern void xy_log_char(char ch);

#ifdef RELEASE
uint8_t g_xy_log_dinamic_level = XY_LOG_LEVEL_WARN;
#else
uint8_t g_xy_log_dinamic_level = XY_LOG_LEVEL_DEBUG;
#endif

void xy_log_str(char *str)
{
    if (str) {
        while (*str != 0x00) {
            xy_log_char(*str);
            str++;
        }
    }
}


void xy_log_raw(char *data, size_t len)
{
    char *ch = data;
    while (len--) {
        xy_log_char(*ch);
        ch++;
    }
}

void xy_log_init(void)
{
    xy_stdio_printf_init(xy_log_str);
}

void xy_log_set_dynamic_level(uint8_t level)
{
#ifdef RELEASE
    if (level <= XY_LOG_LEVEL_WARN) {
        g_xy_log_dinamic_level = level;
    }
#else
    if (level <= XY_LOG_LEVEL_DEBUG) {
        g_xy_log_dinamic_level = level;
    }
#endif
}

uint8_t xy_log_dynamic_level(void)
{
    return g_xy_log_dinamic_level;
}
