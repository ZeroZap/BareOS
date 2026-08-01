#include "xy_hal_n32l40x_internal.h"

int xy_hal_lptimer_init(void *timer, const xy_hal_lptimer_config_t *config)
{
    LPTIM_Module *t = (LPTIM_Module *)timer;
    LPTIM_InitType init;
    EXTI_InitType exti;
    NVIC_InitType nvic;
    if (!t || !config) return XY_HAL_INVALID_PARAM;
    RCC_EnableRETPeriphClk(RCC_RET_PERIPH_LPTIM, ENABLE);
    LPTIM_StructInit(&init);
    init.ClockSource = LPTIM_CLK_SOURCE_INTERNAL;
    init.Prescaler = LPTIM_PRESCALER_DIV1;
    init.Waveform = LPTIM_OUTPUT_WAVEFORM_PWM;
    init.Polarity = LPTIM_OUTPUT_POLARITY_REGULAR;
    if (LPTIM_Init(t, &init) != SUCCESS) return XY_HAL_ERROR;
    LPTIM_ClearFLAG_ARRM(t);
    LPTIM_EnableIT_ARRM(t);
    if (config->wakeup) {
        EXTI_ClrITPendBit(EXTI_LINE24);
        EXTI_InitStruct(&exti);
        exti.EXTI_Line = EXTI_LINE24;
        exti.EXTI_Mode = EXTI_Mode_Interrupt;
        exti.EXTI_Trigger = EXTI_Trigger_Rising;
        exti.EXTI_LineCmd = ENABLE;
        EXTI_InitPeripheral(&exti);

        nvic.NVIC_IRQChannel = LPTIM_WKUP_IRQn;
        nvic.NVIC_IRQChannelPreemptionPriority = config->priority;
        nvic.NVIC_IRQChannelSubPriority = 0u;
        nvic.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&nvic);
    }
    g_xy_hal_n32_lptim.timer = t;
    g_xy_hal_n32_lptim.clock_hz = config->clock_hz;
    g_xy_hal_n32_lptim.active = false;
    if (g_xy_hal_n32_lptim.clock_hz == 0u) return XY_HAL_INVALID_PARAM;
    return XY_HAL_OK;
}

int xy_hal_lptimer_deinit(void *timer)
{
    if (!timer) return XY_HAL_INVALID_PARAM;
    LPTIM_DeInit((LPTIM_Module *)timer);
    if (g_xy_hal_n32_lptim.timer == timer) memset(&g_xy_hal_n32_lptim, 0, sizeof(g_xy_hal_n32_lptim));
    return XY_HAL_OK;
}

int xy_hal_lptimer_start(void *timer, uint32_t timeout_ms, xy_hal_timer_cb_t cb, void *user)
{
    LPTIM_Module *t = (LPTIM_Module *)timer;
    uint64_t ticks;
    uint32_t arr;
    uint32_t sync_delay;
    uint32_t spin;
    if (!t || timeout_ms == 0u) return XY_HAL_INVALID_PARAM;
    if (g_xy_hal_n32_lptim.clock_hz == 0u) return XY_HAL_INVALID_PARAM;
    ticks = ((uint64_t)g_xy_hal_n32_lptim.clock_hz * timeout_ms + 999ULL) / 1000ULL;
    if (ticks == 0u) ticks = 1u;
    if (ticks > 0xffffu) return XY_HAL_INVALID_PARAM;
    arr = (uint32_t)ticks - 1u;
    g_xy_hal_n32_lptim.timer = t;
    g_xy_hal_n32_lptim.timeout_ms = timeout_ms;
    g_xy_hal_n32_lptim.programmed_ticks = (uint32_t)ticks;
    g_xy_hal_n32_lptim.elapsed_ticks = 0u;
    g_xy_hal_n32_lptim.active = true;
    g_xy_hal_n32_lptim.cb = cb;
    g_xy_hal_n32_lptim.user = user;
    LPTIM_Disable(t);
    LPTIM_ClearFLAG_ARRM(t);
    LPTIM_ClearFlag_ARROK(t);
    EXTI_ClrITPendBit(EXTI_LINE24);
    NVIC_ClearPendingIRQ(LPTIM_WKUP_IRQn);
    LPTIM_Enable(t);
    /* The peripheral requires two LPTIM clocks after enable. Allow three;
     * 256 CPU NOPs was too short for a 32-40 kHz source at 48 MHz. */
    sync_delay = (uint32_t)(((uint64_t)SystemCoreClock * 3ULL +
                             g_xy_hal_n32_lptim.clock_hz - 1u) /
                            g_xy_hal_n32_lptim.clock_hz);
    for (spin = 0u; spin < sync_delay; spin++) __NOP();
    LPTIM_SetAutoReload(t, arr);
    if (LPTIM_GetAutoReload(t) != arr) {
        LPTIM_Disable(t);
        g_xy_hal_n32_lptim.active = false;
        return XY_HAL_ERROR;
    }
    /* ARRUPD is not asserted on the tested N32L406 revision. Wait for the
     * low-speed-domain synchronization instead of spinning on that flag. */
    for (spin = 0u; spin < sync_delay; spin++) __NOP();
    LPTIM_ClearFlag_ARROK(t);
    LPTIM_StartCounter(t, LPTIM_OPERATING_MODE_ONESHOT);
    return XY_HAL_OK;
}

int xy_hal_lptimer_stop(void *timer)
{
    if (!timer) return XY_HAL_INVALID_PARAM;
    LPTIM_Disable((LPTIM_Module *)timer);
    LPTIM_ClearFLAG_ARRM((LPTIM_Module *)timer);
    EXTI_ClrITPendBit(EXTI_LINE24);
    NVIC_ClearPendingIRQ(LPTIM_WKUP_IRQn);
    g_xy_hal_n32_lptim.active = false;
    g_xy_hal_n32_lptim.cb = NULL;
    g_xy_hal_n32_lptim.user = NULL;
    return XY_HAL_OK;
}

uint32_t xy_hal_lptimer_now_ms(void *timer)
{
    LPTIM_Module *t = (LPTIM_Module *)timer;
    uint32_t ticks;
    uint32_t hz = g_xy_hal_n32_lptim.clock_hz;
    if (!t || hz == 0u) return 0u;
    ticks = g_xy_hal_n32_lptim.elapsed_ticks;
    if (ticks == 0u && g_xy_hal_n32_lptim.active) ticks = LPTIM_GetCounter(t);
    return (uint32_t)(((uint64_t)ticks * 1000ULL) / hz);
}

void xy_hal_lptimer_irq_handler(void *timer)
{
    LPTIM_Module *t = (LPTIM_Module *)timer;
    bool matched = false;
    if (!t) return;
    if (LPTIM_IsActiveFlag_ARRM(t)) {
        LPTIM_ClearFLAG_ARRM(t);
        EXTI_ClrITPendBit(EXTI_LINE24);
        g_xy_hal_n32_lptim.elapsed_ticks = g_xy_hal_n32_lptim.programmed_ticks;
        g_xy_hal_n32_lptim.active = false;
        matched = true;
    }
    if (matched && g_xy_hal_n32_lptim.timer == t && g_xy_hal_n32_lptim.cb)
        g_xy_hal_n32_lptim.cb(t, g_xy_hal_n32_lptim.user);
}
