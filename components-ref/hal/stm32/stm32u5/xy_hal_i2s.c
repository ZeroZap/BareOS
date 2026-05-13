/**
 * @file xy_hal_i2s.c
 * @brief I2S HAL STM32U5 Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal_i2s.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

/* I2S context structure */
typedef struct {
    I2S_HandleTypeDef *hi2s;
    xy_hal_i2s_callback_t callback;
    void *arg;
    uint8_t initialized;
} i2s_ctx_t;

#define MAX_I2S_INSTANCES 4
static i2s_ctx_t g_i2s_ctx[MAX_I2S_INSTANCES];

static i2s_ctx_t *find_i2s_ctx(void *i2s)
{
    for (size_t i = 0; i < MAX_I2S_INSTANCES; i++) {
        if (g_i2s_ctx[i].hi2s == i2s) {
            return &g_i2s_ctx[i];
        }
    }
    return NULL;
}

static i2s_ctx_t *alloc_i2s_ctx(void)
{
    for (size_t i = 0; i < MAX_I2S_INSTANCES; i++) {
        if (g_i2s_ctx[i].hi2s == NULL) {
            return &g_i2s_ctx[i];
        }
    }
    return NULL;
}

xy_hal_error_t xy_hal_i2s_init(void *i2s, const xy_hal_i2s_config_t *config)
{
    if (!i2s || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    I2S_HandleTypeDef *hi2s = (I2S_HandleTypeDef *)i2s;

    i2s_ctx_t *ctx = find_i2s_ctx(i2s);
    if (ctx != NULL && ctx->initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    if (ctx == NULL) {
        ctx = alloc_i2s_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    /* Configure I2S */
    hi2s->Init.Mode               = (config->mode == XY_HAL_I2S_MODE_MASTER_TX ||
                                      config->mode == XY_HAL_I2S_MODE_MASTER_RX) ?
                                     I2S_MODE_MASTER_TX : I2S_MODE_SLAVE_TX;
    hi2s->Init.Standard           = I2S_STANDARD_PHILIPS;
    hi2s->Init.DataFormat         = I2S_DATAFORMAT_16B;
    hi2s->Init.MCLKOutput         = I2S_MCLKOUTPUT_ENABLE;
    hi2s->Init.AudioFreq          = I2S_AUDIOFREQ_44K;
    hi2s->Init.CPOL               = I2S_CPOL_LOW;
    hi2s->Init.ClockSource        = I2S_CLOCK_PLL;
    hi2s->Init.FullDuplexMode     = I2S_FULLDUPLEXMODE_DISABLE;

    if (HAL_I2S_Init(hi2s) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->hi2s        = hi2s;
    ctx->callback    = NULL;
    ctx->arg         = NULL;
    ctx->initialized = 1;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2s_deinit(void *i2s)
{
    if (!i2s) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    i2s_ctx_t *ctx = find_i2s_ctx(i2s);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_I2S_DeInit((I2S_HandleTypeDef *)i2s) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->initialized = 0;
    ctx->hi2s        = NULL;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2s_transmit(void *i2s, const uint8_t *data, size_t len,
                                   uint32_t timeout)
{
    if (!i2s || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    i2s_ctx_t *ctx = find_i2s_ctx(i2s);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_I2S_Transmit((I2S_HandleTypeDef *)i2s, (uint16_t *)data,
                         (uint16_t)len, timeout) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2s_receive(void *i2s, uint8_t *data, size_t len,
                                  uint32_t timeout)
{
    if (!i2s || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    i2s_ctx_t *ctx = find_i2s_ctx(i2s);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_I2S_Receive((I2S_HandleTypeDef *)i2s, (uint16_t *)data,
                        (uint16_t)len, timeout) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2s_transmit_dma(void *i2s, const uint8_t *data, size_t len)
{
    if (!i2s || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    i2s_ctx_t *ctx = find_i2s_ctx(i2s);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_I2S_Transmit_DMA((I2S_HandleTypeDef *)i2s, (uint16_t *)data,
                             (uint16_t)len) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2s_receive_dma(void *i2s, uint8_t *data, size_t len)
{
    if (!i2s || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    i2s_ctx_t *ctx = find_i2s_ctx(i2s);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_I2S_Receive_DMA((I2S_HandleTypeDef *)i2s, (uint16_t *)data,
                            (uint16_t)len) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2s_register_callback(void *i2s,
                                            xy_hal_i2s_callback_t callback,
                                            void *arg)
{
    if (!i2s) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    i2s_ctx_t *ctx = find_i2s_ctx(i2s);
    if (ctx == NULL) {
        ctx = alloc_i2s_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->hi2s = (I2S_HandleTypeDef *)i2s;
    }

    ctx->callback = callback;
    ctx->arg      = arg;

    return XY_HAL_OK;
}

/* HAL Callbacks */
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    i2s_ctx_t *ctx = find_i2s_ctx(hi2s);
    if (ctx && ctx->callback) {
        ctx->callback(hi2s, XY_HAL_I2S_EVENT_TX_HALF_DONE, ctx->arg);
    }
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    i2s_ctx_t *ctx = find_i2s_ctx(hi2s);
    if (ctx && ctx->callback) {
        ctx->callback(hi2s, XY_HAL_I2S_EVENT_TX_DONE, ctx->arg);
    }
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    i2s_ctx_t *ctx = find_i2s_ctx(hi2s);
    if (ctx && ctx->callback) {
        ctx->callback(hi2s, XY_HAL_I2S_EVENT_RX_DONE, ctx->arg);
    }
}

void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
    i2s_ctx_t *ctx = find_i2s_ctx(hi2s);
    if (ctx && ctx->callback) {
        ctx->callback(hi2s, XY_HAL_I2S_EVENT_ERROR, ctx->arg);
    }
}

#endif /* STM32U5 || STM32U5xx */
