/**
 * @file xy_hal_rng.c
 * @brief RNG (Random Number Generator) HAL STM32U5 Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal_rng.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

/* RNG context structure */
typedef struct {
    RNG_HandleTypeDef *hrng;
    xy_hal_rng_callback_t callback;
    void *arg;
    uint8_t initialized;
} rng_ctx_t;

static rng_ctx_t g_rng_ctx = { 0 };

/* Software RNG state */
static uint32_t g_sw_rng_state = 0;

xy_hal_error_t xy_hal_rng_init(void *rng, const xy_hal_rng_config_t *config)
{
    if (!rng) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (g_rng_ctx.initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    RNG_HandleTypeDef *hrng = (RNG_HandleTypeDef *)rng;

    /* Configure RNG if provided */
    if (config) {
        if (config->clock_enable) {
            __HAL_RCC_RNG_CLK_ENABLE();
        }
    }

    if (HAL_RNG_Init(hrng) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    g_rng_ctx.hrng       = hrng;
    g_rng_ctx.callback   = NULL;
    g_rng_ctx.arg        = NULL;
    g_rng_ctx.initialized = 1;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rng_deinit(void *rng)
{
    if (!rng) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_rng_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_RNG_DeInit((RNG_HandleTypeDef *)rng) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    g_rng_ctx.initialized = 0;
    g_rng_ctx.hrng        = NULL;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rng_enable(void *rng)
{
    if (!rng) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_rng_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    __HAL_RCC_RNG_CLK_ENABLE();
    __HAL_RNG_ENABLE((RNG_HandleTypeDef *)rng);

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rng_disable(void *rng)
{
    if (!rng) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_rng_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    __HAL_RNG_DISABLE((RNG_HandleTypeDef *)rng);
    __HAL_RCC_RNG_CLK_DISABLE();

    return XY_HAL_OK;
}

int32_t xy_hal_rng_get_random(void *rng, uint32_t timeout)
{
    if (!rng) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_rng_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t random = 0;
    HAL_StatusTypeDef status = HAL_RNG_GenerateRandomNumber(
        (RNG_HandleTypeDef *)rng, &random, timeout);

    if (status != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return (int32_t)random;
}

xy_hal_error_t xy_hal_rng_get_random_nb(void *rng, uint32_t *value)
{
    if (!rng || !value) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_rng_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (!__HAL_RNG_GET_FLAG((RNG_HandleTypeDef *)rng, RNG_FLAG_DRDY)) {
        return XY_HAL_ERROR_BUSY;
    }

    *value = ((RNG_HandleTypeDef *)rng)->Instance->DR;
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_rng_get_buffer(void *rng, uint32_t *buffer,
                                     size_t count, uint32_t timeout)
{
    if (!rng || !buffer || count == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_rng_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    for (size_t i = 0; i < count; i++) {
        HAL_StatusTypeDef status = HAL_RNG_GenerateRandomNumber(
            (RNG_HandleTypeDef *)rng, &buffer[i], timeout);
        if (status != HAL_OK) {
            return XY_HAL_ERROR_FAIL;
        }
    }

    return XY_HAL_OK;
}

int32_t xy_hal_rng_get_random_range(void *rng, int32_t min, int32_t max)
{
    if (min >= max) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uint32_t random = (uint32_t)xy_hal_rng_get_random(rng, 1000);
    if (random == 0) {
        return XY_HAL_ERROR_FAIL;
    }

    return (int32_t)(min + (random % ((uint32_t)max - (uint32_t)min + 1)));
}

xy_hal_error_t xy_hal_rng_register_callback(void *rng, xy_hal_rng_event_t event,
                                            xy_hal_rng_callback_t callback,
                                            void *arg)
{
    XY_UNUSED(event);
    if (!rng) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_rng_ctx.initialized) {
        g_rng_ctx.hrng = (RNG_HandleTypeDef *)rng;
        g_rng_ctx.initialized = 1;
    }

    g_rng_ctx.callback = callback;
    g_rng_ctx.arg      = arg;

    return XY_HAL_OK;
}

int xy_hal_rng_is_ready(void *rng)
{
    if (!rng) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_rng_ctx.initialized) {
        return 0;
    }

    return __HAL_RNG_GET_FLAG((RNG_HandleTypeDef *)rng, RNG_FLAG_DRDY) ? 1 : 0;
}

xy_hal_error_t xy_hal_rng_clear_error(void *rng)
{
    if (!rng) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_rng_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    __HAL_RNG_CLEAR_FLAG((RNG_HandleTypeDef *)rng, RNG_FLAG_SECS | RNG_FLAG_CECS);

    return XY_HAL_OK;
}

int xy_hal_rng_get_error_code(void *rng)
{
    if (!rng) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_rng_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    return (int)((RNG_HandleTypeDef *)rng)->ErrorCode;
}

uint32_t xy_hal_rng_soft_random(uint32_t seed)
{
    if (seed != 0) {
        g_sw_rng_state = seed;
    }

    /* Simple LCG PRNG */
    g_sw_rng_state = g_sw_rng_state * 1664525UL + 1013904223UL;
    return g_sw_rng_state;
}

void xy_hal_rng_soft_buffer(uint32_t *buffer, size_t count, uint32_t seed)
{
    xy_hal_rng_soft_random(seed);
    for (size_t i = 0; i < count; i++) {
        buffer[i] = xy_hal_rng_soft_random(0);
    }
}

/* ==================== HAL Callbacks ==================== */

/**
 * @brief RNG ready callback
 */
void HAL_RNG_ReadyDataCallback(RNG_HandleTypeDef *hrng)
{
    if (g_rng_ctx.hrng == hrng && g_rng_ctx.callback) {
        g_rng_ctx.callback(XY_HAL_RNG_EVENT_READY, g_rng_ctx.arg);
    }
}

/**
 * @brief RNG error callback
 */
void HAL_RNG_ErrorCallback(RNG_HandleTypeDef *hrng)
{
    if (g_rng_ctx.hrng == hrng && g_rng_ctx.callback) {
        g_rng_ctx.callback(XY_HAL_RNG_EVENT_ERROR, g_rng_ctx.arg);
    }
}

#endif /* STM32U5 || STM32U5xx */
