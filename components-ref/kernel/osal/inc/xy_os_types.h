/**
 * @file xy_os_types.h
 * @brief OSAL (Operating System Abstraction Layer) Basic Types
 * @version 1.0.0
 * 
 * This header provides basic type definitions that are needed
 * before xy_os.h can be included. It avoids circular dependencies.
 */

#ifndef XY_OS_TYPES_H
#define XY_OS_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* OS Tick type - used for time tracking */
typedef uint32_t xy_os_tick_t;

/* Forward declaration of OS status type */
typedef int xy_os_status_t;

/* Forward declaration of thread ID type */
typedef void *xy_os_thread_id_t;

/* Forward declaration of timer ID type */
typedef void *xy_os_timer_id_t;

/* Forward declaration of mutex ID type */
typedef void *xy_os_mutex_id_t;

/* Forward declaration of semaphore ID type */
typedef void *xy_os_semaphore_id_t;

/* Forward declaration of event flags ID type */
typedef void *xy_os_event_flags_id_t;

/* Forward declaration of message queue ID type */
typedef void *xy_os_msgqueue_id_t;

/* Forward declaration of memory pool ID type */
typedef void *xy_os_mempool_id_t;

#ifdef __cplusplus
}
#endif

#endif /* XY_OS_TYPES_H */