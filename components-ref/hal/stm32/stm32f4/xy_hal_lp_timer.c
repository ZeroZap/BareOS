/**
 * @file xy_hal_lp_timer_stm32.c
 * @brief Low Power Timer HAL STM32 Implementation
 * @version 1.0
 * @date 2025-10-26
 */

#include "../inc/xy_hal_lp_timer.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"

#ifdef LPTIM1

static uint32_t xy_to_stm32_clk_src(xy_hal_lptimer_clk_src_t clk_src)
{
    return (clk_src == XY_HAL_LPTIMER_CLK_INTERNAL)
               ? LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC
               : LPTIM_CLOCKSOURCE_ULPTIM;
}

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

int xy_hal_lptimer_init(void *lptimer, const xy_hal_lptimer_config_t *config)
{
    if (!lptimer || !config) {
        return -1;
    }

    LPTIM_HandleTypeDef *hlptim = (LPTIM_HandleTypeDef *)lptimer;

    hlptim->Init.Clock.Source    = xy_to_stm32_clk_src(config->clk_src);
    hlptim->Init.Clock.Prescaler = xy_to_stm32_prescaler(config->prescaler);
    hlptim->Init.Trigger.Source  = LPTIM_TRIGSOURCE_SOFTWARE;
    hlptim->Init.OutputPolarity  = LPTIM_OUTPUTPOLARITY_HIGH;
    hlptim->Init.UpdateMode      = LPTIM_UPDATE_IMMEDIATE;
    hlptim->Init.CounterSource   = LPTIM_COUNTERSOURCE_INTERNAL;

    if (HAL_LPTIM_Init(hlptim) != HAL_OK) {
        return -1;
    }

    return 0;
}

int xy_hal_lptimer_deinit(void *lptimer)
{
    if (!lptimer) {
        return -1;
    }

    if (HAL_LPTIM_DeInit((LPTIM_HandleTypeDef *)lptimer) != HAL_OK) {
        return -1;
    }

    return 0;
}

int xy_hal_lptimer_start(void *lptimer)
{
    if (!lptimer) {
        return -1;
    }

    LPTIM_HandleTypeDef *hlptim = (LPTIM_HandleTypeDef *)lptimer;

    if (HAL_LPTIM_Counter_Start(hlptim) != HAL_OK) {
        return -1;
    }

    return 0;
}

int xy_hal_lptimer_stop(void *lptimer)
{
    if (!lptimer) {
        return -1;
    }

    if (HAL_LPTIM_Counter_Stop((LPTIM_HandleTypeDef *)lptimer) != HAL_OK) {
        return -1;
    }

    return 0;
}

int xy_hal_lptimer_get_counter(void *lptimer)
{
    if (!lptimer) {
        return -1;
    }

    LPTIM_HandleTypeDef *hlptim = (LPTIM_HandleTypeDef *)lptimer;
    return (int)HAL_LPTIM_ReadCounter(hlptim);
}

int xy_hal_lptimer_register_callback(void *lptimer,
                                     xy_hal_lptimer_callback_t callback,
                                     void *arg)
{
    /* Store callback in context */
    return 0;
}

#else /* LPTIM1 not available */

int xy_hal_lptimer_init(void *lptimer, const xy_hal_lptimer_config_t *config)
{
    return -1; /* Not supported on this MCU */
}

int xy_hal_lptimer_deinit(void *lptimer)
{
    return -1;
}

int xy_hal_lptimer_start(void *lptimer)
{
    return -1;
}

int xy_hal_lptimer_stop(void *lptimer)
{
    return -1;
}

int xy_hal_lptimer_get_counter(void *lptimer)
{
    return -1;
}

int xy_hal_lptimer_register_callback(void *lptimer,
                                     xy_hal_lptimer_callback_t callback,
                                     void *arg)
{
    return -1;
}

#endif /* LPTIM1 */

#endif /* STM32_HAL_ENABLED */
