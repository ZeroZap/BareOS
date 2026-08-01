#include "xy_hal_n32l40x_internal.h"

xy_hal_n32_gpio_irq_state_t g_xy_hal_n32_gpio_irq[4][16];
xy_hal_n32_dev_cb_state_t g_xy_hal_n32_uart_cb[5];
xy_hal_n32_dev_cb_state_t g_xy_hal_n32_dma_cb[8];
xy_hal_n32_lptimer_state_t g_xy_hal_n32_lptim;
xy_hal_power_policy_t g_xy_hal_n32_power_policy = {
    XY_HAL_POWER_SLEEP,
    XY_HAL_WAKE_GPIO | XY_HAL_WAKE_LPTIMER | XY_HAL_WAKE_UART,
    0u,
    0u,
    true,
    true,
};
uint16_t g_xy_hal_n32_pm_locks[XY_HAL_PM_LOCK_COUNT];
bool g_xy_hal_n32_systick_running = true;

bool xy_hal_n32_expired(uint32_t start, uint32_t timeout_ms)
{
    return timeout_ms != 0xffffffffu &&
           (uint32_t)(xy_hal_time_ms() - start) >= timeout_ms;
}

GPIO_Module *xy_hal_n32_gpio_port(uint32_t pin)
{
    switch (XY_HAL_N32_PIN_PORT(pin)) {
    case XY_HAL_N32_PORT_A: return GPIOA;
    case XY_HAL_N32_PORT_B: return GPIOB;
    case XY_HAL_N32_PORT_C: return GPIOC;
    case XY_HAL_N32_PORT_D: return GPIOD;
    default: return NULL;
    }
}

uint16_t xy_hal_n32_gpio_pin_mask(uint32_t pin)
{
    return (uint16_t)(1u << XY_HAL_N32_PIN_INDEX(pin));
}

uint32_t xy_hal_n32_gpio_clk(uint32_t port)
{
    switch (port) {
    case XY_HAL_N32_PORT_A: return RCC_APB2_PERIPH_GPIOA;
    case XY_HAL_N32_PORT_B: return RCC_APB2_PERIPH_GPIOB;
    case XY_HAL_N32_PORT_C: return RCC_APB2_PERIPH_GPIOC;
    case XY_HAL_N32_PORT_D: return RCC_APB2_PERIPH_GPIOD;
    default: return 0u;
    }
}

uint8_t xy_hal_n32_gpio_pin_source(uint32_t pin)
{
    return (uint8_t)XY_HAL_N32_PIN_INDEX(pin);
}

uint32_t xy_hal_n32_exti_line(uint32_t pin)
{
    return 1u << XY_HAL_N32_PIN_INDEX(pin);
}

void xy_hal_n32_enable_uart_clock(USART_Module *uart)
{
    if (uart == USART1) RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_USART1, ENABLE);
    else if (uart == USART2) RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_USART2, ENABLE);
    else if (uart == USART3) RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_USART3, ENABLE);
    else if (uart == UART4) RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_UART4, ENABLE);
    else if (uart == UART5) RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_UART5, ENABLE);
}

void xy_hal_n32_enable_i2c_clock(I2C_Module *i2c)
{
    if (i2c == I2C1) RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_I2C1, ENABLE);
    else if (i2c == I2C2) RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_I2C2, ENABLE);
}

void xy_hal_n32_enable_spi_clock(SPI_Module *spi)
{
    if (spi == SPI1) RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_SPI1, ENABLE);
    else if (spi == SPI2) RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_SPI2, ENABLE);
}

void xy_hal_n32_enable_tim_clock(TIM_Module *tim)
{
    if (tim == TIM1) RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_TIM1, ENABLE);
    else if (tim == TIM8) RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_TIM8, ENABLE);
    else if (tim == TIM2) RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_TIM2, ENABLE);
    else if (tim == TIM3) RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_TIM3, ENABLE);
    else if (tim == TIM4) RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_TIM4, ENABLE);
    else if (tim == TIM5) RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_TIM5, ENABLE);
    else if (tim == TIM6) RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_TIM6, ENABLE);
    else if (tim == TIM7) RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_TIM7, ENABLE);
    else if (tim == TIM9) RCC_EnableAPB1PeriphClk(RCC_APB1_PERIPH_TIM9, ENABLE);
}

xy_hal_n32_dev_cb_state_t *xy_hal_n32_uart_state(USART_Module *uart)
{
    if (uart == USART1) return &g_xy_hal_n32_uart_cb[0];
    if (uart == USART2) return &g_xy_hal_n32_uart_cb[1];
    if (uart == USART3) return &g_xy_hal_n32_uart_cb[2];
    if (uart == UART4) return &g_xy_hal_n32_uart_cb[3];
    if (uart == UART5) return &g_xy_hal_n32_uart_cb[4];
    return NULL;
}

xy_hal_n32_dev_cb_state_t *xy_hal_n32_dma_state(DMA_ChannelType *dma)
{
    if (dma == DMA_CH1) return &g_xy_hal_n32_dma_cb[0];
    if (dma == DMA_CH2) return &g_xy_hal_n32_dma_cb[1];
    if (dma == DMA_CH3) return &g_xy_hal_n32_dma_cb[2];
    if (dma == DMA_CH4) return &g_xy_hal_n32_dma_cb[3];
    if (dma == DMA_CH5) return &g_xy_hal_n32_dma_cb[4];
    if (dma == DMA_CH6) return &g_xy_hal_n32_dma_cb[5];
    if (dma == DMA_CH7) return &g_xy_hal_n32_dma_cb[6];
    if (dma == DMA_CH8) return &g_xy_hal_n32_dma_cb[7];
    return NULL;
}

void __attribute__((weak)) xy_hal_n32l40x_after_stop_restore_clock(void)
{
}

void __attribute__((weak)) xy_hal_n32l40x_before_sleep(void)
{
}

void __attribute__((weak)) xy_hal_n32l40x_after_wake(void)
{
}
