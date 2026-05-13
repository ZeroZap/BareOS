/**
 * @file xy_hal_lp_timer.h
 * @brief Low Power Timer Hardware Abstraction Layer
 * @version 2.0
 * @date 2025-10-26
 */

#ifndef XY_HAL_LP_TIMER_H
#define XY_HAL_LP_TIMER_H
#include "xy_hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "xy_hal.h"

/* LP Timer Clock Source */
typedef enum {
    XY_HAL_LPTIMER_CLK_INTERNAL = 0, /**< Internal clock */
    XY_HAL_LPTIMER_CLK_EXTERNAL,     /**< External clock */
} xy_hal_lptimer_clk_src_t;

/* LP Timer Prescaler */
typedef enum {
    XY_HAL_LPTIMER_PRESCALER_1 = 0, /**< /1 */
    XY_HAL_LPTIMER_PRESCALER_2,     /**< /2 */
    XY_HAL_LPTIMER_PRESCALER_4,     /**< /4 */
    XY_HAL_LPTIMER_PRESCALER_8,     /**< /8 */
    XY_HAL_LPTIMER_PRESCALER_16,    /**< /16 */
    XY_HAL_LPTIMER_PRESCALER_32,    /**< /32 */
    XY_HAL_LPTIMER_PRESCALER_64,    /**< /64 */
    XY_HAL_LPTIMER_PRESCALER_128,   /**< /128 */
} xy_hal_lptimer_prescaler_t;

/* LP Timer Configuration Structure */
typedef struct {
    xy_hal_lptimer_clk_src_t clk_src;     /**< Clock source */
    xy_hal_lptimer_prescaler_t prescaler; /**< Prescaler */
    uint32_t period;                      /**< Period value */
} xy_hal_lptimer_config_t;

/* LP Timer callback */
typedef void (*xy_hal_lptimer_callback_t)(void *lptimer, void *arg);

/**
 * @brief Initialize low power timer
 * @param lptimer LP Timer instance
 * @param config LP Timer configuration
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_lptimer_init(void *lptimer,
                                   const xy_hal_lptimer_config_t *config);

/**
 * @brief Deinitialize low power timer
 * @param lptimer LP Timer instance
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_lptimer_deinit(void *lptimer);

/**
 * @brief Start low power timer
 * @param lptimer LP Timer instance
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_lptimer_start(void *lptimer);

/**
 * @brief Stop low power timer
 * @param lptimer LP Timer instance
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_lptimer_stop(void *lptimer);

/**
 * @brief Get low power timer counter
 * @param lptimer LP Timer instance
 * @return Counter value, negative on error
 */
int xy_hal_lptimer_get_counter(void *lptimer);

/**
 * @brief Register low power timer callback
 * @param lptimer LP Timer instance
 * @param callback Callback function
 * @param arg User argument
 * @return 0 on success, negative on error
 */
xy_hal_error_t
xy_hal_lptimer_register_callback(void *lptimer,
                                 xy_hal_lptimer_callback_t callback, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_LP_TIMER_H */