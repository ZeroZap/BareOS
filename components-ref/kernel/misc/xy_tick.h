/**
 * @file xy_tick.h
 * @brief XinYi System Tick Module - Provides millisecond tick count
 * @version 1.0.0
 * @date 2026-02-28
 *
 * This module provides a simple system tick counter for bare-metal applications.
 * It should be called from a timer interrupt (e.g., SysTick) at a fixed frequency.
 */

#ifndef _XY_TICK_H_
#define _XY_TICK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Initialize the tick module
 * @param tick_freq_hz Tick frequency in Hz (e.g., 1000 for 1ms)
 */
void xy_tick_init(uint32_t tick_freq_hz);

/**
 * @brief Get current tick count
 * @return Current tick count since initialization
 */
uint32_t xy_tick_get(void);

/**
 * @brief Get tick frequency
 * @return Tick frequency in Hz
 */
uint32_t xy_tick_get_freq(void);

/**
 * @brief Get elapsed ticks since a given tick count
 * @param start_tick Starting tick count
 * @return Elapsed ticks
 */
static inline uint32_t xy_tick_elapsed_since(uint32_t start_tick)
{
    return xy_tick_get() - start_tick;
}

/**
 * @brief Check if elapsed time has reached a target
 * @param start_tick Starting tick count
 * @param interval Target interval in ticks
 * @return 1 if elapsed >= interval, 0 otherwise
 */
static inline int xy_tick_has_elapsed(uint32_t start_tick, uint32_t interval)
{
    return (xy_tick_elapsed_since(start_tick) >= interval) ? 1 : 0;
}

/**
 * @brief Wait for a specified number of ticks (busy-wait)
 * @param ticks Number of ticks to wait
 */
void xy_tick_delay(uint32_t ticks);

/**
 * @brief Wait until absolute tick count (busy-wait)
 * @param tick Absolute tick count to wait until
 */
void xy_tick_delay_until(uint32_t tick);

/**
 * @brief Increment tick counter (call from timer ISR)
 */
void xy_tick_increment(void);

/**
 * @brief Set current tick count
 * @param tick New tick value
 */
void xy_tick_set(uint32_t tick);

/**
 * @brief Get tick count value safely (handles overflow)
 * @param tick Pointer to store tick value
 */
void xy_tick_get_safe(uint32_t *tick);

#ifdef __cplusplus
}
#endif

#endif /* _XY_TICK_H_ */
