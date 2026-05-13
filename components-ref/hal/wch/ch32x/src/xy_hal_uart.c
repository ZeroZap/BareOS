/**
 * @file xy_hal_uart.c
 * @brief WCH CH32V30x UART HAL Implementation
 * @version 1.0.0
 * @date 2026-03-02 YOLO 通宵
 */

#include "xy_hal_uart.h"
#include "xy_log.h"

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

#ifdef MCU_CH32

#include "ch32v30x.h"

/**
 * @brief USART 时钟使能
 */
static void xy_hal_usart_enable_clock(void *instance)
{
    if (instance == USART1) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    } else if (instance == USART2) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    } else if (instance == USART3) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    }
}

xy_hal_error_t xy_hal_uart_init(void *instance, const xy_hal_uart_config_t *config)
{
    USART_InitTypeDef USART_InitStructure;
    
    if (!instance || !config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    /* 使能时钟 */
    xy_hal_usart_enable_clock(instance);
    
    /* 配置 USART 参数 */
    USART_InitStructure.USART_BaudRate = config->baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    
    USART_Init(instance, &USART_InitStructure);
    USART_Cmd(instance, ENABLE);
    
    xy_log_d("WCH UART init: instance=%p, baud=%d\n", instance, config->baudrate);
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_deinit(void *instance)
{
    if (!instance) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    USART_Cmd(instance, DISABLE);
    USART_DeInit(instance);
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_send(void *instance, const uint8_t *data, uint16_t len, uint32_t timeout)
{
    uint16_t i;
    uint32_t start;
    
    if (!instance || !data) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    for (i = 0; i < len; i++) {
        start = xy_os_tick_get();
        
        while (USART_GetFlagStatus(instance, USART_FLAG_TXE) == RESET) {
            if (timeout > 0 && (xy_os_tick_get() - start) > timeout) {
                return XY_HAL_ERROR_TIMEOUT;
            }
        }
        
        USART_SendData(instance, data[i]);
    }
    
    /* 等待发送完成 */
    start = xy_os_tick_get();
    while (USART_GetFlagStatus(instance, USART_FLAG_TC) == RESET) {
        if (timeout > 0 && (xy_os_tick_get() - start) > timeout) {
            return XY_HAL_ERROR_TIMEOUT;
        }
    }
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_uart_receive(void *instance, uint8_t *data, uint16_t len, uint32_t timeout)
{
    uint16_t i;
    uint32_t start;
    
    if (!instance || !data) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    for (i = 0; i < len; i++) {
        start = xy_os_tick_get();
        
        while (USART_GetFlagStatus(instance, USART_FLAG_RXNE) == RESET) {
            if (timeout > 0 && (xy_os_tick_get() - start) > timeout) {
                return XY_HAL_ERROR_TIMEOUT;
            }
        }
        
        data[i] = USART_ReceiveData(instance);
    }
    
    return XY_HAL_OK;
}

#else

xy_hal_error_t xy_hal_uart_init(void *instance, const xy_hal_uart_config_t *config)
{
    (void)instance;
    (void)config;
    return XY_HAL_ERROR_NOT_SUPPORT;
}

#endif
