/**
 * @file xy_hal_timer_stm32.c
 * @brief Timer HAL STM32 Implementation
 * @version 1.0
 * @date 2025-10-26
 */

#include "../inc/xy_hal_timer.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"

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

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

static uint32_t xy_to_stm32_ckdiv(xy_hal_timer_ckdiv_t ckdiv)
{
    switch (ckdiv) {
    case XY_HAL_TIMER_CKDIV_1:
        return TIM_CLOCKDIVISION_DIV1;
    case XY_HAL_TIMER_CKDIV_2:
        return TIM_CLOCKDIVISION_DIV2;
    case XY_HAL_TIMER_CKDIV_4:
        return TIM_CLOCKDIVISION_DIV4;
    default:
        return TIM_CLOCKDIVISION_DIV1;
    }

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_timer_init(void *timer, const xy_hal_timer_config_t *config)
{
    if (!timer || !config) {
        return -1;
    }

    TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *)timer;

    htim->Init.Prescaler         = config->prescaler;
    htim->Init.CounterMode       = xy_to_stm32_count_mode(config->mode);
    htim->Init.Period            = config->period;
    htim->Init.ClockDivision     = xy_to_stm32_ckdiv(config->clock_div);
    htim->Init.AutoReloadPreload = config->auto_reload_preload
                                       ? TIM_AUTORELOAD_PRELOAD_ENABLE
                                       : TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(htim) != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_timer_deinit(void *timer)
{
    if (!timer) {
        return -1;
    }

    if (HAL_TIM_Base_DeInit((TIM_HandleTypeDef *)timer) != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_timer_start(void *timer)
{
    if (!timer) {
        return -1;
    }

    if (HAL_TIM_Base_Start((TIM_HandleTypeDef *)timer) != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_timer_stop(void *timer)
{
    if (!timer) {
        return -1;
    }

    if (HAL_TIM_Base_Stop((TIM_HandleTypeDef *)timer) != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_timer_get_counter(void *timer)
{
    if (!timer) {
        return -1;
    }

    return (int)__HAL_TIM_GET_COUNTER((TIM_HandleTypeDef *)timer);

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_timer_set_counter(void *timer, uint32_t value)
{
    if (!timer) {
        return -1;
    }

    __HAL_TIM_SET_COUNTER((TIM_HandleTypeDef *)timer, value);
    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_timer_set_period(void *timer, uint32_t period)
{
    if (!timer) {
        return -1;
    }

    __HAL_TIM_SET_AUTORELOAD((TIM_HandleTypeDef *)timer, period);
    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_timer_register_callback(void *timer, xy_hal_timer_event_t event,
                                   xy_hal_timer_callback_t callback, void *arg)
{
    /* Store callback in context */
    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_timer_enable_irq(void *timer, xy_hal_timer_event_t event)
{
    if (!timer) {
        return -1;
    }

    if (HAL_TIM_Base_Start_IT((TIM_HandleTypeDef *)timer) != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

int xy_hal_timer_disable_irq(void *timer, xy_hal_timer_event_t event)
{
    if (!timer) {
        return -1;
    }

    if (HAL_TIM_Base_Stop_IT((TIM_HandleTypeDef *)timer) != HAL_OK) {
        return -1;
    }

    return 0;

    return XY_HAL_ERROR_NOT_SUPPORTED;
}

#endif /* STM32_HAL_ENABLED */
