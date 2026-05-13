#include "sensor_core.h"

#if SENSOR_ENABLE_INTERRUPT

/**
 * @brief 初始化中断配置
 */
sensor_err_t sensor_interrupt_init(sensor_device_t *sensor,
                                   sensor_interrupt_config_t *config)
{
    if (sensor == NULL || config == NULL) {
        return SENSOR_EINVAL;
    }

    memcpy(&sensor->interrupt_cfg, config, sizeof(sensor_interrupt_config_t));

    SENSOR_LOG("Interrupt configured: type=0x%02X", config->type);

    return SENSOR_EOK;
}

/**
 * @brief 使能/失能中断
 */
sensor_err_t sensor_interrupt_enable(sensor_device_t *sensor, uint32_t int_type,
                                     bool enable)
{
    if (sensor == NULL) {
        return SENSOR_EINVAL;
    }

    if (sensor->ops->interrupt_enable != NULL) {
        return sensor->ops->interrupt_enable(sensor, int_type, enable);
    }

    if (enable) {
        sensor->interrupt_cfg.type |= int_type;
    } else {
        sensor->interrupt_cfg.type &= ~int_type;
    }

    return SENSOR_EOK;
}

/**
 * @brief 清除中断标志
 */
sensor_err_t sensor_interrupt_clear(sensor_device_t *sensor, uint32_t int_type)
{
    if (sensor == NULL) {
        return SENSOR_EINVAL;
    }

    if (sensor->ops->interrupt_clear != NULL) {
        return sensor->ops->interrupt_clear(sensor, int_type);
    }

    return SENSOR_EOK;
}

/**
 * @brief 中断处理函数 (在ISR中调用)
 */
void sensor_interrupt_handler(sensor_device_t *sensor, uint32_t int_type)
{
    if (sensor == NULL) {
        return;
    }

    /* 调用用户注册的中断处理函数 */
    if (sensor->interrupt_cfg.handler != NULL) {
        sensor->interrupt_cfg.handler(sensor, int_type);
    }

    /* 处理数据就绪中断 */
    if (int_type & SENSOR_INT_DATA_READY) {
        sensor_data_t data;
        if (sensor_read(sensor, &data) == SENSOR_EOK) {
            if (sensor->callback != NULL) {
                sensor->callback(sensor, &data, sensor->user_data);
            }
        }
    }

#if SENSOR_ENABLE_FIFO
    /* 处理FIFO水位中断 */
    if (int_type & SENSOR_INT_FIFO_WATERMARK) {
        if (sensor->fifo != NULL) {
            SENSOR_LOG("FIFO watermark reached: count=%u", sensor->fifo->count);
        }
    }

    /* 处理FIFO满中断 */
    if (int_type & SENSOR_INT_FIFO_FULL) {
        SENSOR_LOG("FIFO full");
    }
#endif

#if SENSOR_ENABLE_MOTION_DETECT
    /* 处理运动检测中断 */
    if (int_type & SENSOR_INT_MOTION_DETECT) {
        SENSOR_LOG("Motion detected");
    }
#endif

#if SENSOR_ENABLE_THRESHOLD
    /* 处理阈值中断 */
    if (int_type & SENSOR_INT_THRESHOLD_LOW) {
        SENSOR_LOG("Low threshold exceeded");
    }

    if (int_type & SENSOR_INT_THRESHOLD_HIGH) {
        SENSOR_LOG("High threshold exceeded");
    }
#endif
}

#endif /* SENSOR_ENABLE_INTERRUPT */