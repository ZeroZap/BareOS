#include "sensor_core.h"

#if SENSOR_ENABLE_DMA

/**
 * @brief 初始化DMA
 */
sensor_err_t sensor_dma_init(sensor_device_t *sensor,
                             sensor_dma_config_t *config)
{
    if (sensor == NULL || config == NULL) {
        return SENSOR_EINVAL;
    }

    if (!(sensor->info.flags & SENSOR_FLAG_DMA_SUPPORT)) {
        return SENSOR_ENOSYS;
    }

    memcpy(&sensor->dma_cfg, config, sizeof(sensor_dma_config_t));

    SENSOR_LOG("DMA initialized: buffer_size=%u", config->buffer_size);

    /* 这里需要根据实际HAL实现配置DMA */
    /* 示例:
    HAL_DMA_Init(sensor->dma_cfg.dma_channel, ...);
    */

    return SENSOR_EOK;
}

/**
 * @brief 反初始化DMA
 */
sensor_err_t sensor_dma_deinit(sensor_device_t *sensor)
{
    if (sensor == NULL) {
        return SENSOR_EINVAL;
    }

    sensor->dma_cfg.enable = false;

    /* HAL_DMA_DeInit(sensor->dma_cfg.dma_channel); */

    SENSOR_LOG("DMA deinitialized");

    return SENSOR_EOK;
}

/**
 * @brief 启动DMA传输
 */
sensor_err_t sensor_dma_start(sensor_device_t *sensor)
{
    if (sensor == NULL || !sensor->dma_cfg.enable) {
        return SENSOR_EINVAL;
    }

    if (sensor->dma_cfg.buffer == NULL || sensor->dma_cfg.buffer_size == 0) {
        return SENSOR_EINVAL;
    }

    SENSOR_LOG("DMA transfer started");

    /* 根据实际HAL实现
    HAL_DMA_Start_IT(sensor->dma_cfg.dma_channel,
                     (uint32_t)&SENSOR_DATA_REG,
                     (uint32_t)sensor->dma_cfg.buffer,
                     sensor->dma_cfg.buffer_size);
    */

    return SENSOR_EOK;
}

/**
 * @brief 停止DMA传输
 */
sensor_err_t sensor_dma_stop(sensor_device_t *sensor)
{
    if (sensor == NULL) {
        return SENSOR_EINVAL;
    }

    /* HAL_DMA_Abort(sensor->dma_cfg.dma_channel); */

    SENSOR_LOG("DMA transfer stopped");

    return SENSOR_EOK;
}

/**
 * @brief DMA传输完成回调 (在DMA中断中调用)
 */
void sensor_dma_complete_callback(sensor_device_t *sensor)
{
    if (sensor == NULL || !sensor->dma_cfg.enable) {
        return;
    }

    if (sensor->dma_cfg.callback != NULL) {
        sensor->dma_cfg.callback(
            sensor->dma_cfg.buffer, sensor->dma_cfg.buffer_size);
    }
}

#endif /* SENSOR_ENABLE_DMA */