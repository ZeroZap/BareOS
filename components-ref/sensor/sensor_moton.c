#include "sensor_core.h"

#if SENSOR_ENABLE_MOTION_DETECT

/* 运动检测状态 */
typedef struct {
    uint32_t motion_start_time;
    bool motion_detected;
    int32_t last_data[3];
    uint32_t freefall_start_time;
    bool freefall_detected;
    uint32_t tap_last_time;
    uint8_t tap_count;
} motion_detect_state_t;

static motion_detect_state_t motion_state = { 0 };

/**
 * @brief 计算绝对值
 */
static inline int32_t abs_int32(int32_t val)
{
    return (val < 0) ? -val : val;
}

#if SENSOR_USE_FLOAT
/**
 * @brief 快速平方根倒数
 */
static float inv_sqrt_fast(float x)
{
    float halfx = 0.5f * x;
    float y     = x;
    long i      = *(long *)&y;
    i           = 0x5f3759df - (i >> 1);
    y           = *(float *)&i;
    y           = y * (1.5f - (halfx * y * y));
    return y;
}
#endif

/**
 * @brief 初始化运动检测
 */
sensor_err_t sensor_motion_detect_init(sensor_device_t *sensor,
                                       sensor_motion_config_t *config)
{
    if (sensor == NULL || config == NULL) {
        return SENSOR_EINVAL;
    }

    if (sensor->info.type != SENSOR_TYPE_ACCELEROMETER) {
        return SENSOR_EINVAL;
    }

    memcpy(&sensor->motion_cfg, config, sizeof(sensor_motion_config_t));
    memset(&motion_state, 0, sizeof(motion_detect_state_t));

    SENSOR_LOG("Motion detection initialized: threshold=%u, duration=%u",
               config->threshold, config->duration);

    return SENSOR_EOK;
}

/**
 * @brief 使能/失能运动检测
 */
sensor_err_t sensor_motion_detect_enable(sensor_device_t *sensor, bool enable)
{
    if (sensor == NULL) {
        return SENSOR_EINVAL;
    }

    sensor->motion_cfg.enable = enable;

    if (!enable) {
        memset(&motion_state, 0, sizeof(motion_detect_state_t));
    }

    return SENSOR_EOK;
}

/**
 * @brief 处理运动检测
 */
sensor_err_t sensor_motion_detect_process(sensor_device_t *sensor,
                                          sensor_data_t *data)
{
    if (sensor == NULL || data == NULL || !sensor->motion_cfg.enable) {
        return SENSOR_EINVAL;
    }

    /* 计算加速度变化量 */
    int32_t delta_x =
        abs_int32(data->value.val_3axis.x - motion_state.last_data[0]);
    int32_t delta_y =
        abs_int32(data->value.val_3axis.y - motion_state.last_data[1]);
    int32_t delta_z =
        abs_int32(data->value.val_3axis.z - motion_state.last_data[2]);

    /* 计算总变化量 */
    int32_t total_delta = delta_x + delta_y + delta_z;

    /* 检查是否超过阈值 */
    if (total_delta > (int32_t)sensor->motion_cfg.threshold) {
        if (!motion_state.motion_detected) {
            motion_state.motion_start_time = data->timestamp;
            motion_state.motion_detected   = true;
        }

        /* 检查持续时间 */
        uint32_t duration = data->timestamp - motion_state.motion_start_time;
        if (duration >= sensor->motion_cfg.duration) {
#if SENSOR_ENABLE_INTERRUPT
            /* 触发运动检测中断 */
            if (sensor->interrupt_cfg.handler != NULL) {
                sensor->interrupt_cfg.handler(sensor, SENSOR_INT_MOTION_DETECT);
            }
#endif
            SENSOR_LOG("Motion detected: delta=%d", total_delta);
        }
    } else {
        motion_state.motion_detected = false;
    }

    /* 保存当前数据 */
    motion_state.last_data[0] = data->value.val_3axis.x;
    motion_state.last_data[1] = data->value.val_3axis.y;
    motion_state.last_data[2] = data->value.val_3axis.z;

    return SENSOR_EOK;
}

/**
 * @brief 初始化自由落体检测
 */
sensor_err_t sensor_freefall_detect_init(sensor_device_t *sensor,
                                         sensor_freefall_config_t *config)
{
    if (sensor == NULL || config == NULL) {
        return SENSOR_EINVAL;
    }

    if (sensor->info.type != SENSOR_TYPE_ACCELEROMETER) {
        return SENSOR_EINVAL;
    }

    SENSOR_LOG(
        "Freefall detection initialized: threshold=%u", config->threshold);

    return SENSOR_EOK;
}

/**
 * @brief 自由落体检测处理
 */
sensor_err_t sensor_freefall_detect_process(sensor_device_t *sensor,
                                            sensor_data_t *data)
{
    if (sensor == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }

    /* 计算加速度模 */
    int32_t x = data->value.val_3axis.x;
    int32_t y = data->value.val_3axis.y;
    int32_t z = data->value.val_3axis.z;

#if SENSOR_USE_FLOAT
    float magnitude = sqrtf((float)(x * x + y * y + z * z));
#else
    /* 使用整数近似计算 */
    int64_t sum_sq     = (int64_t)x * x + (int64_t)y * y + (int64_t)z * z;
    uint32_t magnitude = 0;

    /* 简单的平方根近似 (牛顿迭代法) */
    if (sum_sq > 0) {
        uint32_t x0 = sum_sq / 2;
        uint32_t x1 = (x0 + sum_sq / x0) / 2;
        uint32_t x2 = (x1 + sum_sq / x1) / 2;
        magnitude   = x2;
    }
#endif

    /* 检查是否接近0 (自由落体) */
    /* 阈值通常设置为 0.5g (500mg) 以下 */
    if (magnitude < 500) {
        if (!motion_state.freefall_detected) {
            motion_state.freefall_start_time = data->timestamp;
            motion_state.freefall_detected   = true;
        } else {
            /* 检查持续时间 */
            uint32_t duration =
                data->timestamp - motion_state.freefall_start_time;
            if (duration >= 50) { /* 至少50ms */
#if SENSOR_ENABLE_INTERRUPT
                if (sensor->interrupt_cfg.handler != NULL) {
                    sensor->interrupt_cfg.handler(sensor, SENSOR_INT_FREE_FALL);
                }
#endif
                SENSOR_LOG(
                    "Freefall detected: magnitude=%u", (uint32_t)magnitude);
            }
        }
    } else {
        motion_state.freefall_detected = false;
    }

    return SENSOR_EOK;
}

/**
 * @brief 初始化敲击检测
 */
sensor_err_t sensor_tap_detect_init(sensor_device_t *sensor,
                                    sensor_tap_config_t *config)
{
    if (sensor == NULL || config == NULL) {
        return SENSOR_EINVAL;
    }

    if (sensor->info.type != SENSOR_TYPE_ACCELEROMETER) {
        return SENSOR_EINVAL;
    }

    SENSOR_LOG("Tap detection initialized: threshold=%u, double_tap=%d",
               config->threshold, config->double_tap);

    return SENSOR_EOK;
}

/**
 * @brief 敲击检测处理
 */
sensor_err_t sensor_tap_detect_process(sensor_device_t *sensor,
                                       sensor_data_t *data)
{
    if (sensor == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }

    static int32_t prev_magnitude = 0;

    /* 计算当前加速度幅值 */
    int32_t current_magnitude = abs_int32(data->value.val_3axis.x)
                                + abs_int32(data->value.val_3axis.y)
                                + abs_int32(data->value.val_3axis.z);

    /* 计算变化量 */
    int32_t delta = abs_int32(current_magnitude - prev_magnitude);

    /* 检测敲击 */
    if (delta > 2000) { /* 阈值需要根据实际调整 */
        uint32_t current_time = data->timestamp;
        uint32_t time_diff    = current_time - motion_state.tap_last_time;

        /* 双击检测 */
        if (time_diff < 500 && time_diff > 50) { /* 50-500ms之间 */
            motion_state.tap_count++;
            if (motion_state.tap_count >= 2) {
                SENSOR_LOG("Double tap detected");
                motion_state.tap_count = 0;
            }
        } else {
            motion_state.tap_count = 1;
            SENSOR_LOG("Single tap detected");
        }

        motion_state.tap_last_time = current_time;

#if SENSOR_ENABLE_INTERRUPT
        if (sensor->interrupt_cfg.handler != NULL) {
            sensor->interrupt_cfg.handler(sensor, SENSOR_INT_TAP);
        }
#endif
    }

    prev_magnitude = current_magnitude;

    return SENSOR_EOK;
}

#endif /* SENSOR_ENABLE_MOTION_DETECT */