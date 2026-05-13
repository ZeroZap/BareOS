#include "sensor_core.h"

#if SENSOR_ENABLE_FILTER

/* 移动平均滤波器状态 */
typedef struct {
    sensor_data_t *buffer;
    uint8_t size;
    uint8_t index;
    bool filled;
} moving_average_state_t;

#if SENSOR_USE_FLOAT
/* 低通滤波器状态 */
typedef struct {
    sensor_data_t prev_output;
    float alpha;
} low_pass_state_t;
#endif

/**
 * @brief 初始化滤波器
 */
sensor_err_t sensor_filter_init(sensor_device_t *sensor,
                                sensor_filter_config_t *config)
{
    if (sensor == NULL || config == NULL) {
        return SENSOR_EINVAL;
    }

    memcpy(&sensor->filter_cfg, config, sizeof(sensor_filter_config_t));

    switch (config->type) {
    case SENSOR_FILTER_MOVING_AVERAGE:
    case SENSOR_FILTER_MEDIAN: {
        moving_average_state_t *state = (moving_average_state_t *)SENSOR_MALLOC(
            sizeof(moving_average_state_t));
        if (state == NULL) {
            return SENSOR_ENOMEM;
        }

        state->size   = config->window_size;
        state->index  = 0;
        state->filled = false;
        state->buffer =
            (sensor_data_t *)SENSOR_CALLOC(state->size, sizeof(sensor_data_t));
        if (state->buffer == NULL) {
            SENSOR_FREE(state);
            return SENSOR_ENOMEM;
        }

        sensor->filter_state = state;
        break;
    }

#if SENSOR_USE_FLOAT
    case SENSOR_FILTER_LOW_PASS: {
        low_pass_state_t *state =
            (low_pass_state_t *)SENSOR_MALLOC(sizeof(low_pass_state_t));
        if (state == NULL) {
            return SENSOR_ENOMEM;
        }

        memset(&state->prev_output, 0, sizeof(sensor_data_t));

        /* 计算滤波系数 */
        float dt     = 1.0f / sensor->odr;
        float rc     = 1.0f / (2.0f * 3.14159265f * config->cutoff_freq);
        state->alpha = dt / (rc + dt);

        sensor->filter_state = state;
        break;
    }
#endif

    default:
        return SENSOR_ENOSYS;
    }

    SENSOR_LOG("Filter initialized: type=%d", config->type);

    return SENSOR_EOK;
}

/**
 * @brief 反初始化滤波器
 */
sensor_err_t sensor_filter_deinit(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->filter_state == NULL) {
        return SENSOR_EINVAL;
    }

    if (sensor->filter_cfg.type == SENSOR_FILTER_MOVING_AVERAGE
        || sensor->filter_cfg.type == SENSOR_FILTER_MEDIAN) {
        moving_average_state_t *state =
            (moving_average_state_t *)sensor->filter_state;
        if (state->buffer != NULL) {
            SENSOR_FREE(state->buffer);
        }
    }

    SENSOR_FREE(sensor->filter_state);
    sensor->filter_state = NULL;

    SENSOR_LOG("Filter deinitialized");

    return SENSOR_EOK;
}

/**
 * @brief 移动平均滤波
 */
static sensor_err_t sensor_filter_moving_average(sensor_device_t *sensor,
                                                 sensor_data_t *data)
{
    moving_average_state_t *state =
        (moving_average_state_t *)sensor->filter_state;

    /* 保存当前数据 */
    memcpy(&state->buffer[state->index], data, sizeof(sensor_data_t));
    state->index = (state->index + 1) % state->size;

    if (!state->filled && state->index == 0) {
        state->filled = true;
    }

    /* 计算平均值 */
    int64_t sum_x = 0, sum_y = 0, sum_z = 0;
    uint8_t count = state->filled ? state->size : state->index;

    if (count == 0) {
        return SENSOR_EOK;
    }

    for (uint8_t i = 0; i < count; i++) {
        sum_x += state->buffer[i].value.val_3axis.x;
        sum_y += state->buffer[i].value.val_3axis.y;
        sum_z += state->buffer[i].value.val_3axis.z;
    }

    data->value.val_3axis.x = (int32_t)(sum_x / count);
    data->value.val_3axis.y = (int32_t)(sum_y / count);
    data->value.val_3axis.z = (int32_t)(sum_z / count);

    return SENSOR_EOK;
}

#if SENSOR_USE_FLOAT
/**
 * @brief 低通滤波
 */
static sensor_err_t sensor_filter_low_pass(sensor_device_t *sensor,
                                           sensor_data_t *data)
{
    low_pass_state_t *state = (low_pass_state_t *)sensor->filter_state;

    /* y[n] = alpha * x[n] + (1 - alpha) * y[n-1] */
    data->value.val_3axis.x =
        (int32_t)(state->alpha * data->value.val_3axis.x
                  + (1.0f - state->alpha)
                        * state->prev_output.value.val_3axis.x);
    data->value.val_3axis.y =
        (int32_t)(state->alpha * data->value.val_3axis.y
                  + (1.0f - state->alpha)
                        * state->prev_output.value.val_3axis.y);
    data->value.val_3axis.z =
        (int32_t)(state->alpha * data->value.val_3axis.z
                  + (1.0f - state->alpha)
                        * state->prev_output.value.val_3axis.z);

    memcpy(&state->prev_output, data, sizeof(sensor_data_t));

    return SENSOR_EOK;
}
#endif

/**
 * @brief 中值滤波
 */
static int compare_int32(const void *a, const void *b)
{
    int32_t arg1 = *(const int32_t *)a;
    int32_t arg2 = *(const int32_t *)b;

    if (arg1 < arg2)
        return -1;
    if (arg1 > arg2)
        return 1;
    return 0;
}

static sensor_err_t sensor_filter_median(sensor_device_t *sensor,
                                         sensor_data_t *data)
{
    moving_average_state_t *state =
        (moving_average_state_t *)sensor->filter_state;

    /* 保存当前数据 */
    memcpy(&state->buffer[state->index], data, sizeof(sensor_data_t));
    state->index = (state->index + 1) % state->size;

    if (!state->filled && state->index == 0) {
        state->filled = true;
    }

    uint8_t count = state->filled ? state->size : state->index;
    if (count == 0) {
        return SENSOR_EOK;
    }

    /* 提取并排序数据 */
    int32_t *temp_x = (int32_t *)SENSOR_MALLOC(sizeof(int32_t) * count);
    int32_t *temp_y = (int32_t *)SENSOR_MALLOC(sizeof(int32_t) * count);
    int32_t *temp_z = (int32_t *)SENSOR_MALLOC(sizeof(int32_t) * count);

    if (temp_x == NULL || temp_y == NULL || temp_z == NULL) {
        if (temp_x)
            SENSOR_FREE(temp_x);
        if (temp_y)
            SENSOR_FREE(temp_y);
        if (temp_z)
            SENSOR_FREE(temp_z);
        return SENSOR_ENOMEM;
    }

    for (uint8_t i = 0; i < count; i++) {
        temp_x[i] = state->buffer[i].value.val_3axis.x;
        temp_y[i] = state->buffer[i].value.val_3axis.y;
        temp_z[i] = state->buffer[i].value.val_3axis.z;
    }

    /* 简单排序 - 冒泡排序 (适用于小数据集) */
    for (uint8_t i = 0; i < count - 1; i++) {
        for (uint8_t j = 0; j < count - i - 1; j++) {
            if (temp_x[j] > temp_x[j + 1]) {
                int32_t tmp   = temp_x[j];
                temp_x[j]     = temp_x[j + 1];
                temp_x[j + 1] = tmp;
            }
            if (temp_y[j] > temp_y[j + 1]) {
                int32_t tmp   = temp_y[j];
                temp_y[j]     = temp_y[j + 1];
                temp_y[j + 1] = tmp;
            }
            if (temp_z[j] > temp_z[j + 1]) {
                int32_t tmp   = temp_z[j];
                temp_z[j]     = temp_z[j + 1];
                temp_z[j + 1] = tmp;
            }
        }
    }

    /* 取中值 */
    data->value.val_3axis.x = temp_x[count / 2];
    data->value.val_3axis.y = temp_y[count / 2];
    data->value.val_3axis.z = temp_z[count / 2];

    SENSOR_FREE(temp_x);
    SENSOR_FREE(temp_y);
    SENSOR_FREE(temp_z);

    return SENSOR_EOK;
}

/**
 * @brief 处理滤波
 */
sensor_err_t sensor_filter_process(sensor_device_t *sensor, sensor_data_t *data)
{
    if (sensor == NULL || data == NULL) {
        return SENSOR_EINVAL;
    }

    if (!sensor->filter_cfg.enable || sensor->filter_state == NULL) {
        return SENSOR_EOK;
    }

    switch (sensor->filter_cfg.type) {
    case SENSOR_FILTER_MOVING_AVERAGE:
        return sensor_filter_moving_average(sensor, data);

#if SENSOR_USE_FLOAT
    case SENSOR_FILTER_LOW_PASS:
        return sensor_filter_low_pass(sensor, data);
#endif

    case SENSOR_FILTER_MEDIAN:
        return sensor_filter_median(sensor, data);

    default:
        return SENSOR_ENOSYS;
    }
}

/**
 * @brief 使能/失能滤波器
 */
sensor_err_t sensor_filter_enable(sensor_device_t *sensor, bool enable)
{
    if (sensor == NULL) {
        return SENSOR_EINVAL;
    }

    sensor->filter_cfg.enable = enable;
    return SENSOR_EOK;
}

/**
 * @brief 重置滤波器状态
 */
sensor_err_t sensor_filter_reset(sensor_device_t *sensor)
{
    if (sensor == NULL || sensor->filter_state == NULL) {
        return SENSOR_EINVAL;
    }

    if (sensor->filter_cfg.type == SENSOR_FILTER_MOVING_AVERAGE
        || sensor->filter_cfg.type == SENSOR_FILTER_MEDIAN) {
        moving_average_state_t *state =
            (moving_average_state_t *)sensor->filter_state;
        state->index  = 0;
        state->filled = false;
        memset(state->buffer, 0, sizeof(sensor_data_t) * state->size);
    }
#if SENSOR_USE_FLOAT
    else if (sensor->filter_cfg.type == SENSOR_FILTER_LOW_PASS) {
        low_pass_state_t *state = (low_pass_state_t *)sensor->filter_state;
        memset(&state->prev_output, 0, sizeof(sensor_data_t));
    }
#endif

    SENSOR_LOG("Filter reset");

    return SENSOR_EOK;
}

#endif /* SENSOR_ENABLE_FILTER */