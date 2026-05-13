/**
 * @file xy_hal_tgpio.c
 * @brief Time-Sensitive GPIO HAL STM32U5 Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal_tgpio.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"

xy_hal_error_t xy_hal_tgpio_set(void *port, uint8_t pin)
{
    if (!port || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ((GPIO_TypeDef *)port)->BSRR = (1U << pin);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_tgpio_reset(void *port, uint8_t pin)
{
    if (!port || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ((GPIO_TypeDef *)port)->BSRR = (1U << (pin + 16));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_tgpio_toggle(void *port, uint8_t pin)
{
    if (!port || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    ((GPIO_TypeDef *)port)->ODR ^= (1U << pin);
    return XY_HAL_OK;
}

int xy_hal_tgpio_read(void *port, uint8_t pin)
{
    if (!port || pin > 15) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    return (((GPIO_TypeDef *)port)->IDR & (1U << pin)) ? 1 : 0;
}

#endif /* STM32U5 || STM32U5xx */
