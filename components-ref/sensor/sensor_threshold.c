#include "sensor_core.h"

#if SENSOR_ENABLE_THRESHOLD

/**
 * @brief 初始化阈值检测
 */
sensor_err_t sensor_threshold_init(sensor_device_t *sensor,
                                   sensor_threshold_config_t *config)
{
    if (sensor == NULL || config == NULL) {
        return SENSOR_EINVAL;
    }

    memcpy(&sensor->threshold_cfg, config, sizeof(sensor_threshold_config_t));

    SENSOR_LOG("Threshold configured");

    return SENSOR_EOK;
}

/**
 * @brief 使能/失能阈值检测
 */
sensor_err_t sensor_threshold_enable(sensor_device_t *sensor, bool enable)
{
    if (sensor == NULL) {
        return SENSOR_EINVAL;
    }

    sensor->threshold_cfg.enable = enable;

    return SENSOR_EOK;
}

/**
 * @brief 检查数据是否超过阈值
 */
bool sensor_threshold_check(sensor_device_t *sensor, sensor_data_t *data)
{
    if (sensor == NULL || data == NULL || !sensor->threshold_cfg.enable) {
        return false;
    }

    bool threshold_exceeded = false;
    uint32_t int_type       = 0;

    /* 根据传感器类型检查阈值 */
    if (sensor->info.type == SENSOR_TYPE_ACCELEROMETER
        || sensor->info.type == SENSOR_TYPE_GYROSCOPE
        || sensor->info.type == SENSOR_TYPE_MAGNETOMETER) {

        /* 3轴传感器 */
        if (data->value.val_3axis.x
                < sensor->threshold_cfg.low_threshold.val_3axis.x
            || data->value.val_3axis.x
                   > sensor->threshold_cfg.high_threshold.val_3axis.x
            || data->value.val_3axis.y
                   < sensor->threshold_cfg.low_threshold.val_3axis.y
            || data->value.val_3axis.y
                   > sensor->threshold_cfg.high_threshold.val_3axis.y
            || data->value.val_3axis.z
                   < sensor->threshold_cfg.low_threshold.val_3axis.z
            || data->value.val_3axis.z
                   > sensor->threshold_cfg.high_threshold.val_3axis.z) {
            threshold_exceeded = true;
        }

    } else {
        /* 单值传感器 */
        if (data->value.val_int32
            < sensor->threshold_cfg.low_threshold.val_int32) {
            threshold_exceeded = true;
            int_type           = SENSOR_INT_THRESHOLD_LOW;
        } else if (data->value.val_int32
                   > sensor->threshold_cfg.high_threshold.val_int32) {
            threshold_exceeded = true;
            int_type           = SENSOR_INT_THRESHOLD_HIGH;
        }
    }

#if SENSOR_ENABLE_INTERRUPT
    /* 触发中断 */
    if (threshold_exceeded && sensor->interrupt_cfg.handler != NULL) {
        if (int_type == 0) {
            int_type = SENSOR_INT_THRESHOLD_HIGH;
        }
        sensor->interrupt_cfg.handler(sensor, int_type);
    }
#endif

    return threshold_exceeded;
}

#endif /* SENSOR_ENABLE_THRESHOLD */