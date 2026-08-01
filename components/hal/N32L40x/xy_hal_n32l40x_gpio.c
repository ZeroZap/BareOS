#include "xy_hal_n32l40x_internal.h"

static GPIO_PuPdType map_pull(xy_hal_gpio_pull_t pull)
{
    if (pull == XY_HAL_GPIO_PULLUP) return GPIO_Pull_Up;
    if (pull == XY_HAL_GPIO_PULLDOWN) return GPIO_Pull_Down;
    return GPIO_No_Pull;
}

static GPIO_SpeedType map_speed(xy_hal_gpio_speed_t speed)
{
    return speed >= XY_HAL_GPIO_SPEED_HIGH ? GPIO_Slew_Rate_High : GPIO_Slew_Rate_Low;
}

static GPIO_CurrentType map_current(xy_hal_gpio_speed_t speed)
{
    switch (speed) {
    case XY_HAL_GPIO_SPEED_LOW: return GPIO_DC_2mA;
    case XY_HAL_GPIO_SPEED_MEDIUM: return GPIO_DC_4mA;
    case XY_HAL_GPIO_SPEED_HIGH: return GPIO_DC_8mA;
    default: return GPIO_DC_12mA;
    }
}

static GPIO_ModeType map_mode(const xy_hal_gpio_config_t *config)
{
    if (config->mode == XY_HAL_GPIO_ANALOG) return GPIO_Mode_Analog;
    if (config->mode == XY_HAL_GPIO_INPUT) return GPIO_Mode_Input;
    if (config->mode == XY_HAL_GPIO_OUTPUT) {
        return config->output_type == XY_HAL_GPIO_OPEN_DRAIN ? GPIO_Mode_Out_OD : GPIO_Mode_Out_PP;
    }
    return config->output_type == XY_HAL_GPIO_OPEN_DRAIN ? GPIO_Mode_AF_OD : GPIO_Mode_AF_PP;
}

int xy_hal_gpio_init(uint32_t pin, const xy_hal_gpio_config_t *config)
{
    GPIO_Module *port = xy_hal_n32_gpio_port(pin);
    uint32_t port_idx = XY_HAL_N32_PIN_PORT(pin);
    GPIO_InitType init;
    if (!port || !config || XY_HAL_N32_PIN_INDEX(pin) > 15u) return XY_HAL_INVALID_PARAM;
    RCC_EnableAPB2PeriphClk(xy_hal_n32_gpio_clk(port_idx) | RCC_APB2_PERIPH_AFIO, ENABLE);
    memset(&init, 0, sizeof(init));
    init.Pin = xy_hal_n32_gpio_pin_mask(pin);
    init.GPIO_Current = map_current(config->speed);
    init.GPIO_Slew_Rate = map_speed(config->speed);
    init.GPIO_Pull = map_pull(config->pull);
    init.GPIO_Mode = map_mode(config);
    init.GPIO_Alternate = config->alternate;
    GPIO_InitPeripheral(port, &init);
    return XY_HAL_OK;
}

int xy_hal_gpio_config_af(uint32_t pin, uint32_t alternate,
                          xy_hal_gpio_speed_t speed,
                          xy_hal_gpio_output_t output_type,
                          xy_hal_gpio_pull_t pull)
{
    xy_hal_gpio_config_t cfg;
    cfg.pin = pin;
    cfg.mode = XY_HAL_GPIO_ALT;
    cfg.pull = pull;
    cfg.output_type = output_type;
    cfg.speed = speed;
    cfg.alternate = alternate;
    return xy_hal_gpio_init(pin, &cfg);
}

int xy_hal_gpio_write(uint32_t pin, int level)
{
    GPIO_Module *port = xy_hal_n32_gpio_port(pin);
    if (!port) return XY_HAL_INVALID_PARAM;
    GPIO_WriteBit(port, xy_hal_n32_gpio_pin_mask(pin), level ? Bit_SET : Bit_RESET);
    return XY_HAL_OK;
}

int xy_hal_gpio_read(uint32_t pin)
{
    GPIO_Module *port = xy_hal_n32_gpio_port(pin);
    if (!port) return XY_HAL_INVALID_PARAM;
    return GPIO_ReadInputDataBit(port, xy_hal_n32_gpio_pin_mask(pin)) ? 1 : 0;
}

int xy_hal_gpio_toggle(uint32_t pin)
{
    GPIO_Module *port = xy_hal_n32_gpio_port(pin);
    uint16_t mask = xy_hal_n32_gpio_pin_mask(pin);
    if (!port) return XY_HAL_INVALID_PARAM;
    GPIO_WriteBit(port, mask, GPIO_ReadOutputDataBit(port, mask) ? Bit_RESET : Bit_SET);
    return XY_HAL_OK;
}

int xy_hal_gpio_irq_configure(uint32_t pin, const xy_hal_gpio_irq_config_t *config)
{
    uint32_t port_idx = XY_HAL_N32_PIN_PORT(pin);
    uint32_t idx = XY_HAL_N32_PIN_INDEX(pin);
    EXTI_InitType exti;
    if (port_idx > XY_HAL_N32_PORT_D || idx > 15u || !config) return XY_HAL_INVALID_PARAM;
    if (config->trigger == XY_HAL_GPIO_IRQ_LOW_LEVEL || config->trigger == XY_HAL_GPIO_IRQ_HIGH_LEVEL) return XY_HAL_NOT_SUPPORTED;
    RCC_EnableAPB2PeriphClk(xy_hal_n32_gpio_clk(port_idx) | RCC_APB2_PERIPH_AFIO, ENABLE);
    GPIO_ConfigEXTILine((uint8_t)port_idx, xy_hal_n32_gpio_pin_source(pin));
    EXTI_InitStruct(&exti);
    exti.EXTI_Line = xy_hal_n32_exti_line(pin);
    exti.EXTI_Mode = EXTI_Mode_Interrupt;
    if (config->trigger == XY_HAL_GPIO_IRQ_FALLING) exti.EXTI_Trigger = EXTI_Trigger_Falling;
    else if (config->trigger == XY_HAL_GPIO_IRQ_BOTH) exti.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    else exti.EXTI_Trigger = EXTI_Trigger_Rising;
    exti.EXTI_LineCmd = config->trigger == XY_HAL_GPIO_IRQ_NONE ? DISABLE : ENABLE;
    EXTI_InitPeripheral(&exti);
    g_xy_hal_n32_gpio_irq[port_idx][idx].cb = config->callback;
    g_xy_hal_n32_gpio_irq[port_idx][idx].user = config->user;
    g_xy_hal_n32_gpio_irq[port_idx][idx].trigger = config->trigger;
    g_xy_hal_n32_gpio_irq[port_idx][idx].enabled = exti.EXTI_LineCmd == ENABLE;
    (void)config->priority;
    (void)config->wakeup;
    return XY_HAL_OK;
}

int xy_hal_gpio_irq_enable(uint32_t pin)
{
    uint32_t port_idx = XY_HAL_N32_PIN_PORT(pin);
    uint32_t idx = XY_HAL_N32_PIN_INDEX(pin);
    if (port_idx > XY_HAL_N32_PORT_D || idx > 15u) return XY_HAL_INVALID_PARAM;
    g_xy_hal_n32_gpio_irq[port_idx][idx].enabled = true;
    return XY_HAL_OK;
}

int xy_hal_gpio_irq_disable(uint32_t pin)
{
    uint32_t port_idx = XY_HAL_N32_PIN_PORT(pin);
    uint32_t idx = XY_HAL_N32_PIN_INDEX(pin);
    if (port_idx > XY_HAL_N32_PORT_D || idx > 15u) return XY_HAL_INVALID_PARAM;
    g_xy_hal_n32_gpio_irq[port_idx][idx].enabled = false;
    return XY_HAL_OK;
}

void xy_hal_gpio_irq_handler(uint32_t pin)
{
    uint32_t port_idx = XY_HAL_N32_PIN_PORT(pin);
    uint32_t idx = XY_HAL_N32_PIN_INDEX(pin);
    xy_hal_n32_gpio_irq_state_t *st;
    if (port_idx > XY_HAL_N32_PORT_D || idx > 15u) return;
    if (EXTI_GetITStatus(xy_hal_n32_exti_line(pin)) != RESET) EXTI_ClrITPendBit(xy_hal_n32_exti_line(pin));
    st = &g_xy_hal_n32_gpio_irq[port_idx][idx];
    if (st->enabled && st->cb) st->cb(pin, st->user);
}
