/**
 * @file xy_hal_pin.c
 * @brief GPIO/Pin HAL STM32 Implementation
 * @version 2.0
 * @date 2025-10-26
 */

#include "../../inc/xy_hal_pin.h"
#include "../../inc/xy_hal.h"

#ifdef STM32_HAL_ENABLED

#include "stm32_hal.h"
#include <string.h>

/* Pin IRQ context structure */
typedef struct {
    xy_hal_pin_irq_handler_t handler;
    void *arg;
} pin_irq_ctx_t;

/* IRQ context array for all GPIO pins (16 pins max per port) */
static pin_irq_ctx_t g_pin_irq_ctx[16];

/**
 * @brief Convert XY pin mode to STM32 GPIO mode
 */
static uint32_t xy_to_stm32_mode(xy_hal_pin_mode_t mode)
{
    switch (mode) {
    case XY_HAL_PIN_MODE_INPUT:
        return XY_HAL_GPIO_MODE_INPUT;
    case XY_HAL_PIN_MODE_OUTPUT:
        return GPIO_MODE_OUTPUT_PP;
    case XY_HAL_PIN_MODE_AF:
        return GPIO_MODE_AF_PP;
    case XY_HAL_PIN_MODE_ANALOG:
        return GPIO_MODE_ANALOG;
    default:
        return XY_HAL_GPIO_MODE_INPUT;
    }
}

/**
 * @brief Convert XY pull mode to STM32 GPIO pull
 */
static uint32_t xy_to_stm32_pull(xy_hal_pin_pull_t pull)
{
    switch (pull) {
    case XY_HAL_PIN_PULL_NONE:
        return GPIO_NOPULL;
    case XY_HAL_PIN_PULL_UP:
        return GPIO_PULLUP;
    case XY_HAL_PIN_PULL_DOWN:
        return GPIO_PULLDOWN;
    default:
        return GPIO_NOPULL;
    }
}

/**
 * @brief Convert XY speed to STM32 GPIO speed
 */
static uint32_t xy_to_stm32_speed(xy_hal_pin_speed_t speed)
{
    switch (speed) {
    case XY_HAL_PIN_SPEED_LOW:
        return GPIO_SPEED_FREQ_LOW;
    case XY_HAL_PIN_SPEED_MEDIUM:
        return GPIO_SPEED_FREQ_MEDIUM;
    case XY_HAL_PIN_SPEED_HIGH:
        return GPIO_SPEED_FREQ_HIGH;
    case XY_HAL_PIN_SPEED_VERY_HIGH:
        return GPIO_SPEED_FREQ_VERY_HIGH;
    default:
        return GPIO_SPEED_FREQ_LOW;
    }
}

xy_hal_error_t xy_hal_pin_init(void *port, uint8_t pin,
                               const xy_hal_pin_config_t *config)
{
    if (!port || !config || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    GPIO_InitTypeDef gpio_init = { 0 };
    gpio_init.Pin              = (1U << pin);
    gpio_init.Mode             = xy_to_stm32_mode(config->mode);
    gpio_init.Pull             = xy_to_stm32_pull(config->pull);
    gpio_init.Speed            = xy_to_stm32_speed(config->speed);

    if (config->mode == XY_HAL_PIN_MODE_AF) {
        gpio_init.Alternate = config->alternate;
    }

    if (config->mode == XY_HAL_PIN_MODE_OUTPUT
        && config->otype == XY_HAL_PIN_OTYPE_OD) {
        gpio_init.Mode = GPIO_MODE_OUTPUT_OD;
    }

    HAL_GPIO_Init((GPIO_TypeDef *)port, &gpio_init);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pin_deinit(void *port, uint8_t pin)
{
    if (!port || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    HAL_GPIO_DeInit((GPIO_TypeDef *)port, (1U << pin));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pin_write(void *port, uint8_t pin,
                                xy_hal_pin_state_t state)
{
    if (!port || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    HAL_GPIO_WritePin((GPIO_TypeDef *)port, (1U << pin),
                      state == XY_HAL_PIN_HIGH ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return XY_HAL_OK;
}

int xy_hal_pin_read(void *port, uint8_t pin)
{
    if (!port || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    GPIO_PinState state = HAL_GPIO_ReadPin((GPIO_TypeDef *)port, (1U << pin));
    return (state == GPIO_PIN_SET) ? XY_HAL_PIN_HIGH : XY_HAL_PIN_LOW;
}

xy_hal_error_t xy_hal_pin_toggle(void *port, uint8_t pin)
{
    if (!port || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    HAL_GPIO_TogglePin((GPIO_TypeDef *)port, (1U << pin));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pin_attach_irq(void *port, uint8_t pin,
                                     xy_hal_pin_irq_mode_t mode,
                                     xy_hal_pin_irq_handler_t handler,
                                     void *arg)
{
    if (!port || pin > 15 || !handler) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    uint32_t irq_mode;
    switch (mode) {
    case XY_HAL_PIN_IRQ_RISING:
        irq_mode = GPIO_MODE_IT_RISING;
        break;
    case XY_HAL_PIN_IRQ_FALLING:
        irq_mode = GPIO_MODE_IT_FALLING;
        break;
    case XY_HAL_PIN_IRQ_BOTH:
        irq_mode = GPIO_MODE_IT_RISING_FALLING;
        break;
    default:
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    GPIO_InitTypeDef gpio_init = { 0 };
    gpio_init.Pin              = (1U << pin);
    gpio_init.Mode             = irq_mode;
    gpio_init.Pull             = GPIO_NOPULL;
    HAL_GPIO_Init((GPIO_TypeDef *)port, &gpio_init);

    /* Store callback */
    g_pin_irq_ctx[pin].handler = handler;
    g_pin_irq_ctx[pin].arg     = arg;

    /* Enable NVIC interrupt */
    IRQn_Type irqn;
    switch (pin) {
    case 0:
        irqn = EXTI0_IRQn;
        break;
    case 1:
        irqn = EXTI1_IRQn;
        break;
    case 2:
        irqn = EXTI2_IRQn;
        break;
    case 3:
        irqn = EXTI3_IRQn;
        break;
    case 4:
        irqn = EXTI4_IRQn;
        break;
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        irqn = EXTI9_5_IRQn;
        break;
    default:
        irqn = EXTI15_10_IRQn;
        break;
    }
    HAL_NVIC_SetPriority(irqn, 5, 0);
    HAL_NVIC_EnableIRQ(irqn);

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pin_detach_irq(void *port, uint8_t pin)
{
    if (!port || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    HAL_GPIO_DeInit((GPIO_TypeDef *)port, (1U << pin));
    g_pin_irq_ctx[pin].handler = NULL;
    g_pin_irq_ctx[pin].arg     = NULL;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pin_irq_enable(void *port, uint8_t pin)
{
    if (pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    IRQn_Type irqn;
    switch (pin) {
    case 0:
        irqn = EXTI0_IRQn;
        break;
    case 1:
        irqn = EXTI1_IRQn;
        break;
    case 2:
        irqn = EXTI2_IRQn;
        break;
    case 3:
        irqn = EXTI3_IRQn;
        break;
    case 4:
        irqn = EXTI4_IRQn;
        break;
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        irqn = EXTI9_5_IRQn;
        break;
    default:
        irqn = EXTI15_10_IRQn;
        break;
    }
    HAL_NVIC_EnableIRQ(irqn);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pin_irq_disable(void *port, uint8_t pin)
{
    if (pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    IRQn_Type irqn;
    switch (pin) {
    case 0:
        irqn = EXTI0_IRQn;
        break;
    case 1:
        irqn = EXTI1_IRQn;
        break;
    case 2:
        irqn = EXTI2_IRQn;
        break;
    case 3:
        irqn = EXTI3_IRQn;
        break;
    case 4:
        irqn = EXTI4_IRQn;
        break;
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        irqn = EXTI9_5_IRQn;
        break;
    default:
        irqn = EXTI15_10_IRQn;
        break;
    }
    HAL_NVIC_DisableIRQ(irqn);
    return XY_HAL_OK;
}

/**
 * @brief GPIO EXTI callback (called from HAL)
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    for (int i = 0; i < 16; i++) {
        if ((GPIO_Pin & (1U << i)) && g_pin_irq_ctx[i].handler) {
            g_pin_irq_ctx[i].handler(g_pin_irq_ctx[i].arg);
        }
    }
}

#endif /* STM32_HAL_ENABLED */
