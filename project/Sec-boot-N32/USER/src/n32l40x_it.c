/**
 * @file n32l40x_it.c
 * @brief Minimal SecBoot-N32 interrupt handlers.
 */

#include "n32l40x_it.h"
#include "n32l40x_cfg.h"
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

void UART5_IRQHandler(void)
{
    n32_uart5_secboot_isr();
}
