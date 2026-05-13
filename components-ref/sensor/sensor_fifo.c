#include "sensor_core.h"

#if SENSOR_ENABLE_FIFO

/**
 * @brief 初始化FIFO
 */
sensor_err_t sensor_fifo_init(sensor_device_t *sensor, uint32_t size)
{
    if (sensor == NULL || size == 0) {
        return SENSOR_EINVAL;
    }

    if (sensor->fifo != NULL) {
        return SENSOR_ERROR;
    }

    sensor->fifo = (sensor_fifo_t *)SENSOR_MALLOC(sizeof(sensor_fifo_t));
    if (sensor->fifo == NULL) {
        return SENSOR_ENOMEM;
    }

    sensor->fifo->buffer =
        (sensor_data_t *)SENSOR_MALLOC(sizeof(sensor_data_t) * size);
    if (sensor->fifo->buffer == NULL) {
        SENSOR_FREE(sensor->fifo);
        sensor->fifo = NULL;
        return SENSOR_ENOMEM;
    }

    sensor->fifo->size      = size;
    sensor->fifo->head      = 0;
    sensor->fifo->tail      = 0;
    sensor->fifo->count     = 0;
    sensor->fifo->watermark = size / 2;
    sensor->fifo->mode      = SENSOR_FIFO_MODE_FIFO;
    sensor->fifo->overflow  = false;

    SENSOR_LOG("FIFO initialized: size=%u", size);

    return SENSOR_EOK;
}

/**
 * @brief 反初始化FIFO
 */
sensor_err_t sensor_fifo_deinit(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->fifo == NULL) {
        return SENSOR_EINVAL;
    }

    if (sensor->fifo->buffer != NULL) {
        SENSOR_FREE(sensor->fifo->buffer);
    }

    SENSOR_FREE(sensor->fifo);
    sensor->fifo = NULL;

    SENSOR_LOG("FIFO deinitialized");

    return SENSOR_EOK;
}

/**
 * @brief 使能/失能FIFO
 */
sensor_err_t sensor_fifo_enable(sensor_device_t *sensor, bool enable)
{
    if (sensor == NULL || sensor->fifo == NULL) {
        return SENSOR_EINVAL;
    }

    if (sensor->ops->fifo_enable != NULL) {
        return sensor->ops->fifo_enable(sensor, enable);
    }

    return SENSOR_EOK;
}

/**
 * @brief 写入数据到FIFO (内部使用)
 */
static sensor_err_t sensor_fifo_write_internal(sensor_fifo_t *fifo,
                                               const sensor_data_t *data)
{
    if (fifo->count >= fifo->size) {
        fifo->overflow = true;

        if (fifo->mode == SENSOR_FIFO_MODE_FIFO) {
            return SENSOR_EOVERFLOW;
        } else if (fifo->mode == SENSOR_FIFO_MODE_STREAM) {
            fifo->tail = (fifo->tail + 1) % fifo->size;
            fifo->count--;
        }
    }

    memcpy(&fifo->buffer[fifo->head], data, sizeof(sensor_data_t));
    fifo->head = (fifo->head + 1) % fifo->size;
    fifo->count++;

    return SENSOR_EOK;
}

/**
 * @brief 从FIFO读取数据
 */
sensor_err_t sensor_fifo_read(sensor_device_t *sensor, sensor_data_t *data,
                              uint32_t count, uint32_t *read_count)
{
    if (sensor == NULL || sensor->fifo == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }

    sensor_fifo_t *fifo   = sensor->fifo;
    uint32_t actual_count = 0;

    if (sensor->ops->fifo_read != NULL) {
        return sensor->ops->fifo_read(sensor, data, count, read_count);
    }

    while (actual_count < count && fifo->count > 0) {
        memcpy(&data[actual_count], &fifo->buffer[fifo->tail],
               sizeof(sensor_data_t));
        fifo->tail = (fifo->tail + 1) % fifo->size;
        fifo->count--;
        actual_count++;
    }

    if (read_count != NULL) {
        *read_count = actual_count;
    }

    return SENSOR_EOK;
}

/**
 * @brief 清空FIFO
 */
sensor_err_t sensor_fifo_flush(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->fifo == NULL) {
        return SENSOR_EINVAL;
    }

    if (sensor->ops->fifo_flush != NULL) {
        return sensor->ops->fifo_flush(sensor);
    }

    sensor->fifo->head     = 0;
    sensor->fifo->tail     = 0;
    sensor->fifo->count    = 0;
    sensor->fifo->overflow = false;

    return SENSOR_EOK;
}

/**
 * @brief 获取FIFO中的数据数量
 */
sensor_err_t sensor_fifo_get_count(sensor_device_t *sensor, uint32_t *count)
{
    if (sensor == NULL || sensor->fifo == NULL || count == NULL) {
        return SENSOR_EINVAL;
    }

    if (sensor->ops->fifo_get_count != NULL) {
        return sensor->ops->fifo_get_count(sensor, count);
    }

    *count = sensor->fifo->count;
    return SENSOR_EOK;
}

/**
 * @brief 设置FIFO水位线
 */
sensor_err_t sensor_fifo_set_watermark(sensor_device_t *sensor,
                                       uint16_t watermark)
{
    if (sensor == NULL || sensor->fifo == NULL) {
        return SENSOR_EINVAL;
    }

    if (watermark > sensor->fifo->size) {
        return SENSOR_EINVAL;
    }

    sensor->fifo->watermark = watermark;
    return SENSOR_EOK;
}

/**
 * @brief 检查FIFO是否满
 */
bool sensor_fifo_is_full(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->fifo == NULL) {
        return false;
    }

    return (sensor->fifo->count >= sensor->fifo->size);
}

/**
 * @brief 检查FIFO是否空
 */
bool sensor_fifo_is_empty(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->fifo == NULL) {
        return true;
    }

    return (sensor->fifo->count == 0);
}

/**
 * @brief FIFO数据推送 (供驱动层调用)
 */
sensor_err_t sensor_fifo_push(sensor_device_t *sensor,
                              const sensor_data_t *data)
{
    if (sensor == NULL || sensor->fifo == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }

    sensor_err_t ret = sensor_fifo_write_internal(sensor->fifo, data);

#if SENSOR_ENABLE_INTERRUPT
    if (sensor->fifo->count >= sensor->fifo->watermark) {
        if (sensor->interrupt_cfg.handler != NULL) {
            sensor->interrupt_cfg.handler(sensor, SENSOR_INT_FIFO_WATERMARK);
        }
    }
#endif

    return ret;
}

#endif /* SENSOR_ENABLE_FIFO */