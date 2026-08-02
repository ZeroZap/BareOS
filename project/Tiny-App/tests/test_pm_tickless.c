#include <stdio.h>

#include "xy_pm.h"
#include "xy_tick.h"

static uint32_t s_now_ms;
static uint32_t s_wake_after_ms;
static uint32_t s_started_ms;
static uint32_t s_irq_state;
static xy_hal_power_mode_t s_allowed_mode;
static bool s_sleep_allowed;
static xy_tick_t s_timeout_ticks;
static xy_tick_t s_hook_ticks;
static uint32_t s_hook_calls;
static uint16_t s_lock_count[XY_HAL_PM_LOCK_COUNT];
static xy_hal_power_mode_t s_entered_mode;
static uint32_t s_power_enter_calls;
static uint32_t s_lptimer_stop_calls;
static bool s_reject_during_lptimer_start;
static bool s_shorten_timeout_during_lptimer_start;
static uint32_t s_reported_wake_sources;

static int check(int condition, const char *name)
{
    if (!condition) {
        printf("FAIL: %s\n", name);
        return 1;
    }
    return 0;
}

static bool sleep_check(void *arg)
{
    (void)arg;
    return s_sleep_allowed;
}

static xy_tick_t next_timeout(void *arg)
{
    (void)arg;
    return s_timeout_ticks;
}

static void wake_hook(xy_tick_t elapsed_ticks, void *arg)
{
    (void)arg;
    s_hook_ticks = elapsed_ticks;
    s_hook_calls++;
}

uint32_t xy_hal_irq_save(void)
{
    uint32_t previous = s_irq_state;
    s_irq_state = 1u;
    return previous;
}

void xy_hal_irq_restore(uint32_t key)
{
    s_irq_state = key;
}

int xy_hal_lptimer_start(void *timer, uint32_t timeout_ms,
                         xy_hal_timer_cb_t cb, void *user)
{
    (void)timer;
    (void)cb;
    (void)user;
    s_started_ms = timeout_ms;
    s_now_ms = 0u;
    if (s_reject_during_lptimer_start) {
        s_sleep_allowed = false;
    }
    if (s_shorten_timeout_during_lptimer_start) {
        s_timeout_ticks = xy_tick_from_ms(20u);
    }
    return XY_HAL_OK;
}

int xy_hal_lptimer_stop(void *timer)
{
    (void)timer;
    s_lptimer_stop_calls++;
    return XY_HAL_OK;
}

uint32_t xy_hal_lptimer_now_ms(void *timer)
{
    (void)timer;
    return s_now_ms;
}

int xy_hal_power_enter(xy_hal_power_mode_t mode, uint32_t wake_sources,
                       uint32_t timeout_ms)
{
    (void)wake_sources;
    (void)timeout_ms;
    s_entered_mode = mode;
    s_power_enter_calls++;
    s_now_ms += s_wake_after_ms;
    xy_pm_report_wake_sources(s_reported_wake_sources);
    return XY_HAL_OK;
}

int xy_hal_power_acquire_lock(xy_hal_pm_lock_t lock)
{
    if ((uint32_t)lock >= (uint32_t)XY_HAL_PM_LOCK_COUNT ||
        s_lock_count[lock] == UINT16_MAX) {
        return XY_HAL_INVALID_PARAM;
    }
    s_lock_count[lock]++;
    return XY_HAL_OK;
}

int xy_hal_power_release_lock(xy_hal_pm_lock_t lock)
{
    if ((uint32_t)lock >= (uint32_t)XY_HAL_PM_LOCK_COUNT ||
        s_lock_count[lock] == 0u) {
        return XY_HAL_INVALID_PARAM;
    }
    s_lock_count[lock]--;
    return XY_HAL_OK;
}

uint16_t xy_hal_power_get_lock_count(xy_hal_pm_lock_t lock)
{
    if ((uint32_t)lock >= (uint32_t)XY_HAL_PM_LOCK_COUNT) {
        return 0u;
    }
    return s_lock_count[lock];
}

xy_hal_power_mode_t xy_hal_power_get_allowed_mode(void)
{
    if (s_lock_count[XY_HAL_PM_LOCK_CPU] || s_lock_count[XY_HAL_PM_LOCK_SLEEP]) {
        return XY_HAL_POWER_RUN;
    }
    if (s_lock_count[XY_HAL_PM_LOCK_STOP]) {
        return XY_HAL_POWER_SLEEP;
    }
    if (s_lock_count[XY_HAL_PM_LOCK_STANDBY]) {
        return XY_HAL_POWER_STOP;
    }
    return s_allowed_mode;
}

static void reset_fixture(void)
{
    static int timer_handle;
    xy_pm_config_t config;

    config.lptimer = &timer_handle;
    config.deepest_mode = XY_HAL_POWER_STOP;
    config.wake_sources = XY_HAL_WAKE_LPTIMER | XY_HAL_WAKE_UART;
    config.min_sleep_ms = 5u;
    config.max_sleep_ms = 1000u;

    xy_tick_init();
    xy_pm_init(&config);
    xy_pm_register_sleep_check(sleep_check, NULL);
    xy_pm_register_timeout(next_timeout, NULL);
    xy_pm_register_wake_hook(wake_hook, NULL);

    s_now_ms = 0u;
    s_wake_after_ms = 0u;
    s_started_ms = 0u;
    s_irq_state = 0u;
    s_allowed_mode = XY_HAL_POWER_STANDBY;
    s_sleep_allowed = true;
    s_timeout_ticks = XY_PM_TIMEOUT_FOREVER;
    s_hook_ticks = 0u;
    s_hook_calls = 0u;
    s_entered_mode = XY_HAL_POWER_RUN;
    s_power_enter_calls = 0u;
    s_lptimer_stop_calls = 0u;
    s_reject_during_lptimer_start = false;
    s_shorten_timeout_during_lptimer_start = false;
    s_reported_wake_sources = 0u;
    for (uint32_t i = 0u; i < (uint32_t)XY_HAL_PM_LOCK_COUNT; i++) {
        s_lock_count[i] = 0u;
    }
}

static int test_full_sleep(void)
{
    xy_pm_stats_t stats;
    int failed = 0;

    reset_fixture();
    s_timeout_ticks = xy_tick_from_ms(500u);
    s_wake_after_ms = 500u;

    failed += check(xy_pm_tickless_idle() == 1, "full sleep succeeds");
    failed += check(s_started_ms == 500u, "full sleep deadline programmed");
    failed += check(xy_tick_now_ms() == 500u, "full sleep tick compensation");
    failed += check(s_hook_calls == 1u && s_hook_ticks == 500u, "full sleep wake hook");
    failed += check(s_irq_state == 0u, "full sleep irq restored");

    xy_pm_get_stats(&stats);
    failed += check(stats.idle_calls == 1u && stats.sleep_count == 1u &&
                    stats.abort_count == 0u, "full sleep stats counts");
    failed += check(stats.last_planned_ms == 500u && stats.last_elapsed_ms == 500u,
                    "full sleep stats durations");
    failed += check(stats.total_deep_sleep_ms == 500u &&
                    stats.max_deep_sleep_ms == 500u && stats.early_wake_count == 0u,
                    "full sleep residency stats");
    failed += check(stats.lptimer_wake_count == 1u &&
                    stats.last_wake_sources == XY_HAL_WAKE_LPTIMER,
                    "full sleep classified as LPTIM wake");
    return failed;
}

static int test_early_wakeup(void)
{
    xy_pm_stats_t stats;
    int failed = 0;

    reset_fixture();
    s_timeout_ticks = xy_tick_from_ms(800u);
    s_wake_after_ms = 300u;

    failed += check(xy_pm_tickless_idle() == 1, "early wake succeeds");
    failed += check(s_started_ms == 800u, "early wake deadline programmed");
    failed += check(xy_tick_now_ms() == 300u, "early wake compensates actual time");
    xy_pm_get_stats(&stats);
    failed += check(stats.last_planned_ms == 800u && stats.last_elapsed_ms == 300u,
                    "early wake stats durations");
    failed += check(stats.total_deep_sleep_ms == 300u &&
                    stats.max_deep_sleep_ms == 300u && stats.early_wake_count == 1u,
                    "early wake residency stats");
    return failed;
}

static int test_shallow_without_lptimer(void)
{
    xy_pm_config_t config;
    xy_pm_stats_t stats;
    int failed = 0;

    reset_fixture();
    config.lptimer = NULL;
    config.deepest_mode = XY_HAL_POWER_SLEEP;
    config.wake_sources = XY_HAL_WAKE_UART;
    config.min_sleep_ms = 0u;
    config.max_sleep_ms = 0u;
    xy_pm_init(&config);
    xy_pm_register_sleep_check(sleep_check, NULL);

    failed += check(!xy_pm_is_tickless_available(),
                    "shallow-only config has no tickless timer");
    failed += check(xy_pm_tickless_idle() == 1,
                    "shallow sleep works without LPTIM");
    failed += check(s_entered_mode == XY_HAL_POWER_SLEEP &&
                    s_power_enter_calls == 1u && s_started_ms == 0u,
                    "shallow-only config enters SLEEP directly");
    xy_pm_get_stats(&stats);
    failed += check(stats.shallow_sleep_count == 1u && stats.sleep_count == 0u &&
                    stats.abort_count == 0u,
                    "shallow-only stats");
    return failed;
}

static int test_wake_source_stats(void)
{
    xy_pm_stats_t stats;
    int failed = 0;

    reset_fixture();
    xy_pm_report_wake_sources(XY_HAL_WAKE_UART);
    s_timeout_ticks = xy_tick_from_ms(500u);
    s_wake_after_ms = 200u;
    s_reported_wake_sources = XY_HAL_WAKE_UART | XY_HAL_WAKE_GPIO |
                              XY_HAL_WAKE_RTC | XY_HAL_WAKE_WDG;

    failed += check(xy_pm_tickless_idle() == 1, "multi-source wake succeeds");
    xy_pm_get_stats(&stats);
    failed += check(stats.uart_wake_count == 1u && stats.gpio_wake_count == 1u &&
                    stats.lptimer_wake_count == 0u && stats.rtc_wake_count == 1u &&
                    stats.wdg_wake_count == 1u && stats.unknown_wake_count == 0u,
                    "reported wake sources counted independently");
    failed += check(stats.last_wake_sources ==
                    (XY_HAL_WAKE_UART | XY_HAL_WAKE_GPIO |
                     XY_HAL_WAKE_RTC | XY_HAL_WAKE_WDG),
                    "last wake source mask retained");

    s_wake_after_ms = 500u;
    s_reported_wake_sources = 0u;
    failed += check(xy_pm_tickless_idle() == 1, "LPTIM wake succeeds");
    xy_pm_get_stats(&stats);
    failed += check(stats.uart_wake_count == 1u && stats.gpio_wake_count == 1u &&
                    stats.lptimer_wake_count == 1u &&
                    stats.last_wake_sources == XY_HAL_WAKE_LPTIMER,
                    "LPTIM wake counted without changing prior counts");

    xy_pm_reset_stats();
    xy_pm_get_stats(&stats);
    failed += check(stats.uart_wake_count == 0u && stats.gpio_wake_count == 0u &&
                    stats.lptimer_wake_count == 0u && stats.rtc_wake_count == 0u &&
                    stats.wdg_wake_count == 0u && stats.unknown_wake_count == 0u &&
                    stats.last_wake_sources == 0u,
                    "wake source stats reset");
    return failed;
}

static int test_sleep_rejected(void)
{
    xy_pm_stats_t stats;
    int failed = 0;

    reset_fixture();
    s_timeout_ticks = xy_tick_from_ms(500u);
    s_sleep_allowed = false;

    failed += check(xy_pm_tickless_idle() == 0, "pending work rejects sleep");
    failed += check(s_started_ms == 0u && xy_tick_now() == 0u,
                    "rejected sleep does not program or advance");
    xy_pm_get_stats(&stats);
    failed += check(stats.abort_count == 1u && stats.sleep_count == 0u,
                    "rejected sleep stats");

    reset_fixture();
    s_timeout_ticks = xy_tick_from_ms(2u);
    failed += check(xy_pm_tickless_idle() == 0, "short residency rejects sleep");
    return failed;
}

static int test_unregister(void)
{
    int failed = 0;

    reset_fixture();
    failed += check(xy_pm_unregister_sleep_check(sleep_check, NULL) == XY_HAL_OK,
                    "unregister sleep check");
    failed += check(xy_pm_unregister_timeout(next_timeout, NULL) == XY_HAL_OK,
                    "unregister timeout");
    failed += check(xy_pm_unregister_wake_hook(wake_hook, NULL) == XY_HAL_OK,
                    "unregister wake hook");
    failed += check(xy_pm_next_timeout_ticks() == XY_PM_TIMEOUT_FOREVER,
                    "unregistered timeout absent");
    failed += check(xy_pm_unregister_timeout(next_timeout, NULL) == XY_HAL_INVALID_PARAM,
                    "duplicate unregister rejected");
    return failed;
}

static int test_work_arrives_during_lptimer_start(void)
{
    xy_pm_stats_t stats;
    int failed = 0;

    reset_fixture();
    s_timeout_ticks = xy_tick_from_ms(500u);
    s_reject_during_lptimer_start = true;

    failed += check(xy_pm_tickless_idle() == 0,
                    "work arriving during LPTIM setup rejects STOP");
    failed += check(s_started_ms == 500u && s_lptimer_stop_calls == 1u,
                    "rejected STOP cancels programmed LPTIM");
    failed += check(s_power_enter_calls == 0u && xy_tick_now() == 0u,
                    "rejected STOP does not enter power mode or advance tick");
    failed += check(s_irq_state == 0u, "setup race restores irq state");
    xy_pm_get_stats(&stats);
    failed += check(stats.abort_count == 1u && stats.sleep_count == 0u,
                    "setup race stats");
    return failed;
}

static int test_earlier_deadline_during_lptimer_start(void)
{
    int failed = 0;

    reset_fixture();
    s_timeout_ticks = xy_tick_from_ms(500u);
    s_shorten_timeout_during_lptimer_start = true;

    failed += check(xy_pm_tickless_idle() == 0,
                    "earlier deadline during LPTIM setup rejects STOP");
    failed += check(s_started_ms == 500u && s_lptimer_stop_calls == 1u,
                    "earlier deadline cancels programmed LPTIM");
    failed += check(s_power_enter_calls == 0u && xy_tick_now() == 0u,
                    "earlier deadline does not oversleep");
    return failed;
}

static int test_stop_lock(void)
{
    xy_pm_stats_t stats;
    int failed = 0;

    reset_fixture();
    s_timeout_ticks = xy_tick_from_ms(500u);
    s_wake_after_ms = 500u;

    failed += check(xy_pm_acquire_lock(XY_HAL_PM_LOCK_STOP) == XY_HAL_OK,
                    "STOP lock acquired");
    failed += check(xy_pm_acquire_lock(XY_HAL_PM_LOCK_STOP) == XY_HAL_OK,
                    "STOP lock reference acquired");
    failed += check(xy_pm_get_lock_count(XY_HAL_PM_LOCK_STOP) == 2u,
                    "STOP lock count diagnosed");
    failed += check(xy_pm_tickless_idle() == 1, "STOP lock enters shallow sleep");
    failed += check(s_started_ms == 0u && xy_tick_now() == 0u,
                    "shallow sleep does not program LPTIM or advance tick");
    failed += check(s_entered_mode == XY_HAL_POWER_SLEEP && s_power_enter_calls == 1u,
                    "STOP lock selects shallow power mode");

    failed += check(xy_pm_release_lock(XY_HAL_PM_LOCK_STOP) == XY_HAL_OK,
                    "STOP lock reference released");
    failed += check(xy_pm_get_lock_count(XY_HAL_PM_LOCK_STOP) == 1u,
                    "STOP lock count decremented");
    s_timeout_ticks = xy_tick_from_ms(2u);
    failed += check(xy_pm_tickless_idle() == 1,
                    "short deadline with STOP lock uses shallow sleep");
    failed += check(s_power_enter_calls == 2u && s_started_ms == 0u,
                    "short shallow sleep bypasses LPTIM minimum residency");
    s_timeout_ticks = xy_tick_from_ms(500u);
    failed += check(xy_pm_release_lock(XY_HAL_PM_LOCK_STOP) == XY_HAL_OK,
                    "STOP lock released");
    failed += check(xy_pm_get_lock_count(XY_HAL_PM_LOCK_STOP) == 0u,
                    "STOP lock count cleared");
    failed += check(xy_pm_tickless_idle() == 1, "tickless resumes after STOP unlock");
    failed += check(xy_pm_release_lock(XY_HAL_PM_LOCK_STOP) == XY_HAL_INVALID_PARAM,
                    "STOP lock underflow rejected");
    failed += check(xy_pm_get_lock_count((xy_hal_pm_lock_t)XY_HAL_PM_LOCK_COUNT) == 0u,
                    "invalid lock diagnostic is zero");

    xy_pm_get_stats(&stats);
    failed += check(stats.abort_count == 0u && stats.sleep_count == 1u &&
                    stats.shallow_sleep_count == 2u,
                    "STOP lock stats");
    failed += check(stats.unknown_wake_count == 2u,
                    "unreported shallow wakes counted as unknown");
    return failed;
}

int main(void)
{
    int failed = 0;

    failed += test_full_sleep();
    failed += test_early_wakeup();
    failed += test_shallow_without_lptimer();
    failed += test_wake_source_stats();
    failed += test_sleep_rejected();
    failed += test_unregister();
    failed += test_work_arrives_during_lptimer_start();
    failed += test_earlier_deadline_during_lptimer_start();
    failed += test_stop_lock();

    if (failed == 0) {
        printf("PM tickless tests passed\n");
    }
    return failed;
}
