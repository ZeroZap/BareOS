#ifndef __SENSOR_APDS9960_H__
#define __SENSOR_APDS9960_H__

#include "sensor_core.h"

/* APDS9960 I2C地址 */
#define APDS9960_ADDR 0x39

/* 寄存器定义 */
#define APDS9960_REG_ENABLE  0x80
#define APDS9960_REG_ID      0x92
#define APDS9960_REG_CDATAL  0x94
#define APDS9960_REG_RDATAL  0x96
#define APDS9960_REG_GDATAL  0x98
#define APDS9960_REG_BDATAL  0x9A
#define APDS9960_REG_PDATA   0x9C
#define APDS9960_REG_GCONF4  0xAB
#define APDS9960_REG_GSTATUS 0xAF
#define APDS9960_REG_GFIFO_U 0xFC

/* 手势方向 */
typedef enum {
    APDS9960_GESTURE_NONE = 0,
    APDS9960_GESTURE_UP,
    APDS9960_GESTURE_DOWN,
    APDS9960_GESTURE_LEFT,
    APDS9960_GESTURE_RIGHT,
    APDS9960_GESTURE_NEAR,
    APDS9960_GESTURE_FAR,
} apds9960_gesture_t;

/* RGB数据结构 */
typedef struct {
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint16_t c; /* Clear/ambient */
} apds9960_rgb_t;

/* 私有数据 */
typedef struct {
    uint8_t i2c_addr;
    apds9960_gesture_t last_gesture;
} apds9960_priv_t;

sensor_device_t *apds9960_create_rgb(const char *name, void *i2c_bus);
sensor_device_t *apds9960_create_proximity(const char *name, void *i2c_bus);
sensor_device_t *apds9960_create_gesture(const char *name, void *i2c_bus);

#endif /* __SENSOR_APDS9960_H__ */