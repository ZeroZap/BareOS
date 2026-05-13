/**
 * @file xy_hal_can.c
 * @brief WCH CH32V30x CAN HAL Implementation
 * @version 1.0.0
 * @date 2026-03-02
 */

#include "xy_hal_can.h"
#include "xy_log.h"

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

#ifdef MCU_CH32

#include "ch32v30x.h"

/**
 * @brief CAN 时钟使能
 */
static void xy_hal_can_enable_clock(void *instance)
{
    if (instance == CAN1) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    } else if (instance == CAN2) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN2, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    }
}

xy_hal_error_t xy_hal_can_init(void *instance, const xy_hal_can_config_t *config)
{
    CAN_InitTypeDef CAN_InitStructure;
    
    if (!instance || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 使能时钟 */
    xy_hal_can_enable_clock(instance);
    
    /* CAN GPIO 配置 */
    if (instance == CAN1) {
        /* PA11-RX, PA12-TX */
        GPIO_InitTypeDef GPIO_InitStructure;
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
        
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
        GPIO_Init(GPIOA, &GPIO_InitStructure);
        
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOA, &GPIO_InitStructure);
    }
    
    /* CAN 参数配置 */
    CAN_InitStructure.CAN_TTCM = DISABLE;
    CAN_InitStructure.CAN_ABOM = ENABLE;
    CAN_InitStructure.CAN_AWUM = DISABLE;
    CAN_InitStructure.CAN_NART = DISABLE;
    CAN_InitStructure.CAN_RFLM = DISABLE;
    CAN_InitStructure.CAN_TXFP = DISABLE;
    CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
    
    /* 波特率配置 */
    /* CAN 时钟 = 72MHz, 波特率 = 72M / (1+8+6) / 5 = 1Mbps */
    CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
    CAN_InitStructure.CAN_BS1 = CAN_BS1_8tq;
    CAN_InitStructure.CAN_BS2 = CAN_BS2_6tq;
    CAN_InitStructure.CAN_Prescaler = config->baudrate_prescaler;
    
    CAN_Init(instance, &CAN_InitStructure);
    
    /* 过滤器配置 (接受所有消息) */
    CAN_FilterInitTypeDef CAN_FilterInitStructure;
    CAN_FilterInitStructure.CAN_FilterNumber = 0;
    CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
    CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
    CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;
    CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(&CAN_FilterInitStructure);
    
    /* 使能中断 */
    CAN_ITConfig(instance, CAN_IT_FMP0, ENABLE);
    
    xy_log_d("WCH CAN init: instance=%p, baudrate=%d\n",
             instance, config->baudrate_prescaler);
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_can_deinit(void *instance)
{
    if (!instance) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    CAN_DeInit(instance);
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_can_send(void *instance, const xy_hal_can_msg_t *msg, uint32_t timeout)
{
    CanTxMsg TxMessage;
    uint8_t transmit_mailbox;
    uint32_t start;
    
    if (!instance || !msg) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 配置发送消息 */
    TxMessage.StdId = msg->std_id;
    TxMessage.ExtId = msg->ext_id;
    TxMessage.IDE = (msg->id_type == XY_HAL_CAN_ID_STD) ? CAN_ID_STD : CAN_ID_EXT;
    TxMessage.RTR = CAN_RTR_DATA;
    TxMessage.DLC = msg->dlc;
    
    memcpy(TxMessage.Data, msg->data, msg->dlc);
    
    /* 发送 */
    transmit_mailbox = CAN_Transmit(instance, &TxMessage);
    
    if (transmit_mailbox == CAN_TxStatus_NoMailBox) {
        return XY_HAL_ERROR;
    }
    
    /* 等待发送完成 */
    start = xy_os_tick_get();
    while (CAN_GetLastTransmitStatus(instance, transmit_mailbox) == CAN_TxStatus_Pending) {
        if (timeout > 0 && (xy_os_tick_get() - start) > timeout) {
            return XY_HAL_ERROR_TIMEOUT;
        }
    }
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_can_receive(void *instance, xy_hal_can_msg_t *msg, uint32_t timeout)
{
    CanRxMsg RxMessage;
    uint32_t start;
    
    if (!instance || !msg) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 等待接收 */
    start = xy_os_tick_get();
    while (CAN_MessagePending(instance, CAN_FIFO0) == 0) {
        if (timeout > 0 && (xy_os_tick_get() - start) > timeout) {
            return XY_HAL_ERROR_TIMEOUT;
        }
    }
    
    /* 读取消息 */
    CAN_Receive(instance, CAN_FIFO0, &RxMessage);
    
    msg->std_id = RxMessage.StdId;
    msg->ext_id = RxMessage.ExtId;
    msg->id_type = (RxMessage.IDE == CAN_ID_STD) ? XY_HAL_CAN_ID_STD : XY_HAL_CAN_ID_EXT;
    msg->dlc = RxMessage.DLC;
    memcpy(msg->data, RxMessage.Data, RxMessage.DLC);
    
    return XY_HAL_OK;
}

#else

xy_hal_error_t xy_hal_can_init(void *instance, const xy_hal_can_config_t *config)
{
    (void)instance;
    (void)config;
    return XY_HAL_ERROR_NOT_SUPPORT;
}

#endif
