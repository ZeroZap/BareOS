/**
 * @file xy_hal_can.c
 * @brief CAN HAL STM32U5 Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal_can.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

/* CAN context structure */
typedef struct {
    CAN_HandleTypeDef *hcan;
    xy_hal_can_callback_t callback;
    void *arg;
    uint8_t initialized;
} can_ctx_t;

#define MAX_CAN_INSTANCES 2
static can_ctx_t g_can_ctx[MAX_CAN_INSTANCES];

static can_ctx_t *find_can_ctx(void *can)
{
    for (size_t i = 0; i < MAX_CAN_INSTANCES; i++) {
        if (g_can_ctx[i].hcan == can) {
            return &g_can_ctx[i];
        }
    }
    return NULL;
}

static can_ctx_t *alloc_can_ctx(void)
{
    for (size_t i = 0; i < MAX_CAN_INSTANCES; i++) {
        if (g_can_ctx[i].hcan == NULL) {
            return &g_can_ctx[i];
        }
    }
    return NULL;
}

xy_hal_error_t xy_hal_can_init(void *can, const xy_hal_can_config_t *config)
{
    if (!can || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    CAN_HandleTypeDef *hcan = (CAN_HandleTypeDef *)can;

    can_ctx_t *ctx = find_can_ctx(can);
    if (ctx != NULL && ctx->initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    if (ctx == NULL) {
        ctx = alloc_can_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
    }

    /* Configure CAN */
    hcan->Init.Prescaler = config->prescaler;
    hcan->Init.Mode      = CAN_MODE_NORMAL;
    hcan->Init.SyncJumpWidth = CAN_SJW_1TQ;
    hcan->Init.TimeSeg1  = CAN_BS1_13TQ;
    hcan->Init.TimeSeg2  = CAN_BS2_2TQ;
    hcan->Init.TimeTriggeredMode = DISABLE;
    hcan->Init.AutoBusOff      = DISABLE;
    hcan->Init.AutoWakeUp      = DISABLE;
    hcan->Init.AutoRetransmission = ENABLE;
    hcan->Init.ReceiveFifoLocked  = DISABLE;
    hcan->Init.TransmitFifoPriority = DISABLE;

    if (HAL_CAN_Init(hcan) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->hcan        = hcan;
    ctx->callback    = NULL;
    ctx->arg         = NULL;
    ctx->initialized = 1;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_can_deinit(void *can)
{
    if (!can) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    can_ctx_t *ctx = find_can_ctx(can);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_CAN_DeInit((CAN_HandleTypeDef *)can) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    ctx->initialized = 0;
    ctx->hcan        = NULL;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_can_start(void *can)
{
    if (!can) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    can_ctx_t *ctx = find_can_ctx(can);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_CAN_Start((CAN_HandleTypeDef *)can) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_can_stop(void *can)
{
    if (!can) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    can_ctx_t *ctx = find_can_ctx(can);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (HAL_CAN_Stop((CAN_HandleTypeDef *)can) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_can_send(void *can, const xy_hal_can_msg_t *msg,
                               uint32_t timeout)
{
    if (!can || !msg) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    can_ctx_t *ctx = find_can_ctx(can);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    CAN_TxHeaderTypeDef tx_header = { 0 };
    tx_header.StdId = msg->frame_type == XY_HAL_CAN_FRAME_STD ? msg->id : 0;
    tx_header.ExtId = msg->frame_type == XY_HAL_CAN_FRAME_EXT ? msg->id : 0;
    tx_header.IDE   = msg->frame_type == XY_HAL_CAN_FRAME_EXT ?
                      CAN_ID_EXT : CAN_ID_STD;
    tx_header.RTR   = msg->data_type == XY_HAL_CAN_REMOTE_FRAME ?
                      CAN_RTR_REMOTE : CAN_RTR_DATA;
    tx_header.DLC   = msg->dlc;

    uint32_t tx_mailbox;
    if (HAL_CAN_AddTxMessage((CAN_HandleTypeDef *)can, &tx_header,
                             (uint8_t *)msg->data, &tx_mailbox) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    /* Wait for transmission */
    uint32_t start = HAL_GetTick();
    while (HAL_CAN_IsTxMessagePending((CAN_HandleTypeDef *)can, tx_mailbox)) {
        if ((HAL_GetTick() - start) >= timeout) {
            HAL_CAN_AbortTxRequest((CAN_HandleTypeDef *)can, tx_mailbox);
            return XY_HAL_ERROR_TIMEOUT;
        }
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_can_receive(void *can, xy_hal_can_msg_t *msg,
                                  xy_hal_can_fifo_t fifo, uint32_t timeout)
{
    if (!can || !msg) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    can_ctx_t *ctx = find_can_ctx(can);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    CAN_RxFifo0MsgHeaderTypeDef rx_header = { 0 };
    uint32_t start = HAL_GetTick();

    while (HAL_CAN_GetRxFifoFillLevel((CAN_HandleTypeDef *)can, CAN_RX_FIFO0) == 0) {
        if ((HAL_GetTick() - start) >= timeout) {
            return XY_HAL_ERROR_TIMEOUT;
        }
    }

    if (HAL_CAN_GetRxMessage((CAN_HandleTypeDef *)can, CAN_RX_FIFO0,
                             &rx_header, msg->data) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    msg->id         = (rx_header.IDE == CAN_ID_EXT) ? rx_header.ExtId : rx_header.StdId;
    msg->frame_type = (rx_header.IDE == CAN_ID_EXT) ?
                      XY_HAL_CAN_FRAME_EXT : XY_HAL_CAN_FRAME_STD;
    msg->data_type  = (rx_header.RTR == CAN_RTR_REMOTE) ?
                      XY_HAL_CAN_REMOTE_FRAME : XY_HAL_CAN_DATA_FRAME;
    msg->dlc        = rx_header.DLC;
    msg->fifo       = 0;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_can_register_callback(void *can,
                                            xy_hal_can_callback_t callback,
                                            void *arg)
{
    if (!can) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    can_ctx_t *ctx = find_can_ctx(can);
    if (ctx == NULL) {
        ctx = alloc_can_ctx();
        if (ctx == NULL) {
            return XY_HAL_ERROR_NO_RESOURCE;
        }
        ctx->hcan = (CAN_HandleTypeDef *)can;
    }

    ctx->callback = callback;
    ctx->arg      = arg;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_can_set_filter(void *can, const xy_hal_can_filter_config_t *filter_cfg)
{
    if (!can || !filter_cfg) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    can_ctx_t *ctx = find_can_ctx(can);
    if (ctx == NULL || !ctx->initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    CAN_FilterTypeDef hal_filter_cfg = { 0 };
    hal_filter_cfg.FilterBank       = filter_cfg->bank_number;
    hal_filter_cfg.FilterMode       = CAN_FILTERMODE_IDMASK;
    hal_filter_cfg.FilterScale      = CAN_FILTERSCALE_32BIT;
    hal_filter_cfg.FilterIdHigh     = (filter_cfg->filter_id >> 16) & 0xFFFF;
    hal_filter_cfg.FilterIdLow      = filter_cfg->filter_id & 0xFFFF;
    hal_filter_cfg.FilterMaskIdHigh = (filter_cfg->filter_mask >> 16) & 0xFFFF;
    hal_filter_cfg.FilterMaskIdLow  = filter_cfg->filter_mask & 0xFFFF;
    hal_filter_cfg.FilterFIFOAssignment = filter_cfg->fifo_assignment == 0 ?
                                         CAN_FILTER_FIFO0 : CAN_FILTER_FIFO1;
    hal_filter_cfg.FilterActivation = ENABLE;

    if (HAL_CAN_ConfigFilter((CAN_HandleTypeDef *)can, &hal_filter_cfg) != HAL_OK) {
        return XY_HAL_ERROR_FAIL;
    }

    return XY_HAL_OK;
}

/* HAL Callbacks */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    can_ctx_t *ctx = find_can_ctx(hcan);
    if (ctx && ctx->callback) {
        ctx->callback(hcan, XY_HAL_CAN_EVENT_RX_PENDING, ctx->arg);
    }
}

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
    can_ctx_t *ctx = find_can_ctx(hcan);
    if (ctx && ctx->callback) {
        ctx->callback(hcan, XY_HAL_CAN_EVENT_TX_COMPLETE, ctx->arg);
    }
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    can_ctx_t *ctx = find_can_ctx(hcan);
    if (ctx && ctx->callback) {
        ctx->callback(hcan, XY_HAL_CAN_EVENT_ERROR, ctx->arg);
    }
}

#endif /* STM32U5 || STM32U5xx */
