#ifndef __SENSOR_CONFIG_MINIMAL_H__
#define __SENSOR_CONFIG_MINIMAL_H__

/* 最小化配置 - 仅保留核心功能 */

#define SENSOR_MAX_DEVICES  8
#define SENSOR_NAME_MAX_LEN 16

/* 禁用所有高级功能 */
#define SENSOR_ENABLE_FIFO          0
#define SENSOR_ENABLE_INTERRUPT     0
#define SENSOR_ENABLE_CALIBRATION   0
#define SENSOR_ENABLE_FILTER        0
#define SENSOR_ENABLE_DMA           0
#define SENSOR_ENABLE_FUSION        0
#define SENSOR_ENABLE_POWER_MGMT    0
#define SENSOR_ENABLE_SELF_TEST     0
#define SENSOR_ENABLE_THRESHOLD     0
#define SENSOR_ENABLE_MOTION_DETECT 0

/* 系统配置 */
#define SENSOR_USE_FILE_SYSTEM 0
#define SENSOR_USE_OSAL        0
#define SENSOR_USE_MALLOC      0 /* 使用静态内存池 */
#define SENSOR_USE_FLOAT       0 /* 仅使用整数运算 */

/* 调试 */
#define SENSOR_DEBUG 0

#endif