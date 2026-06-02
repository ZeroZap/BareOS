/**
 * @file main.c
 * @author N32cube
 */
 //!!!!!!!!!!!!!!!!NOTE!!!!!!!!!!!!!!!
 // Code cannot be added between /* NTFx CODE START xxxxx*/ and /* NTFx CODE END xxxxx*/
/* NTFx CODE START Include*/
#include "main.h"
#include <stdio.h>
#include <stdint.h>
/* NTFx CODE END Include*/

/**
 * @brief  Main program.
 */
int main(void)
{
    /* NTFx CODE START Config*/
    RCC_Configuration();
    GPIO_Configuration();
    NVIC_Configuration();
    USART_Configuration();
    n32_debug_log_write("UUUU PLB-N32 UART4 log ready\r\n");
    LPTIM_Configuration();
    RTC_Configuration();
    IWDG_Configuration();
    ADC_Configuration();
    USBFS_Configuration();
    SPI_Configuration();
    I2C_Configuration();
    DMA_Configuration();
    /* NTFx CODE END Config*/
    while(1)
    {
        n32_debug_log_write("UUUU PLB-N32 UART4 heartbeat\r\n");
        SysTick_Delayms(1000);
    }
}


