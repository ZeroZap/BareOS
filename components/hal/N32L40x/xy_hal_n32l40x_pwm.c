#include "xy_hal_n32l40x_internal.h"

static uint16_t tim_channel(uint8_t channel)
{
    switch (channel) {
    case 1u: return TIM_CH_1;
    case 2u: return TIM_CH_2;
    case 3u: return TIM_CH_3;
    case 4u: return TIM_CH_4;
    default: return 0xffffu;
    }
}

static void tim_init_oc(TIM_Module *tim, uint8_t channel, OCInitType *oc)
{
    switch (channel) {
    case 1u: TIM_InitOc1(tim, oc); TIM_ConfigOc1Preload(tim, TIM_OC_PRE_LOAD_ENABLE); break;
    case 2u: TIM_InitOc2(tim, oc); TIM_ConfigOc2Preload(tim, TIM_OC_PRE_LOAD_ENABLE); break;
    case 3u: TIM_InitOc3(tim, oc); TIM_ConfigOc3Preload(tim, TIM_OC_PRE_LOAD_ENABLE); break;
    case 4u: TIM_InitOc4(tim, oc); TIM_ConfigOc4Preload(tim, TIM_OC_PRE_LOAD_ENABLE); break;
    default: break;
    }
}

static void tim_set_cmp(TIM_Module *tim, uint8_t channel, uint16_t cmp)
{
    switch (channel) {
    case 1u: TIM_SetCmp1(tim, cmp); break;
    case 2u: TIM_SetCmp2(tim, cmp); break;
    case 3u: TIM_SetCmp3(tim, cmp); break;
    case 4u: TIM_SetCmp4(tim, cmp); break;
    default: break;
    }
}

static void calc_pwm(uint32_t clock_hz, uint32_t frequency_hz,
                     uint16_t duty_permille, uint16_t *psc,
                     uint16_t *period, uint16_t *pulse)
{
    uint32_t ticks;
    uint32_t prescaler = 0u;

    if (clock_hz == 0u) clock_hz = 1000000u;
    if (frequency_hz == 0u) frequency_hz = 1u;
    if (duty_permille > 1000u) duty_permille = 1000u;

    ticks = clock_hz / frequency_hz;
    if (ticks == 0u) ticks = 1u;
    while (ticks > 65536u && prescaler < 65535u) {
        prescaler++;
        ticks = clock_hz / ((prescaler + 1u) * frequency_hz);
    }
    if (ticks == 0u) ticks = 1u;
    if (ticks > 65536u) ticks = 65536u;

    *psc = (uint16_t)prescaler;
    *period = (uint16_t)(ticks - 1u);
    *pulse = (uint16_t)((ticks * duty_permille) / 1000u);
}

int xy_hal_pwm_init(void *pwm, const xy_hal_pwm_config_t *config)
{
    TIM_Module *tim = (TIM_Module *)pwm;
    TIM_TimeBaseInitType base;
    OCInitType oc;
    uint16_t psc;
    uint16_t period;
    uint16_t pulse;

    if (!tim || !config || tim_channel(config->channel) == 0xffffu) return XY_HAL_INVALID_PARAM;
    xy_hal_n32_enable_tim_clock(tim);
    calc_pwm(config->clock_hz, config->frequency_hz, config->duty_permille, &psc, &period, &pulse);

    TIM_InitTimBaseStruct(&base);
    base.Prescaler = psc;
    base.CntMode = TIM_CNT_MODE_UP;
    base.Period = period;
    base.ClkDiv = TIM_CLK_DIV1;
    base.RepetCnt = 0u;
    TIM_InitTimeBase(tim, &base);
    TIM_ConfigArPreload(tim, ENABLE);

    TIM_InitOcStruct(&oc);
    oc.OcMode = TIM_OCMODE_PWM1;
    oc.OutputState = TIM_OUTPUT_STATE_ENABLE;
    oc.OutputNState = TIM_OUTPUT_NSTATE_DISABLE;
    oc.Pulse = pulse;
    oc.OcPolarity = config->polarity == XY_HAL_PWM_POLARITY_LOW ? TIM_OC_POLARITY_LOW : TIM_OC_POLARITY_HIGH;
    oc.OcNPolarity = TIM_OCN_POLARITY_HIGH;
    oc.OcIdleState = TIM_OC_IDLE_STATE_RESET;
    oc.OcNIdleState = TIM_OCN_IDLE_STATE_RESET;
    tim_init_oc(tim, config->channel, &oc);
    return XY_HAL_OK;
}

int xy_hal_pwm_deinit(void *pwm, uint8_t channel)
{
    TIM_Module *tim = (TIM_Module *)pwm;
    if (!tim) return XY_HAL_INVALID_PARAM;
    if (channel == 0u) TIM_DeInit(tim);
    else (void)xy_hal_pwm_stop(pwm, channel);
    return XY_HAL_OK;
}

int xy_hal_pwm_start(void *pwm, uint8_t channel)
{
    TIM_Module *tim = (TIM_Module *)pwm;
    uint16_t ch = tim_channel(channel);
    if (!tim || ch == 0xffffu) return XY_HAL_INVALID_PARAM;
    TIM_EnableCapCmpCh(tim, ch, TIM_CAP_CMP_ENABLE);
    if (tim == TIM1 || tim == TIM8) TIM_EnableCtrlPwmOutputs(tim, ENABLE);
    TIM_Enable(tim, ENABLE);
    return XY_HAL_OK;
}

int xy_hal_pwm_stop(void *pwm, uint8_t channel)
{
    TIM_Module *tim = (TIM_Module *)pwm;
    uint16_t ch = tim_channel(channel);
    if (!tim || ch == 0xffffu) return XY_HAL_INVALID_PARAM;
    TIM_EnableCapCmpCh(tim, ch, TIM_CAP_CMP_DISABLE);
    return XY_HAL_OK;
}

int xy_hal_pwm_set(void *pwm, uint8_t channel, uint32_t frequency_hz,
                   uint16_t duty_permille)
{
    TIM_Module *tim = (TIM_Module *)pwm;
    uint16_t ch = tim_channel(channel);
    uint32_t ticks;
    uint16_t pulse;
    if (!tim || ch == 0xffffu) return XY_HAL_INVALID_PARAM;
    if (duty_permille > 1000u) duty_permille = 1000u;
    (void)frequency_hz;
    ticks = (uint32_t)TIM_GetAutoReload(tim) + 1u;
    pulse = (uint16_t)((ticks * duty_permille) / 1000u);
    tim_set_cmp(tim, channel, pulse);
    return XY_HAL_OK;
}
