/**
 * @file at_power_integration.h
 * @brief 电源管理集成接口
 */

#ifndef AT_POWER_INTEGRATION_H
#define AT_POWER_INTEGRATION_H

#include "at_client.h"
#include "at_power.h"
#include "at_4g.h"

/* 集成式电源管理结构 */
typedef struct {
    /* 基础电源管理 */
    at_power_config_t base_config;

    /* 4G模块专用电源管理 */
    struct {
        bool psm_enabled;         /* PSM是否启用 */
        bool edrx_enabled;        /* eDRX是否启用 */
        uint8_t power_save_level; /* 省电等级 0-9 */
        uint32_t last_data_time;  /* 最后数据时间 */
    } gsm_power;

    /* 应用场景配置 */
    enum {
        POWER_MODE_PERFORMANCE = 0, /* 性能模式 */
        POWER_MODE_BALANCED,        /* 均衡模式 */
        POWER_MODE_POWER_SAVE,      /* 省电模式 */
        POWER_MODE_ULTRA_SAVE,      /* 超级省电模式 */
        POWER_MODE_CUSTOM           /* 自定义模式 */
    } power_mode;

    /* 回调函数 */
    struct {
        void (*on_power_state_change)(pwr_state_t old_state,
                                      pwr_state_t new_state);
        void (*on_low_battery)(uint8_t level);
        void (*on_thermal_warning)(int8_t temperature);
        void (*on_power_fault)(int error_code);
    } callbacks;
} at_power_manager_t;

/* API函数 */

/**
 * @brief 创建电源管理器
 */
at_power_manager_t *at_power_manager_create(at_device_t *dev);

/**
 * @brief 设置电源模式
 */
int at_power_set_mode(at_device_t *dev, int power_mode);

/**
 * @brief 自动电源优化
 */
int at_power_auto_optimize(at_device_t *dev);

/**
 * @brief 功耗分析
 */
int at_power_analyze(at_device_t *dev, float *avg_current_ma,
                     float *estimated_battery_life_hours);

/**
 * @brief 电源故障恢复
 */
int at_power_fault_recovery(at_device_t *dev);

/**
 * @brief 定时任务调度（考虑功耗）
 */
int at_power_schedule_task(at_device_t *dev, void (*task)(void *), void *param,
                           uint32_t interval_ms, uint32_t timeout_ms);

#endif /* AT_POWER_INTEGRATION_H */