#ifndef XY_PM_H
#define XY_PM_H

#include <stdint.h>
#include "xy_hal.h"
#include "xy_tick.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XY_PM_MAX_SLEEP_CHECKS
#define XY_PM_MAX_SLEEP_CHECKS 8u
#endif

#ifndef XY_PM_MAX_TIMEOUTS
#define XY_PM_MAX_TIMEOUTS 8u
#endif

#ifndef XY_PM_MAX_WAKE_HOOKS
#define XY_PM_MAX_WAKE_HOOKS 8u
#endif

#define XY_PM_TIMEOUT_FOREVER ((xy_tick_t)0xFFFFFFFFu)

typedef bool (*xy_pm_sleep_check_fn)(void *arg);
typedef xy_tick_t (*xy_pm_timeout_fn)(void *arg);
typedef void (*xy_pm_wake_hook_fn)(xy_tick_t elapsed_ticks, void *arg);

typedef struct {
    void *lptimer;
    xy_hal_power_mode_t deepest_mode;
    uint32_t wake_sources;
    uint32_t min_sleep_ms;
    uint32_t max_sleep_ms;
} xy_pm_config_t;

typedef struct {
    uint32_t idle_calls;
    uint32_t sleep_count;
    uint32_t abort_count;
    uint32_t last_planned_ms;
    uint32_t last_elapsed_ms;
} xy_pm_stats_t;

void xy_pm_init(const xy_pm_config_t *config);

int xy_pm_register_sleep_check(xy_pm_sleep_check_fn fn, void *arg);
int xy_pm_register_timeout(xy_pm_timeout_fn fn, void *arg);
int xy_pm_register_wake_hook(xy_pm_wake_hook_fn fn, void *arg);
int xy_pm_unregister_sleep_check(xy_pm_sleep_check_fn fn, void *arg);
int xy_pm_unregister_timeout(xy_pm_timeout_fn fn, void *arg);
int xy_pm_unregister_wake_hook(xy_pm_wake_hook_fn fn, void *arg);

int xy_pm_acquire_lock(xy_hal_pm_lock_t lock);
int xy_pm_release_lock(xy_hal_pm_lock_t lock);
/* Returns the current reference count, or zero when lock is invalid. */
uint16_t xy_pm_get_lock_count(xy_hal_pm_lock_t lock);

bool xy_pm_can_sleep(void);
xy_tick_t xy_pm_next_timeout_ticks(void);
int xy_pm_tickless_idle(void);

bool xy_pm_is_initialized(void);
bool xy_pm_is_tickless_available(void);
void xy_pm_get_stats(xy_pm_stats_t *stats);
void xy_pm_reset_stats(void);

#ifdef __cplusplus
}
#endif

#endif
