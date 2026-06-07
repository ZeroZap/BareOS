/**
 * @file main.c
 * @author N32cube
 */
 //!!!!!!!!!!!!!!!!NOTE!!!!!!!!!!!!!!!
 // Code cannot be added between /* NTFx CODE START xxxxx*/ and /* NTFx CODE END xxxxx*/
/* NTFx CODE START Include*/
#include "main.h"
#include "secboot_n32_v1.h"
#include "xy_log.h"
#include <stdio.h>
#include <stdint.h>
/* NTFx CODE END Include*/

extern __IO uint32_t mwTick;

static void secboot_log_reset_flags(void)
{
    xy_log_i("SecBoot-N32 reset flags: PIN=%u POR=%u SFTRST=%u IWDG=%u WWDG=%u LPWR=%u",
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
    USART_Configuration();
    xy_log_init();
    xy_log_i("SecBoot-N32 UART4 log ready");
    n32_uart5_secboot_init();
    xy_log_i("SecBoot-N32 UART5 secboot V1 transport ready");
    secboot_log_reset_flags();
    IWDG_Configuration();
    secboot_n32_v1_init();
    /* NTFx CODE END Config*/
    xy_log_i("SecBoot-N32 main loop start");
    secboot_n32_v1_print_layout();
    secboot_n32_v1_send_banner();
    next_heartbeat = mwTick + 1000U;
    while(1)
    {
        IWDG_ReloadKey();
        n32_uart5_secboot_poll();
        secboot_n32_v1_poll();
        if ((int32_t)(mwTick - next_heartbeat) >= 0) {
            xy_log_i("SecBoot-N32 heartbeat UART5 rx=%u tx=%u rb=%u drop=%u last=%02x",
                     (unsigned int)g_n32_uart5_rx_count,
                     (unsigned int)g_n32_uart5_tx_count,
                     (unsigned int)g_n32_uart5_rb_pending,
                     (unsigned int)g_n32_uart5_rx_drop_count,
                     (unsigned int)g_n32_uart5_last_rx);
            next_heartbeat = mwTick + 1000U;
        }
    }
}


