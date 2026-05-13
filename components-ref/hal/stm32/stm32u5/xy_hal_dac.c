/**
 * @file xy_hal_dac.c
 * @brief DAC HAL STM32U5 Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal_dac.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include "stm32u5xx_hal_dac.h"
#include <string.h>

/* DAC context structure */
typedef struct {
    DAC_HandleTypeDef *hdac;
    xy_hal_dac_callback_t callback;
    void *arg;
    uint8_t initialized;
} dac_ctx_t;

static dac_ctx_t g_dac_ctx = { 0 };

/**
 * @brief Convert XY DAC resolution to STM32
 */
static uint32_t xy_to_stm32_resolution(xy_hal_dac_resolution_t resolution)
{
    switch (resolution) {
    case XY_HAL_DAC_RESOLUTION_8B:
        return DAC_ALIGN_8B_R;
    case XY_HAL_DAC_RESOLUTION_12B:
        return DAC_ALIGN_12B_R;
    default:
        return DAC_ALIGN_12B_R;
    }
}

/**
 * @brief Convert XY channel to STM32
 */
static uint32_t xy_to_stm32_channel(xy_hal_dac_channel_t channel)
{
    switch (channel) {
    case XY_HAL_DAC_CHANNEL_1:
        return DAC_CHANNEL_1;
    case XY_HAL_DAC_CHANNEL_2:
        return DAC_CHANNEL_2;
    default:
        return DAC_CHANNEL_1;
    }
}

xy_hal_error_t xy_hal_dac_init(void *dac, const xy_hal_dac_config_t *config)
{
    if (!dac || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (g_dac_ctx.initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    DAC_HandleTypeDef *hdac = (DAC_HandleTypeDef *)dac;

    if (HAL_DAC_Init(hdac) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    g_dac_ctx.hdac       = hdac;
    g_dac_ctx.callback   = NULL;
    g_dac_ctx.arg        = NULL;
    g_dac_ctx.initialized = 1;

    XY_UNUSED(config); /* Configuration applied per channel */

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_deinit(void *dac)
{
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_DAC_DeInit((DAC_HandleTypeDef *)dac) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    g_dac_ctx.initialized = 0;
    g_dac_ctx.hdac        = NULL;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_enable_channel(void *dac, xy_hal_dac_channel_t channel)
{
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t stm_channel = xy_to_stm32_channel(channel);

    DAC_ChannelConfTypeDef sConfig = { 0 };
    sConfig.DAC_SampleAndHold      = DAC_SAMPLEANDHOLD_DISABLE;
    sConfig.DAC_Trigger            = DAC_TRIGGER_SOFTWARE;
    sConfig.DAC_OutputBuffer       = DAC_OUTPUTBUFFER_ENABLE;
    sConfig.DAC_ConnectOnChipTrigger = DAC_ONCHIPTRIGGERCONNECT_DISABLE;
    sConfig.DAC_TRIG1Source        = DAC_TRIG1SOURCE_T6_TRGO;
    sConfig.DAC_TRIG2Source        = DAC_TRIG2SOURCE_T8_TRGO;
    sConfig.DAC_UserTrimming       = DAC_TRIMMING_FACTORY;

    if (HAL_DAC_ConfigChannel((DAC_HandleTypeDef *)dac, &sConfig,
                              stm_channel) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_disable_channel(void *dac, xy_hal_dac_channel_t channel)
{
    XY_UNUSED(channel);
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    /* Channel disable not directly supported - stop output instead */
    return xy_hal_dac_stop_output(dac, channel);
}

xy_hal_error_t xy_hal_dac_set_value(void *dac, xy_hal_dac_channel_t channel,
                                    uint32_t value, xy_hal_dac_align_t align)
{
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t stm_channel = xy_to_stm32_channel(channel);
    uint32_t stm_align;

    switch (align) {
    case XY_HAL_DAC_DATAALIGN_RIGHT:
        stm_align = DAC_ALIGN_12B_R;
        break;
    case XY_HAL_DAC_DATAALIGN_LEFT:
        stm_align = DAC_ALIGN_12B_L;
        break;
    default:
        stm_align = DAC_ALIGN_12B_R;
        break;
    }

    if (HAL_DAC_SetValue((DAC_HandleTypeDef *)dac, stm_channel,
                         stm_align, value) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

int xy_hal_dac_get_value(void *dac, xy_hal_dac_channel_t channel)
{
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t stm_channel = xy_to_stm32_channel(channel);
    return (int)HAL_DAC_GetValue((DAC_HandleTypeDef *)dac, stm_channel);
}

xy_hal_error_t xy_hal_dac_start_output(void *dac, xy_hal_dac_channel_t channel)
{
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t stm_channel = xy_to_stm32_channel(channel);

    if (HAL_DAC_Start((DAC_HandleTypeDef *)dac, stm_channel) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_stop_output(void *dac, xy_hal_dac_channel_t channel)
{
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t stm_channel = xy_to_stm32_channel(channel);

    if (HAL_DAC_Stop((DAC_HandleTypeDef *)dac, stm_channel) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_software_trigger(void *dac, xy_hal_dac_channel_t channel)
{
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t stm_channel = xy_to_stm32_channel(channel);

    if (HAL_DAC_TriggerSoftwareConvert((DAC_HandleTypeDef *)dac,
                                       stm_channel) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_output_dma(void *dac, xy_hal_dac_channel_t channel,
                                     const uint32_t *data, size_t count)
{
    if (!dac || !data || count == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t stm_channel = xy_to_stm32_channel(channel);

    if (HAL_DAC_Start_DMA((DAC_HandleTypeDef *)dac, stm_channel,
                          (uint32_t *)data, (uint32_t)count,
                          DAC_ALIGN_12B_R) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dac_generate_triangle(void *dac, xy_hal_dac_channel_t channel,
                                            uint32_t amplitude)
{
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t stm_channel = xy_to_stm32_channel(channel);

    /* Triangle wave generation requires specific configuration */
    XY_UNUSED(amplitude);

    /* Not fully implemented - requires hardware-specific setup */
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_dac_generate_noise(void *dac, xy_hal_dac_channel_t channel,
                                         uint32_t mask)
{
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t stm_channel = xy_to_stm32_channel(channel);
    XY_UNUSED(mask);

    /* Noise generation requires specific configuration */
    return XY_HAL_ERROR_NOT_SUPPORTED;
}

xy_hal_error_t xy_hal_dac_register_callback(void *dac, xy_hal_dac_channel_t channel,
                                            xy_hal_dac_callback_t callback,
                                            void *arg)
{
    XY_UNUSED(channel);
    if (!dac) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_dac_ctx.initialized) {
        g_dac_ctx.hdac = (DAC_HandleTypeDef *)dac;
        g_dac_ctx.initialized = 1;
    }

    g_dac_ctx.callback = callback;
    g_dac_ctx.arg      = arg;

    return XY_HAL_OK;
}

uint32_t xy_hal_dac_value_to_mv(void *dac, uint32_t value, uint32_t vref)
{
    XY_UNUSED(dac);
    /* Assuming 12-bit resolution */
    return (value * vref) / 4095;
}

/* ==================== HAL Callbacks ==================== */

/**
 * @brief DAC DMA underrun callback
 */
void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
    if (g_dac_ctx.hdac == hdac && g_dac_ctx.callback) {
        g_dac_ctx.callback(hdac, XY_HAL_DAC_CHANNEL_1,
                          XY_HAL_DAC_EVENT_READY, g_dac_ctx.arg);
    }
}

/**
 * @brief DAC DMA half complete callback
 */
void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
    XY_UNUSED(hdac);
}

/**
 * @brief DAC error callback
 */
void HAL_DAC_ErrorCallbackCh1(DAC_HandleTypeDef *hdac)
{
    if (g_dac_ctx.hdac == hdac && g_dac_ctx.callback) {
        g_dac_ctx.callback(hdac, XY_HAL_DAC_CHANNEL_1,
                          XY_HAL_DAC_EVENT_DMAUNDERRUN, g_dac_ctx.arg);
    }
}

#endif /* STM32U5 || STM32U5xx */
