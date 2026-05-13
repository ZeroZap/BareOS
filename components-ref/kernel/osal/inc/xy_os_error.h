/**
 * @file xy_os_error.h
 * @brief OSAL Error Type Definition
 */

#ifndef XY_OS_ERROR_H
#define XY_OS_ERROR_H

#include "xy_os_types.h"

/* xy_os_error_t is an alias for xy_os_status_t */
typedef xy_os_status_t xy_os_error_t;

/* Error code aliases for compatibility */
#define XY_OS_EINVAL XY_OS_ERROR_PARAMETER
#define XY_OS_ENOMEM XY_OS_ERROR_NO_MEMORY
#define XY_OS_ETIMEOUT XY_OS_ERROR_TIMEOUT
#define XY_OS_EBUSY XY_OS_ERROR_RESOURCE
#define XY_OS_ENOTSUP XY_OS_ERROR
#define XY_OS_ENOENT XY_OS_ERROR_RESOURCE
#define XY_OS_EIO XY_OS_ERROR
#define XY_OS_EPERM XY_OS_ERROR
#define XY_OS_EAGAIN XY_OS_ERROR_TIMEOUT

/* Callback type */
typedef void (*xy_os_callback_t)(void *arg);

#endif /* XY_OS_ERROR_H */
