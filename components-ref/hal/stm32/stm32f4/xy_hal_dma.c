/**
 * @file xy_hal_dma_stm32.c
 * @brief DMA HAL STM32 Implementation
 * @version 1.0
 * @date 2025-10-26
 */

#include "../inc/xy_hal_dma.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"

static uint32_t xy_to_stm32_direction(xy_hal_dma_direction_t direction)
{
    switch (direction) {
    case XY_HAL_DMA_DIR_PERIPH_TO_MEM:
        return XY_HAL_DMA_DIR_PERIPH_TO_MEM;
    case XY_HAL_DMA_DIR_MEM_TO_PERIPH:
        return XY_HAL_DMA_DIR_MEM_TO_PERIPH;
    case XY_HAL_DMA_DIR_MEM_TO_MEM:
        return XY_HAL_DMA_DIR_MEM_TO_MEM;
    default:
        return XY_HAL_DMA_DIR_PERIPH_TO_MEM;
    }
}

static uint32_t xy_to_stm32_mode(xy_hal_dma_mode_t mode)
{
    return (mode == XY_HAL_DMA_MODE_CIRCULAR) ? XY_HAL_DMA_MODE_CIRCULAR : XY_HAL_DMA_MODE_NORMAL;
}

static uint32_t xy_to_stm32_priority(xy_hal_dma_priority_t priority)
{
    switch (priority) {
    case XY_HAL_DMA_PRIORITY_LOW:
        return XY_HAL_DMA_PRIORITY_LOW;
    case XY_HAL_DMA_PRIORITY_MEDIUM:
        return XY_HAL_DMA_PRIORITY_MEDIUM;
    case XY_HAL_DMA_PRIORITY_HIGH:
        return XY_HAL_DMA_PRIORITY_HIGH;
    case XY_HAL_DMA_PRIORITY_VERY_HIGH:
        return XY_HAL_DMA_PRIORITY_VERY_HIGH;
    default:
        return XY_HAL_DMA_PRIORITY_LOW;
    }
}

static uint32_t xy_to_stm32_width(xy_hal_dma_width_t width)
{
    switch (width) {
    case XY_HAL_DMA_WIDTH_BYTE:
        return XY_HAL_DMA_PDATAALIGN_BYTE;
    case XY_HAL_DMA_WIDTH_HALFWORD:
        return XY_HAL_DMA_PDATAALIGN_HALFWORD;
    case XY_HAL_DMA_WIDTH_WORD:
        return XY_HAL_DMA_PDATAALIGN_WORD;
    default:
        return XY_HAL_DMA_PDATAALIGN_BYTE;
    }
}

static uint32_t xy_to_stm32_incr(xy_hal_dma_incr_t incr)
{
    return (incr == XY_HAL_DMA_INCR_ENABLE) ? DMA_PINC_ENABLE
                                            : DMA_PINC_DISABLE;
}

int xy_hal_dma_init(void *dma, const xy_hal_dma_config_t *config)
{
    if (!dma || !config) {
        return -1;
    }

    DMA_HandleTypeDef *hdma = (DMA_HandleTypeDef *)dma;

    hdma->Init.Direction = xy_to_stm32_direction(config->direction);
    hdma->Init.Mode      = xy_to_stm32_mode(config->mode);
    hdma->Init.Priority  = xy_to_stm32_priority(config->priority);

    hdma->Init.PeriphDataAlignment = xy_to_stm32_width(config->periph_width);
    hdma->Init.MemDataAlignment    = xy_to_stm32_width(config->mem_width);

    hdma->Init.PeriphInc = xy_to_stm32_incr(config->periph_incr);
    hdma->Init.MemInc    = xy_to_stm32_incr(config->mem_incr);

    if (HAL_DMA_Init(hdma) != HAL_OK) {
        return -1;
    }

    return 0;
}

int xy_hal_dma_deinit(void *dma)
{
    if (!dma) {
        return -1;
    }

    if (HAL_DMA_DeInit((DMA_HandleTypeDef *)dma) != HAL_OK) {
        return -1;
    }

    return 0;
}

int xy_hal_dma_start(void *dma, uint32_t src_addr, uint32_t dst_addr,
                     size_t data_len)
{
    if (!dma) {
        return -1;
    }

    if (HAL_DMA_Start((DMA_HandleTypeDef *)dma, src_addr, dst_addr, data_len)
        != HAL_OK) {
        return -1;
    }

    return 0;
}

int xy_hal_dma_stop(void *dma)
{
    if (!dma) {
        return -1;
    }

    if (HAL_DMA_Abort((DMA_HandleTypeDef *)dma) != HAL_OK) {
        return -1;
    }

    return 0;
}

int xy_hal_dma_register_callback(void *dma, xy_hal_dma_event_t event,
                                 xy_hal_dma_callback_t callback, void *arg)
{
    /* Store callback in context */
    return 0;
}

int xy_hal_dma_get_counter(void *dma)
{
    if (!dma) {
        return -1;
    }

    DMA_HandleTypeDef *hdma = (DMA_HandleTypeDef *)dma;

#if defined(DMA_SxNDT_NDT)
    /* STM32F4/F7 style */
    return (int)hdma->Instance->NDTR;
#else
    /* STM32F1/F0 style */
    return (int)hdma->Instance->CNDTR;
#endif
}

int xy_hal_dma_poll_complete(void *dma, uint32_t timeout)
{
    if (!dma) {
        return -1;
    }

    if (HAL_DMA_PollForTransfer(
            (DMA_HandleTypeDef *)dma, HAL_DMA_FULL_TRANSFER, timeout)
        != HAL_OK) {
        return -1;
    }

    return 0;
}

#endif /* STM32_HAL_ENABLED */
