/**
 * @file xy_dmp.c
 * @brief Digital Motion Processor - MPU6050 姿态解算实现
 * @version 1.0.0
 * @date 2026-03-01 早晨
 */

#include "xy_dmp.h"
#include "xy_log.h"
#include <math.h>
#include <stdbool.h>
#include <string.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

#define DLPF_44HZ_DELAY_MS  23  /* 44Hz DLPF 对应的延迟 */

/**
 * @brief 四元数转欧拉角
 */
// static void // xy_dmp_quaternion_to_euler(const xy_quaternion_t *q, xy_euler_t *euler)
{
    /* Roll (x-axis rotation) */
    float sinr_cosp = 2.0F * (q->w * q->x + q->y * q->z);
    float cosr_cosp = 1.0F - 2.0F * (q->x * q->x + q->y * q->y);
    euler->roll = atan2f(sinr_cosp, cosr_cosp);
    
    /* Pitch (y-axis rotation) */
    float sinp = 2.0F * (q->w * q->y - q->z * q->x);
    if (fabsf(sinp) >= 1.0F) {
        euler->pitch = copysignf(M_PI_2, sinp);  /* 使用 90 度 */
    } else {
        euler->pitch = asinf(sinp);
    }
    
    /* Yaw (z-axis rotation) */
    float siny_cosp = 2.0F * (q->w * q->z + q->x * q->y);
    float cosy_cosp = 1.0F - 2.0F * (q->y * q->y + q->z * q->z);
    euler->yaw = atan2f(siny_cosp, cosy_cosp);
}

/**
 * @brief 四元数转重力向量
 */
static void xy_dmp_quaternion_to_gravity(const xy_quaternion_t *q, float *gravity)
{
    gravity[0] = 2.0F * (q->x * q->z - q->w * q->y);
    gravity[1] = 2.0F * (q->w * q->x + q->y * q->z);
    gravity[2] = q->w * q->w - q->x * q->x - q->y * q->y + q->z * q->z;
}

/**
 * @brief 互补滤波姿态解算
 */
static void xy_dmp_complementary_filter(xy_dmp_t *dmp, 
                                        float ax, float ay, float az,
                                        float gx, float gy, float gz)
{
    float dt = DLPF_44HZ_DELAY_MS / 1000.0F;
    
    /* 从加速度计计算角度 */
    float accel_roll = atan2f(ay, az);
    float accel_pitch = atan2f(-ax, sqrtf(ay * ay + az * az));
    
    /* 从陀螺仪积分角度 */
    dmp->euler.roll = dmp->euler.roll + gx * dt;
    dmp->euler.pitch = dmp->euler.pitch + gy * dt;
    dmp->euler.yaw = dmp->euler.yaw + gz * dt;
    
    /* 互补滤波 (加速度计 0.98, 陀螺仪 0.02) */
    dmp->euler.roll = 0.98F * (dmp->euler.roll + gx * dt) + 0.02F * accel_roll;
    dmp->euler.pitch = 0.98F * (dmp->euler.pitch + gy * dt) + 0.02F * accel_pitch;
    
    /* 归一化欧拉角 */
    if (dmp->euler.roll > M_PI) dmp->euler.roll -= 2.0F * M_PI;
    if (dmp->euler.roll < -M_PI) dmp->euler.roll += 2.0F * M_PI;
    if (dmp->euler.pitch > M_PI_2) dmp->euler.pitch = M_PI_2;
    if (dmp->euler.pitch < -M_PI_2) dmp->euler.pitch = -M_PI_2;
    if (dmp->euler.yaw > M_PI) dmp->euler.yaw -= 2.0F * M_PI;
    if (dmp->euler.yaw < -M_PI) dmp->euler.yaw += 2.0F * M_PI;
    
    /* 从欧拉角计算四元数 */
    float cy = cosf(dmp->euler.yaw * 0.5F);
    float sy = sinf(dmp->euler.yaw * 0.5F);
    float cp = cosf(dmp->euler.pitch * 0.5F);
    float sp = sinf(dmp->euler.pitch * 0.5F);
    float cr = cosf(dmp->euler.roll * 0.5F);
    float sr = sinf(dmp->euler.roll * 0.5F);
    
    dmp->q.w = cr * cp * cy + sr * sp * sy;
    dmp->q.x = sr * cp * cy - cr * sp * sy;
    dmp->q.y = cr * sp * cy + sr * cp * sy;
    dmp->q.z = cr * cp * sy - sr * sp * cy;
    
    /* 计算重力向量 */
    xy_dmp_quaternion_to_gravity(&dmp->q, dmp->gravity);
}

int xy_dmp_init(xy_dmp_t *dmp, xy_mpu6050_t *mpu)
{
    if (!dmp || !mpu) {
        return XY_DMP_INVALID_PARAM;
    }
    
    memset(dmp, 0, sizeof(*dmp));
    dmp->mpu = mpu;
    
    /* 初始四元数为单位四元数 */
    dmp->q.w = 1.0F;
    dmp->q.x = 0.0F;
    dmp->q.y = 0.0F;
    dmp->q.z = 0.0F;
    
    dmp->initialized = 1;
    xy_log_i("DMP initialized\n");
    return XY_DMP_OK;
}

int xy_dmp_update(xy_dmp_t *dmp)
{
    float ax, ay, az;
    float gx, gy, gz;
    int ret;
    
    if (!dmp || !dmp->initialized) {
        return XY_DMP_NOT_INIT;
    }
    
    /* 读取 MPU6050 数据 */
    ret = xy_mpu6050_read_accel(dmp->mpu, &ax, &ay, &az);
    if (ret != XY_MPU6050_OK) {
        return XY_DMP_ERROR;
    }
    
    ret = xy_mpu6050_read_gyro(dmp->mpu, &gx, &gy, &gz);
    if (ret != XY_MPU6050_OK) {
        return XY_DMP_ERROR;
    }
    
    /* 陀螺仪转换为弧度/秒 */
    gx = gx * M_PI / 180.0F;
    gy = gy * M_PI / 180.0F;
    gz = gz * M_PI / 180.0F;
    
    /* 互补滤波解算姿态 */
    xy_dmp_complementary_filter(dmp, ax, ay, az, gx, gy, gz);
    
    dmp->last_update = xy_os_tick_get();
    
    return XY_DMP_OK;
}

int xy_dmp_get_quaternion(xy_dmp_t *dmp, xy_quaternion_t *q)
{
    if (!dmp || !q) {
        return XY_DMP_INVALID_PARAM;
    }
    
    *q = dmp->q;
    return XY_DMP_OK;
}

int xy_dmp_get_euler(xy_dmp_t *dmp, xy_euler_t *euler)
{
    if (!dmp || !euler) {
        return XY_DMP_INVALID_PARAM;
    }
    
    *euler = dmp->euler;
    return XY_DMP_OK;
}

int xy_dmp_get_angles(xy_dmp_t *dmp, float *roll, float *pitch, float *yaw)
{
    if (!dmp) {
        return XY_DMP_INVALID_PARAM;
    }
    
    if (roll) {
        *roll = dmp->euler.roll * 180.0F / M_PI;
    }
    if (pitch) {
        *pitch = dmp->euler.pitch * 180.0F / M_PI;
    }
    if (yaw) {
        *yaw = dmp->euler.yaw * 180.0F / M_PI;
    }
    
    return XY_DMP_OK;
}

int xy_dmp_get_gravity(xy_dmp_t *dmp, float *gx, float *gy, float *gz)
{
    if (!dmp) {
        return XY_DMP_INVALID_PARAM;
    }
    
    if (gx) {
        *gx = dmp->gravity[0];
    }
    if (gy) {
        *gy = dmp->gravity[1];
    }
    if (gz) {
        *gz = dmp->gravity[2];
    }
    
    return XY_DMP_OK;
}

int xy_dmp_calibrate(xy_dmp_t *dmp, uint16_t samples)
{
    int ret;
    float roll_sum = 0, pitch_sum = 0;
    uint16_t i;
    
    if (!dmp || samples == 0) {
        return XY_DMP_INVALID_PARAM;
    }
    
    xy_log_i("DMP calibrating (%d samples)...\n", samples);
    
    /* 采集样本 (假设水平放置) */
    for (i = 0; i < samples; i++) {
        ret = xy_dmp_update(dmp);
        if (ret != XY_DMP_OK) {
            return ret;
        }
        
        roll_sum += dmp->euler.roll;
        pitch_sum += dmp->euler.pitch;
        
        xy_hal_delay_ms(DLPF_44HZ_DELAY_MS);
    }
    
    /* 计算零偏 (水平放置时 roll 和 pitch 应为 0) */
    float roll_offset = roll_sum / samples;
    float pitch_offset = pitch_sum / samples;
    
    xy_log_i("DMP calibration complete\n");
    xy_log_i("Roll offset: %.4f rad (%.2f deg)\n", 
             roll_offset, roll_offset * 180.0F / M_PI);
    xy_log_i("Pitch offset: %g rad (%g deg)\n",
             pitch_offset, pitch_offset * 180.0F / M_PI);
    
    return XY_DMP_OK;
}
