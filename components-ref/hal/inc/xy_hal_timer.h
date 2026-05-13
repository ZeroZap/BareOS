/**
 * @file xy_hal_timer.h
 * @brief Timer Hardware Abstraction Layer
 * @version 2.0
 * @date 2025-10-26
 */

#ifndef XY_HAL_TIMER_H
#define XY_HAL_TIMER_H
#include "xy_hal_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "xy_hal.h"

/* Timer Counter Mode */
typedef enum {
    XY_HAL_TIMER_COUNT_UP = 0,  /**< Count up mode */
    XY_HAL_TIMER_COUNT_DOWN,    /**< Count down mode */
    XY_HAL_TIMER_COUNT_CENTER1, /**< Center-aligned mode 1 */
    XY_HAL_TIMER_COUNT_CENTER2, /**< Center-aligned mode 2 */
    XY_HAL_TIMER_COUNT_CENTER3, /**< Center-aligned mode 3 */
} xy_hal_timer_count_mode_t;

/* Timer Clock Division */
typedef enum {
    XY_HAL_TIMER_CKDIV_1 = 0, /**< No division */
    XY_HAL_TIMER_CKDIV_2,     /**< Division by 2 */
    XY_HAL_TIMER_CKDIV_4,     /**< Division by 4 */
} xy_hal_timer_ckdiv_t;

/* Timer Configuration Structure */
typedef struct {
    uint32_t prescaler;             /**< Prescaler value (0-65535) */
    uint32_t period;                /**< Period value (auto-reload) */
    xy_hal_timer_count_mode_t mode; /**< Counter mode */
    xy_hal_timer_ckdiv_t clock_div; /**< Clock division */
    uint8_t
        auto_reload_preload; /**< Auto-reload preload: 1=enable, 0=disable */
} xy_hal_timer_config_t;

/* Timer Event Types */
typedef enum {
    XY_HAL_TIMER_EVENT_UPDATE = 0, /**< Update event (overflow) */
    XY_HAL_TIMER_EVENT_CC1,        /**< Capture/Compare 1 event */
    XY_HAL_TIMER_EVENT_CC2,        /**< Capture/Compare 2 event */
    XY_HAL_TIMER_EVENT_CC3,        /**< Capture/Compare 3 event */
    XY_HAL_TIMER_EVENT_CC4,        /**< Capture/Compare 4 event */
} xy_hal_timer_event_t;

/* Timer callback */
typedef void (*xy_hal_timer_callback_t)(void *timer, xy_hal_timer_event_t event,
                                        void *arg);

/**
 * @brief Initialize timer
 * @param timer Timer instance
 * @param config Timer configuration
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_timer_init(void *timer,
                                 const xy_hal_timer_config_t *config);

/**
 * @brief Deinitialize timer
 * @param timer Timer instance
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_timer_deinit(void *timer);

/**
 * @brief Start timer
 * @param timer Timer instance
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_timer_start(void *timer);

/**
 * @brief Stop timer
 * @param timer Timer instance
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_timer_stop(void *timer);

/**
 * @brief Get timer counter value
 * @param timer Timer instance
 * @return Counter value, negative on error
 */
int xy_hal_timer_get_counter(void *timer);

/**
 * @brief Set timer counter value
 * @param timer Timer instance
 * @param value Counter value
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_timer_set_counter(void *timer, uint32_t value);

/**
 * @brief Set timer period
 * @param timer Timer instance
 * @param period Period value
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_timer_set_period(void *timer, uint32_t period);

/**
 * @brief Register timer callback
 * @param timer Timer instance
 * @param event Event type
 * @param callback Callback function
 * @param arg User argument
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_timer_register_callback(void *timer,
                                              xy_hal_timer_event_t event,
                                              xy_hal_timer_callback_t callback,
                                              void *arg);

/**
 * @brief Enable timer interrupt
 * @param timer Timer instance
 * @param event Event type
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_timer_enable_irq(void *timer, xy_hal_timer_event_t event);

/**
 * @brief Disable timer interrupt
 * @param timer Timer instance
 * @param event Event type
 * @return 0 on success, negative on error
 */
xy_hal_error_t xy_hal_timer_disable_irq(void *timer,
                                        xy_hal_timer_event_t event);

#ifdef __cplusplus
}
#endif

#endif /* XY_HAL_TIMER_H */