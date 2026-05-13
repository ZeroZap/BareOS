#include "xy_hal_gpio.h"
#include "xy_hal_error.h"
#include "xy_hal_pc.h"  // Include PC-specific header for struct definition
#include <stdio.h>

// PC simulation GPIO implementation
xy_hal_error_t xy_hal_gpio_init(xy_hal_gpio_port_t port, uint8_t pin, const xy_hal_gpio_config_t *config) {
    printf("PC GPIO: Initializing GPIO port_id=%d, pin=%d\n", port->port_id, pin);
    return XY_HAL_OK;
}

xy_hal_error_t xy_hal_gpio_write(xy_hal_gpio_port_t port, uint8_t pin, uint8_t value) {
    printf("PC GPIO: Writing GPIO port_id=%d, pin=%d, value=%d\n", port->port_id, pin, value);
    return XY_HAL_OK;
}

int32_t xy_hal_gpio_read(xy_hal_gpio_port_t port, uint8_t pin) {
    printf("PC GPIO: Reading GPIO port_id=%d, pin=%d\n", port->port_id, pin);
    // Return simulated value (0 or 1)
    return 1; // Simulate high level
}