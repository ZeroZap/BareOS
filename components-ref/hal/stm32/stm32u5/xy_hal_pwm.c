/**
 * @file xy_hal_pwm.c
 * @brief PWM HAL STM32U5 Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal_pwm.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

/* PWM context structure */
typedef struct {
    TIM_HandleTypeDef *htimer;
    uint32_t frequency;
    uint8_t initialized;
} pwm_ctx_t;

/* Maximum PWM instances */
#define MAX_PWM_INSTANCES 8
static pwm_ctx_t g_pwm_ctx[MAX_PWM_INSTANCES];

/**
 * @brief Find PWM context by handle
 */
static pwm_ctx_t *find_pwm_ctx(void *timer)
{
    for (size_t i = 0; i < MAX_PWM_INSTANCES; i++) {
        if (g_pwm_ctx[i].htimer == timer) {
            return &g_pwm_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Allocate new PWM context
 */
static pwm_ctx_t *alloc_pwm_ctx(void)
{
    for (size_t i = 0; i < MAX_PWM_INSTANCES; i++) {
        if (g_pwm_ctx[i].htimer == NULL) {
            return &g_pwm_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Convert XY channel to STM32 channel
 */
static uint32_t xy_to_stm32_channel(xy_hal_pwm_channel_t channel)
{
    switch (channel) {
    case XY_HAL_PWM_CHANNEL_1:
        return TIM_CHANNEL_1;
    case XY_HAL_PWM_CHANNEL_2:
        return TIM_CHANNEL_2;
    case XY_HAL_PWM_CHANNEL_3:
        return TIM_CHANNEL_3;
    case XY_HAL_PWM_CHANNEL_4:
        return TIM_CHANNEL_4;
    default:
        return TIM_CHANNEL_1;
    }
}

/**
 * @brief Convert XY polarity to STM32
 */
static uint32_t xy_to_stm32_polarity(xy_hal_pwm_polarity_t polarity)
{
    switch (polarity) {
    case XY_HAL_PWM_POLARITY_HIGH:
        return TIM_OCPOLARITY_HIGH;
    case XY_HAL_PWM_POLARITY_LOW:
        return TIM_OCPOLARITY_LOW;
    default:
        return TIM_OCPOLARITY_HIGH;
    }
}

/**
 * @brief Get timer clock frequency
 */
static uint32_t get_timer_clock(TIM_HandleTypeDef *htimer)
{
    uint32_t clock = HAL_RCC_GetPCLK1Freq();

    /* Check if timer is on APB1 or APB2 */
#if defined(TIM1)
    if (htimer->Instance == TIM1) {
        clock = HAL_RCC_GetPCLK2Freq();
    }
#endif
#if defined(TIM8)
    if (htimer->Instance == TIM8) {
        clock = HAL_RCC_GetPCLK2Freq();
    }
#endif
#if defined(TIM15)
    if (htimer->Instance == TIM15) {
        clock = HAL_RCC_GetPCLK2Freq();
    }
#endif
#if defined(TIM16)
    if (htimer->Instance == TIM16) {
        clock = HAL_RCC_GetPCLK2Freq();
    }
#endif
#if defined(TIM17)
    if (htimer->Instance == TIM17) {
        clock = HAL_RCC_GetPCLK2Freq();
    }
#endif

    return clock;
}

xy_hal_error_t xy_hal_pwm_init(void *timer, xy_hal_pwm_channel_t channel,
                               const xy_hal_pwm_config_t *config)
{
    if (!timer || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    TIM_HandleTypeDef *htimer = (TIM_HandleTypeDef *)timer;

    /* Check if already initialized */
    pwm_ctx_t *ctx = find_pwm_ctx(timer);
    if (ctx != NULL && ctx->initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    /* Allocate context if not exists */
    if (ctx == NULL) {
        ctx = alloc_pwm_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    uint32_t timer_clock = get_timer_clock(htimer);

    /* Calculate prescaler and period for desired frequency */
    /* Target: 1MHz timer clock for easy duty cycle calculation */
    uint32_t prescaler = (timer_clock / 1000000) - 1;
    uint32_t period    = (1000000 / config->frequency) - 1;

    htimer->Init.Prescaler      = prescaler;
    htimer->Init.CounterMode    = TIM_COUNTERMODE_UP;
    htimer->Init.Period         = period;
    htimer->Init.ClockDivision  = TIM_CLOCKDIVISION_DIV1;
    htimer->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_PWM_Init(htimer) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    /* Configure PWM channel */
    TIM_OC_InitTypeDef sConfigOC = { 0 };
    sConfigOC.OCMode       = TIM_OCMODE_PWM1;
    sConfigOC.Pulse        = 0;
    sConfigOC.OCPolarity   = xy_to_stm32_polarity(config->polarity);
    sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState  = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

    uint32_t stm_channel = xy_to_stm32_channel(channel);
    if (HAL_TIM_PWM_ConfigChannel(htimer, &sConfigOC, stm_channel) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    /* Initialize context */
    ctx->htimer     = htimer;
    ctx->frequency  = config->frequency;
    ctx->initialized = 1;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pwm_deinit(void *timer, xy_hal_pwm_channel_t channel)
{
    XY_UNUSED(channel);
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    pwm_ctx_t *ctx = find_pwm_ctx(timer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_TIM_PWM_DeInit((TIM_HandleTypeDef *)timer) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->initialized = 0;
    ctx->htimer      = NULL;
    ctx->frequency   = 0;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pwm_start(void *timer, xy_hal_pwm_channel_t channel)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    pwm_ctx_t *ctx = find_pwm_ctx(timer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t stm_channel = xy_to_stm32_channel(channel);
    if (HAL_TIM_PWM_Start((TIM_HandleTypeDef *)timer, stm_channel) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pwm_stop(void *timer, xy_hal_pwm_channel_t channel)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    pwm_ctx_t *ctx = find_pwm_ctx(timer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t stm_channel = xy_to_stm32_channel(channel);
    if (HAL_TIM_PWM_Stop((TIM_HandleTypeDef *)timer, stm_channel) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pwm_set_duty_cycle(void *timer,
                                         xy_hal_pwm_channel_t channel,
                                         uint32_t duty_cycle)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    /* Clamp duty cycle to 0-10000 */
    if (duty_cycle > 10000) {
        duty_cycle = 10000;
    }

    TIM_HandleTypeDef *htimer = (TIM_HandleTypeDef *)timer;
    uint32_t period           = htimer->Init.Period;

    /* Calculate compare value: duty_cycle / 10000 * period */
    uint32_t compare = (duty_cycle * period) / 10000;

    uint32_t stm_channel = xy_to_stm32_channel(channel);
    __HAL_TIM_SET_COMPARE(htimer, stm_channel, compare);

    return XY_HAL_OK;
}

int xy_hal_pwm_get_duty_cycle(void *timer, xy_hal_pwm_channel_t channel)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    TIM_HandleTypeDef *htimer = (TIM_HandleTypeDef *)timer;
    uint32_t period           = htimer->Init.Period;
    uint32_t stm_channel      = xy_to_stm32_channel(channel);

    uint32_t compare = __HAL_TIM_GET_COMPARE(htimer, stm_channel);

    /* Calculate duty cycle: compare / period * 10000 */
    if (period == 0) {
        return 0;
    }

    return (int)((compare * 10000) / period);
}

xy_hal_error_t xy_hal_pwm_set_frequency(void *timer, uint32_t frequency)
{
    if (!timer || frequency == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    pwm_ctx_t *ctx = find_pwm_ctx(timer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    TIM_HandleTypeDef *htimer = (TIM_HandleTypeDef *)timer;
    uint32_t timer_clock      = get_timer_clock(htimer);

    /* Recalculate period for new frequency */
    uint32_t prescaler = htimer->Init.Prescaler;
    uint32_t period    = (timer_clock / ((prescaler + 1) * frequency)) - 1;

    if (period > 65535) {
        /* Frequency too low, adjust prescaler */
        prescaler = (timer_clock / (65536 * frequency));
        period    = (timer_clock / ((prescaler + 1) * frequency)) - 1;
        htimer->Init.Prescaler = prescaler;
    }

    htimer->Init.Period = period;
    __HAL_TIM_SET_AUTORELOAD(htimer, period);

    ctx->frequency = frequency;

    return XY_HAL_OK;
}

int xy_hal_pwm_get_frequency(void *timer)
{
    if (!timer) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    pwm_ctx_t *ctx = find_pwm_ctx(timer);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    return (int)ctx->frequency;
}

#endif /* STM32U5 || STM32U5xx */
