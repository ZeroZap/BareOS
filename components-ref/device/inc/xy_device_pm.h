/**
 * @file xy_device_pm.h
 * @brief Device Power Management
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 设备电源管理模块 - 支持低功耗模式
 */

#ifndef XY_DEVICE_PM_H
#define XY_DEVICE_PM_H

#include "xy_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Power State Definitions ==================== */

/**
 * @brief 设备电源状态
 */
typedef enum {
    XY_DEVICE_PM_STATE_ACTIVE = 0,     /**< 全功率运行 */
    XY_DEVICE_PM_STATE_SLEEP,          /**< 睡眠模式 (快速唤醒) */
    XY_DEVICE_PM_STATE_DEEP_SLEEP,     /**< 深度睡眠 (低功耗) */
    XY_DEVICE_PM_STATE_OFF,            /**< 关闭状态 */
} xy_device_pm_state_t;

/**
 * @brief 电源管理策略
 */
typedef enum {
    XY_DEVICE_PM_POLICY_ALWAYS_ON = 0, /**< 始终开启 */
    XY_DEVICE_PM_POLICY_AUTO,          /**< 自动管理 */
    XY_DEVICE_PM_POLICY_MANUAL,        /**< 手动控制 */
} xy_device_pm_policy_t;

/* ==================== Power Management Operations ==================== */

/**
 * @brief 设备电源管理操作集
 */
typedef struct {
    int (*set_state)(xy_device_t *dev, xy_device_pm_state_t state);
    int (*get_state)(xy_device_t *dev, xy_device_pm_state_t *state);
    int (*set_wakeup)(xy_device_t *dev, bool enable);
    int (*get_power_consumption)(xy_device_t *dev, uint32_t *uw);
} xy_device_pm_ops_t;

/* ==================== Power Management API ==================== */

/**
 * @brief 初始化设备电源管理
 * @param dev 设备句柄
 * @param pm_ops 电源管理操作集
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_device_pm_init(xy_device_t *dev, const xy_device_pm_ops_t *pm_ops);

/**
 * @brief 设置设备电源状态
 * @param dev 设备句柄
 * @param state 目标电源状态
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_device_pm_set_state(xy_device_t *dev, xy_device_pm_state_t state);

/**
 * @brief 获取设备电源状态
 * @param dev 设备句柄
 * @param state 电源状态 (输出)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_device_pm_get_state(xy_device_t *dev, xy_device_pm_state_t *state);

/**
 * @brief 启用/禁用唤醒功能
 * @param dev 设备句柄
 * @param enable true=启用，false=禁用
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_device_pm_set_wakeup(xy_device_t *dev, bool enable);

/**
 * @brief 获取设备功耗 (微瓦)
 * @param dev 设备句柄
 * @param uw 功耗值 (输出，单位微瓦)
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_device_pm_get_consumption(xy_device_t *dev, uint32_t *uw);

/**
 * @brief 设置电源管理策略
 * @param dev 设备句柄
 * @param policy 电源管理策略
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_device_pm_set_policy(xy_device_t *dev, xy_device_pm_policy_t policy);

/**
 * @brief 获取电源管理策略
 * @param dev 设备句柄
 * @return 当前策略
 */
xy_device_pm_policy_t xy_device_pm_get_policy(xy_device_t *dev);

/**
 * @brief 设备进入睡眠模式
 * @param dev 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_device_pm_sleep(xy_device_t *dev);

/**
 * @brief 设备唤醒
 * @param dev 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_device_pm_wakeup(xy_device_t *dev);

/**
 * @brief 设备关闭
 * @param dev 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_device_pm_off(xy_device_t *dev);

/**
 * @brief 设备开启
 * @param dev 设备句柄
 * @return XY_DEVICE_OK 成功，其他值失败
 */
int xy_device_pm_on(xy_device_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* XY_DEVICE_PM_H */
