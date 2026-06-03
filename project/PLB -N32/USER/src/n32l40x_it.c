/**
 * @file n32l40x_it.c
 * @author N32cube
 */

/* NTFx CODE START */
#include "n32l40x_it.h"
#include "n32l40x.h"
#include "n32l40x_iwdg.h"
#include "usb_istr.h"
#include "usb_int.h"
/* NTFx CODE END */

/* NTFx CODE START */
extern __IO uint32_t mwTick;
extern void n32_debug_log_write(const char *str);
/**
 * @brief  This function handles NMI exception.
 */
void NMI_Handler(void)
{
/* NTFx CODE END */

}
/* NTFx CODE START */
/**
 * @brief  This function handles Hard Fault exception.
 */
void HardFault_Handler(void)
{
    n32_debug_log_write("\r\n[FAULT] HardFault\r\n");
    /* Go to infinite loop when Hard Fault exception occurs */
    while (1)
    {
        IWDG_ReloadKey();
    /* NTFx CODE END */

    }
}
/* NTFx CODE START */
/**
 * @brief  This function handles Memory Manage exception.
 */
void MemManage_Handler(void)
{
    n32_debug_log_write("\r\n[FAULT] MemManage\r\n");
    /* Go to infinite loop when Memory Manage exception occurs */
    while (1)
    {
        IWDG_ReloadKey();
/* NTFx CODE END */

    }
}
/* NTFx CODE START */
/**
 * @brief  This function handles Bus Fault exception.
 */
void BusFault_Handler(void)
{
    n32_debug_log_write("\r\n[FAULT] BusFault\r\n");
    /* Go to infinite loop when Bus Fault exception occurs */
    while (1)
    {
        IWDG_ReloadKey();
/* NTFx CODE END */

    }
}
/* NTFx CODE START */
/**
 * @brief  This function handles Usage Fault exception.
 */
void UsageFault_Handler(void)
{
    n32_debug_log_write("\r\n[FAULT] UsageFault\r\n");
    /* Go to infinite loop when Usage Fault exception occurs */
    while (1)
    {
        IWDG_ReloadKey();
/* NTFx CODE END */

    }
}
/* NTFx CODE START */
/**
 * @brief  This function handles SVCall exception.
 */
void SVC_Handler(void)
{
/* NTFx CODE END */

}
/* NTFx CODE START */
/**
 * @brief  This function handles Debug Monitor exception.
 */
void DebugMon_Handler(void)
{
/* NTFx CODE END */

}
/* NTFx CODE START */
/**
 * @brief  This function handles SysTick Handler.
 */
void SysTick_Handler(void)
{
    mwTick++;
/* NTFx CODE END */

}
/* NTFx CODE START(EXTI1_IRQHandler)*/
/**
* @brief  This function handles EXTI1IRQHandler.
*/
void EXTI1_IRQHandler(void)
{
/* NTFx CODE END */

/* NTFx CODE START */
    if (EXTI_GetITStatus(EXTI_LINE1))
    {
        /*clear IT flag*/
        EXTI_ClrITPendBit(EXTI_LINE1);
/* NTFx CODE END */

    }
    
/* NTFx CODE START */
}
/* NTFx CODE END(EXTI1_IRQHandler)*/

/* NTFx CODE START(DMA_Channel1_IRQHandler)*/
/**
* @brief  This function handles DMA_Channel1IRQHandler.
*/
void DMA_Channel1_IRQHandler(void)
{
/* NTFx CODE END */

/* NTFx CODE START */
    if (DMA_GetIntStatus(DMA_INT_HTX1, DMA))
    {
        /*clear IT flag*/
        DMA_ClrIntPendingBit(DMA_INT_HTX1, DMA);
/* NTFx CODE END */

    }
/* NTFx CODE START */
    if (DMA_GetIntStatus(DMA_INT_TXC1, DMA))
    {
        /*clear IT flag*/
        DMA_ClrIntPendingBit(DMA_INT_TXC1, DMA);
/* NTFx CODE END */

    }
/* NTFx CODE START */
    if (DMA_GetIntStatus(DMA_INT_ERR1, DMA))
    {
        /*clear IT flag*/
        DMA_ClrIntPendingBit(DMA_INT_ERR1, DMA);
/* NTFx CODE END */

    }
    
/* NTFx CODE START */
}
/* NTFx CODE END(DMA_Channel1_IRQHandler)*/

/* NTFx CODE START(USB_LP_IRQHandler)*/
/**
* @brief  This function handles USB_LPIRQHandler.
*/
void USB_LP_IRQHandler(void)
{
/* NTFx CODE END */

    /*USB interrupt  function*/    USB_Istr();
    
/* NTFx CODE START */
}
/* NTFx CODE END(USB_LP_IRQHandler)*/

/* NTFx CODE START(USART1_IRQHandler)*/
/**
* @brief  This function handles USART1IRQHandler.
*/
void USART1_IRQHandler(void)
{
/* NTFx CODE END */

/* NTFx CODE START */
    if (USART_GetIntStatus(USART1, USART_INT_IDLEF))
    {
        /*clear IT flag*/
        USART_ReceiveData(USART1);
/* NTFx CODE END */

    }
/* NTFx CODE START */
    if (USART_GetIntStatus(USART1, USART_INT_OREF)||
        USART_GetIntStatus(USART1, USART_INT_NEF)||
        USART_GetIntStatus(USART1, USART_INT_FEF)||
        USART_GetIntStatus(USART1, USART_INT_PEF))
    {
        /*clear IT flag*/
        /*Read the sts register first,and the read the DAT register to clear the all error flag*/
        (void)USART1->STS;
        (void)USART1->DAT;
/* NTFx CODE END */

    }
    
/* NTFx CODE START */
}
/* NTFx CODE END(USART1_IRQHandler)*/

/* NTFx CODE START(USART2_IRQHandler)*/
/**
* @brief  This function handles USART2IRQHandler.
*/
void USART2_IRQHandler(void)
{
/* NTFx CODE END */

/* NTFx CODE START */
    if (USART_GetIntStatus(USART2, USART_INT_IDLEF))
    {
        /*clear IT flag*/
        USART_ReceiveData(USART2);
/* NTFx CODE END */

    }
/* NTFx CODE START */
    if (USART_GetIntStatus(USART2, USART_INT_OREF)||
        USART_GetIntStatus(USART2, USART_INT_NEF)||
        USART_GetIntStatus(USART2, USART_INT_FEF)||
        USART_GetIntStatus(USART2, USART_INT_PEF))
    {
        /*clear IT flag*/
        /*Read the sts register first,and the read the DAT register to clear the all error flag*/
        (void)USART2->STS;
        (void)USART2->DAT;
/* NTFx CODE END */

    }
    
/* NTFx CODE START */
}
/* NTFx CODE END(USART2_IRQHandler)*/

/* NTFx CODE START(USBWakeUp_IRQHandler)*/
/**
* @brief  This function handles USBWakeUpIRQHandler.
*/
void USBWakeUp_IRQHandler(void)
{
/* NTFx CODE END */

/* NTFx CODE START */
    if (EXTI_GetITStatus(EXTI_LINE17))
    {
        /*clear IT flag*/
        EXTI_ClrITPendBit(EXTI_LINE17);
/* NTFx CODE END */

    }
    
/* NTFx CODE START */
}
/* NTFx CODE END(USBWakeUp_IRQHandler)*/

/* NTFx CODE START(UART4_IRQHandler)*/
/**
* @brief  This function handles UART4IRQHandler.
*/
void UART4_IRQHandler(void)
{
/* NTFx CODE END */

/* NTFx CODE START */
    if (USART_GetIntStatus(UART4, USART_INT_IDLEF))
    {
        /*clear IT flag*/
        USART_ReceiveData(UART4);
/* NTFx CODE END */

    }
/* NTFx CODE START */
    if (USART_GetIntStatus(UART4, USART_INT_OREF)||
        USART_GetIntStatus(UART4, USART_INT_NEF)||
        USART_GetIntStatus(UART4, USART_INT_FEF)||
        USART_GetIntStatus(UART4, USART_INT_PEF))
    {
        /*clear IT flag*/
        /*Read the sts register first,and the read the DAT register to clear the all error flag*/
        (void)UART4->STS;
        (void)UART4->DAT;
/* NTFx CODE END */

    }
    
/* NTFx CODE START */
}
/* NTFx CODE END(UART4_IRQHandler)*/

