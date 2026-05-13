#include "sensor_core.h"

/* 设备链表头 */
static sensor_device_t *sensor_list_head = NULL;
static sensor_mutex_t sensor_list_mutex;
static bool mutex_initialized = false;

/* 初始化互斥锁 */
static void sensor_mutex_init(void)
{
    if (!mutex_initialized) {
        SENSOR_MUTEX_CREATE(sensor_list_mutex);
        mutex_initialized = true;
    }
}

/**
 * @brief 注册传感器设备
 */
sensor_err_t sensor_register(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->ops == NULL) {
        return SENSOR_EINVAL;
    }

    if (sensor->ops->init == NULL || sensor->ops->read == NULL) {
        return SENSOR_EINVAL;
    }

    sensor_mutex_init();
    SENSOR_MUTEX_LOCK(sensor_list_mutex);

    /* 检查是否已存在 */
    sensor_device_t *dev = sensor_list_head;
    while (dev != NULL) {
        if (strcmp(dev->info.name, sensor->info.name) == 0) {
            SENSOR_MUTEX_UNLOCK(sensor_list_mutex);
            return SENSOR_ERROR;
        }
        dev = dev->next;
    }

    /* 添加到链表头 */
    sensor->next     = sensor_list_head;
    sensor_list_head = sensor;
    sensor->status   = SENSOR_STATUS_IDLE;

    SENSOR_MUTEX_UNLOCK(sensor_list_mutex);
    SENSOR_LOG("Sensor registered: %s", sensor->info.name);

    return SENSOR_EOK;
}

/**
 * @brief 注销传感器设备
 */
sensor_err_t sensor_unregister(sensor_device_t *sensor)
{
    if (sensor == NULL) {
        return SENSOR_EINVAL;
    }

    SENSOR_MUTEX_LOCK(sensor_list_mutex);

    sensor_device_t *prev = NULL;
    sensor_device_t *curr = sensor_list_head;

    while (curr != NULL) {
        if (curr == sensor) {
            if (prev == NULL) {
                sensor_list_head = curr->next;
            } else {
                prev->next = curr->next;
            }
            sensor->status = SENSOR_STATUS_IDLE;
            SENSOR_MUTEX_UNLOCK(sensor_list_mutex);
            SENSOR_LOG("Sensor unregistered: %s", sensor->info.name);
            return SENSOR_EOK;
        }
        prev = curr;
        curr = curr->next;
    }

    SENSOR_MUTEX_UNLOCK(sensor_list_mutex);
    return SENSOR_ENODEV;
}

/**
 * @brief 根据名称查找传感器
 */
sensor_device_t *sensor_find_by_name(const char *name)
{
    if (name == NULL) {
        return NULL;
    }

    SENSOR_MUTEX_LOCK(sensor_list_mutex);

    sensor_device_t *dev = sensor_list_head;
    while (dev != NULL) {
        if (strcmp(dev->info.name, name) == 0) {
            SENSOR_MUTEX_UNLOCK(sensor_list_mutex);
            return dev;
        }
        dev = dev->next;
    }

    SENSOR_MUTEX_UNLOCK(sensor_list_mutex);
    return NULL;
}

/**
 * @brief 根据类型查找传感器
 */
sensor_device_t *sensor_find_by_type(sensor_type_t type)
{
    SENSOR_MUTEX_LOCK(sensor_list_mutex);

    sensor_device_t *dev = sensor_list_head;
    while (dev != NULL) {
        if (dev->info.type == type) {
            SENSOR_MUTEX_UNLOCK(sensor_list_mutex);
            return dev;
        }
        dev = dev->next;
    }

    SENSOR_MUTEX_UNLOCK(sensor_list_mutex);
    return NULL;
}

/**
 * @brief 初始化传感器
 */
sensor_err_t sensor_init(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->ops == NULL || sensor->ops->init == NULL) {
        return SENSOR_EINVAL;
    }

    if (sensor->status != SENSOR_STATUS_IDLE) {
        return SENSOR_EBUSY;
    }

    sensor_err_t ret = sensor->ops->init(sensor);
    if (ret == SENSOR_EOK) {
        sensor->status = SENSOR_STATUS_READY;
        SENSOR_LOG("Sensor initialized: %s", sensor->info.name);
    }

    return ret;
}

/**
 * @brief 反初始化传感器
 */
sensor_err_t sensor_deinit(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->ops == NULL || sensor->ops->deinit == NULL) {
        return SENSOR_EINVAL;
    }

    sensor_err_t ret = sensor->ops->deinit(sensor);
    if (ret == SENSOR_EOK) {
        sensor->status = SENSOR_STATUS_IDLE;
        SENSOR_LOG("Sensor deinitialized: %s", sensor->info.name);
    }

    return ret;
}

/**
 * @brief 使能/失能传感器
 */
sensor_err_t sensor_enable(sensor_device_t *sensor, bool enable)
{
    if (sensor == NULL) {
        return SENSOR_EINVAL;
    }

    if (sensor->ops->enable != NULL) {
        return sensor->ops->enable(sensor, enable);
    }

    return SENSOR_ENOSYS;
}

/**
 * @brief 读取传感器数据
 */
sensor_err_t sensor_read(sensor_device_t *sensor, sensor_data_t *data)
{
    if (sensor == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }

    if (sensor->status != SENSOR_STATUS_READY) {
        return SENSOR_ERROR;
    }

    if (sensor->ops->read == NULL) {
        return SENSOR_ENOSYS;
    }

    sensor->status   = SENSOR_STATUS_BUSY;
    sensor_err_t ret = sensor->ops->read(sensor, data);
    sensor->status   = SENSOR_STATUS_READY;

    if (ret == SENSOR_EOK) {
#if SENSOR_ENABLE_CALIBRATION
        /* 应用校准 */
        if (sensor->calibration.valid) {
            sensor_apply_calibration(sensor, data);
        }
#endif

#if SENSOR_ENABLE_FILTER
        /* 应用滤波 */
        if (sensor->filter_cfg.enable) {
            sensor_filter_process(sensor, data);
        }
#endif

#if SENSOR_ENABLE_THRESHOLD
        /* 检查阈值 */
        if (sensor->threshold_cfg.enable) {
            sensor_threshold_check(sensor, data);
        }
#endif

        /* 调用回调函数 */
        if (sensor->callback != NULL) {
            sensor->callback(sensor, data, sensor->user_data);
        }
    }

    return ret;
}

/**
 * @brief 写入传感器数据
 */
sensor_err_t sensor_write(sensor_device_t *sensor, const sensor_data_t *data)
{
    if (sensor == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }

    if (sensor->ops->write == NULL) {
        return SENSOR_ENOSYS;
    }

    return sensor->ops->write(sensor, data);
}

/**
 * @brief 配置传感器参数
 */
sensor_err_t sensor_config(sensor_device_t *sensor, sensor_config_type_t cfg,
                           void *value)
{
    if (sensor == NULL || value == NULL) {
        return SENSOR_EINVAL;
    }

    if (sensor->ops->config == NULL) {
        return SENSOR_ENOSYS;
    }

    return sensor->ops->config(sensor, cfg, value);
}

/**
 * @brief 控制传感器
 */
sensor_err_t sensor_control(sensor_device_t *sensor, int cmd, void *args)
{
    if (sensor == NULL) {
        return SENSOR_EINVAL;
    }

    if (sensor->ops->control == NULL) {
        return SENSOR_ENOSYS;
    }

    return sensor->ops->control(sensor, cmd, args);
}

/**
 * @brief 设置数据回调函数
 */
sensor_err_t sensor_set_callback(sensor_device_t *sensor,
                                 sensor_callback_t callback, void *user_data)
{
    if (sensor == NULL) {
        return SENSOR_EINVAL;
    }

    sensor->callback  = callback;
    sensor->user_data = user_data;

    return SENSOR_EOK;
}

/**
 * @brief 获取传感器信息
 */
const sensor_info_t *sensor_get_info(sensor_device_t *sensor)
{
    if (sensor == NULL) {
        return NULL;
    }

    return &sensor->info;
}

/**
 * @brief 获取传感器状态
 */
sensor_status_t sensor_get_status(sensor_device_t *sensor)
{
    if (sensor == NULL) {
        return SENSOR_STATUS_ERROR;
    }

    return sensor->status;
}