/**
 * @file xy_hal_dma.c
 * @brief DMA HAL STM32U5 Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal_dma.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

/* DMA context structure */
typedef struct {
    DMA_HandleTypeDef *hdma;
    xy_hal_dma_callback_t callbacks[3];
    void *args[3];
    uint8_t initialized;
} dma_ctx_t;

/* Maximum DMA instances */
#define MAX_DMA_INSTANCES 16
static dma_ctx_t g_dma_ctx[MAX_DMA_INSTANCES];

/**
 * @brief Find DMA context by handle
 */
static dma_ctx_t *find_dma_ctx(void *dma)
{
    for (size_t i = 0; i < MAX_DMA_INSTANCES; i++) {
        if (g_dma_ctx[i].hdma == dma) {
            return &g_dma_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Allocate new DMA context
 */
static dma_ctx_t *alloc_dma_ctx(void)
{
    for (size_t i = 0; i < MAX_DMA_INSTANCES; i++) {
        if (g_dma_ctx[i].hdma == NULL) {
            return &g_dma_ctx[i];
        }
    }
    return NULL;
}

/**
 * @brief Convert XY DMA direction to STM32
 */
static uint32_t xy_to_stm32_direction(xy_hal_dma_direction_t dir)
{
    switch (dir) {
    case XY_HAL_DMA_DIR_PERIPH_TO_MEM:
        return DMA_PERIPH_TO_MEMORY;
    case XY_HAL_DMA_DIR_MEM_TO_PERIPH:
        return DMA_MEMORY_TO_PERIPH;
    case XY_HAL_DMA_DIR_MEM_TO_MEM:
        return DMA_MEMORY_TO_MEMORY;
    default:
        return DMA_PERIPH_TO_MEMORY;
    }
}

/**
 * @brief Convert XY DMA mode to STM32
 */
static uint32_t xy_to_stm32_mode(xy_hal_dma_mode_t mode)
{
    switch (mode) {
    case XY_HAL_DMA_MODE_NORMAL:
        return DMA_NORMAL;
    case XY_HAL_DMA_MODE_CIRCULAR:
        return DMA_CIRCULAR;
    default:
        return DMA_NORMAL;
    }
}

/**
 * @brief Convert XY DMA priority to STM32
 */
static uint32_t xy_to_stm32_priority(xy_hal_dma_priority_t priority)
{
    switch (priority) {
    case XY_HAL_DMA_PRIORITY_LOW:
        return DMA_PRIORITY_LOW;
    case XY_HAL_DMA_PRIORITY_MEDIUM:
        return DMA_PRIORITY_MEDIUM;
    case XY_HAL_DMA_PRIORITY_HIGH:
        return DMA_PRIORITY_HIGH;
    case XY_HAL_DMA_PRIORITY_VERY_HIGH:
        return DMA_PRIORITY_VERY_HIGH;
    default:
        return DMA_PRIORITY_LOW;
    }
}

/**
 * @brief Convert XY DMA width to STM32
 */
static uint32_t xy_to_stm32_width(xy_hal_dma_width_t width)
{
    switch (width) {
    case XY_HAL_DMA_WIDTH_BYTE:
        return DMA_PDATAALIGN_BYTE;
    case XY_HAL_DMA_WIDTH_HALFWORD:
        return DMA_PDATAALIGN_HALFWORD;
    case XY_HAL_DMA_WIDTH_WORD:
        return DMA_PDATAALIGN_WORD;
    default:
        return DMA_PDATAALIGN_BYTE;
    }
}

/**
 * @brief Convert XY DMA increment to STM32
 */
static uint32_t xy_to_stm32_incr(xy_hal_dma_incr_t incr)
{
    return (incr == XY_HAL_DMA_INCR_ENABLE) ? DMA_PINC_ENABLE : DMA_PINC_DISABLE;
}

xy_hal_error_t xy_hal_dma_init(void *dma, const xy_hal_dma_config_t *config)
{
    if (!dma || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    DMA_HandleTypeDef *hdma = (DMA_HandleTypeDef *)dma;

    /* Check if already initialized */
    dma_ctx_t *ctx = find_dma_ctx(dma);
    if (ctx != NULL && ctx->initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    /* Allocate context if not exists */
    if (ctx == NULL) {
        ctx = alloc_dma_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    hdma->Init.Direction     = xy_to_stm32_direction(config->direction);
    hdma->Init.Mode          = xy_to_stm32_mode(config->mode);
    hdma->Init.Priorty       = xy_to_stm32_priority(config->priority);
    hdma->Init.PeriphInc     = xy_to_stm32_incr(config->periph_incr);
    hdma->Init.MemInc        = xy_to_stm32_incr(config->mem_incr);
    hdma->Init.PeriphDataAlignment = xy_to_stm32_width(config->periph_width);
    hdma->Init.MemDataAlignment    = xy_to_stm32_width(config->mem_width);

    if (HAL_DMA_Init(hdma) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    /* Initialize context */
    ctx->hdma        = hdma;
    memset(ctx->callbacks, 0, sizeof(ctx->callbacks));
    memset(ctx->args, 0, sizeof(ctx->args));
    ctx->initialized = 1;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dma_deinit(void *dma)
{
    if (!dma) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    dma_ctx_t *ctx = find_dma_ctx(dma);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_DMA_DeInit((DMA_HandleTypeDef *)dma) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->initialized = 0;
    ctx->hdma        = NULL;
    memset(ctx->callbacks, 0, sizeof(ctx->callbacks));
    memset(ctx->args, 0, sizeof(ctx->args));

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dma_start(void *dma, uint32_t src_addr, uint32_t dst_addr,
                                size_t data_len)
{
    if (!dma || data_len == 0) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    dma_ctx_t *ctx = find_dma_ctx(dma);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    HAL_StatusTypeDef status = HAL_DMA_Start((DMA_HandleTypeDef *)dma, src_addr,
                                             dst_addr, (uint16_t)data_len);

    if (status != HAL_OK) {
        return XY_HAL_ERROR_BUSY;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dma_stop(void *dma)
{
    if (!dma) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    dma_ctx_t *ctx = find_dma_ctx(dma);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_DMA_Abort((DMA_HandleTypeDef *)dma) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_dma_register_callback(void *dma, xy_hal_dma_event_t event,
                                            xy_hal_dma_callback_t callback,
                                            void *arg)
{
    if (!dma) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    dma_ctx_t *ctx = find_dma_ctx(dma);
    if (ctx == NULL) {
        ctx = alloc_dma_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->hdma = (DMA_HandleTypeDef *)dma;
    }

    /* Map event to callback index */
    size_t idx = (size_t)event;
    if (idx >= 3) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    ctx->callbacks[idx] = callback;
    ctx->args[idx]      = arg;

    return XY_HAL_OK;
}

int xy_hal_dma_get_counter(void *dma)
{
    if (!dma) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    return (int)((DMA_HandleTypeDef *)dma)->Instance->CNDTR;
}

xy_hal_error_t xy_hal_dma_poll_complete(void *dma, uint32_t timeout)
{
    if (!dma) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    dma_ctx_t *ctx = find_dma_ctx(dma);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    uint32_t start = HAL_GetTick();
    while (((DMA_HandleTypeDef *)dma)->Instance->CNDTR != 0) {
        if ((HAL_GetTick() - start) >= timeout) {
            return XY_HAL_ERROR_TIMEOUT;
        }
    }

    return XY_HAL_OK;
}

/* ==================== HAL Callbacks ==================== */

/**
 * @brief DMA TX complete callback
 */
void HAL_DMA_TC_Callback(DMA_HandleTypeDef *hdma)
{
    dma_ctx_t *ctx = find_dma_ctx(hdma);
    if (ctx && ctx->callbacks[0]) {
        ctx->callbacks[0](hdma, XY_HAL_DMA_EVENT_COMPLETE, ctx->args[0]);
    }
}

/**
 * @brief DMA half transfer callback
 */
void HAL_DMA_HalfTransferCplt_Callback(DMA_HandleTypeDef *hdma)
{
    dma_ctx_t *ctx = find_dma_ctx(hdma);
    if (ctx && ctx->callbacks[1]) {
        ctx->callbacks[1](hdma, XY_HAL_DMA_EVENT_HALF_COMPLETE, ctx->args[1]);
    }
}

/**
 * @brief DMA error callback
 */
void HAL_DMA_ErrorCallback(DMA_HandleTypeDef *hdma)
{
    dma_ctx_t *ctx = find_dma_ctx(hdma);
    if (ctx && ctx->callbacks[2]) {
        ctx->callbacks[2](hdma, XY_HAL_DMA_EVENT_ERROR, ctx->args[2]);
    }
}

#endif /* STM32U5 || STM32U5xx */
