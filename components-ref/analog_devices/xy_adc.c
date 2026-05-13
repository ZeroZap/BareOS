/**
 * @file xy_adc.c
 * @brief ADC/DAC Helper Library Implementation
 * @version 1.0.0
 * @date 2026-02-28
 */

#include "xy_adc.h"
#include <string.h>

/* ==================== ADC Implementation ==================== */

int xy_adc_init(xy_adc_t *adc, uint32_t ref_voltage_mv, xy_adc_resolution_t resolution)
{
    if (!adc) {
        return XY_ADC_INVALID_PARAM;
    }

    memset(adc, 0, sizeof(*adc));

    adc->ref = ADC_REF_INTERNAL;
    adc->ref_voltage_mv = ref_voltage_mv;
    adc->res = resolution;
    adc->channel_count = 0;
    adc->initialized = true;

    return XY_ADC_OK;
}

int xy_adc_deinit(xy_adc_t *adc)
{
    if (!adc) {
        return XY_ADC_INVALID_PARAM;
    }

    adc->initialized = false;
    return XY_ADC_OK;
}

int xy_adc_config_channel(xy_adc_t *adc, uint8_t channel, bool enabled)
{
    if (!adc) {
        return XY_ADC_INVALID_PARAM;
    }

    if (channel >= ADC_MAX_CHANNELS) {
        return XY_ADC_INVALID_PARAM;
    }

    if (enabled) {
        /* 检查是否已存在 */
        for (uint8_t i = 0; i < adc->channel_count; i++) {
            if (adc->channels[i].channel == channel) {
                adc->channels[i].enabled = true;
                return XY_ADC_OK;
            }
        }

        /* 添加新通道 */
        if (adc->channel_count < ADC_MAX_CHANNELS) {
            adc->channels[adc->channel_count].channel = channel;
            adc->channels[adc->channel_count].res = adc->res;
            adc->channels[adc->channel_count].enabled = true;
            adc->channel_count++;
        }
    } else {
        /* 禁用通道 */
        for (uint8_t i = 0; i < adc->channel_count; i++) {
            if (adc->channels[i].channel == channel) {
                adc->channels[i].enabled = false;
                break;
            }
        }
    }

    return XY_ADC_OK;
}

int xy_adc_sample(xy_adc_t *adc, uint8_t channel, xy_adc_sample_t *sample)
{
    if (!adc || !sample) {
        return XY_ADC_INVALID_PARAM;
    }

    if (!adc->initialized) {
        return XY_ADC_NOT_READY;
    }

    /* 模拟采样值 (实际应用中需要读取硬件) */
    uint32_t max_value = ADC_MAX_VALUE(adc->res);

    sample->channel = channel;
    sample->raw_value = (uint32_t)(max_value * 0.5); /* 模拟 50% 电压 */
    sample->voltage_mv = xy_adc_raw_to_voltage(adc, sample->raw_value);
    sample->valid = true;

    return XY_ADC_OK;
}

int xy_adc_sample_multi(xy_adc_t *adc, xy_adc_sample_t *samples, uint8_t count)
{
    if (!adc || !samples) {
        return XY_ADC_INVALID_PARAM;
    }

    int sampled = 0;
    for (uint8_t i = 0; i < count && i < adc->channel_count; i++) {
        if (adc->channels[i].enabled) {
            if (xy_adc_sample(adc, adc->channels[i].channel, &samples[sampled]) == XY_ADC_OK) {
                sampled++;
            }
        }
    }

    return sampled;
}

uint32_t xy_adc_raw_to_voltage(xy_adc_t *adc, uint32_t raw_value)
{
    if (!adc || adc->res == 0) {
        return 0;
    }

    uint32_t max_value = ADC_MAX_VALUE(adc->res);
    return (raw_value * adc->ref_voltage_mv) / max_value;
}

uint32_t xy_adc_voltage_to_raw(xy_adc_t *adc, uint32_t voltage_mv)
{
    if (!adc || adc->res == 0) {
        return 0;
    }

    uint32_t max_value = ADC_MAX_VALUE(adc->res);
    return (voltage_mv * max_value) / adc->ref_voltage_mv;
}

/* ==================== DAC Implementation ==================== */

int xy_dac_init(xy_dac_t *dac, uint32_t ref_voltage_mv, xy_dac_resolution_t resolution)
{
    if (!dac) {
        return XY_ADC_INVALID_PARAM;
    }

    memset(dac, 0, sizeof(*dac));

    dac->ref_voltage_mv = ref_voltage_mv;
    dac->res = resolution;
    dac->channel_count = 0;
    dac->initialized = true;

    return XY_ADC_OK;
}

int xy_dac_deinit(xy_dac_t *dac)
{
    if (!dac) {
        return XY_ADC_INVALID_PARAM;
    }

    dac->initialized = false;
    return XY_ADC_OK;
}

int xy_dac_config_channel(xy_dac_t *dac, uint8_t channel, bool enabled)
{
    if (!dac) {
        return XY_ADC_INVALID_PARAM;
    }

    if (channel >= DAC_MAX_CHANNELS) {
        return XY_ADC_INVALID_PARAM;
    }

    if (enabled) {
        for (uint8_t i = 0; i < dac->channel_count; i++) {
            if (dac->channels[i].channel == channel) {
                dac->channels[i].enabled = true;
                return XY_ADC_OK;
            }
        }

        if (dac->channel_count < DAC_MAX_CHANNELS) {
            dac->channels[dac->channel_count].channel = channel;
            dac->channels[dac->channel_count].res = dac->res;
            dac->channels[dac->channel_count].enabled = true;
            dac->channel_count++;
        }
    }

    return XY_ADC_OK;
}

int xy_dac_set_voltage(xy_dac_t *dac, uint8_t channel, uint32_t voltage_mv)
{
    if (!dac) {
        return XY_ADC_INVALID_PARAM;
    }

    uint32_t raw_value = xy_adc_voltage_to_raw((xy_adc_t *)dac, voltage_mv);
    return xy_dac_set_raw(dac, channel, raw_value);
}

int xy_dac_set_raw(xy_dac_t *dac, uint8_t channel, uint32_t raw_value)
{
    if (!dac) {
        return XY_ADC_INVALID_PARAM;
    }

    /* 实际应用中需要写入硬件寄存器 */
    (void)channel;
    (void)raw_value;

    return XY_ADC_OK;
}

uint32_t xy_dac_get_voltage(xy_dac_t *dac, uint8_t channel)
{
    if (!dac) {
        return 0;
    }

    (void)channel;
    return dac->ref_voltage_mv / 2; /* 模拟返回 50% 电压 */
}
