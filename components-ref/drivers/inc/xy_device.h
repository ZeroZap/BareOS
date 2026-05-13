
#ifndef __XY_DEVICE_H__
#define __XY_DEVICE_H__
#include "xy.h"

#include "xy_rb.h"
typedef struct rt_device *rt_device_t;
struct xy_device {
    struct xy_obj parent;
    size_t type;
    uint16_t flag;
    uint16_t ref_count;
    uint8_t device_id; // 设备ID，同类设备一般不超过 255的吧

    xy_err_t (*rx_indicate)(rt_device_t device, xy_size_t size);
};
#endif
