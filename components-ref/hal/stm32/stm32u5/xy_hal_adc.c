/**
 * @file xy_hal_adc.c
 * @brief ADC HAL STM32U5 Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal_adc.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

/* ADC context structure */
typedef struct {
    ADC_HandleTypeDef *hadc;
    xy_hal_adc_callback_t callback;
    void *arg;
    uint8_t initialized;
    uint32_t resolution;
} adc_ctx_t;

/* Maximum ADC instances */
#define MAX_ADC_INSTANCES 4
static adc_ctx_t g_adc_ctx[MAX_ADC_INSTANCES];

/**
 * @brief Find ADC context by handle
 */
static adc_ctx_t *find_adc_ctx(void *adc)
{
    for (size_t i = 0; i < MAX_ADC_INSTANCES; i++) {
        if (g_adc_ctx[i].hadc == adc) {
            return &g_adc_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Allocate new ADC context
 */
static adc_ctx_t *alloc_adc_ctx(void)
{
    for (size_t i = 0; i < MAX_ADC_INSTANCES; i++) {
        if (g_adc_ctx[i].hadc == NULL) {
            return &g_adc_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Convert XY ADC resolution to STM32
 */
static uint32_t xy_to_stm32_resolution(xy_hal_adc_resolution_t resolution)
{
    switch (resolution) {
    case XY_HAL_ADC_RESOLUTION_6B:
        return ADC_RESOLUTION_6B;
    case XY_HAL_ADC_RESOLUTION_8B:
        return ADC_RESOLUTION_8B;
    case XY_HAL_ADC_RESOLUTION_10B:
        return ADC_RESOLUTION_10B;
    case XY_HAL_ADC_RESOLUTION_12B:
        return ADC_RESOLUTION_12B;
    case XY_HAL_ADC_RESOLUTION_14B:
        return ADC_RESOLUTION_14B;
    case XY_HAL_ADC_RESOLUTION_16B:
        return ADC_RESOLUTION_16B;
    default:
        return ADC_RESOLUTION_12B;
    }
}

/**
 * @brief Convert XY ADC align to STM32
 */
static uint32_t xy_to_stm32_align(xy_hal_adc_align_t align)
{
    switch (align) {
    case XY_HAL_ADC_DATAALIGN_RIGHT:
        return ADC_DATAALIGN_RIGHT;
    case XY_HAL_ADC_DATAALIGN_LEFT:
        return ADC_DATAALIGN_LEFT;
    default:
        return ADC_DATAALIGN_RIGHT;
    }
}

/**
 * @brief Convert XY ADC scan mode to STM32
 */
static uint32_t xy_to_stm32_scan(xy_hal_adc_scan_t scan)
{
    return (scan == XY_HAL_ADC_SCAN_ENABLE) ? ADC_SCAN_ENABLE : ADC_SCAN_DISABLE;
}

/**
 * @brief Convert XY ADC continuous mode to STM32
 */
static uint32_t xy_to_stm32_continuous(xy_hal_adc_continuous_t continuous)
{
    return (continuous == XY_HAL_ADC_CONTINUOUS_ENABLE) ?
           ADC_CONTINUOUS_MODE : ADC_SINGLESHOT_MODE;
}

xy_hal_error_t xy_hal_adc_init(void *adc, const xy_hal_adc_config_t *config)
{
    if (!adc || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    ADC_HandleTypeDef *hadc = (ADC_HandleTypeDef *)adc;

    /* Check if already initialized */
    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (ctx != NULL && ctx->initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    /* Allocate context if not exists */
    if (ctx == NULL) {
        ctx = alloc_adc_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    hadc->Init.ClockPrescaler        = config->clock_div;
    hadc->Init.Resolution            = xy_to_stm32_resolution(config->resolution);
    hadc->Init.DataAlign             = xy_to_stm32_align(config->align);
    hadc->Init.ScanConvMode          = xy_to_stm32_scan(config->scan_mode);
    hadc->Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    hadc->Init.ContinuousConvMode    = xy_to_stm32_continuous(config->continuous);
    hadc->Init.DiscontinuousConvMode = DISABLE;
    hadc->Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc->Init.DMAContinuousRequests = DISABLE;
    hadc->Init.NbrOfConversion       = 1;
    hadc->Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
    hadc->Init.LeftBitShift          = ADC_LEFTBITSHIFT_NONE;
    hadc->Init.OversamplingMode      = DISABLE;

    if (HAL_ADC_Init(hadc) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->hadc        = hadc;
    ctx->callback    = NULL;
    ctx->arg         = NULL;
    ctx->initialized = 1;
    ctx->resolution  = config->resolution;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_deinit(void *adc)
{
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_ADC_DeInit((ADC_HandleTypeDef *)adc) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->initialized = 0;
    ctx->hadc        = NULL;
    ctx->callback    = NULL;
    ctx->arg         = NULL;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_config_channels(void *adc,
                                          const xy_hal_adc_channel_t *channels,
                                          size_t count)
{
    if (!adc || !channels || count == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    ADC_ChannelConfTypeDef sConfig = { 0 };

    for (size_t i = 0; i < count; i++) {
        sConfig.Channel      = channels[i].channel;
        sConfig.Rank         = channels[i].rank;
        sConfig.SamplingTime = channels[i].sampling_time;
        sConfig.Offset       = 0;

        if (HAL_ADC_ConfigChannel((ADC_HandleTypeDef *)adc, &sConfig) != HAL_OK) {
            return XY_HAL_ERROR_FAIL;
        }
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_start(void *adc)
{
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_ADC_Start((ADC_HandleTypeDef *)adc) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_stop(void *adc)
{
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_ADC_Stop((ADC_HandleTypeDef *)adc) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

int xy_hal_adc_read(void *adc, uint8_t channel, uint32_t timeout)
{
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    ADC_ChannelConfTypeDef sConfig = { 0 };
    sConfig.Channel      = channel;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
    sConfig.Offset       = 0;

    if (HAL_ADC_ConfigChannel((ADC_HandleTypeDef *)adc, &sConfig) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    if (HAL_ADC_Start((ADC_HandleTypeDef *)adc) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    if (HAL_ADC_PollForConversion((ADC_HandleTypeDef *)adc, timeout) != HAL_OK) {
        HAL_ADC_Stop((ADC_HandleTypeDef *)adc);
        return XY_HAL_ERROR_TIMEOUT;
    }

    uint32_t value = HAL_ADC_GetValue((ADC_HandleTypeDef *)adc);
    HAL_ADC_Stop((ADC_HandleTypeDef *)adc);

    return (int)value;
}

int xy_hal_adc_read_nb(void *adc, uint8_t channel)
{
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    ADC_ChannelConfTypeDef sConfig = { 0 };
    sConfig.Channel      = channel;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
    sConfig.Offset       = 0;

    if (HAL_ADC_ConfigChannel((ADC_HandleTypeDef *)adc, &sConfig) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    if (HAL_ADC_Start((ADC_HandleTypeDef *)adc) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    /* Non-blocking check */
    if (HAL_ADC_PollForConversion((ADC_HandleTypeDef *)adc, 0) != HAL_OK) {
        return XY_HAL_ERROR_BUSY;
    }

    uint32_t value = HAL_ADC_GetValue((ADC_HandleTypeDef *)adc);
    HAL_ADC_Stop((ADC_HandleTypeDef *)adc);

    return (int)value;
}

xy_hal_error_t xy_hal_adc_read_dma(void *adc, uint32_t *buffer, size_t count)
{
    if (!adc || !buffer || count == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_ADC_Start_DMA((ADC_HandleTypeDef *)adc, (uint32_t *)buffer,
                          (uint32_t)count) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

int xy_hal_adc_get_resolution(void *adc)
{
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    switch (ctx->resolution) {
    case XY_HAL_ADC_RESOLUTION_6B:
        return 6;
    case XY_HAL_ADC_RESOLUTION_8B:
        return 8;
    case XY_HAL_ADC_RESOLUTION_10B:
        return 10;
    case XY_HAL_ADC_RESOLUTION_12B:
        return 12;
    case XY_HAL_ADC_RESOLUTION_14B:
        return 14;
    case XY_HAL_ADC_RESOLUTION_16B:
        return 16;
    default:
        return 12;
    }
}

uint32_t xy_hal_adc_get_max_value(void *adc)
{
    int resolution = xy_hal_adc_get_resolution(adc);
    if (resolution < 0) {
        return 0;
    }
    return (1U << resolution) - 1;
}

uint32_t xy_hal_adc_value_to_mv(void *adc, uint32_t value, uint32_t vref)
{
    uint32_t max_value = xy_hal_adc_get_max_value(adc);
    if (max_value == 0) {
        return 0;
    }
    return (value * vref) / max_value;
}

xy_hal_error_t xy_hal_adc_enable_irq(void *adc, xy_hal_adc_event_t event)
{
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    switch (event) {
    case XY_HAL_ADC_EVENT_EOC:
        __HAL_ADC_ENABLE_IT((ADC_HandleTypeDef *)adc, ADC_IT_EOC);
        break;
    case XY_HAL_ADC_EVENT_OVR:
        __HAL_ADC_ENABLE_IT((ADC_HandleTypeDef *)adc, ADC_IT_OVR);
        break;
    case XY_HAL_ADC_EVENT_AWD:
        __HAL_ADC_ENABLE_IT((ADC_HandleTypeDef *)adc, ADC_IT_AWD);
        break;
    default:
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_disable_irq(void *adc, xy_hal_adc_event_t event)
{
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    switch (event) {
    case XY_HAL_ADC_EVENT_EOC:
        __HAL_ADC_DISABLE_IT((ADC_HandleTypeDef *)adc, ADC_IT_EOC);
        break;
    case XY_HAL_ADC_EVENT_OVR:
        __HAL_ADC_DISABLE_IT((ADC_HandleTypeDef *)adc, ADC_IT_OVR);
        break;
    case XY_HAL_ADC_EVENT_AWD:
        __HAL_ADC_DISABLE_IT((ADC_HandleTypeDef *)adc, ADC_IT_AWD);
        break;
    default:
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_register_callback(void *adc, xy_hal_adc_event_t event,
                                            xy_hal_adc_callback_t callback,
                                            void *arg)
{
    XY_UNUSED(event);
    if (!adc) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    adc_ctx_t *ctx = find_adc_ctx(adc);
    if (ctx == NULL) {
        ctx = alloc_adc_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->hadc = (ADC_HandleTypeDef *)adc;
    }

    ctx->callback = callback;
    ctx->arg      = arg;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_enable_temp_sensor(void)
{
    /* Enable temperature sensor for STM32U5 */
    __HAL_RCC_ADC_CONFIG(RCC_ADCCLKSOURCE_HSI);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_adc_disable_temp_sensor(void)
{
    return XY_HAL_OK;
}

/* ==================== HAL Callbacks ==================== */

/**
 * @brief ADC conversion complete callback
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    adc_ctx_t *ctx = find_adc_ctx(hadc);
    if (ctx && ctx->callback) {
        ctx->callback(hadc, XY_HAL_ADC_EVENT_EOC, ctx->arg);
    }
}

/**
 * @brief ADC overflow callback
 */
void HAL_ADC_LevelOutOfWindowCallback(ADC_HandleTypeDef *hadc)
{
    adc_ctx_t *ctx = find_adc_ctx(hadc);
    if (ctx && ctx->callback) {
        ctx->callback(hadc, XY_HAL_ADC_EVENT_AWD, ctx->arg);
    }
}

/**
 * @brief ADC error callback
 */
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
    adc_ctx_t *ctx = find_adc_ctx(hadc);
    if (ctx && ctx->callback) {
        ctx->callback(hadc, XY_HAL_ADC_EVENT_OVR, ctx->arg);
    }
}

#endif /* STM32U5 || STM32U5xx */
