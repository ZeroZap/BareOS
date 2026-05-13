/**
 * @file xy_device_pm.c
 * @brief Device Power Management Implementation
 * @version 1.0.0
 * @date 2026-03-15
 * 
 * @note 设备电源管理实现 - 支持低功耗模式
 */

#include "inc/xy_device_pm.h"
#include <string.h>

/* ==================== Private Types ==================== */

/**
 * @brief 设备电源管理私有数据
 */
typedef struct {
    xy_device_pm_state_t current_state;    /**< 当前电源状态 */
    xy_device_pm_state_t last_state;       /**< 上一个状态 (用于唤醒) */
    xy_device_pm_policy_t policy;          /**< 电源管理策略 */
    const xy_device_pm_ops_t *ops;         /**< 电源管理操作集 */
    uint32_t idle_timeout_ms;              /**< 空闲超时 (毫秒) */
    uint32_t last_activity_time;           /**< 最后活动时间 */
    bool wakeup_enabled;                   /**< 唤醒功能使能 */
} xy_device_pm_data_t;

/* ==================== Private Variables ==================== */

/* 每个设备最多支持 16 个电源管理实例 */
static xy_device_pm_data_t pm_data[16];
static bool pm_initialized = false;

/* ==================== Private Functions ==================== */

/**
 * @brief 查找设备的电源管理数据
 */
static xy_device_pm_data_t *pm_find_data(xy_device_t *dev)
{
    if (!pm_initialized) {
        return NULL;
    }
    
    /* 简单实现：使用 user_data 存储指针 */
    return (xy_device_pm_data_t *)dev->user_data;
}

/**
 * @brief 获取当前时间戳 (毫秒)
 */
static uint32_t pm_get_tick_ms(void)
{
    return xy_hal_get_tick();  /* ✅ 系统 tick */
    /* 这里使用弱定义，允许用户重写 */
    return 0;
}

__attribute__((weak)) uint32_t pm_get_tick_ms(void)
{
    return 0;
}

/* ==================== Public Implementation ==================== */

int xy_device_pm_init(xy_device_t *dev, const xy_device_pm_ops_t *pm_ops)
{
    if (!dev || !pm_ops) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    /* 查找或分配电源管理数据 */
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data) {
        /* 简单实现：分配第一个可用槽位 */
        for (int i = 0; i < 16; i++) {
            if (pm_data[i].ops == NULL) {
                data = &pm_data[i];
                memset(data, 0, sizeof(*data));
                dev->user_data = data;
                break;
            }
        }
        
        if (!data) {
            return XY_DEVICE_NO_MEM;
        }
    }
    
    /* 初始化电源管理数据 */
    data->current_state = XY_DEVICE_PM_STATE_ACTIVE;
    data->last_state = XY_DEVICE_PM_STATE_ACTIVE;
    data->policy = XY_DEVICE_PM_POLICY_AUTO;
    data->ops = pm_ops;
    data->idle_timeout_ms = 0; /* 默认禁用空闲检测 */
    data->last_activity_time = pm_get_tick_ms();
    data->wakeup_enabled = false;
    
    pm_initialized = true;
    
    return XY_DEVICE_OK;
}

int xy_device_pm_set_state(xy_device_t *dev, xy_device_pm_state_t state)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data || !data->ops) {
        return XY_DEVICE_NOT_INIT;
    }
    
    /* 状态未改变 */
    if (data->current_state == state) {
        return XY_DEVICE_OK;
    }
    
    /* 调用底层操作 */
    if (data->ops->set_state) {
        int ret = data->ops->set_state(dev, state);
        if (ret != XY_DEVICE_OK) {
            return ret;
        }
    }
    
    /* 更新状态 */
    data->last_state = data->current_state;
    data->current_state = state;
    data->last_activity_time = pm_get_tick_ms();
    
    return XY_DEVICE_OK;
}

int xy_device_pm_get_state(xy_device_t *dev, xy_device_pm_state_t *state)
{
    if (!dev || !state) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data) {
        return XY_DEVICE_NOT_INIT;
    }
    
    *state = data->current_state;
    return XY_DEVICE_OK;
}

int xy_device_pm_set_wakeup(xy_device_t *dev, bool enable)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data || !data->ops) {
        return XY_DEVICE_NOT_INIT;
    }
    
    data->wakeup_enabled = enable;
    
    /* 调用底层操作 */
    if (data->ops->set_wakeup) {
        return data->ops->set_wakeup(dev, enable);
    }
    
    return XY_DEVICE_OK;
}

int xy_device_pm_get_consumption(xy_device_t *dev, uint32_t *uw)
{
    if (!dev || !uw) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data || !data->ops) {
        return XY_DEVICE_NOT_INIT;
    }
    
    /* 调用底层操作 */
    if (data->ops->get_power_consumption) {
        return data->ops->get_power_consumption(dev, uw);
    }
    
    /* 默认实现：根据状态估算 */
    switch (data->current_state) {
        case XY_DEVICE_PM_STATE_ACTIVE:
            *uw = 10000; /* 10mW */
            break;
        case XY_DEVICE_PM_STATE_SLEEP:
            *uw = 100; /* 100uW */
            break;
        case XY_DEVICE_PM_STATE_DEEP_SLEEP:
            *uw = 10; /* 10uW */
            break;
        case XY_DEVICE_PM_STATE_OFF:
            *uw = 1; /* 1uW (漏电) */
            break;
        default:
            *uw = 0;
    }
    
    return XY_DEVICE_OK;
}

int xy_device_pm_set_policy(xy_device_t *dev, xy_device_pm_policy_t policy)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data) {
        return XY_DEVICE_NOT_INIT;
    }
    
    data->policy = policy;
    return XY_DEVICE_OK;
}

xy_device_pm_policy_t xy_device_pm_get_policy(xy_device_t *dev)
{
    if (!dev) {
        return XY_DEVICE_PM_POLICY_ALWAYS_ON;
    }
    
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data) {
        return XY_DEVICE_PM_POLICY_ALWAYS_ON;
    }
    
    return data->policy;
}

int xy_device_pm_sleep(xy_device_t *dev)
{
    return xy_device_pm_set_state(dev, XY_DEVICE_PM_STATE_SLEEP);
}

int xy_device_pm_wakeup(xy_device_t *dev)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data) {
        return XY_DEVICE_NOT_INIT;
    }
    
    /* 恢复到上一个状态或 ACTIVE */
    xy_device_pm_state_t target_state = data->last_state;
    if (target_state == XY_DEVICE_PM_STATE_OFF || 
        target_state == XY_DEVICE_PM_STATE_DEEP_SLEEP) {
        target_state = XY_DEVICE_PM_STATE_ACTIVE;
    }
    
    return xy_device_pm_set_state(dev, target_state);
}

int xy_device_pm_off(xy_device_t *dev)
{
    return xy_device_pm_set_state(dev, XY_DEVICE_PM_STATE_OFF);
}

int xy_device_pm_on(xy_device_t *dev)
{
    return xy_device_pm_set_state(dev, XY_DEVICE_PM_STATE_ACTIVE);
}

/* ==================== Auto Power Management ==================== */

/**
 * @brief 检查设备是否空闲超时
 */
void xy_device_pm_check_idle(xy_device_t *dev)
{
    if (!dev) {
        return;
    }
    
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data || data->policy != XY_DEVICE_PM_POLICY_AUTO) {
        return;
    }
    
    if (data->idle_timeout_ms == 0) {
        return; /* 空闲检测禁用 */
    }
    
    uint32_t now = pm_get_tick_ms();
    uint32_t idle_time = now - data->last_activity_time;
    
    if (idle_time >= data->idle_timeout_ms) {
        /* 空闲超时，进入睡眠 */
        if (data->current_state == XY_DEVICE_PM_STATE_ACTIVE) {
            xy_device_pm_sleep(dev);
        }
    }
}

/**
 * @brief 设置空闲超时
 */
int xy_device_pm_set_idle_timeout(xy_device_t *dev, uint32_t timeout_ms)
{
    if (!dev) {
        return XY_DEVICE_INVALID_PARAM;
    }
    
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data) {
        return XY_DEVICE_NOT_INIT;
    }
    
    data->idle_timeout_ms = timeout_ms;
    return XY_DEVICE_OK;
}

/**
 * @brief 记录设备活动
 */
void xy_device_pm_record_activity(xy_device_t *dev)
{
    if (!dev) {
        return;
    }
    
    xy_device_pm_data_t *data = pm_find_data(dev);
    
    if (!data) {
        return;
    }
    
    data->last_activity_time = pm_get_tick_ms();
    
    /* 如果是自动策略且当前在睡眠，唤醒设备 */
    if (data->policy == XY_DEVICE_PM_POLICY_AUTO &&
        data->current_state != XY_DEVICE_PM_STATE_ACTIVE) {
        xy_device_pm_wakeup(dev);
    }
}

/* ==================== End of File ==================== */
