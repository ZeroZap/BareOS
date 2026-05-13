/**
 * @file xy_hal_pwm.c
 * @brief WCH CH32V30x PWM HAL Implementation
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_hal_pwm.h"
#include "xy_log.h"

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

#ifdef MCU_CH32

#include "ch32v30x.h"

/**
 * @brief Timer 时钟使能
 */
static void xy_hal_timer_enable_clock(void *instance)
{
    if (instance == TIM1) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    } else if (instance == TIM2) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    } else if (instance == TIM3) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    } else if (instance == TIM4) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    }
}

xy_hal_error_t xy_hal_pwm_init(void *instance, uint8_t channel,
                               const xy_hal_pwm_config_t *config)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCStructure;
    
    if (!instance || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 使能时钟 */
    xy_hal_timer_enable_clock(instance);
    
    /* 配置时基 */
    TIM_TimeBaseStructure.TIM_Period = config->period - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = config->prescaler - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    
    TIM_TimeBaseInit(instance, &TIM_TimeBaseStructure);
    
    /* 配置输出比较 */
    TIM_OCStructure.TIM_OCMode = config->mode == XY_HAL_PWM_MODE_PWM1 ?
                                 TIM_OCMode_PWM1 : TIM_OCMode_PWM2;
    TIM_OCStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCStructure.TIM_Pulse = 0;  /* 初始占空比 0% */
    TIM_OCStructure.TIM_OCPolarity = config->polarity == XY_HAL_PWM_POLARITY_HIGH ?
                                     TIM_OCPolarity_High : TIM_OCPolarity_Low;
    
    /* 根据通道配置 */
    if (channel == 0) {
        TIM_OC1Init(instance, &TIM_OCStructure);
        TIM_OC1PreloadConfig(instance, TIM_OCPreload_Enable);
    } else if (channel == 1) {
        TIM_OC2Init(instance, &TIM_OCStructure);
        TIM_OC2PreloadConfig(instance, TIM_OCPreload_Enable);
    } else if (channel == 2) {
        TIM_OC3Init(instance, &TIM_OCStructure);
        TIM_OC3PreloadConfig(instance, TIM_OCPreload_Enable);
    } else if (channel == 3) {
        TIM_OC4Init(instance, &TIM_OCStructure);
        TIM_OC4PreloadConfig(instance, TIM_OCPreload_Enable);
    }
    
    /* 使能定时器 */
    TIM_Cmd(instance, ENABLE);
    
    /* 对于 TIM1，还需要使能主输出 */
    if (instance == TIM1) {
        TIM_CtrlPWMOutputs(instance, ENABLE);
    }
    
    xy_log_d("WCH PWM init: instance=%p, channel=%d, period=%d\n",
             instance, channel, config->period);
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pwm_deinit(void *instance, uint8_t channel)
{
    if (!instance) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 禁用通道 */
    if (channel == 0) {
        TIM_OC1Disable(instance);
    } else if (channel == 1) {
        TIM_OC2Disable(instance);
    } else if (channel == 2) {
        TIM_OC3Disable(instance);
    } else if (channel == 3) {
        TIM_OC4Disable(instance);
    }
    
    TIM_Cmd(instance, DISABLE);
    
    if (instance == TIM1) {
        TIM_CtrlPWMOutputs(instance, DISABLE);
    }
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pwm_set_duty(void *instance, uint8_t channel,
                                   uint16_t duty)
{
    if (!instance || channel > 3) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 设置占空比 */
    if (channel == 0) {
        TIM_SetCompare1(instance, duty);
    } else if (channel == 1) {
        TIM_SetCompare2(instance, duty);
    } else if (channel == 2) {
        TIM_SetCompare3(instance, duty);
    } else if (channel == 3) {
        TIM_SetCompare4(instance, duty);
    }
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pwm_get_duty(void *instance, uint8_t channel,
                                   uint16_t *duty)
{
    if (!instance || !duty || channel > 3) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    if (channel == 0) {
        *duty = TIM_GetCapture1(instance);
    } else if (channel == 1) {
        *duty = TIM_GetCapture2(instance);
    } else if (channel == 2) {
        *duty = TIM_GetCapture3(instance);
    } else if (channel == 3) {
        *duty = TIM_GetCapture4(instance);
    }
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pwm_start(void *instance, uint8_t channel)
{
    if (!instance) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 使能通道 */
    if (channel == 0) {
        TIM_CCxCmd(instance, TIM_Channel_1, TIM_CCx_Enable);
    } else if (channel == 1) {
        TIM_CCxCmd(instance, TIM_Channel_2, TIM_CCx_Enable);
    } else if (channel == 2) {
        TIM_CCxCmd(instance, TIM_Channel_3, TIM_CCx_Enable);
    } else if (channel == 3) {
        TIM_CCxCmd(instance, TIM_Channel_4, TIM_CCx_Enable);
    }
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pwm_stop(void *instance, uint8_t channel)
{
    if (!instance) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 禁用通道 */
    if (channel == 0) {
        TIM_CCxCmd(instance, TIM_Channel_1, TIM_CCx_Disable);
    } else if (channel == 1) {
        TIM_CCxCmd(instance, TIM_Channel_2, TIM_CCx_Disable);
    } else if (channel == 2) {
        TIM_CCxCmd(instance, TIM_Channel_3, TIM_CCx_Disable);
    } else if (channel == 3) {
        TIM_CCxCmd(instance, TIM_Channel_4, TIM_CCx_Disable);
    }
    
    return XY_HAL_OK;
}

#else

xy_hal_error_t xy_hal_pwm_init(void *instance, uint8_t channel,
                               const xy_hal_pwm_config_t *config)
{
    (void)instance;
    (void)channel;
    (void)config;
    return XY_HAL_ERROR_NOT_SUPPORT;
}

#endif
