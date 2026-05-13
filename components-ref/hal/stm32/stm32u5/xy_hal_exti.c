/**
 * @file xy_hal_exti.c
 * @brief EXTI HAL STM32U5 Implementation
 * @version 2.0
 * @date 2026-02-28
 */

#include "../../inc/xy_hal_exti.h"

#if defined(STM32U5) || defined(STM32U5xx)

#include "stm32u5xx_hal.h"
#include <string.h>

/* EXTI callback structure */
typedef struct {
    xy_hal_exti_callback_t callback;
    void *arg;
} exti_callback_t;

/* EXTI context */
typedef struct {
    exti_callback_t callbacks[XY_HAL_EXTI_LINE_MAX];
    uint8_t initialized;
} exti_ctx_t;

static exti_ctx_t g_exti_ctx = { 0 };

/**
 * @brief Convert XY EXTI line to STM32
 */
static uint32_t xy_to_stm32_line(xy_hal_exti_line_t line)
{
    return (1U << (uint32_t)line);
}

/**
 * @brief Convert XY trigger to STM32
 */
static void xy_to_stm32_trigger(xy_hal_exti_trigger_t trigger)
{
    switch (trigger) {
    case XY_HAL_EXTI_TRIGGER_RISING:
        __HAL_RCC_SYSCFG_CLK_ENABLE();
        break;
    case XY_HAL_EXTI_TRIGGER_FALLING:
        __HAL_RCC_SYSCFG_CLK_ENABLE();
        break;
    case XY_HAL_EXTI_TRIGGER_BOTH:
        __HAL_RCC_SYSCFG_CLK_ENABLE();
        break;
    default:
        break;
    }
}

xy_hal_error_t xy_hal_exti_init(void)
{
    if (g_exti_ctx.initialized) {
        return XY_HAL_ERROR_ALREADY_INIT;
    }

    memset(g_exti_ctx.callbacks, 0, sizeof(g_exti_ctx.callbacks));
    g_exti_ctx.initialized = 1;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_deinit(void)
{
    if (!g_exti_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    memset(g_exti_ctx.callbacks, 0, sizeof(g_exti_ctx.callbacks));
    g_exti_ctx.initialized = 0;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_configure(const xy_hal_exti_config_t *config)
{
    if (!config) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_exti_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    xy_to_stm32_trigger(config->trigger);

    /* Configure trigger based on mode */
    switch (config->trigger) {
    case XY_HAL_EXTI_TRIGGER_RISING:
        __HAL_EXTI_ENABLE_RISING_TRIGGER(xy_to_stm32_line(config->line));
        __HAL_EXTI_DISABLE_FALLING_TRIGGER(xy_to_stm32_line(config->line));
        break;
    case XY_HAL_EXTI_TRIGGER_FALLING:
        __HAL_EXTI_DISABLE_RISING_TRIGGER(xy_to_stm32_line(config->line));
        __HAL_EXTI_ENABLE_FALLING_TRIGGER(xy_to_stm32_line(config->line));
        break;
    case XY_HAL_EXTI_TRIGGER_BOTH:
        __HAL_EXTI_ENABLE_RISING_TRIGGER(xy_to_stm32_line(config->line));
        __HAL_EXTI_ENABLE_FALLING_TRIGGER(xy_to_stm32_line(config->line));
        break;
    default:
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_enable(xy_hal_exti_line_t line)
{
    if (!g_exti_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    __HAL_EXTI_ENABLE_IT(xy_to_stm32_line(line));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_disable(xy_hal_exti_line_t line)
{
    if (!g_exti_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    __HAL_EXTI_DISABLE_IT(xy_to_stm32_line(line));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_register_callback(xy_hal_exti_line_t line,
                                             xy_hal_exti_callback_t callback,
                                             void *arg)
{
    if (!callback) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_exti_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    if (line >= XY_HAL_EXTI_LINE_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    g_exti_ctx.callbacks[line].callback = callback;
    g_exti_ctx.callbacks[line].arg      = arg;

    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_unregister_callback(xy_hal_exti_line_t line)
{
    if (line >= XY_HAL_EXTI_LINE_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    if (!g_exti_ctx.initialized) {
        return XY_HAL_ERROR_NOT_INIT;
    }

    g_exti_ctx.callbacks[line].callback = NULL;
    g_exti_ctx.callbacks[line].arg      = NULL;

    return XY_HAL_OK;
}

int xy_hal_exti_get_pending(xy_hal_exti_line_t line)
{
    if (line >= XY_HAL_EXTI_LINE_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    return __HAL_EXTI_GET_FLAG(xy_to_stm32_line(line)) ? 1 : 0;
}

xy_hal_error_t xy_hal_exti_set_pending(xy_hal_exti_line_t line)
{
    if (line >= XY_HAL_EXTI_LINE_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    __HAL_EXTI_GENERATE_SWIT(xy_to_stm32_line(line));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_clear_pending(xy_hal_exti_line_t line)
{
    if (line >= XY_HAL_EXTI_LINE_MAX) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }

    __HAL_EXTI_CLEAR_FLAG(xy_to_stm32_line(line));
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_exti_generate_software_interrupt(xy_hal_exti_line_t line)
{
    return xy_hal_exti_set_pending(line);
}

void xy_hal_exti_irq_handler(xy_hal_exti_line_t line)
{
    if (line >= XY_HAL_EXTI_LINE_MAX) {
        return;
    }

    if (g_exti_ctx.callbacks[line].callback) {
        g_exti_ctx.callbacks[line].callback(line, g_exti_ctx.callbacks[line].arg);
    }
}

xy_hal_error_t xy_hal_exti_map_gpio(uint8_t port, uint8_t pin,
                                    xy_hal_exti_line_t line)
{
    XY_UNUSED(port);
    XY_UNUSED(pin);
    XY_UNUSED(line);

    /* GPIO to EXTI mapping is handled by HAL_GPIO_Init */
    /* This function is a placeholder for platform-specific mapping */
    return XY_HAL_OK;
}

/* EXTI callback from HAL - called from STM32 HAL */
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
    for (int i = 0; i < 16; i++) {
        if (GPIO_Pin & (1U << i)) {
            xy_hal_exti_irq_handler((xy_hal_exti_line_t)i);
        }
    }
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    for (int i = 0; i < 16; i++) {
        if (GPIO_Pin & (1U << i)) {
            xy_hal_exti_irq_handler((xy_hal_exti_line_t)i);
        }
    }
}

#endif /* STM32U5 || STM32U5xx */
