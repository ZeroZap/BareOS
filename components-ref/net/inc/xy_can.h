/**
 * @file xy_can.h
 * @brief XinYi CAN Protocol Header
 */

#ifndef XY_CAN_H
#define XY_CAN_H

#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

// CAN message structure
typedef struct {
    uint32_t id;        // CAN ID
    uint8_t data[8];    // Data bytes
    uint8_t len;        // Data length (0-8)
    uint8_t rtr;        // Remote transmission request
} xy_can_msg_t;

// CAN functions
int xy_can_init(void);
int xy_can_send(const xy_can_msg_t* msg);
int xy_can_recv(xy_can_msg_t* msg, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // XY_CAN_H