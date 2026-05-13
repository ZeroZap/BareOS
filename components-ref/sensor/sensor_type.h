#ifndef __SENSOR_TYPE_H__
#define __SENSOR_TYPE_H__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "sensor_config.h"

/* ==================== 传感器类型 ==================== */
typedef enum {
    SENSOR_TYPE_NONE = 0x00,

    /* 运动传感器 */
    SENSOR_TYPE_ACCELEROMETER       = 0x01,
    SENSOR_TYPE_GYROSCOPE           = 0x02,
    SENSOR_TYPE_MAGNETOMETER        = 0x03,
    SENSOR_TYPE_ORIENTATION         = 0x04,
    SENSOR_TYPE_GRAVITY             = 0x05,
    SENSOR_TYPE_LINEAR_ACCELERATION = 0x06,
    SENSOR_TYPE_ROTATION_VECTOR     = 0x07,

    /* 环境传感器 */
    SENSOR_TYPE_TEMPERATURE    = 0x10,
    SENSOR_TYPE_HUMIDITY       = 0x11,
    SENSOR_TYPE_PRESSURE       = 0x12,
    SENSOR_TYPE_AMBIENT_LIGHT  = 0x13,
    SENSOR_TYPE_AMBIENT_TEMP   = 0x14,
    SENSOR_TYPE_GAS_RESISTANCE = 0x15,
    SENSOR_TYPE_CO2            = 0x16,
    SENSOR_TYPE_TVOC           = 0x17,

    /* 位置传感器 */
    SENSOR_TYPE_PROXIMITY = 0x20,
    SENSOR_TYPE_DISTANCE  = 0x21,
    SENSOR_TYPE_GPS       = 0x22,
    SENSOR_TYPE_MAGNETIC = 0x23,

    /* 光学传感器 */
    SENSOR_TYPE_LIGHT = 0x30,
    SENSOR_TYPE_IR    = 0x31,
    SENSOR_TYPE_UV    = 0x32,
    SENSOR_TYPE_UV_INDEX = 0x33,
    SENSOR_TYPE_RGB   = 0x34,

    /* 生物传感器 */
    SENSOR_TYPE_HEART_RATE             = 0x40,
    SENSOR_TYPE_HEART_RATE_VARIABILITY = 0x41,
    SENSOR_TYPE_SPO2                   = 0x42,
    SENSOR_TYPE_BLOOD_PRESSURE         = 0x43,
    SENSOR_TYPE_ECG                    = 0x44,

    /* 其他传感器 */
    SENSOR_TYPE_VOLTAGE = 0x50,
    SENSOR_TYPE_CURRENT = 0x51,
    SENSOR_TYPE_POWER   = 0x52,
    SENSOR_TYPE_PM2_5   = 0x53,
    SENSOR_TYPE_PM10    = 0x54,
    SENSOR_TYPE_NOISE   = 0x55,
    SENSOR_TYPE_ANGLE   = 0x56,     ///< 角度编码器
    SENSOR_TYPE_RELATIVE_HUMIDITY = 0x57,  ///< 相对湿度
    SENSOR_TYPE_GAS     = 0x58,     ///< 气体传感器
    SENSOR_TYPE_CO      = 0x59,     ///< 一氧化碳
    SENSOR_TYPE_SOUND   = 0x5A,     ///< 声音/噪音

    SENSOR_TYPE_CUSTOM = 0xFF,
} sensor_type_t;

/* ==================== 单位定义 ==================== */
typedef enum {
    SENSOR_UNIT_NONE = 0,

    /* 加速度 */
    SENSOR_UNIT_METER_PER_SECOND_SQUARED,
    SENSOR_UNIT_MILLI_G,

    /* 角速度 */
    SENSOR_UNIT_RADIAN_PER_SECOND,
    SENSOR_UNIT_DEGREE_PER_SECOND,

    /* 磁场 */
    SENSOR_UNIT_GAUSS,
    SENSOR_UNIT_MICRO_TESLA,

    /* 温度 */
    SENSOR_UNIT_CELSIUS,
    SENSOR_UNIT_FAHRENHEIT,
    SENSOR_UNIT_KELVIN,

    /* 湿度 */
    SENSOR_UNIT_PERCENT,

    /* 压力 */
    SENSOR_UNIT_PASCAL,
    SENSOR_UNIT_KILOPASCAL,
    SENSOR_UNIT_HECTOPASCAL,

    /* 光照 */
    SENSOR_UNIT_LUX,

    /* 距离 */
    SENSOR_UNIT_METER,
    SENSOR_UNIT_CENTIMETER,
    SENSOR_UNIT_MILLIMETER,

    /* 浓度 */
    SENSOR_UNIT_PPM,
    SENSOR_UNIT_PPB,
    SENSOR_UNIT_UG_PER_CUBIC_METER,

    /* 电气 */
    SENSOR_UNIT_VOLT,
    SENSOR_UNIT_MILLIVOLT,
    SENSOR_UNIT_AMPERE,
    SENSOR_UNIT_MILLIAMPERE,
    SENSOR_UNIT_WATT,

    /* 生物 */
    SENSOR_UNIT_BPM,
    SENSOR_UNIT_MMHG,
} sensor_unit_t;

/* ==================== 数据值类型 ==================== */
typedef union {
    int32_t val_int32;
    uint32_t val_uint32;
#if SENSOR_USE_FLOAT
    float val_float;
    double val_double;
#endif
    struct {
        int32_t x;
        int32_t y;
        int32_t z;
    } val_3axis;
#if SENSOR_ENABLE_FUSION
    struct {
        float w;
        float x;
        float y;
        float z;
    } val_quaternion;
#endif
    uint8_t val_bytes[16];
} sensor_value_t;

/* ==================== 传感器数据 ==================== */
typedef struct {
    sensor_type_t type;
    sensor_unit_t unit;
    sensor_value_t value;
    uint32_t timestamp;
    uint8_t accuracy;
    uint8_t reserved[3];
} sensor_data_t;

/* ==================== 错误码 ==================== */
typedef enum {
    SENSOR_EOK       = 0,
    SENSOR_ERROR     = -1,
    SENSOR_EINVAL    = -2,
    SENSOR_ENODEV    = -3,
    SENSOR_EBUSY     = -4,
    SENSOR_ETIMEOUT  = -5,
    SENSOR_ENOMEM    = -6,
    SENSOR_ENOSYS    = -7,
    SENSOR_EIO       = -8,
    SENSOR_ECALIB    = -9,
    SENSOR_EOVERFLOW = -10,
} sensor_err_t;

/* ==================== 传感器状态 ==================== */
typedef enum {
    SENSOR_STATUS_IDLE,
    SENSOR_STATUS_READY,
    SENSOR_STATUS_BUSY,
    SENSOR_STATUS_ERROR,
#if SENSOR_ENABLE_CALIBRATION
    SENSOR_STATUS_CALIBRATING,
#endif
#if SENSOR_ENABLE_SELF_TEST
    SENSOR_STATUS_SELF_TEST,
#endif
} sensor_status_t;

/* ==================== 触发模式 ==================== */
typedef enum {
    SENSOR_TRIGGER_MODE_POLLING,
#if SENSOR_ENABLE_INTERRUPT
    SENSOR_TRIGGER_MODE_INTERRUPT,
#endif
#if SENSOR_ENABLE_FIFO
    SENSOR_TRIGGER_MODE_FIFO,
#endif
#if SENSOR_ENABLE_DMA
    SENSOR_TRIGGER_MODE_DMA,
#endif
} sensor_trigger_mode_t;

/* ==================== FIFO配置 ==================== */
#if SENSOR_ENABLE_FIFO
typedef enum {
    SENSOR_FIFO_MODE_BYPASS,
    SENSOR_FIFO_MODE_FIFO,
    SENSOR_FIFO_MODE_STREAM,
    SENSOR_FIFO_MODE_TRIGGER,
} sensor_fifo_mode_t;

typedef struct {
    bool enable;
    uint16_t watermark;
    uint16_t size;
    uint8_t mode;
} sensor_fifo_config_t;
#endif

/* ==================== 中断配置 ==================== */
#if SENSOR_ENABLE_INTERRUPT
typedef enum {
    SENSOR_INT_NONE       = 0x00,
    SENSOR_INT_DATA_READY = 0x01,
#if SENSOR_ENABLE_FIFO
    SENSOR_INT_FIFO_WATERMARK = 0x02,
    SENSOR_INT_FIFO_FULL      = 0x04,
#endif
#if SENSOR_ENABLE_MOTION_DETECT
    SENSOR_INT_MOTION_DETECT = 0x08,
#endif
#if SENSOR_ENABLE_THRESHOLD
    SENSOR_INT_THRESHOLD_LOW  = 0x10,
    SENSOR_INT_THRESHOLD_HIGH = 0x20,
#endif
    SENSOR_INT_FREE_FALL = 0x40,
    SENSOR_INT_TAP       = 0x80,
} sensor_interrupt_type_t;

struct sensor_device;
typedef void (*sensor_interrupt_handler_t)(struct sensor_device *sensor,
                                           uint32_t int_type);

typedef struct {
    uint32_t type;
    bool active_high;
    bool latch;
    sensor_interrupt_handler_t handler;
} sensor_interrupt_config_t;
#endif

/* ==================== 校准配置 ==================== */
#if SENSOR_ENABLE_CALIBRATION
typedef enum {
    SENSOR_CALIB_OFFSET,
    SENSOR_CALIB_SCALE,
    SENSOR_CALIB_CROSS_AXIS,
    SENSOR_CALIB_TEMPERATURE,
} sensor_calibration_type_t;

typedef struct {
    sensor_calibration_type_t type;
    sensor_value_t offset;
    sensor_value_t scale;
#if SENSOR_USE_FLOAT
    float matrix[9];
#endif
    bool valid;
} sensor_calibration_data_t;
#endif

/* ==================== 滤波器配置 ==================== */
#if SENSOR_ENABLE_FILTER
typedef enum {
    SENSOR_FILTER_NONE,
    SENSOR_FILTER_LOW_PASS,
    SENSOR_FILTER_HIGH_PASS,
    SENSOR_FILTER_BAND_PASS,
    SENSOR_FILTER_MOVING_AVERAGE,
    SENSOR_FILTER_MEDIAN,
#if SENSOR_USE_FLOAT
    SENSOR_FILTER_KALMAN,
#endif
} sensor_filter_type_t;

typedef struct {
    sensor_filter_type_t type;
#if SENSOR_USE_FLOAT
    float cutoff_freq;
#else
    uint32_t cutoff_freq;
#endif
    uint8_t order;
    uint8_t window_size;
    bool enable;
} sensor_filter_config_t;
#endif

/* ==================== DMA配置 ==================== */
#if SENSOR_ENABLE_DMA
typedef struct {
    bool enable;
    void *dma_channel;
    uint8_t *buffer;
    uint32_t buffer_size;
    void (*callback)(void *buf, uint32_t len);
} sensor_dma_config_t;
#endif

/* ==================== 功耗模式 ==================== */
#if SENSOR_ENABLE_POWER_MGMT
typedef enum {
    SENSOR_POWER_MODE_SHUTDOWN,
    SENSOR_POWER_MODE_SLEEP,
    SENSOR_POWER_MODE_STANDBY,
    SENSOR_POWER_MODE_LOW_POWER,
    SENSOR_POWER_MODE_NORMAL,
    SENSOR_POWER_MODE_HIGH_PERFORMANCE,
} sensor_power_mode_t;

typedef struct {
    uint32_t active_time;
    uint32_t sleep_time;
#if SENSOR_USE_FLOAT
    float average_current;
    float energy_consumed;
#else
    uint32_t average_current_ua;
    uint32_t energy_consumed_uwh;
#endif
} sensor_power_stats_t;
#endif

/* ==================== 阈值配置 ==================== */
#if SENSOR_ENABLE_THRESHOLD
typedef struct {
    sensor_value_t low_threshold;
    sensor_value_t high_threshold;
    uint32_t duration;
    bool enable;
} sensor_threshold_config_t;
#endif

/* ==================== 运动检测配置 ==================== */
#if SENSOR_ENABLE_MOTION_DETECT
typedef struct {
    uint32_t threshold;
    uint32_t duration;
    bool enable;
} sensor_motion_config_t;

typedef struct {
    uint32_t threshold;
    uint32_t duration;
    bool enable;
} sensor_freefall_config_t;

typedef struct {
    uint32_t threshold;
    uint32_t duration;
    bool double_tap;
    bool enable;
} sensor_tap_config_t;
#endif

/* ==================== 自测试结果 ==================== */
#if SENSOR_ENABLE_SELF_TEST
typedef struct {
    bool passed;
    uint32_t error_code;
    char message[64];
} sensor_self_test_result_t;
#endif

/* ==================== 传感器融合配置 ==================== */
#if SENSOR_ENABLE_FUSION
typedef enum {
    SENSOR_FUSION_NONE,
    SENSOR_FUSION_6DOF,
    SENSOR_FUSION_9DOF,
} sensor_fusion_type_t;

struct sensor_device;
typedef struct {
    sensor_fusion_type_t type;
#if SENSOR_USE_FLOAT
    float beta;
#else
    uint32_t beta;
#endif
    bool enable;
    struct sensor_device *sensors[3];
    uint8_t sensor_count;
} sensor_fusion_config_t;
#endif

/* ==================== 配置参数类型 ==================== */
typedef enum {
    SENSOR_CFG_ODR,
    SENSOR_CFG_RANGE,
    SENSOR_CFG_RESOLUTION,
#if SENSOR_ENABLE_POWER_MGMT
    SENSOR_CFG_POWER_MODE,
#endif
    SENSOR_CFG_TRIGGER_MODE,
#if SENSOR_ENABLE_THRESHOLD
    SENSOR_CFG_THRESHOLD,
#endif
#if SENSOR_ENABLE_CALIBRATION
    SENSOR_CFG_CALIBRATION,
#endif
#if SENSOR_ENABLE_FILTER
    SENSOR_CFG_FILTER,
#endif
#if SENSOR_ENABLE_FIFO
    SENSOR_CFG_FIFO,
#endif
#if SENSOR_ENABLE_INTERRUPT
    SENSOR_CFG_INTERRUPT,
#endif
#if SENSOR_ENABLE_DMA
    SENSOR_CFG_DMA,
#endif
#if SENSOR_ENABLE_MOTION_DETECT
    SENSOR_CFG_MOTION_DETECT,
    SENSOR_CFG_FREEFALL_DETECT,
    SENSOR_CFG_TAP_DETECT,
#endif
#if SENSOR_ENABLE_FUSION
    SENSOR_CFG_FUSION,
#endif
} sensor_config_type_t;

#endif /* __SENSOR_TYPE_H__ */