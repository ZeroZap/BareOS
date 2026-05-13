#include "sensor_core.h"

#if SENSOR_ENABLE_POWER_MGMT

/**
 * @brief 设置功耗模式
 */
sensor_err_t sensor_set_power_mode(sensor_device_t *sensor,
                                   sensor_power_mode_t mode)
{
    if (sensor == NULL) {
        return SENSOR_EINVAL;
    }

    /* 记录当前活动时间 */
    uint32_t current_time = SENSOR_GET_TICK();
    if (sensor->power_mode == SENSOR_POWER_MODE_NORMAL
        || sensor->power_mode == SENSOR_POWER_MODE_HIGH_PERFORMANCE) {
        sensor->power_stats.active_time +=
            current_time - sensor->last_active_time;
    } else {
        sensor->power_stats.sleep_time +=
            current_time - sensor->last_active_time;
    }

    /* 调用驱动层接口 */
    if (sensor->ops->set_power_mode != NULL) {
        sensor_err_t ret = sensor->ops->set_power_mode(sensor, mode);
        if (ret != SENSOR_EOK) {
            return ret;
        }
    }

    sensor->power_mode       = mode;
    sensor->last_active_time = current_time;

    SENSOR_LOG("Power mode changed to %d", mode);

    return SENSOR_EOK;
}

/**
 * @brief 获取功耗模式
 */
sensor_power_mode_t sensor_get_power_mode(sensor_device_t *sensor)
{
    if (sensor == NULL) {
        return SENSOR_POWER_MODE_SHUTDOWN;
    }

    return sensor->power_mode;
}

/**
 * @brief 获取功耗统计
 */
sensor_err_t sensor_get_power_stats(sensor_device_t *sensor,
                                    sensor_power_stats_t *stats)
{
    if (sensor == NULL || stats == NULL) {
        return SENSOR_EINVAL;
    }

    /* 更新当前时间统计 */
    uint32_t current_time = SENSOR_GET_TICK();
    if (sensor->power_mode == SENSOR_POWER_MODE_NORMAL
        || sensor->power_mode == SENSOR_POWER_MODE_HIGH_PERFORMANCE) {
        sensor->power_stats.active_time +=
            current_time - sensor->last_active_time;
    } else {
        sensor->power_stats.sleep_time +=
            current_time - sensor->last_active_time;
    }
    sensor->last_active_time = current_time;

    /* 调用驱动层接口 */
    if (sensor->ops->get_power_stats != NULL) {
        return sensor->ops->get_power_stats(sensor, stats);
    }

    memcpy(stats, &sensor->power_stats, sizeof(sensor_power_stats_t));
    return SENSOR_EOK;
}

/**
 * @brief 重置功耗统计
 */
sensor_err_t sensor_reset_power_stats(sensor_device_t *sensor)
{
    if (sensor == NULL) {
        return SENSOR_EINVAL;
    }

    memset(&sensor->power_stats, 0, sizeof(sensor_power_stats_t));
    sensor->last_active_time = SENSOR_GET_TICK();

    SENSOR_LOG("Power statistics reset");

    return SENSOR_EOK;
}

#endif /* SENSOR_ENABLE_POWER_MGMT */