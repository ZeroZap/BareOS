/**
 * @file xy_hal_timer.c
 * @brief Timer HAL STM32U5 Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal_timer.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

/* Maximum timer channels for callbacks */
#define MAX_TIMER_CHANNELS 4

/* Timer context structure */
typedef struct {
    TIM_HandleTypeDef *htimer;
    xy_hal_timer_callback_t callbacks[MAX_TIMER_CHANNELS];
    void *args[MAX_TIMER_CHANNELS];
    uint8_t initialized;
} timer_ctx_t;

/* Maximum timer instances */
#define MAX_TIMER_INSTANCES 8
static timer_ctx_t g_timer_ctx[MAX_TIMER_INSTANCES];

/**
 * @brief Find timer context by handle
 */
static timer_ctx_t *find_timer_ctx(void *timer)
{
    for (size_t i = 0; i < MAX_TIMER_INSTANCES; i++) {
        if (g_timer_ctx[i].htimer == timer) {
            return &g_timer_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Allocate new timer context
 */
static timer_ctx_t *alloc_timer_ctx(void)
{
    for (size_t i = 0; i < MAX_TIMER_INSTANCES; i++) {
        if (g_timer_ctx[i].htimer == NULL) {
            return &g_timer_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Convert XY count mode to STM32
 */
static uint32_t xy_to_stm32_count_mode(xy_hal_timer_count_mode_t mode)
{
    switch (mode) {
    case XY_HAL_TIMER_COUNT_UP:
        return TIM_COUNTERMODE_UP;
    case XY_HAL_TIMER_COUNT_DOWN:
        return TIM_COUNTERMODE_DOWN;
    case XY_HAL_TIMER_COUNT_CENTER1:
        return TIM_COUNTERMODE_CENTERALIGNED1;
    case XY_HAL_TIMER_COUNT_CENTER2:
        return TIM_COUNTERMODE_CENTERALIGNED2;
    case XY_HAL_TIMER_COUNT_CENTER3:
        return TIM_COUNTERMODE_CENTERALIGNED3;
    default:
        return TIM_COUNTERMODE_UP;
    }
}

/**
 * @brief Convert XY clock division to STM32
 */
static uint32_t xy_to_stm32_clock_div(xy_hal_timer_ckdiv_t div)
{
    switch (div) {
    case XY_HAL_TIMER_CKDIV_1:
        return TIM_CLOCKDIVISION_DIV1;
    case XY_HAL_TIMER_CKDIV_2:
        return TIM_CLOCKDIVISION_DIV2;
    case XY_HAL_TIMER_CKDIV_4:
        return TIM_CLOCKDIVISION_DIV4;
    default:
        return TIM_CLOCKDIVISION_DIV1;
    }
}

xy_hal_error_t xy_hal_timer_init(void *timer,
                                 const xy_hal_timer_config_t *config)
{
    if (!timer || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    TIM_HandleTypeDef *htimer = (TIM_HandleTypeDef *)timer;

    /* Check if already initialized */
    timer_ctx_t *ctx = find_timer_ctx(timer);
    if (ctx != NULL && ctx->initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    /* Allocate context if not exists */
    if (ctx == NULL) {
        ctx = alloc_timer_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    htimer->Init.Prescaler      = config->prescaler;
    htimer->Init.CounterMode    = xy_to_stm32_count_mode(config->mode);
    htimer->Init.Period         = config->period;
    htimer->Init.ClockDivision  = xy_to_stm32_clock_div(config->clock_div);
    htimer->Init.AutoReloadPreload = config->auto_reload_preload ?
                                      TIM_AUTORELOAD_PRELOAD_ENABLE :
                                      TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(htimer) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    /* Initialize context */
    ctx->htimer = htimer;
    memset(ctx->callbacks, 0, sizeof(ctx->callbacks));
    memset(ctx->args, 0, sizeof(ctx->args));
    ctx->initialized = 1;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_deinit(void *timer)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    timer_ctx_t *ctx = find_timer_ctx(timer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_TIM_Base_DeInit((TIM_HandleTypeDef *)timer) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->initialized = 0;
    ctx->htimer      = NULL;
    memset(ctx->callbacks, 0, sizeof(ctx->callbacks));
    memset(ctx->args, 0, sizeof(ctx->args));

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_start(void *timer)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    timer_ctx_t *ctx = find_timer_ctx(timer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_TIM_Base_Start((TIM_HandleTypeDef *)timer) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_stop(void *timer)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    timer_ctx_t *ctx = find_timer_ctx(timer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_TIM_Base_Stop((TIM_HandleTypeDef *)timer) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

int xy_hal_timer_get_counter(void *timer)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    return (int)__HAL_TIM_GET_COUNTER((TIM_HandleTypeDef *)timer);
}

xy_hal_error_t xy_hal_timer_set_counter(void *timer, uint32_t value)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    __HAL_TIM_SET_COUNTER((TIM_HandleTypeDef *)timer, value);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_set_period(void *timer, uint32_t period)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    __HAL_TIM_SET_AUTORELOAD((TIM_HandleTypeDef *)timer, period);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_register_callback(void *timer,
                                              xy_hal_timer_event_t event,
                                              xy_hal_timer_callback_t callback,
                                              void *arg)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    timer_ctx_t *ctx = find_timer_ctx(timer);
    if (ctx == NULL) {
        ctx = alloc_timer_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->htimer = (TIM_HandleTypeDef *)timer;
    }

    /* Map event to channel index */
    size_t idx = (size_t)event;
    if (idx >= MAX_TIMER_CHANNELS) {
        idx = 0; /* Use index 0 for update event */
    }

    ctx->callbacks[idx] = callback;
    ctx->args[idx]      = arg;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_enable_irq(void *timer, xy_hal_timer_event_t event)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    switch (event) {
    case XY_HAL_TIMER_EVENT_UPDATE:
        __HAL_TIM_ENABLE_IT((TIM_HandleTypeDef *)timer, TIM_IT_UPDATE);
        break;
    case XY_HAL_TIMER_EVENT_CC1:
        __HAL_TIM_ENABLE_IT((TIM_HandleTypeDef *)timer, TIM_IT_CC1);
        break;
    case XY_HAL_TIMER_EVENT_CC2:
        __HAL_TIM_ENABLE_IT((TIM_HandleTypeDef *)timer, TIM_IT_CC2);
        break;
    case XY_HAL_TIMER_EVENT_CC3:
        __HAL_TIM_ENABLE_IT((TIM_HandleTypeDef *)timer, TIM_IT_CC3);
        break;
    case XY_HAL_TIMER_EVENT_CC4:
        __HAL_TIM_ENABLE_IT((TIM_HandleTypeDef *)timer, TIM_IT_CC4);
        break;
    default:
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_timer_disable_irq(void *timer, xy_hal_timer_event_t event)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    switch (event) {
    case XY_HAL_TIMER_EVENT_UPDATE:
        __HAL_TIM_DISABLE_IT((TIM_HandleTypeDef *)timer, TIM_IT_UPDATE);
        break;
    case XY_HAL_TIMER_EVENT_CC1:
        __HAL_TIM_DISABLE_IT((TIM_HandleTypeDef *)timer, TIM_IT_CC1);
        break;
    case XY_HAL_TIMER_EVENT_CC2:
        __HAL_TIM_DISABLE_IT((TIM_HandleTypeDef *)timer, TIM_IT_CC2);
        break;
    case XY_HAL_TIMER_EVENT_CC3:
        __HAL_TIM_DISABLE_IT((TIM_HandleTypeDef *)timer, TIM_IT_CC3);
        break;
    case XY_HAL_TIMER_EVENT_CC4:
        __HAL_TIM_DISABLE_IT((TIM_HandleTypeDef *)timer, TIM_IT_CC4);
        break;
    default:
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    return XY_HAL_OK;
}

/* ==================== HAL Callbacks ==================== */

/**
 * @brief Timer update callback
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htimer)
{
    timer_ctx_t *ctx = find_timer_ctx(htimer);
    if (ctx && ctx->callbacks[0]) {
        ctx->callbacks[0](htimer, XY_HAL_TIMER_EVENT_UPDATE, ctx->args[0]);
    }
}

/**
 * @brief Timer capture/compare 1 callback
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htimer)
{
    timer_ctx_t *ctx = find_timer_ctx(htimer);
    if (ctx) {
        /* Determine which channel triggered */
        if (htimer->Channel == HAL_TIM_ACTIVE_CHANNEL_1 && ctx->callbacks[1]) {
            ctx->callbacks[1](htimer, XY_HAL_TIMER_EVENT_CC1, ctx->args[1]);
        } else if (htimer->Channel == HAL_TIM_ACTIVE_CHANNEL_2
                   && ctx->callbacks[2]) {
            ctx->callbacks[2](htimer, XY_HAL_TIMER_EVENT_CC2, ctx->args[2]);
        } else if (htimer->Channel == HAL_TIM_ACTIVE_CHANNEL_3
                   && ctx->callbacks[3]) {
            ctx->callbacks[3](htimer, XY_HAL_TIMER_EVENT_CC3, ctx->args[3]);
        }
    }
}

#endif /* STM32U5 || STM32U5xx */
