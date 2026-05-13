#include "xy_sys.h"

uint32_t _sys_tick = 0;

extern inline void xy_sys_tick_init(void)
{
    _sys_tick = 0;
}
extern inline void xy_sys_tick_inrease(void)
{
    _sys_tick += XY_SYS_TICK_INCREASE_STEP;
}

extern inline uint32_t xy_sys_tick(void)
{
    return _sys_tick;
}