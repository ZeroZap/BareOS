/**
 * @file xy_hal_n32l40x.h
 * @brief N32L40x BareOS HAL port helpers.
 */

#ifndef XY_HAL_N32L40X_H
#define XY_HAL_N32L40X_H

#include <stdint.h>
#include "xy_hal.h"
#include "n32l40x.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XY_HAL_N32_PORT_A 0u
#define XY_HAL_N32_PORT_B 1u
#define XY_HAL_N32_PORT_C 2u
#define XY_HAL_N32_PORT_D 3u

#define XY_HAL_N32_PIN(port, index) ((((uint32_t)(port) & 0x0fu) << 4) | \
                                     ((uint32_t)(index) & 0x0fu))
#define XY_HAL_N32_PIN_PORT(pin)    (((uint32_t)(pin) >> 4) & 0x0fu)
#define XY_HAL_N32_PIN_INDEX(pin)   ((uint32_t)(pin) & 0x0fu)

typedef struct {
    uint32_t irq_mask;
    xy_hal_event_cb_t callback;
    void *user;
} xy_hal_n32_irq_state_t;

typedef struct {
    uint32_t scl_pin;
    uint32_t sda_pin;
    uint32_t alternate;
    xy_hal_gpio_pull_t pull;
    xy_hal_gpio_speed_t speed;
    int (*reset_target)(void *ctx);
    void *reset_ctx;
} xy_hal_n32_i2c_recovery_pins_t;

int xy_hal_n32l40x_i2c_config_recovery(I2C_Module *i2c,
                                        const xy_hal_n32_i2c_recovery_pins_t *pins,
                                        const xy_hal_i2c_recovery_config_t *config);

/* Board/system hooks. Override from BSP when clock tree or watchdog differs. */
void xy_hal_n32l40x_after_stop_restore_clock(void);
void xy_hal_n32l40x_before_sleep(void);
void xy_hal_n32l40x_after_wake(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_N32L40X_H */
