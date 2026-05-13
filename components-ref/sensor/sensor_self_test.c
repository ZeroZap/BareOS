#include "sensor_core.h"

#if SENSOR_ENABLE_SELF_TEST

/**
 * @brief 传感器自测试
 */
sensor_err_t sensor_self_test(sensor_device_t *sensor,
                              sensor_self_test_result_t *result)
{
    if (sensor == NULL || result == NULL) {
        return SENSOR_EINVAL;
    }

    memset(result, 0, sizeof(sensor_self_test_result_t));
    sensor->status = SENSOR_STATUS_SELF_TEST;

    SENSOR_LOG("Starting self test for %s", sensor->info.name);

    /* 如果驱动提供了自测试接口 */
    if (sensor->ops->self_test != NULL) {
        sensor_err_t ret = sensor->ops->self_test(sensor, result);
        sensor->status   = SENSOR_STATUS_READY;
        return ret;
    }

    /* 通用自测试流程 */

    /* 1. 检查设备通信 */
    sensor_data_t data;
    if (sensor->ops->read(sensor, &data) != SENSOR_EOK) {
        result->passed     = false;
        result->error_code = 1;
        strncpy(result->message, "Communication failed",
                sizeof(result->message) - 1);
        sensor->status = SENSOR_STATUS_READY;
        SENSOR_LOG("Self test failed: communication error");
        return SENSOR_ERROR;
    }

    /* 2. 检查数据范围 */
    if (sensor->info.type == SENSOR_TYPE_ACCELEROMETER) {
        /* 检查加速度计数据是否合理 (应该检测到重力) */
        int32_t magnitude = abs(data.value.val_3axis.x)
                            + abs(data.value.val_3axis.y)
                            + abs(data.value.val_3axis.z);

        if (magnitude < 800 || magnitude > 1200) { /* 期望约 1g */
            result->passed     = false;
            result->error_code = 2;
            snprintf(result->message, sizeof(result->message),
                     "Data out of range: %d mg", magnitude);
            sensor->status = SENSOR_STATUS_READY;
            SENSOR_LOG("Self test failed: data out of range");
            return SENSOR_ERROR;
        }
    }

    /* 3. 噪声测试 - 采集多个样本检查稳定性 */
    const uint32_t sample_count = 10;
    int32_t min_x = 0x7FFFFFFF, max_x = -0x7FFFFFFF;

    for (uint32_t i = 0; i < sample_count; i++) {
        if (sensor->ops->read(sensor, &data) == SENSOR_EOK) {
            if (data.value.val_3axis.x < min_x)
                min_x = data.value.val_3axis.x;
            if (data.value.val_3axis.x > max_x)
                max_x = data.value.val_3axis.x;
        }

        SENSOR_DELAY_MS(10);
    }

    int32_t noise = max_x - min_x;
    if (noise > 100) { /* 噪声阈值 */
        result->passed     = false;
        result->error_code = 3;
        snprintf(result->message, sizeof(result->message),
                 "High noise level: %d", noise);
        sensor->status = SENSOR_STATUS_READY;
        SENSOR_LOG("Self test failed: high noise");
        return SENSOR_ERROR;
    }

    /* 测试通过 */
    result->passed     = true;
    result->error_code = 0;
    strncpy(result->message, "Self test passed", sizeof(result->message) - 1);

    sensor->status = SENSOR_STATUS_READY;

    SENSOR_LOG("Self test passed");

    return SENSOR_EOK;
}

#endif /* SENSOR_ENABLE_SELF_TEST */