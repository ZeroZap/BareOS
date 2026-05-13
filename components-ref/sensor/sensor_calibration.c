#include "sensor_core.h"

#if SENSOR_ENABLE_CALIBRATION

/**
 * @brief 通用校准接口
 */
sensor_err_t sensor_calibrate(sensor_device_t *sensor,
                              sensor_calibration_type_t type)
{
    if (sensor == NULL) {
        return SENSOR_EINVAL;
    }

    sensor->status = SENSOR_STATUS_CALIBRATING;

    if (sensor->ops->calibrate != NULL) {
        sensor_err_t ret = sensor->ops->calibrate(sensor, type);
        sensor->status   = SENSOR_STATUS_READY;
        return ret;
    }

    /* 默认执行偏移校准 */
    sensor_err_t ret =
        sensor_calibrate_offset(sensor, SENSOR_CALIBRATION_SAMPLES);
    sensor->status = SENSOR_STATUS_READY;

    return ret;
}

/**
 * @brief 偏移校准
 */
sensor_err_t sensor_calibrate_offset(sensor_device_t *sensor,
                                     uint32_t sample_count)
{
    if (sensor == NULL || sample_count == 0) {
        return SENSOR_EINVAL;
    }

    SENSOR_LOG("Starting offset calibration with %u samples", sample_count);

    sensor->status = SENSOR_STATUS_CALIBRATING;

    if (sensor->ops->calibrate != NULL) {
        sensor_err_t ret = sensor->ops->calibrate(sensor, SENSOR_CALIB_OFFSET);
        sensor->status   = SENSOR_STATUS_READY;
        return ret;
    }

    /* 软件校准 */
    sensor_data_t data;
    int64_t sum_x = 0, sum_y = 0, sum_z = 0;
    uint32_t valid_samples = 0;

    for (uint32_t i = 0; i < sample_count; i++) {
        if (sensor->ops->read(sensor, &data) == SENSOR_EOK) {
            sum_x += data.value.val_3axis.x;
            sum_y += data.value.val_3axis.y;
            sum_z += data.value.val_3axis.z;
            valid_samples++;
        }

        SENSOR_DELAY_MS(10);
    }

    if (valid_samples < sample_count / 2) {
        sensor->status = SENSOR_STATUS_READY;
        SENSOR_LOG("Calibration failed: insufficient samples");
        return SENSOR_ECALIB;
    }

    /* 计算平均偏移 */
    sensor->calibration.type               = SENSOR_CALIB_OFFSET;
    sensor->calibration.offset.val_3axis.x = sum_x / valid_samples;
    sensor->calibration.offset.val_3axis.y = sum_y / valid_samples;
    sensor->calibration.offset.val_3axis.z = sum_z / valid_samples;

    /* 对于加速度计，Z轴应该考虑重力 */
    if (sensor->info.type == SENSOR_TYPE_ACCELEROMETER) {
        sensor->calibration.offset.val_3axis.z -= 1000; /* 1g in mg */
    }

    sensor->calibration.valid = true;
    sensor->status            = SENSOR_STATUS_READY;

    SENSOR_LOG("Calibration successful: offset=(%d, %d, %d)",
               sensor->calibration.offset.val_3axis.x,
               sensor->calibration.offset.val_3axis.y,
               sensor->calibration.offset.val_3axis.z);

    return SENSOR_EOK;
}

/**
 * @brief 获取校准数据
 */
sensor_err_t sensor_get_calibration(sensor_device_t *sensor,
                                    sensor_calibration_data_t *calib)
{
    if (sensor == NULL || calib == NULL) {
        return SENSOR_EINVAL;
    }

    if (sensor->ops->get_calibration != NULL) {
        return sensor->ops->get_calibration(sensor, calib);
    }

    if (!sensor->calibration.valid) {
        return SENSOR_ECALIB;
    }

    memcpy(calib, &sensor->calibration, sizeof(sensor_calibration_data_t));
    return SENSOR_EOK;
}

/**
 * @brief 设置校准数据
 */
sensor_err_t sensor_set_calibration(sensor_device_t *sensor,
                                    const sensor_calibration_data_t *calib)
{
    if (sensor == NULL || calib == NULL) {
        return SENSOR_EINVAL;
    }

    if (sensor->ops->set_calibration != NULL) {
        return sensor->ops->set_calibration(sensor, calib);
    }

    memcpy(&sensor->calibration, calib, sizeof(sensor_calibration_data_t));

    SENSOR_LOG("Calibration data loaded");

    return SENSOR_EOK;
}

/**
 * @brief 应用校准到数据
 */
sensor_err_t sensor_apply_calibration(sensor_device_t *sensor,
                                      sensor_data_t *data)
{
    if (sensor == NULL || data == NULL || !sensor->calibration.valid) {
        return SENSOR_EINVAL;
    }

    /* 应用偏移校准 */
    if (sensor->calibration.type == SENSOR_CALIB_OFFSET) {
        data->value.val_3axis.x -= sensor->calibration.offset.val_3axis.x;
        data->value.val_3axis.y -= sensor->calibration.offset.val_3axis.y;
        data->value.val_3axis.z -= sensor->calibration.offset.val_3axis.z;
    }

    /* 应用比例校准 */
    if (sensor->calibration.type == SENSOR_CALIB_SCALE) {
        data->value.val_3axis.x =
            (int32_t)((int64_t)data->value.val_3axis.x
                      * sensor->calibration.scale.val_3axis.x / 1000);
        data->value.val_3axis.y =
            (int32_t)((int64_t)data->value.val_3axis.y
                      * sensor->calibration.scale.val_3axis.y / 1000);
        data->value.val_3axis.z =
            (int32_t)((int64_t)data->value.val_3axis.z
                      * sensor->calibration.scale.val_3axis.z / 1000);
    }

    return SENSOR_EOK;
}

#if SENSOR_USE_FILE_SYSTEM
#include <stdio.h>

/**
 * @brief 保存校准数据到文件
 */
sensor_err_t sensor_save_calibration(sensor_device_t *sensor,
                                     const char *filename)
{
    if (sensor == NULL || filename == NULL) {
        return SENSOR_EINVAL;
    }

    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        return SENSOR_EIO;
    }

    size_t written =
        fwrite(&sensor->calibration, sizeof(sensor_calibration_data_t), 1, fp);
    fclose(fp);

    if (written != 1) {
        return SENSOR_EIO;
    }

    SENSOR_LOG("Calibration saved to %s", filename);

    return SENSOR_EOK;
}

/**/
    ***@brief ** / sensor_err_t sensor_load_calibration(sensor_device_t *sensor, const char *filename)
{
    if (sensor == NULL || filename == NULL) {
        return SENSOR_EINVAL;
    }

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        return SENSOR_EIO;
    }

    size_t read =
        fread(&sensor->calibration, sizeof(sensor_calibration_data_t), 1, fp);
    fclose(fp);

    if (read != 1) {
        return SENSOR_EIO;
    }

    SENSOR_LOG("Calibration loaded from %s", filename);

    return SENSOR_EOK;
}
#endif /* SENSOR_USE_FILE_SYSTEM */

#endif /* SENSOR_ENABLE_CALIBRATION */