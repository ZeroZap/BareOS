/**
 * @file xy_hal_pin_pc.c
 * @brief PC/Linux simulation layer for GPIO/PIN HAL
 */

#include "xy_hal_pin.h"
#include <stdio.h>
#include <stdlib.h>

// Simple PC simulation - just track state
typedef struct {
    int value;
    int mode;
    int pull;
} pin_state_t;

static pin_state_t pin_states[256];

xy_hal_error_t xy_hal_pin_init(void *port, uint8_t pin, const xy_hal_pin_config_t *config)
{
    (void)port;
    int idx = pin;
    
    if (idx >= 256) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    pin_states[idx].mode = (int)config->mode;
    pin_states[idx].pull = (int)config->pull;
    pin_states[idx].value = 0;
    
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_pin_write(void *port, uint8_t pin, xy_hal_pin_state_t state)
{
    (void)port;
    int idx = pin;
    
    if (idx >= 256) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    pin_states[idx].value = (state == XY_HAL_PIN_HIGH) ? 1 : 0;
    return XY_HAL_OK;
}

int xy_hal_pin_read(void *port, uint8_t pin)
{
    (void)port;
    int idx = pin;
    
    if (idx >= 256) {
        return 0;
    }
    
    return pin_states[idx].value;
}

xy_hal_error_t xy_hal_pin_toggle(void *port, uint8_t pin)
{
    (void)port;
    int idx = pin;
    
    if (idx >= 256) {
        return XY_HAL_ERROR_INVALID_PARAM;
    }
    
    pin_states[idx].value = !pin_states[idx].value;
    return XY_HAL_OK;
}

/* Handle-based pin write for device driver compatibility */
int xy_hal_pin_write_handle(void *pin_handle, int state)
{
    (void)pin_handle;
    (void)state;
    /* PC simulation: just acknowledge */
    return 0;
}
