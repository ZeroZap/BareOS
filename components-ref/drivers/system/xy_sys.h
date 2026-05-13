#ifndef _XY_SYS_H_
#define _XY_SYS_H_
#include "xy_typedef.h"

#define XY_SYS_TICK_INCREASE_STEP 1

inline void xy_sys_tick_init(void);
inline void xy_sys_tick_inrease(void);
inline uint32_t xy_sys_tick(void);
uint32_t xy_sys_reset_flag(void);
void xy_sys_power_off(void);
void xy_sys_power_on(void);
void xy_sys_power_reset(void);
void xy_sys_power_sleep(uint8_t level);

#endif