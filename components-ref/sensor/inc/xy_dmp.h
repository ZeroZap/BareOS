/**
 * @file xy_dmp.h
 * @brief Digital Motion Processor - MPU6050 姿态解算
 * @version 1.0.0
 * @date 2026-03-01 早晨
 */

#ifndef XY_DMP_H
#define XY_DMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "xy_mpu6050.h"
#include <stdint.h>

/**
 * @brief 错误码
 */
#define XY_DMP_OK               0
#define XY_DMP_ERROR            (-1)
#define XY_DMP_INVALID_PARAM    (-2)
#define XY_DMP_NOT_INIT         (-3)

/**
 * @brief 四元数结构
 */
typedef struct {
    float w;
    float x;
    float y;
    float z;
} xy_quaternion_t;

/**
 * @brief 欧拉角结构 (弧度)
 */
typedef struct {
    float roll;     /* 横滚角 */
    float pitch;    /* 俯仰角 */
    float yaw;      /* 偏航角 */
} xy_euler_t;

/**
 * @brief DMP 设备结构
 */
typedef struct {
    xy_mpu6050_t *mpu;          /**< MPU6050 设备 */
    xy_quaternion_t q;          /**< 四元数 */
    xy_euler_t euler;           /**< 欧拉角 */
    float gravity[3];           /**< 重力向量 */
    uint32_t last_update;       /**< 上次更新时间 */
    uint8_t initialized;        /**< 初始化标志 */
} xy_dmp_t;

/**
 * @brief 初始化 DMP
 * @param dmp DMP 设备句柄
 * @param mpu MPU6050 设备句柄
 * @return XY_DMP_OK 成功，其他值失败
 */
int xy_dmp_init(xy_dmp_t *dmp, xy_mpu6050_t *mpu);

/**
 * @brief 更新姿态解算
 * @param dmp DMP 设备句柄
 * @return XY_DMP_OK 成功，其他值失败
 */
int xy_dmp_update(xy_dmp_t *dmp);

/**
 * @brief 获取四元数
 * @param dmp DMP 设备句柄
 * @param q 四元数指针
 * @return XY_DMP_OK 成功，其他值失败
 */
int xy_dmp_get_quaternion(xy_dmp_t *dmp, xy_quaternion_t *q);

/**
 * @brief 获取欧拉角 (弧度)
 * @param dmp DMP 设备句柄
 * @param euler 欧拉角指针
 * @return XY_DMP_OK 成功，其他值失败
 */
int xy_dmp_get_euler(xy_dmp_t *dmp, xy_euler_t *euler);

/**
 * @brief 获取欧拉角 (角度)
 * @param dmp DMP 设备句柄
 * @param roll 横滚角指针 (角度)
 * @param pitch 俯仰角指针 (角度)
 * @param yaw 偏航角指针 (角度)
 * @return XY_DMP_OK 成功，其他值失败
 */
int xy_dmp_get_angles(xy_dmp_t *dmp, float *roll, float *pitch, float *yaw);

/**
 * @brief 获取重力向量
 * @param dmp DMP 设备句柄
 * @param gx X 轴重力分量
 * @param gy Y 轴重力分量
 * @param gz Z 轴重力分量
 * @return XY_DMP_OK 成功，其他值失败
 */
int xy_dmp_get_gravity(xy_dmp_t *dmp, float *gx, float *gy, float *gz);

/**
 * @brief 校准零偏
 * @param dmp DMP 设备句柄
 * @param samples 采样次数
 * @return XY_DMP_OK 成功，其他值失败
 */
int xy_dmp_calibrate(xy_dmp_t *dmp, uint16_t samples);

#ifdef __cplusplus
}
#endif

#endif /* XY_DMP_H */
