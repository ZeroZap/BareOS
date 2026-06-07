/**
 * @file n32l40x_it.c
 * @author N32cube
 */

#include "n32l40x_it.h"
#include "n32l40x_cfg.h"
#include "n32l40x.h"
#include "n32l40x_iwdg.h"

extern __IO uint32_t mwTick;
extern void n32_debug_log_write(const char *str);
volatile unsigned int g_sys_tick_ms;

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
    n32_debug_log_write("\r\n[FAULT] HardFault\r\n");
    while (1) {
        IWDG_ReloadKey();
    }
}

void MemManage_Handler(void)
{
    n32_debug_log_write("\r\n[FAULT] MemManage\r\n");
    while (1) {
        IWDG_ReloadKey();
    }
}

void BusFault_Handler(void)
{
    n32_debug_log_write("\r\n[FAULT] BusFault\r\n");
    while (1) {
        IWDG_ReloadKey();
    }
}

void UsageFault_Handler(void)
{
    n32_debug_log_write("\r\n[FAULT] UsageFault\r\n");
    while (1) {
        IWDG_ReloadKey();
    }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void SysTick_Handler(void)
{
    mwTick++;
    g_sys_tick_ms++;
}

void EXTI1_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_LINE1)) {
        EXTI_ClrITPendBit(EXTI_LINE1);
    }
}

void DMA_Channel1_IRQHandler(void)
{
    if (DMA_GetIntStatus(DMA_INT_HTX1, DMA)) {
        DMA_ClrIntPendingBit(DMA_INT_HTX1, DMA);
    }
    if (DMA_GetIntStatus(DMA_INT_TXC1, DMA)) {
        DMA_ClrIntPendingBit(DMA_INT_TXC1, DMA);
    }
    if (DMA_GetIntStatus(DMA_INT_ERR1, DMA)) {
        DMA_ClrIntPendingBit(DMA_INT_ERR1, DMA);
    }
}

void USB_LP_IRQHandler(void)
{
    /* USB is not used by SecBoot-N32 V1. */
}

void USART1_IRQHandler(void)
{
    if (USART_GetIntStatus(USART1, USART_INT_IDLEF)) {
        USART_ReceiveData(USART1);
    }
    if (USART_GetIntStatus(USART1, USART_INT_OREF) ||
        USART_GetIntStatus(USART1, USART_INT_NEF) ||
        USART_GetIntStatus(USART1, USART_INT_FEF) ||
        USART_GetIntStatus(USART1, USART_INT_PEF)) {
        (void)USART1->STS;
        (void)USART1->DAT;
    }
}

void USART2_IRQHandler(void)
{
    if (USART_GetIntStatus(USART2, USART_INT_IDLEF)) {
        USART_ReceiveData(USART2);
    }
    if (USART_GetIntStatus(USART2, USART_INT_OREF) ||
        USART_GetIntStatus(USART2, USART_INT_NEF) ||
        USART_GetIntStatus(USART2, USART_INT_FEF) ||
        USART_GetIntStatus(USART2, USART_INT_PEF)) {
        (void)USART2->STS;
        (void)USART2->DAT;
    }
}

void USBWakeUp_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_LINE17)) {
        EXTI_ClrITPendBit(EXTI_LINE17);
    }
}

void UART4_IRQHandler(void)
{
    if (USART_GetIntStatus(UART4, USART_INT_IDLEF)) {
        USART_ReceiveData(UART4);
    }
    if (USART_GetIntStatus(UART4, USART_INT_OREF) ||
        USART_GetIntStatus(UART4, USART_INT_NEF) ||
        USART_GetIntStatus(UART4, USART_INT_FEF) ||
        USART_GetIntStatus(UART4, USART_INT_PEF)) {
        (void)UART4->STS;
        (void)UART4->DAT;
    }
}

void UART5_IRQHandler(void)
{
    n32_uart5_secboot_isr();
}
