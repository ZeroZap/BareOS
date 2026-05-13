/**
 * @file xy_hal_lp_timer.c
 * @brief Low Power Timer HAL STM32U5 Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal_lp_timer.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

/* LP Timer context structure */
typedef struct {
    LPTIM_HandleTypeDef *hlptim;
    xy_hal_lptimer_callback_t callback;
    void *arg;
    uint8_t initialized;
} lptimer_ctx_t;

/* Maximum LPTimer instances */
#define MAX_LPTIMER_INSTANCES 2
static lptimer_ctx_t g_lptimer_ctx[MAX_LPTIMER_INSTANCES];

/**
 * @brief Find LPTimer context by handle
 */
static lptimer_ctx_t *find_lptimer_ctx(void *lptimer)
{
    for (size_t i = 0; i < MAX_LPTIMER_INSTANCES; i++) {
        if (g_lptimer_ctx[i].hlptim == lptimer) {
            return &g_lptimer_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Allocate new LPTimer context
 */
static lptimer_ctx_t *alloc_lptimer_ctx(void)
{
    for (size_t i = 0; i < MAX_LPTIMER_INSTANCES; i++) {
        if (g_lptimer_ctx[i].hlptim == NULL) {
            return &g_lptimer_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Convert XY clock source to STM32
 */
static uint32_t xy_to_stm32_clk_src(xy_hal_lptimer_clk_src_t clk_src)
{
    XY_UNUSED(clk_src);
    /* Default to internal clock for STM32U5 */
    return LPTIM_CLOCKSOURCE_ULPTIM;
}

/**
 * @brief Convert XY prescaler to STM32
 */
static uint32_t xy_to_stm32_prescaler(xy_hal_lptimer_prescaler_t prescaler)
{
    switch (prescaler) {
    case XY_HAL_LPTIMER_PRESCALER_1:
        return LPTIM_PRESCALER_DIV1;
    case XY_HAL_LPTIMER_PRESCALER_2:
        return LPTIM_PRESCALER_DIV2;
    case XY_HAL_LPTIMER_PRESCALER_4:
        return LPTIM_PRESCALER_DIV4;
    case XY_HAL_LPTIMER_PRESCALER_8:
        return LPTIM_PRESCALER_DIV8;
    case XY_HAL_LPTIMER_PRESCALER_16:
        return LPTIM_PRESCALER_DIV16;
    case XY_HAL_LPTIMER_PRESCALER_32:
        return LPTIM_PRESCALER_DIV32;
    case XY_HAL_LPTIMER_PRESCALER_64:
        return LPTIM_PRESCALER_DIV64;
    case XY_HAL_LPTIMER_PRESCALER_128:
        return LPTIM_PRESCALER_DIV128;
    default:
        return LPTIM_PRESCALER_DIV1;
    }
}

xy_hal_error_t xy_hal_lptimer_init(void *lptimer,
                                   const xy_hal_lptimer_config_t *config)
{
    if (!lptimer || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    LPTIM_HandleTypeDef *hlptim = (LPTIM_HandleTypeDef *)lptimer;

    /* Check if already initialized */
    lptimer_ctx_t *ctx = find_lptimer_ctx(lptimer);
    if (ctx != NULL && ctx->initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    /* Allocate context if not exists */
    if (ctx == NULL) {
        ctx = alloc_lptimer_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    hlptim->Init.Clock.Source  = xy_to_stm32_clk_src(config->clk_src);
    hlptim->Init.Clock.Prescaler = xy_to_stm32_prescaler(config->prescaler);
    hlptim->Init.Trigger.Source = LPTIM_TRIGSOURCE_SOFTWARE;
    hlptim->Init.OutputPolarity = LPTIM_OUTPUTPOLARITY_HIGH;
    hlptim->Init.UpdateMode    = LPTIM_UPDATE_IMMEDIATE;
    hlptim->Init.CounterSource = LPTIM_COUNTERSOURCE_INTERNAL;
    hlptim->Init.Input1Source  = LPTIM_INPUT1SOURCE_GPIO;
    hlptim->Init.Input2Source  = LPTIM_INPUT2SOURCE_GPIO;
    hlptim->Init.EncoderMode   = LPTIM_ENCODERMODE_DISABLE;

    if (HAL_LPTIM_Init(hlptim) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    /* Initialize context */
    ctx->hlptim      = hlptim;
    ctx->callback    = NULL;
    ctx->arg         = NULL;
    ctx->initialized = 1;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_lptimer_deinit(void *lptimer)
{
    if (!lptimer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    lptimer_ctx_t *ctx = find_lptimer_ctx(lptimer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_LPTIM_DeInit((LPTIM_HandleTypeDef *)lptimer) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->initialized = 0;
    ctx->hlptim      = NULL;
    ctx->callback    = NULL;
    ctx->arg         = NULL;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_lptimer_start(void *lptimer)
{
    if (!lptimer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    lptimer_ctx_t *ctx = find_lptimer_ctx(lptimer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_LPTIM_Start((LPTIM_HandleTypeDef *)lptimer) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_lptimer_stop(void *lptimer)
{
    if (!lptimer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    lptimer_ctx_t *ctx = find_lptimer_ctx(lptimer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_LPTIM_Stop((LPTIM_HandleTypeDef *)lptimer) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

int xy_hal_lptimer_get_counter(void *lptimer)
{
    if (!lptimer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    lptimer_ctx_t *ctx = find_lptimer_ctx(lptimer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    return (int)HAL_LPTIM_ReadCounter((LPTIM_HandleTypeDef *)lptimer);
}

xy_hal_error_t xy_hal_lptimer_register_callback(void *lptimer,
                                                xy_hal_lptimer_callback_t callback,
                                                void *arg)
{
    if (!lptimer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    lptimer_ctx_t *ctx = find_lptimer_ctx(lptimer);
    if (ctx == NULL) {
        ctx = alloc_lptimer_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->hlptim = (LPTIM_HandleTypeDef *)lptimer;
    }

    ctx->callback = callback;
    ctx->arg      = arg;

    return XY_HAL_OK;
}

/* ==================== HAL Callbacks ==================== */

/**
 * @brief LPTimer compare match callback
 */
void HAL_LPTIM_CompareMatchCallback(LPTIM_HandleTypeDef *hlptim)
{
    lptimer_ctx_t *ctx = find_lptimer_ctx(hlptim);
    if (ctx && ctx->callback) {
        ctx->callback(hlptim, ctx->arg);
    }
}

/**
 * @brief LPTimer autoreload match callback
 */
void HAL_LPTIM_AutoreloadMatchCallback(LPTIM_HandleTypeDef *hlptim)
{
    lptimer_ctx_t *ctx = find_lptimer_ctx(hlptim);
    if (ctx && ctx->callback) {
        ctx->callback(hlptim, ctx->arg);
    }
}

/**
 * @brief LPTimer error callback
 */
void HAL_LPTIM_ErrorCallback(LPTIM_HandleTypeDef *hlptim)
{
    XY_UNUSED(hlptim);
}

#endif /* STM32U5 || STM32U5xx */
