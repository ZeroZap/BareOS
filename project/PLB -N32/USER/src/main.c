/**
 * @file main.c
 * @author N32cube
 */
 //!!!!!!!!!!!!!!!!NOTE!!!!!!!!!!!!!!!
 // Code cannot be added between /* NTFx CODE START xxxxx*/ and /* NTFx CODE END xxxxx*/
/* NTFx CODE START Include*/
#include "main.h"
#include "xy_log.h"
#include <stdio.h>
#include <stdint.h>
/* NTFx CODE END Include*/

void plb_algo_demo_run(void);
extern __IO uint32_t mwTick;

static void plb_log_reset_flags(void)
{
    xy_log_i("PLB-N32 reset flags: PIN=%u POR=%u SFTRST=%u IWDG=%u WWDG=%u LPWR=%u",
             (unsigned int)RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_PINRSTF),
             (unsigned int)RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_PORRSTF),
             (unsigned int)RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_SFTRSTF),
             (unsigned int)RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_IWDGRSTF),
             (unsigned int)RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_WWDGRSTF),
             (unsigned int)RCC_GetFlagStatus(RCC_CTRLSTS_FLAG_LPWRRSTF));
    RCC_ClrFlag();
}

/**
 * @brief  Main program.
 */
int main(void)
{
    uint32_t next_heartbeat;

    /* NTFx CODE START Config*/
    RCC_Configuration();
    GPIO_Configuration();
#if 0
    NVIC_Configuration();
#endif
    USART_Configuration();
    xy_log_init();
    xy_log_i("PLB-N32 UART4 log ready");
    plb_log_reset_flags();
    IWDG_Configuration();
    plb_algo_demo_run();
    /* Keep the algorithm validation image minimal. Enabling unused generated
     * peripherals here can introduce pending interrupts or board-level resets. */
#if 0
    LPTIM_Configuration();
    RTC_Configuration();
    ADC_Configuration();
    USBFS_Configuration();
    SPI_Configuration();
    I2C_Configuration();
    DMA_Configuration();
#endif
    /* NTFx CODE END Config*/
    xy_log_i("PLB-N32 main loop start");
    next_heartbeat = mwTick + 1000U;
    while(1)
    {
        IWDG_ReloadKey();
        if ((int32_t)(mwTick - next_heartbeat) >= 0) {
            xy_log_i("PLB-N32 UART4 heartbeat");
            next_heartbeat = mwTick + 1000U;
        }
    }
}


