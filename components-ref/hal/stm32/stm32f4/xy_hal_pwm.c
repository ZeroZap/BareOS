/**
 * @file xy_hal_pwm_stm32.c
 * @brief PWM HAL STM32 Implementation
 * @version 1.0
 * @date 2025-10-26
 */

#include "../inc/xy_hal_pwm.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"

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

static uint32_t xy_to_stm32_polarity(xy_hal_pwm_polarity_t polarity)
{
    return (polarity == XY_HAL_PWM_POLARITY_HIGH) ? TIM_OCPOLARITY_HIGH
                                                  : TIM_OCPOLARITY_LOW;
}

int xy_hal_pwm_init(void *timer, xy_hal_pwm_channel_t channel,
                    const xy_hal_pwm_config_t *config)
{
    if (!timer || !config) {
        return -1;
    }

    TIM_HandleTypeDef *htim      = (TIM_HandleTypeDef *)timer;
    TIM_OC_InitTypeDef sConfigOC = { 0 };

    /* Calculate period and pulse based on frequency and duty cycle */
    /* This is a simplified calculation, adjust based on actual clock */
    uint32_t timer_clock = 72000000; /* Example: 72MHz */
    uint32_t period      = (timer_clock / config->frequency) - 1;
    uint32_t pulse       = (period * config->duty_cycle) / 10000;

    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = pulse;
    sConfigOC.OCPolarity = xy_to_stm32_polarity(config->polarity);
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    if (HAL_TIM_PWM_ConfigChannel(
            htim, &sConfigOC, xy_to_stm32_channel(channel))
        != HAL_OK) {
        return -1;
    }

    return 0;
}

int xy_hal_pwm_deinit(void *timer, xy_hal_pwm_channel_t channel)
{
    if (!timer) {
        return -1;
    }

    return 0;
}

int xy_hal_pwm_start(void *timer, xy_hal_pwm_channel_t channel)
{
    if (!timer) {
        return -1;
    }

    if (HAL_TIM_PWM_Start(
            (TIM_HandleTypeDef *)timer, xy_to_stm32_channel(channel))
        != HAL_OK) {
        return -1;
    }

    return 0;
}

int xy_hal_pwm_stop(void *timer, xy_hal_pwm_channel_t channel)
{
    if (!timer) {
        return -1;
    }

    if (HAL_TIM_PWM_Stop(
            (TIM_HandleTypeDef *)timer, xy_to_stm32_channel(channel))
        != HAL_OK) {
        return -1;
    }

    return 0;
}

int xy_hal_pwm_set_duty_cycle(void *timer, xy_hal_pwm_channel_t channel,
                              uint32_t duty_cycle)
{
    if (!timer || duty_cycle > 10000) {
        return -1;
    }

    TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *)timer;
    uint32_t period         = __HAL_TIM_GET_AUTORELOAD(htim);
    uint32_t pulse          = (period * duty_cycle) / 10000;

    __HAL_TIM_SET_COMPARE(htim, xy_to_stm32_channel(channel), pulse);

    return 0;
}

int xy_hal_pwm_get_duty_cycle(void *timer, xy_hal_pwm_channel_t channel)
{
    if (!timer) {
        return -1;
    }

    TIM_HandleTypeDef *htim = (TIM_HandleTypeDef *)timer;
    uint32_t period         = __HAL_TIM_GET_AUTORELOAD(htim);
    uint32_t pulse = __HAL_TIM_GET_COMPARE(htim, xy_to_stm32_channel(channel));

    return (int)((pulse * 10000) / period);
}

int xy_hal_pwm_set_frequency(void *timer, uint32_t frequency)
{
    if (!timer || frequency == 0) {
        return -1;
    }

    /* Calculate and set new period */
    /* This is simplified, adjust based on actual implementation */
    return 0;
}

int xy_hal_pwm_get_frequency(void *timer)
{
    if (!timer) {
        return -1;
    }

    /* Calculate frequency from timer configuration */
    return 0;
}

#endif /* STM32_HAL_ENABLED */
