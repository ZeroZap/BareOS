#include "xy_pm.h"

typedef struct {
    xy_pm_sleep_check_fn fn;
    void *arg;
} xy_pm_sleep_check_t;

typedef struct {
    xy_pm_timeout_fn fn;
    void *arg;
} xy_pm_timeout_t;

typedef struct {
    xy_pm_wake_hook_fn fn;
    void *arg;
} xy_pm_wake_hook_t;

static xy_pm_config_t s_pm;
static xy_pm_sleep_check_t s_checks[XY_PM_MAX_SLEEP_CHECKS];
static xy_pm_timeout_t s_timeouts[XY_PM_MAX_TIMEOUTS];
static xy_pm_wake_hook_t s_wake_hooks[XY_PM_MAX_WAKE_HOOKS];
static uint8_t s_check_count;
static uint8_t s_timeout_count;
static uint8_t s_wake_hook_count;
static bool s_initialized;
static xy_pm_stats_t s_stats;

static bool pm_checks_allow_sleep(void)
{
    uint8_t i;

    for (i = 0u; i < s_check_count; i++) {
        if (!s_checks[i].fn(s_checks[i].arg)) {
            return false;
        }
    }
    return true;
}

static int pm_abort(void)
{
    s_stats.abort_count++;
    return 0;
}

void xy_pm_init(const xy_pm_config_t *config)
{
    s_pm.lptimer = NULL;
    s_pm.deepest_mode = XY_HAL_POWER_SLEEP;
    s_pm.wake_sources = XY_HAL_WAKE_LPTIMER | XY_HAL_WAKE_GPIO | XY_HAL_WAKE_UART;
    s_pm.min_sleep_ms = 2u;
    s_pm.max_sleep_ms = 60000u;

    if (config) {
        s_pm = *config;
    }

    s_check_count = 0u;
    s_timeout_count = 0u;
    s_wake_hook_count = 0u;
    s_initialized = true;
    xy_pm_reset_stats();
}

int xy_pm_register_sleep_check(xy_pm_sleep_check_fn fn, void *arg)
{
    if (!fn || s_check_count >= XY_PM_MAX_SLEEP_CHECKS) {
        return XY_HAL_INVALID_PARAM;
    }
    s_checks[s_check_count].fn = fn;
    s_checks[s_check_count].arg = arg;
    s_check_count++;
    return XY_HAL_OK;
}

int xy_pm_register_timeout(xy_pm_timeout_fn fn, void *arg)
{
    if (!fn || s_timeout_count >= XY_PM_MAX_TIMEOUTS) {
        return XY_HAL_INVALID_PARAM;
    }
    s_timeouts[s_timeout_count].fn = fn;
    s_timeouts[s_timeout_count].arg = arg;
    s_timeout_count++;
    return XY_HAL_OK;
}

int xy_pm_register_wake_hook(xy_pm_wake_hook_fn fn, void *arg)
{
    if (!fn || s_wake_hook_count >= XY_PM_MAX_WAKE_HOOKS) {
        return XY_HAL_INVALID_PARAM;
    }
    s_wake_hooks[s_wake_hook_count].fn = fn;
    s_wake_hooks[s_wake_hook_count].arg = arg;
    s_wake_hook_count++;
    return XY_HAL_OK;
}

int xy_pm_unregister_sleep_check(xy_pm_sleep_check_fn fn, void *arg)
{
    uint8_t i;

    for (i = 0u; i < s_check_count; i++) {
        if (s_checks[i].fn == fn && s_checks[i].arg == arg) {
            for (; i + 1u < s_check_count; i++) {
                s_checks[i] = s_checks[i + 1u];
            }
            s_check_count--;
            return XY_HAL_OK;
        }
    }
    return XY_HAL_INVALID_PARAM;
}

int xy_pm_unregister_timeout(xy_pm_timeout_fn fn, void *arg)
{
    uint8_t i;

    for (i = 0u; i < s_timeout_count; i++) {
        if (s_timeouts[i].fn == fn && s_timeouts[i].arg == arg) {
            for (; i + 1u < s_timeout_count; i++) {
                s_timeouts[i] = s_timeouts[i + 1u];
            }
            s_timeout_count--;
            return XY_HAL_OK;
        }
    }
    return XY_HAL_INVALID_PARAM;
}

int xy_pm_unregister_wake_hook(xy_pm_wake_hook_fn fn, void *arg)
{
    uint8_t i;

    for (i = 0u; i < s_wake_hook_count; i++) {
        if (s_wake_hooks[i].fn == fn && s_wake_hooks[i].arg == arg) {
            for (; i + 1u < s_wake_hook_count; i++) {
                s_wake_hooks[i] = s_wake_hooks[i + 1u];
            }
            s_wake_hook_count--;
            return XY_HAL_OK;
        }
    }
    return XY_HAL_INVALID_PARAM;
}

int xy_pm_acquire_lock(xy_hal_pm_lock_t lock)
{
    return xy_hal_power_acquire_lock(lock);
}

int xy_pm_release_lock(xy_hal_pm_lock_t lock)
{
    return xy_hal_power_release_lock(lock);
}

uint16_t xy_pm_get_lock_count(xy_hal_pm_lock_t lock)
{
    return xy_hal_power_get_lock_count(lock);
}

bool xy_pm_can_sleep(void)
{
    if (xy_hal_power_get_allowed_mode() < s_pm.deepest_mode) {
        return false;
    }
    return pm_checks_allow_sleep();
}

xy_tick_t xy_pm_next_timeout_ticks(void)
{
    uint8_t i;
    xy_tick_t next = XY_PM_TIMEOUT_FOREVER;

    for (i = 0u; i < s_timeout_count; i++) {
        xy_tick_t timeout = s_timeouts[i].fn(s_timeouts[i].arg);
        if (timeout < next) {
            next = timeout;
        }
    }

    return next;
}

int xy_pm_tickless_idle(void)
{
    uint8_t i;
    uint32_t key;
    uint32_t start_ms;
    uint32_t elapsed_ms;
    uint32_t sleep_ms;
    xy_tick_t sleep_ticks;
    xy_tick_t elapsed_ticks;

    s_stats.idle_calls++;
    s_stats.last_planned_ms = 0u;
    s_stats.last_elapsed_ms = 0u;

    if (!xy_pm_is_tickless_available()) {
        return pm_abort();
    }

    key = xy_hal_irq_save();
    if (xy_hal_power_get_allowed_mode() < s_pm.deepest_mode) {
        if (xy_hal_power_get_allowed_mode() >= XY_HAL_POWER_SLEEP &&
            pm_checks_allow_sleep() &&
            xy_hal_power_enter(XY_HAL_POWER_SLEEP, s_pm.wake_sources, 0u) == XY_HAL_OK) {
            s_stats.shallow_sleep_count++;
            xy_hal_irq_restore(key);
            return 1;
        }
        xy_hal_irq_restore(key);
        return pm_abort();
    }
    xy_hal_irq_restore(key);

    sleep_ticks = xy_pm_next_timeout_ticks();
    if (sleep_ticks == 0u) {
        return pm_abort();
    }

    if (sleep_ticks == XY_PM_TIMEOUT_FOREVER) {
        sleep_ms = s_pm.max_sleep_ms;
    } else {
        sleep_ms = xy_tick_to_ms(sleep_ticks);
        if (sleep_ms > s_pm.max_sleep_ms) {
            sleep_ms = s_pm.max_sleep_ms;
        }
    }

    if (sleep_ms < s_pm.min_sleep_ms) {
        return pm_abort();
    }
    s_stats.last_planned_ms = sleep_ms;

    key = xy_hal_irq_save();
    if (!xy_pm_can_sleep()) {
        xy_hal_irq_restore(key);
        return pm_abort();
    }

    if (xy_hal_lptimer_start(s_pm.lptimer, sleep_ms, NULL, NULL) != XY_HAL_OK) {
        xy_hal_irq_restore(key);
        return pm_abort();
    }
    start_ms = xy_hal_lptimer_now_ms(s_pm.lptimer);

    if (xy_hal_power_enter(s_pm.deepest_mode, s_pm.wake_sources, sleep_ms) != XY_HAL_OK) {
        xy_hal_lptimer_stop(s_pm.lptimer);
        xy_hal_irq_restore(key);
        return pm_abort();
    }

    elapsed_ms = xy_hal_lptimer_now_ms(s_pm.lptimer) - start_ms;
    xy_hal_lptimer_stop(s_pm.lptimer);

    elapsed_ticks = xy_tick_from_ms(elapsed_ms);
    xy_tick_advance(elapsed_ticks);
    xy_hal_irq_restore(key);

    s_stats.last_elapsed_ms = elapsed_ms;
    s_stats.sleep_count++;

    for (i = 0u; i < s_wake_hook_count; i++) {
        s_wake_hooks[i].fn(elapsed_ticks, s_wake_hooks[i].arg);
    }

    return 1;
}

bool xy_pm_is_initialized(void)
{
    return s_initialized;
}

bool xy_pm_is_tickless_available(void)
{
    return s_initialized && s_pm.lptimer != NULL &&
           s_pm.max_sleep_ms >= s_pm.min_sleep_ms && s_pm.max_sleep_ms != 0u;
}

void xy_pm_get_stats(xy_pm_stats_t *stats)
{
    if (stats) {
        *stats = s_stats;
    }
}

void xy_pm_reset_stats(void)
{
    s_stats.idle_calls = 0u;
    s_stats.sleep_count = 0u;
    s_stats.shallow_sleep_count = 0u;
    s_stats.abort_count = 0u;
    s_stats.last_planned_ms = 0u;
    s_stats.last_elapsed_ms = 0u;
}
