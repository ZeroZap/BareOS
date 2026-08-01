#include "xy_hal_n32l40x_internal.h"

static uint32_t dma_width(xy_hal_dma_width_t width, bool mem)
{
    if (mem) {
        if (width == XY_HAL_DMA_WIDTH_16BIT) return DMA_MemoryDataSize_HalfWord;
        if (width == XY_HAL_DMA_WIDTH_32BIT) return DMA_MemoryDataSize_Word;
        return DMA_MemoryDataSize_Byte;
    }
    if (width == XY_HAL_DMA_WIDTH_16BIT) return DMA_PERIPH_DATA_SIZE_HALFWORD;
    if (width == XY_HAL_DMA_WIDTH_32BIT) return DMA_PERIPH_DATA_SIZE_WORD;
    return DMA_PERIPH_DATA_SIZE_BYTE;
}

static uint32_t dma_priority(xy_hal_dma_priority_t priority)
{
    switch (priority) {
    case XY_HAL_DMA_PRIORITY_MEDIUM: return DMA_PRIORITY_MEDIUM;
    case XY_HAL_DMA_PRIORITY_HIGH: return DMA_PRIORITY_HIGH;
    case XY_HAL_DMA_PRIORITY_VERY_HIGH: return DMA_PRIORITY_VERY_HIGH;
    default: return DMA_PRIORITY_LOW;
    }
}

int xy_hal_dma_init(void *dma, const xy_hal_dma_config_t *config)
{
    DMA_ChannelType *d = (DMA_ChannelType *)dma;
    DMA_InitType init;
    if (!d || !config) return XY_HAL_INVALID_PARAM;
    RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_DMA, ENABLE);
    DMA_DeInit(d);
    memset(&init, 0, sizeof(init));
    init.Direction = config->direction == XY_HAL_DMA_MEM_TO_PERIPH ? DMA_DIR_PERIPH_DST : DMA_DIR_PERIPH_SRC;
    init.PeriphInc = config->periph_inc ? DMA_PERIPH_INC_ENABLE : DMA_PERIPH_INC_DISABLE;
    init.DMA_MemoryInc = config->mem_inc ? DMA_MEM_INC_ENABLE : DMA_MEM_INC_DISABLE;
    init.PeriphDataSize = dma_width(config->periph_width, false);
    init.MemDataSize = dma_width(config->mem_width, true);
    init.CircularMode = config->circular ? DMA_MODE_CIRCULAR : DMA_MODE_NORMAL;
    init.Priority = dma_priority(config->priority);
    init.Mem2Mem = config->direction == XY_HAL_DMA_MEM_TO_MEM ? DMA_M2M_ENABLE : DMA_M2M_DISABLE;
    DMA_Init(d, &init);
    (void)config->request_id;
    return XY_HAL_OK;
}

int xy_hal_dma_deinit(void *dma)
{
    if (!dma) return XY_HAL_INVALID_PARAM;
    DMA_DeInit((DMA_ChannelType *)dma);
    return XY_HAL_OK;
}

int xy_hal_dma_start(void *dma, const xy_hal_dma_transfer_t *transfer)
{
    DMA_ChannelType *d = (DMA_ChannelType *)dma;
    if (!d || !transfer) return XY_HAL_INVALID_PARAM;
    DMA_EnableChannel(d, DISABLE);
    d->TXNUM = (uint32_t)transfer->len;
    d->PADDR = (uint32_t)transfer->periph_addr;
    d->MADDR = (uint32_t)transfer->mem_addr;
    DMA_EnableChannel(d, ENABLE);
    return XY_HAL_OK;
}

int xy_hal_dma_stop(void *dma)
{
    if (!dma) return XY_HAL_INVALID_PARAM;
    DMA_EnableChannel((DMA_ChannelType *)dma, DISABLE);
    return XY_HAL_OK;
}

int xy_hal_dma_get_remaining(void *dma, size_t *remaining)
{
    if (!dma || !remaining) return XY_HAL_INVALID_PARAM;
    *remaining = DMA_GetCurrDataCounter((DMA_ChannelType *)dma);
    return XY_HAL_OK;
}

int xy_hal_dma_set_callback(void *dma, xy_hal_event_cb_t cb, void *user)
{
    xy_hal_n32_dev_cb_state_t *st = xy_hal_n32_dma_state((DMA_ChannelType *)dma);
    if (!st) return XY_HAL_INVALID_PARAM;
    st->dev = dma;
    st->cb = cb;
    st->user = user;
    DMA_ConfigInt((DMA_ChannelType *)dma, DMA_INT_TXC | DMA_INT_HTX | DMA_INT_ERR, cb ? ENABLE : DISABLE);
    return XY_HAL_OK;
}

void xy_hal_dma_irq_handler(void *dma)
{
    xy_hal_n32_dev_cb_state_t *st = xy_hal_n32_dma_state((DMA_ChannelType *)dma);
    if (st && st->cb) st->cb(dma, XY_HAL_EVENT_DMA_DONE, st->user);
}
