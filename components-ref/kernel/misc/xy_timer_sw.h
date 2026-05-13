/**
 * @file xy_timer_sw.h
 * @brief Software Timer Module for Bare-metal Applications
 * @version 1.0.0
 * @date 2026-02-28
 */

#ifndef _XY_TIMER_SW_H_
#define _XY_TIMER_SW_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Software timer ID type
 */
typedef uint8_t xy_timer_sw_id_t;

/**
 * @brief Invalid timer ID
 */
#define XY_TIMER_SW_INVALID_ID  0

/**
 * @brief Software timer error codes
 */
typedef enum {
    XY_TIMER_SW_OK = 0,
    XY_TIMER_SW_ERR_INVALID = -1,
    XY_TIMER_SW_ERR_FULL = -2,
} xy_timer_sw_error_t;

/**
 * @brief Software timer callback type
 */
typedef void (*xy_timer_sw_callback_t)(void *arg);

/**
 * @brief Initialize software timer module
 */
void xy_timer_sw_init(void);

/**
 * @brief Create a software timer
 * @param interval Timer interval in ticks
 * @param callback Callback function
 * @param arg Callback argument
 * @param periodic 1=periodic, 0=one-shot
 * @return Timer ID or XY_TIMER_SW_INVALID_ID on error
 */
xy_timer_sw_id_t xy_timer_sw_create(uint32_t interval, xy_timer_sw_callback_t callback,
                                    void *arg, uint8_t periodic);

/**
 * @brief Start a software timer
 * @param id Timer ID
 * @return XY_TIMER_SW_OK on success, error code on failure
 */
xy_timer_sw_error_t xy_timer_sw_start(xy_timer_sw_id_t id);

/**
 * @brief Stop a software timer
 * @param id Timer ID
 * @return XY_TIMER_SW_OK on success, error code on failure
 */
xy_timer_sw_error_t xy_timer_sw_stop(xy_timer_sw_id_t id);

/**
 * @brief Delete a software timer
 * @param id Timer ID
 * @return XY_TIMER_SW_OK on success, error code on failure
 */
xy_timer_sw_error_t xy_timer_sw_delete(xy_timer_sw_id_t id);

/**
 * @brief Poll software timers (call periodically)
 * @note Call this function from main loop or timer ISR
 */
void xy_timer_sw_poll(void);

#ifdef __cplusplus
}
#endif

#endif /* _XY_TIMER_SW_H_ */
