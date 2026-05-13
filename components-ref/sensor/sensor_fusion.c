#include "sensor_core.h"

#if SENSOR_ENABLE_FUSION

#if SENSOR_USE_FLOAT
#include <math.h>

/* Madgwick滤波器状态 */
typedef struct {
    float q0, q1, q2, q3; /* 四元数 */
    float beta;           /* 收敛速率 */
    float sample_freq;    /* 采样频率 */
} madgwick_state_t;

static madgwick_state_t fusion_state = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.1f, SENSOR_FUSION_SAMPLE_FREQ
};

/**
 * @brief 快速平方根倒数
 */
static float inv_sqrt(float x)
{
    float halfx = 0.5f * x;
    float y     = x;
    long i      = *(long *)&y;
    i           = 0x5f3759df - (i >> 1);
    y           = *(float *)&i;
    y           = y * (1.5f - (halfx * y * y));
    return y;
}

/**
 * @brief 初始化传感器融合
 */
sensor_err_t sensor_fusion_init(sensor_fusion_config_t *config)
{
    if (config == NULL) {
        return SENSOR_EINVAL;
    }

    fusion_state.q0          = 1.0f;
    fusion_state.q1          = 0.0f;
    fusion_state.q2          = 0.0f;
    fusion_state.q3          = 0.0f;
    fusion_state.beta        = config->beta;
    fusion_state.sample_freq = SENSOR_FUSION_SAMPLE_FREQ;

    SENSOR_LOG("Sensor fusion initialized: type=%d, beta=%.3f", config->type,
               config->beta);

    return SENSOR_EOK;
}

/**
 * @brief 反初始化传感器融合
 */
sensor_err_t sensor_fusion_deinit(sensor_fusion_config_t *config)
{
    if (config == NULL) {
        return SENSOR_EINVAL;
    }

    config->enable = false;

    SENSOR_LOG("Sensor fusion deinitialized");

    return SENSOR_EOK;
}

/**
 * @brief Madgwick 6DOF 姿态更新
 */
static void madgwick_update_6dof(float gx, float gy, float gz, float ax,
                                 float ay, float az, float dt)
{
    float recipNorm;
    float s0, s1, s2, s3;
    float qDot1, qDot2, qDot3, qDot4;
    float _2q0, _2q1, _2q2, _2q3, _4q0, _4q1, _4q2, _8q1, _8q2;
    float q0q0, q1q1, q2q2, q3q3;

    /* 角速度转换为弧度 */
    gx *= 0.017453293f; /* deg to rad */
    gy *= 0.017453293f;
    gz *= 0.017453293f;

    /* 四元数微分 */
    qDot1 =
        0.5f
        * (-fusion_state.q1 * gx - fusion_state.q2 * gy - fusion_state.q3 * gz);
    qDot2 =
        0.5f
        * (fusion_state.q0 * gx + fusion_state.q2 * gz - fusion_state.q3 * gy);
    qDot3 =
        0.5f
        * (fusion_state.q0 * gy - fusion_state.q1 * gz + fusion_state.q3 * gx);
    qDot4 =
        0.5f
        * (fusion_state.q0 * gz + fusion_state.q1 * gy - fusion_state.q2 * gx);

    /* 加速度计数据归一化 */
    recipNorm = inv_sqrt(ax * ax + ay * ay + az * az);
    ax *= recipNorm;
    ay *= recipNorm;
    az *= recipNorm;

    /* 辅助变量 */
    _2q0 = 2.0f * fusion_state.q0;
    _2q1 = 2.0f * fusion_state.q1;
    _2q2 = 2.0f * fusion_state.q2;
    _2q3 = 2.0f * fusion_state.q3;
    _4q0 = 4.0f * fusion_state.q0;
    _4q1 = 4.0f * fusion_state.q1;
    _4q2 = 4.0f * fusion_state.q2;
    _8q1 = 8.0f * fusion_state.q1;
    _8q2 = 8.0f * fusion_state.q2;
    q0q0 = fusion_state.q0 * fusion_state.q0;
    q1q1 = fusion_state.q1 * fusion_state.q1;
    q2q2 = fusion_state.q2 * fusion_state.q2;
    q3q3 = fusion_state.q3 * fusion_state.q3;

    /* 梯度下降算法 */
    s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
    s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * fusion_state.q1 - _2q0 * ay
         - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
    s2 = 4.0f * q0q0 * fusion_state.q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay
         - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
    s3 = 4.0f * q1q1 * fusion_state.q3 - _2q1 * ax
         + 4.0f * q2q2 * fusion_state.q3 - _2q2 * ay;

    recipNorm = inv_sqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
    s0 *= recipNorm;
    s1 *= recipNorm;
    s2 *= recipNorm;
    s3 *= recipNorm;

    /* 应用反馈 */
    qDot1 -= fusion_state.beta * s0;
    qDot2 -= fusion_state.beta * s1;
    qDot3 -= fusion_state.beta * s2;
    qDot4 -= fusion_state.beta * s3;

    /* 积分四元数 */
    fusion_state.q0 += qDot1 * dt;
    fusion_state.q1 += qDot2 * dt;
    fusion_state.q2 += qDot3 * dt;
    fusion_state.q3 += qDot4 * dt;

    /* 归一化四元数 */
    recipNorm = inv_sqrt(fusion_state.q0 * fusion_state.q0
                         + fusion_state.q1 * fusion_state.q1
                         + fusion_state.q2 * fusion_state.q2
                         + fusion_state.q3 * fusion_state.q3);
    fusion_state.q0 *= recipNorm;
    fusion_state.q1 *= recipNorm;
    fusion_state.q2 *= recipNorm;
    fusion_state.q3 *= recipNorm;
}

/**
 * @brief 四元数转欧拉角
 */
static void quaternion_to_euler(float q0, float q1, float q2, float q3,
                                float *roll, float *pitch, float *yaw)
{
    *roll =
        atan2f(2.0f * (q0 * q1 + q2 * q3), 1.0f - 2.0f * (q1 * q1 + q2 * q2));
    *pitch = asinf(2.0f * (q0 * q2 - q3 * q1));
    *yaw =
        atan2f(2.0f * (q0 * q3 + q1 * q2), 1.0f - 2.0f * (q2 * q2 + q3 * q3));

    /* 转换为角度 */
    *roll *= 57.29578f; /* rad to deg */
    *pitch *= 57.29578f;
    *yaw *= 57.29578f;
}

/**
 * @brief 更新传感器融合
 */
sensor_err_t sensor_fusion_update(sensor_fusion_config_t *config,
                                  sensor_data_t *output)
{
    if (config == NULL || output == NULL || !config->enable) {
        return SENSOR_EINVAL;
    }

    if (config->type == SENSOR_FUSION_6DOF) {
        /* 6轴融合 */
        if (config->sensor_count < 2) {
            return SENSOR_EINVAL;
        }

        sensor_device_t *accel = config->sensors[0];
        sensor_device_t *gyro  = config->sensors[1];

        sensor_data_t accel_data, gyro_data;

        /* 读取传感器数据 */
        if (sensor_read(accel, &accel_data) != SENSOR_EOK
            || sensor_read(gyro, &gyro_data) != SENSOR_EOK) {
            return SENSOR_EIO;
        }

        /* 转换为浮点数 */
        float ax = accel_data.value.val_3axis.x / 1000.0f; /* mg to g */
        float ay = accel_data.value.val_3axis.y / 1000.0f;
        float az = accel_data.value.val_3axis.z / 1000.0f;

        float gx = gyro_data.value.val_3axis.x / 1.0f; /* °/s */
        float gy = gyro_data.value.val_3axis.y / 1.0f;
        float gz = gyro_data.value.val_3axis.z / 1.0f;

        /* 更新姿态 */
        float dt = 1.0f / fusion_state.sample_freq;
        madgwick_update_6dof(gx, gy, gz, ax, ay, az, dt);

        /* 输出欧拉角 */
        output->type = SENSOR_TYPE_ORIENTATION;
        output->unit = SENSOR_UNIT_DEGREE_PER_SECOND;

        float roll, pitch, yaw;
        quaternion_to_euler(fusion_state.q0, fusion_state.q1, fusion_state.q2,
                            fusion_state.q3, &roll, &pitch, &yaw);

        output->value.val_3axis.x = (int32_t)(roll * 100); /* 0.01° */
        output->value.val_3axis.y = (int32_t)(pitch * 100);
        output->value.val_3axis.z = (int32_t)(yaw * 100);
        output->timestamp         = accel_data.timestamp;

    } else if (config->type == SENSOR_FUSION_9DOF) {
        /* 9轴融合 - 待实现 */
        return SENSOR_ENOSYS;
    } else {
        return SENSOR_EINVAL;
    }

    return SENSOR_EOK;
}

/**
 * @brief 获取四元数
 */
sensor_err_t sensor_fusion_get_quaternion(float *q0, float *q1, float *q2,
                                          float *q3)
{
    if (q0 == NULL || q1 == NULL || q2 == NULL || q3 == NULL) {
        return SENSOR_EINVAL;
    }

    *q0 = fusion_state.q0;
    *q1 = fusion_state.q1;
    *q2 = fusion_state.q2;
    *q3 = fusion_state.q3;

    return SENSOR_EOK;
}

/**
 * @brief 重置融合状态
 */
sensor_err_t sensor_fusion_reset(void)
{
    fusion_state.q0 = 1.0f;
    fusion_state.q1 = 0.0f;
    fusion_state.q2 = 0.0f;
    fusion_state.q3 = 0.0f;

    SENSOR_LOG("Fusion state reset");

    return SENSOR_EOK;
}

#else /* !SENSOR_USE_FLOAT */

/* 无浮点运算版本 - 简化实现 */

sensor_err_t sensor_fusion_init(sensor_fusion_config_t *config)
{
    SENSOR_LOG("Fusion requires floating point support");
    return SENSOR_ENOSYS;
}

sensor_err_t sensor_fusion_deinit(sensor_fusion_config_t *config)
{
    return SENSOR_ENOSYS;
}

sensor_err_t sensor_fusion_update(sensor_fusion_config_t *config,
                                  sensor_data_t *output)
{
    return SENSOR_ENOSYS;
}

sensor_err_t sensor_fusion_reset(void)
{
    return SENSOR_ENOSYS;
}

#endif /* SENSOR_USE_FLOAT */

#endif /* SENSOR_ENABLE_FUSION */