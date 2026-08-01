#include "xy_hal_n32l40x_internal.h"

int xy_hal_power_configure(const xy_hal_power_policy_t *policy)
{
    if (!policy) return XY_HAL_INVALID_PARAM;
    g_xy_hal_n32_power_policy = *policy;
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_PWR, ENABLE);
    return XY_HAL_OK;
}

int xy_hal_power_enter(xy_hal_power_mode_t mode, uint32_t wake_sources, uint32_t timeout_ms)
{
    (void)wake_sources;
    (void)timeout_ms;
    RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_PWR, ENABLE);
    xy_hal_n32l40x_before_sleep();
    if (mode == XY_HAL_POWER_SLEEP) PWR_EnterSLEEPMode(SLEEP_OFF_EXIT, PWR_SLEEPENTRY_WFI);
    else if (mode == XY_HAL_POWER_STOP) {
        xy_hal_systick_suspend();
        PWR_EnterSTOP2Mode(PWR_STOPENTRY_WFI,
                           PWR_CTRL3_RAM1RET | PWR_CTRL3_RAM2RET);
        if (g_xy_hal_n32_lptim.timer) {
            if (LPTIM_IsActiveFlag_ARRM(g_xy_hal_n32_lptim.timer))
                g_xy_hal_n32_lptim.elapsed_ticks = g_xy_hal_n32_lptim.programmed_ticks;
            else
                g_xy_hal_n32_lptim.elapsed_ticks = LPTIM_GetCounter(g_xy_hal_n32_lptim.timer);
        }
        xy_hal_n32l40x_after_stop_restore_clock();
        xy_hal_systick_resume();
    }
    else if (mode == XY_HAL_POWER_STANDBY || mode == XY_HAL_POWER_SHUTDOWN) PWR_EnterSTANDBYMode(PWR_STOPENTRY_WFI, 0u);
    else return XY_HAL_OK;
    if (mode != XY_HAL_POWER_STOP) xy_hal_n32l40x_after_stop_restore_clock();
    xy_hal_n32l40x_after_wake();
    return XY_HAL_OK;
}

int xy_hal_power_acquire_lock(xy_hal_pm_lock_t lock)
{
    if ((uint32_t)lock >= (uint32_t)XY_HAL_PM_LOCK_COUNT ||
        g_xy_hal_n32_pm_locks[lock] == UINT16_MAX)
        return XY_HAL_INVALID_PARAM;
    g_xy_hal_n32_pm_locks[lock]++;
    return XY_HAL_OK;
}

int xy_hal_power_release_lock(xy_hal_pm_lock_t lock)
{
    if ((uint32_t)lock >= (uint32_t)XY_HAL_PM_LOCK_COUNT ||
        g_xy_hal_n32_pm_locks[lock] == 0u)
        return XY_HAL_INVALID_PARAM;
    g_xy_hal_n32_pm_locks[lock]--;
    return XY_HAL_OK;
}

uint16_t xy_hal_power_get_lock_count(xy_hal_pm_lock_t lock)
{
    if ((uint32_t)lock >= (uint32_t)XY_HAL_PM_LOCK_COUNT) return 0u;
    return g_xy_hal_n32_pm_locks[lock];
}

xy_hal_power_mode_t xy_hal_power_get_allowed_mode(void)
{
    if (g_xy_hal_n32_pm_locks[XY_HAL_PM_LOCK_CPU] ||
        g_xy_hal_n32_pm_locks[XY_HAL_PM_LOCK_SLEEP]) return XY_HAL_POWER_RUN;
    if (g_xy_hal_n32_pm_locks[XY_HAL_PM_LOCK_STOP]) return XY_HAL_POWER_SLEEP;
    if (g_xy_hal_n32_pm_locks[XY_HAL_PM_LOCK_STANDBY]) return XY_HAL_POWER_STOP;
    return g_xy_hal_n32_power_policy.deepest_mode;
}

int xy_hal_periph_pm_configure(const xy_hal_periph_pm_config_t *config)
{
    (void)config;
    return config ? XY_HAL_OK : XY_HAL_INVALID_PARAM;
}

int xy_hal_periph_suspend(xy_hal_periph_type_t type, void *instance)
{
    (void)type;
    (void)instance;
    return XY_HAL_OK;
}

int xy_hal_periph_resume(xy_hal_periph_type_t type, void *instance)
{
    (void)type;
    (void)instance;
    return XY_HAL_OK;
}

int xy_hal_tickless_enter(const xy_hal_tickless_request_t *req, xy_hal_tickless_result_t *res)
{
    uint32_t start;
    xy_hal_power_mode_t mode;
    if (!req || !res) return XY_HAL_INVALID_PARAM;
    mode = req->mode;
    if (mode > xy_hal_power_get_allowed_mode()) mode = xy_hal_power_get_allowed_mode();
    start = xy_hal_time_ms();
    xy_hal_systick_suspend();
    (void)xy_hal_power_enter(mode, req->wake_sources, req->sleep_ms);
    xy_hal_systick_resume();
    res->elapsed_ms = (uint32_t)(xy_hal_time_ms() - start);
    if (res->elapsed_ms == 0u && req->sleep_ms != 0u) res->elapsed_ms = req->sleep_ms;
    res->wake_sources = req->wake_sources;
    res->reason = res->elapsed_ms >= req->sleep_ms ? XY_HAL_WAKE_REASON_TIMEOUT : XY_HAL_WAKE_REASON_UNKNOWN;
    return XY_HAL_OK;
}
