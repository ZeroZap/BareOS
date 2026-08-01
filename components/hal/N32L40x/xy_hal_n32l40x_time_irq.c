#include "xy_hal_n32l40x_internal.h"

uint32_t xy_hal_time_ms(void)
{
    return xy_tick_now_ms();
}

uint32_t xy_hal_time_s(void)
{
    return xy_tick_now_s();
}

void xy_hal_delay_ms(uint32_t ms)
{
    uint32_t start = xy_hal_time_ms();
    while ((uint32_t)(xy_hal_time_ms() - start) < ms) {
        IWDG_ReloadKey();
    }
}

int xy_hal_systick_suspend(void)
{
    SysTick->CTRL &= ~(SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk);
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;
    g_xy_hal_n32_systick_running = false;
    return XY_HAL_OK;
}

int xy_hal_systick_resume(void)
{
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;
    SysTick->VAL = 0u;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk;
    g_xy_hal_n32_systick_running = true;
    return XY_HAL_OK;
}

bool xy_hal_systick_is_running(void)
{
    return g_xy_hal_n32_systick_running;
}

uint32_t xy_hal_irq_save(void)
{
    uint32_t key = __get_PRIMASK();
    __disable_irq();
    return key;
}

void xy_hal_irq_restore(uint32_t key)
{
    __set_PRIMASK(key);
}

void xy_hal_irq_enable(void)
{
    __enable_irq();
}

void xy_hal_irq_disable(void)
{
    __disable_irq();
}
