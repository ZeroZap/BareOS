/**
 * @file xy_hal_wdg.c
 * @brief Watchdog HAL STM32U5 Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal_wdg.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

/* WDG context structure */
typedef struct {
    IWDG_HandleTypeDef *hiwdg;
    xy_hal_wdg_callback_t callback;
    void *arg;
    uint8_t initialized;
} wdg_ctx_t;

static wdg_ctx_t g_iwdg_ctx = { 0 };

/* IWDG constants */
#define IWDG_TIMEOUT_MS_MIN     1
#define IWDG_TIMEOUT_MS_MAX     32000

xy_hal_error_t xy_hal_iwdg_init(void *wdg, const xy_hal_iwdg_config_t *config)
{
    if (!wdg || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    IWDG_HandleTypeDef *hiwdg = (IWDG_HandleTypeDef *)wdg;

    if (g_iwdg_ctx.initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    /* Configure IWDG */
    hiwdg->Init.Prescaler = config->prescaler;
    hiwdg->Init.Reload    = config->reload;

    if (HAL_IWDG_Init(hiwdg) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    g_iwdg_ctx.hiwdg      = hiwdg;
    g_iwdg_ctx.callback   = NULL;
    g_iwdg_ctx.arg        = NULL;
    g_iwdg_ctx.initialized = 1;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_iwdg_start(void *wdg)
{
    if (!wdg) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_iwdg_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    /* Start IWDG by reloading */
    if (HAL_IWDG_Start(g_iwdg_ctx.hiwdg) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_iwdg_feed(void *wdg)
{
    if (!wdg) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_iwdg_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    /* Reload IWDG counter */
    if (HAL_IWDG_Reload(g_iwdg_ctx.hiwdg) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

int xy_hal_iwdg_get_remaining_time(void *wdg)
{
    if (!wdg) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_iwdg_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    /* Get remaining counter value */
    IWDG_HandleTypeDef *hiwdg = (IWDG_HandleTypeDef *)wdg;
    uint32_t counter = hiwdg->Instance->RLR;

    /* Calculate approximate time in ms */
    uint32_t prescaler = (1 << ((hiwdg->Instance->PR & IWDG_PR_PR) >> 3));
    uint32_t lsi_freq = 32000; /* LSI frequency ~32kHz */

    return (int)((counter * prescaler * 1000) / lsi_freq);
}

xy_hal_error_t xy_hal_iwdg_set_timeout(void *wdg, uint32_t timeout_ms)
{
    if (!wdg || timeout_ms == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_iwdg_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    /* Calculate prescaler and reload values */
    uint32_t lsi_freq = 32000; /* LSI frequency ~32kHz */
    uint32_t counter  = (timeout_ms * lsi_freq) / 1000;

    /* Find appropriate prescaler */
    uint32_t prescaler_idx = 0;
    while (counter > 0xFFF && prescaler_idx < 7) {
        counter >>= 1;
        prescaler_idx++;
    }

    if (counter > 0xFFF) {
        return XY_HAL_ERROR_INVALID_PARAM; /* Timeout too long */
    }

    if (counter < 1) {
        return XY_HAL_ERROR_INVALID_PARAM; /* Timeout too short */
    }

    IWDG_HandleTypeDef *hiwdg = (IWDG_HandleTypeDef *)wdg;
    hiwdg->Init.Prescaler = prescaler_idx << 3;
    hiwdg->Init.Reload    = counter & 0xFFF;

    return xy_hal_iwdg_init(wdg, &g_iwdg_ctx.hiwdg->Init);
}

xy_hal_error_t xy_hal_wwdg_init(void *wdg, const xy_hal_wwdg_config_t *config)
{
    XY_UNUSED(wdg);
    XY_UNUSED(config);
    /* WWDG implementation - similar to IWDG */
    return XY_HAL_ERROR_NOT_SUPPORT;
}

xy_hal_error_t xy_hal_wwdg_start(void *wdg, uint8_t enable_early_wakeup)
{
    XY_UNUSED(wdg);
    XY_UNUSED(enable_early_wakeup);
    return XY_HAL_ERROR_NOT_SUPPORT;
}

xy_hal_error_t xy_hal_wwdg_feed(void *wdg, uint32_t counter)
{
    XY_UNUSED(wdg);
    XY_UNUSED(counter);
    return XY_HAL_ERROR_NOT_SUPPORT;
}

int xy_hal_wwdg_get_remaining_time(void *wdg)
{
    XY_UNUSED(wdg);
    return XY_HAL_ERROR_NOT_SUPPORT;
}

xy_hal_error_t xy_hal_wwdg_set_window(void *wdg, uint32_t window)
{
    XY_UNUSED(wdg);
    XY_UNUSED(window);
    return XY_HAL_ERROR_NOT_SUPPORT;
}

xy_hal_error_t xy_hal_wwdg_register_ewi_callback(void *wdg,
                                                 xy_hal_wdg_callback_t callback,
                                                 void *arg)
{
    XY_UNUSED(wdg);
    XY_UNUSED(callback);
    XY_UNUSED(arg);
    return XY_HAL_ERROR_NOT_SUPPORT;
}

xy_hal_error_t xy_hal_wdg_enable_irq(void *wdg)
{
    XY_UNUSED(wdg);
    return XY_HAL_ERROR_NOT_SUPPORT;
}

xy_hal_error_t xy_hal_wdg_disable_irq(void *wdg)
{
    XY_UNUSED(wdg);
    return XY_HAL_ERROR_NOT_SUPPORT;
}

void xy_hal_wdg_system_reset(void)
{
    NVIC_SystemReset();
}

#endif /* STM32U5 || STM32U5xx */
