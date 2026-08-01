#ifndef XY_TICK_H
#define XY_TICK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t xy_tick_t;

/* Logical BareOS tick rate. BSPs may override this at compile time. */
#ifndef XY_TICK_HZ
#define XY_TICK_HZ 1000u
#endif

extern volatile xy_tick_t g_xy_tick;

void xy_tick_init(void);
xy_tick_t xy_tick_now(void);
void xy_tick_set(xy_tick_t tick);
void xy_tick_advance(xy_tick_t ticks);

xy_tick_t xy_tick_from_ms(uint32_t ms);
uint32_t xy_tick_to_ms(xy_tick_t ticks);
uint32_t xy_tick_now_ms(void);
uint32_t xy_tick_now_s(void);

#ifdef __cplusplus
}
#endif

#endif
