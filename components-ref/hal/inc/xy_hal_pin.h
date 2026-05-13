/**
 * @file xy_hal_pin.h
 * @brief GPIO/Pin Hardware Abstraction Layer
 * @version 2.0
 * @date 2025-10-26
 */

#ifndef XY_HAL_PIN_H
#define XY_HAL_PIN_H
#include "xy_hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "xy_hal.h"

/* GPIO Pin Modes */
typedef enum {
    XY_HAL_PIN_MODE_INPUT = 0, /**< Input mode */
    XY_HAL_PIN_MODE_OUTPUT,    /**< Output mode */
    XY_HAL_PIN_MODE_AF,        /**< Alternate function */
    XY_HAL_PIN_MODE_ANALOG,    /**< Analog mode */
} xy_hal_pin_mode_t;

/* GPIO Pull modes */
typedef enum {
    XY_HAL_PIN_PULL_NONE = 0, /**< No pull */
    XY_HAL_PIN_PULL_UP,       /**< Pull-up */
    XY_HAL_PIN_PULL_DOWN,     /**< Pull-down */
} xy_hal_pin_pull_t;

/* GPIO Output types */
typedef enum {
    XY_HAL_PIN_OTYPE_PP = 0, /**< Push-pull */
    XY_HAL_PIN_OTYPE_OD,     /**< Open-drain */
} xy_hal_pin_otype_t;

/* GPIO Speed */
typedef enum {
    XY_HAL_PIN_SPEED_LOW = 0,   /**< Low speed */
    XY_HAL_PIN_SPEED_MEDIUM,    /**< Medium speed */
    XY_HAL_PIN_SPEED_HIGH,      /**< High speed */
    XY_HAL_PIN_SPEED_VERY_HIGH, /**< Very high speed */
} xy_hal_pin_speed_t;

/* GPIO Pin State */
typedef enum {
    XY_HAL_PIN_LOW  = 0, /**< Low level */
    XY_HAL_PIN_HIGH = 1, /**< High level */
} xy_hal_pin_state_t;

/* Interrupt Trigger Mode */
typedef enum {
    XY_HAL_PIN_IRQ_RISING = 0, /**< Rising edge */
    XY_HAL_PIN_IRQ_FALLING,    /**< Falling edge */
    XY_HAL_PIN_IRQ_BOTH,       /**< Both edges */
} xy_hal_pin_irq_mode_t;

/* GPIO Configuration Structure */
typedef struct {
    xy_hal_pin_mode_t mode;   /**< Pin mode */
    xy_hal_pin_pull_t pull;   /**< Pull mode */
    xy_hal_pin_otype_t otype; /**< Output type */
    xy_hal_pin_speed_t speed; /**< Speed */
    uint8_t alternate;        /**< Alternate function (0-15) */
} xy_hal_pin_config_t;

/* GPIO interrupt callback */
typedef void (*xy_hal_pin_irq_handler_t)(void *arg);

/**
 * @brief Initialize GPIO pin
 * @param port GPIO port (e.g., GPIOA, GPIOB)
 * @param pin Pin number (0-15)
 * @param config Pin configuration
 * @return XY_HAL_OK on success, XY_HAL_ERROR_INVALID_PARAM if parameters
 * invalid
 */
xy_hal_error_t xy_hal_pin_init(void *port, uint8_t pin,
                               const xy_hal_pin_config_t *config);

/**
 * @brief Deinitialize GPIO pin
 * @param port GPIO port
 * @param pin Pin number
 * @return XY_HAL_OK on success, XY_HAL_ERROR_INVALID_PARAM if parameters
 * invalid
 */
xy_hal_error_t xy_hal_pin_deinit(void *port, uint8_t pin);

/**
 * @brief Write pin state
 * @param port GPIO port
 * @param pin Pin number
 * @param state Pin state (LOW/HIGH)
 * @return XY_HAL_OK on success, XY_HAL_ERROR_INVALID_PARAM if parameters
 * invalid
 */
xy_hal_error_t xy_hal_pin_write(void *port, uint8_t pin,
                                xy_hal_pin_state_t state);

/**
 * @brief Read pin state
 * @param port GPIO port
 * @param pin Pin number
 * @return Pin state (0=LOW, 1=HIGH), negative error code on failure
 */
int xy_hal_pin_read(void *port, uint8_t pin);

/**
 * @brief Toggle pin state
 * @param port GPIO port
 * @param pin Pin number
 * @return XY_HAL_OK on success, XY_HAL_ERROR_INVALID_PARAM if parameters
 * invalid
 */
xy_hal_error_t xy_hal_pin_toggle(void *port, uint8_t pin);

/**
 * @brief Attach interrupt to pin
 * @param port GPIO port
 * @param pin Pin number
 * @param mode Trigger mode
 * @param handler Interrupt handler
 * @param arg User argument
 * @return XY_HAL_OK on success, XY_HAL_ERROR_INVALID_PARAM if parameters
 * invalid, XY_HAL_ERROR_NO_RESOURCE if no IRQ resources available
 */
xy_hal_error_t xy_hal_pin_attach_irq(void *port, uint8_t pin,
                                     xy_hal_pin_irq_mode_t mode,
                                     xy_hal_pin_irq_handler_t handler,
                                     void *arg);

/**
 * @brief Detach interrupt from pin
 * @param port GPIO port
 * @param pin Pin number
 * @return XY_HAL_OK on success, XY_HAL_ERROR_INVALID_PARAM if parameters
 * invalid
 */
xy_hal_error_t xy_hal_pin_detach_irq(void *port, uint8_t pin);

/**
 * @brief Enable pin interrupt
 * @param port GPIO port
 * @param pin Pin number
 * @return XY_HAL_OK on success, XY_HAL_ERROR_INVALID_PARAM if parameters
 * invalid
 */
xy_hal_error_t xy_hal_pin_irq_enable(void *port, uint8_t pin);

/**
 * @brief Disable pin interrupt
 * @param port GPIO port
 * @param pin Pin number
 * @return XY_HAL_OK on success, XY_HAL_ERROR_INVALID_PARAM if parameters
 * invalid
 */
xy_hal_error_t xy_hal_pin_irq_disable(void *port, uint8_t pin);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_PIN_H */