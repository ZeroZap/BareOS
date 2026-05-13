/**
 * @file xy_hal_i2c.c
 * @brief I2C HAL STM32U5 Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal_i2c.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

/* I2C context structure */
typedef struct {
    I2C_HandleTypeDef *hi2c;
    xy_hal_i2c_callback_t callback;
    void *arg;
    uint8_t initialized;
} i2c_ctx_t;

/* Maximum I2C instances */
#define MAX_I2C_INSTANCES 4
static i2c_ctx_t g_i2c_ctx[MAX_I2C_INSTANCES];

/**
 * @brief Find I2C context by handle
 */
static i2c_ctx_t *find_i2c_ctx(void *i2c)
{
    for (size_t i = 0; i < MAX_I2C_INSTANCES; i++) {
        if (g_i2c_ctx[i].hi2c == i2c) {
            return &g_i2c_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Allocate new I2C context
 */
static i2c_ctx_t *alloc_i2c_ctx(void)
{
    for (size_t i = 0; i < MAX_I2C_INSTANCES; i++) {
        if (g_i2c_ctx[i].hi2c == NULL) {
            return &g_i2c_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Convert XY address mode to STM32
 */
static uint32_t xy_to_stm32_addr_mode(xy_hal_i2c_addr_mode_t addr_mode)
{
    switch (addr_mode) {
    case XY_HAL_I2C_ADDR_7BIT:
        return I2C_ADDRESSINGMODE_7BIT;
    case XY_HAL_I2C_ADDR_10BIT:
        return I2C_ADDRESSINGMODE_10BIT;
    default:
        return I2C_ADDRESSINGMODE_7BIT;
    }
}

/**
 * @brief Convert XY duty cycle to STM32
 */
static uint32_t xy_to_stm32_duty(xy_hal_i2c_duty_t duty)
{
    switch (duty) {
    case XY_HAL_I2C_DUTY_2:
        return I2C_DUTY_2;
    case XY_HAL_I2C_DUTY_16_9:
        return I2C_DUTY_16_9;
    default:
        return I2C_DUTY_2;
    }
}

xy_hal_error_t xy_hal_i2c_init(void *i2c, const xy_hal_i2c_config_t *config)
{
    if (!i2c || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)i2c;

    /* Check if already initialized */
    i2c_ctx_t *ctx = find_i2c_ctx(i2c);
    if (ctx != NULL && ctx->initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    /* Allocate context if not exists */
    if (ctx == NULL) {
        ctx = alloc_i2c_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    hi2c->Init.Timing           = __LL_I2C_CONVERT_TIMINGS(
        I2C_PRESC_MAX, I2C_SCLDEL_MAX, I2C_SDADEL_MAX, I2C_SCLH_MAX, I2C_SCLL_MAX);
    hi2c->Init.OwnAddress1      = config->own_address;
    hi2c->Init.Address1Mode     = xy_to_stm32_addr_mode(config->addr_mode);
    hi2c->Init.OwnAddress2      = 0;
    hi2c->Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c->Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
    hi2c->Init.GeneralCallMode  = config->general_call_mode ?
                                   I2C_GENERALCALL_ENABLE : I2C_GENERALCALL_DISABLE;
    hi2c->Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(hi2c) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    /* Configure analog and digital filters */
    if (HAL_I2CEx_ConfigAnalogFilter(hi2c, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    if (HAL_I2CEx_ConfigDigitalFilter(hi2c, 0) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    /* Initialize context */
    ctx->hi2c        = hi2c;
    ctx->callback    = NULL;
    ctx->arg         = NULL;
    ctx->initialized = 1;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_deinit(void *i2c)
{
    if (!i2c) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    i2c_ctx_t *ctx = find_i2c_ctx(i2c);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_I2C_DeInit((I2C_HandleTypeDef *)i2c) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->initialized = 0;
    ctx->hi2c        = NULL;
    ctx->callback    = NULL;
    ctx->arg         = NULL;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_transmit(void *i2c, uint16_t dev_addr,
                                          const uint8_t *data, size_t len,
                                          uint32_t timeout)
{
    if (!i2c || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    i2c_ctx_t *ctx = find_i2c_ctx(i2c);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        (I2C_HandleTypeDef *)i2c, dev_addr, (uint8_t *)data, (uint16_t)len, timeout);

    if (status != HAL_OK) {
        if (status == HAL_TIMEOUT) {
            return XY_HAL_ERROR_TIMEOUT;
        }
        return XY_HAL_ERROR_IO;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_receive(void *i2c, uint16_t dev_addr,
                                         uint8_t *data, size_t len,
                                         uint32_t timeout)
{
    if (!i2c || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    i2c_ctx_t *ctx = find_i2c_ctx(i2c);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_I2C_Master_Receive(
        (I2C_HandleTypeDef *)i2c, dev_addr, data, (uint16_t)len, timeout);

    if (status != HAL_OK) {
        if (status == HAL_TIMEOUT) {
            return XY_HAL_ERROR_TIMEOUT;
        }
        return XY_HAL_ERROR_IO;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_mem_write(void *i2c, uint16_t dev_addr,
                                    uint16_t reg_addr, const uint8_t *data,
                                    size_t len, uint32_t timeout)
{
    if (!i2c || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    i2c_ctx_t *ctx = find_i2c_ctx(i2c);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_I2C_Mem_Write(
        (I2C_HandleTypeDef *)i2c, dev_addr, reg_addr,
        I2C_MEMADD_SIZE_8BIT, (uint8_t *)data, (uint16_t)len, timeout);

    if (status != HAL_OK) {
        if (status == HAL_TIMEOUT) {
            return XY_HAL_ERROR_TIMEOUT;
        }
        return XY_HAL_ERROR_IO;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_mem_read(void *i2c, uint16_t dev_addr,
                                   uint16_t reg_addr, uint8_t *data, size_t len,
                                   uint32_t timeout)
{
    if (!i2c || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    i2c_ctx_t *ctx = find_i2c_ctx(i2c);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(
        (I2C_HandleTypeDef *)i2c, dev_addr, reg_addr,
        I2C_MEMADD_SIZE_8BIT, data, (uint16_t)len, timeout);

    if (status != HAL_OK) {
        if (status == HAL_TIMEOUT) {
            return XY_HAL_ERROR_TIMEOUT;
        }
        return XY_HAL_ERROR_IO;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_transmit_dma(void *i2c, uint16_t dev_addr,
                                              const uint8_t *data, size_t len)
{
    if (!i2c || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    i2c_ctx_t *ctx = find_i2c_ctx(i2c);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit_DMA(
        (I2C_HandleTypeDef *)i2c, dev_addr, (uint8_t *)data, (uint16_t)len);

    if (status != HAL_OK) {
        return XY_HAL_ERROR_BUSY;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_master_receive_dma(void *i2c, uint16_t dev_addr,
                                             uint8_t *data, size_t len)
{
    if (!i2c || !data || len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    i2c_ctx_t *ctx = find_i2c_ctx(i2c);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_I2C_Master_Receive_DMA(
        (I2C_HandleTypeDef *)i2c, dev_addr, data, (uint16_t)len);

    if (status != HAL_OK) {
        return XY_HAL_ERROR_BUSY;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_register_callback(void *i2c,
                                            xy_hal_i2c_callback_t callback,
                                            void *arg)
{
    if (!i2c) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    i2c_ctx_t *ctx = find_i2c_ctx(i2c);
    if (ctx == NULL) {
        ctx = alloc_i2c_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_MEMORY;
        }
        ctx->hi2c = (I2C_HandleTypeDef *)i2c;
    }

    ctx->callback = callback;
    ctx->arg      = arg;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_is_device_ready(void *i2c, uint16_t dev_addr,
                                          uint32_t trials, uint32_t timeout)
{
    if (!i2c) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    i2c_ctx_t *ctx = find_i2c_ctx(i2c);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(
        (I2C_HandleTypeDef *)i2c, dev_addr, trials, timeout);

    if (status != HAL_OK) {
        if (status == HAL_TIMEOUT) {
            return XY_HAL_ERROR_TIMEOUT;
        }
        return XY_HAL_ERROR_IO;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_error(void *i2c)
{
    if (!i2c) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    /* Placeholder for I2C error pin control if needed */
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_i2c_set_error_cb(void *i2c, xy_hal_i2c_callback_t callback,
                                       void *arg)
{
    return xy_hal_i2c_register_callback(i2c, callback, arg);
}

/* ==================== HAL Callbacks ==================== */

/**
 * @brief I2C TX complete callback
 */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    i2c_ctx_t *ctx = find_i2c_ctx(hi2c);
    if (ctx && ctx->callback) {
        ctx->callback(hi2c, XY_HAL_I2C_EVENT_TX_DONE, ctx->arg);
    }
}

/**
 * @brief I2C RX complete callback
 */
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    i2c_ctx_t *ctx = find_i2c_ctx(hi2c);
    if (ctx && ctx->callback) {
        ctx->callback(hi2c, XY_HAL_I2C_EVENT_RX_DONE, ctx->arg);
    }
}

/**
 * @brief I2C error callback
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    i2c_ctx_t *ctx = find_i2c_ctx(hi2c);
    if (ctx && ctx->callback) {
        ctx->callback(hi2c, XY_HAL_I2C_EVENT_ERROR, ctx->arg);
    }
}

#endif /* STM32U5 || STM32U5xx */
