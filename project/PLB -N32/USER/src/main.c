/**
 * @file main.c
 * @author N32cube
 */
 //!!!!!!!!!!!!!!!!NOTE!!!!!!!!!!!!!!!
 // Code cannot be added between /* NTFx CODE START xxxxx*/ and /* NTFx CODE END xxxxx*/
/* NTFx CODE START Include*/
#include "main.h"
#include "plb_n32_flash_fee.h"
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
    uint32_t boot_count = 0;
    plb_n32_server_endpoint_t server;
    int boot_count_update;

    /* NTFx CODE START Config*/
    RCC_Configuration();
    GPIO_Configuration();
#if 0
    NVIC_Configuration();
#endif
    USART_Configuration();
    xy_log_init();
    xy_log_i("PLB-N32 UART4 log ready");
    n32_uart5_secboot_init();
    xy_log_i("PLB-N32 UART5 reserved for sec-boot development");
    plb_log_reset_flags();
    IWDG_Configuration();
    xy_log_i("PLB-N32 FEE base=%x size=%x init=%d",
             (unsigned int)PLB_N32_FEE_BASE_ADDR,
             (unsigned int)PLB_N32_FEE_TOTAL_SIZE,
             (int)plb_n32_fee_init());
    xy_log_i("PLB-N32 EEPROM base=%x size=%x init=%d",
             (unsigned int)PLB_N32_EEPROM_BASE_ADDR,
             (unsigned int)PLB_N32_EEPROM_TOTAL_SIZE,
             (int)plb_n32_eeprom_init());
    boot_count_update = (int)plb_n32_boot_count_update(&boot_count);
    xy_log_i("PLB-N32 boot_count=%u update=%d",
             (unsigned int)boot_count,
             boot_count_update);
    if (plb_n32_server_endpoint_load(&server) == EFLASH_OK) {
        xy_log_i("PLB-N32 server ip=%x port=%u",
                 ((unsigned int)server.ip[0] << 24)
                 | ((unsigned int)server.ip[1] << 16)
                 | ((unsigned int)server.ip[2] << 8)
                 | (unsigned int)server.ip[3],
                 (unsigned int)server.port);
    }
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
        n32_uart5_secboot_poll();
        if ((int32_t)(mwTick - next_heartbeat) >= 0) {
            xy_log_i("PLB-N32 UART4 heartbeat UART5 rx=%u tx=%u rb=%u drop=%u last=%02x",
                     (unsigned int)g_n32_uart5_rx_count,
                     (unsigned int)g_n32_uart5_tx_count,
                     (unsigned int)g_n32_uart5_rb_pending,
                     (unsigned int)g_n32_uart5_rx_drop_count,
                     (unsigned int)g_n32_uart5_last_rx);
            next_heartbeat = mwTick + 1000U;
        }
    }
}


