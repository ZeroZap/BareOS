/**
 * @file xy_net.h
 * @brief XinYi Network Component Main Header
 */

#ifndef XY_NET_H
#define XY_NET_H

#include "xy_net_config.h"
#include "xy_net_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize network component
 * @return 0 on success, negative error code on failure
 */
int xy_net_init(void);

/**
 * @brief Deinitialize network component  
 * @return 0 on success, negative error code on failure
 */
int xy_net_deinit(void);

// Include protocol-specific headers based on configuration
#if XY_NET_ENABLE_MODBUS
#include "mb_slave.h"
#endif

// AT Client (Server disabled for now)
#include "xy_at_client.h"

#ifdef __cplusplus
}
#endif

#endif // XY_NET_H