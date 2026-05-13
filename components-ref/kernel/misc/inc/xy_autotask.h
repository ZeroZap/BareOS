/**
 * @file xy_autotask.h
 * @brief Autonomous Task Scheduler - Auto execute TODO when idle
 * @version 1.0.0
 * @date 2026-03-01
 */

#ifndef XY_AUTOTASK_H
#define XY_AUTOTASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 错误码
 */
#define XY_AUTOTASK_OK          0
#define XY_AUTOTASK_ERROR       (-1)
#define XY_AUTOTASK_INVALID_PARAM (-2)
#define XY_AUTOTASK_BUSY        (-3)
#define XY_AUTOTASK_STOPPED     (-4)

/**
 * @brief 自主任务状态
 */
typedef enum {
    XY_AUTOTASK_STATE_IDLE = 0,     /**< 空闲 */
    XY_AUTOTASK_STATE_RUNNING = 1,  /**< 运行中 */
    XY_AUTOTASK_STATE_STOPPED = 2,  /**< 已停止 */
    XY_AUTOTASK_STATE_PAUSED = 3,   /**< 已暂停 */
} xy_autotask_state_t;

/**
 * @brief 自主任务类型
 */
typedef enum {
    XY_AUTOTASK_TYPE_TODO = 0,      /**< 执行 TODO 任务 */
    XY_AUTOTASK_TYPE_LEARN = 1,     /**< 自主学习 */
    XY_AUTOTASK_TYPE_CLEANUP = 2,   /**< 清理优化 */
    XY_AUTOTASK_TYPE_CUSTOM = 3,    /**< 自定义任务 */
} xy_autotask_type_t;

/**
 * @brief 自主任务回调
 */
typedef int (*xy_autotask_callback_t)(void *arg);
typedef void (*xy_autotask_complete_cb_t)(int result, void *arg);

/**
 * @brief 自主任务配置
 */
typedef struct {
    uint32_t idle_timeout_ms;       /**< 空闲超时 (ms) 触发自主任务 */
    xy_autotask_type_t default_type;/**< 默认任务类型 */
    bool auto_start;                /**< 是否自动启动 */
    uint32_t max_run_time_ms;       /**< 最大运行时间 (ms) */
} xy_autotask_config_t;

/**
 * @brief 自主任务调度器
 */
typedef struct {
    xy_autotask_config_t config;    /**< 配置 */
    xy_autotask_state_t state;      /**< 状态 */
    xy_autotask_type_t current_type;/**< 当前任务类型 */
    
    uint32_t last_activity;         /**< 上次活动时间 */
    uint32_t start_time;            /**< 任务开始时间 */
    uint32_t run_count;             /**< 运行次数 */
    uint32_t stop_count;            /**< 被叫停次数 */
    
    xy_autotask_callback_t task_cb; /**< 任务回调 */
    xy_autotask_complete_cb_t complete_cb; /**< 完成回调 */
    void *user_arg;                 /**< 用户参数 */
    
    bool initialized;               /**< 初始化标志 */
    bool activity_monitoring;       /**< 活动监控启用 */
} xy_autotask_scheduler_t;

/**
 * @brief 初始化自主任务调度器
 * @param scheduler 调度器句柄
 * @param config 配置
 * @return XY_AUTOTASK_OK 成功，其他值失败
 */
int xy_autotask_init(xy_autotask_scheduler_t *scheduler, const xy_autotask_config_t *config);

/**
 * @brief 反初始化
 * @param scheduler 调度器句柄
 * @return XY_AUTOTASK_OK 成功，其他值失败
 */
int xy_autotask_deinit(xy_autotask_scheduler_t *scheduler);

/**
 * @brief 启动调度器
 * @param scheduler 调度器句柄
 * @return XY_AUTOTASK_OK 成功，其他值失败
 */
int xy_autotask_start(xy_autotask_scheduler_t *scheduler);

/**
 * @brief 停止调度器
 * @param scheduler 调度器句柄
 * @return XY_AUTOTASK_OK 成功，其他值失败
 */
int xy_autotask_stop(xy_autotask_scheduler_t *scheduler);

/**
 * @brief 记录活动 (重置空闲计时器)
 * @param scheduler 调度器句柄
 * @return XY_AUTOTASK_OK 成功，其他值失败
 */
int xy_autotask_record_activity(xy_autotask_scheduler_t *scheduler);

/**
 * @brief 暂停自主任务
 * @param scheduler 调度器句柄
 * @return XY_AUTOTASK_OK 成功，其他值失败
 */
int xy_autotask_pause(xy_autotask_scheduler_t *scheduler);

/**
 * @brief 恢复自主任务
 * @param scheduler 调度器句柄
 * @return XY_AUTOTASK_OK 成功，其他值失败
 */
int xy_autotask_resume(xy_autotask_scheduler_t *scheduler);

/**
 * @brief 设置任务类型
 * @param scheduler 调度器句柄
 * @param type 任务类型
 * @return XY_AUTOTASK_OK 成功，其他值失败
 */
int xy_autotask_set_task_type(xy_autotask_scheduler_t *scheduler, xy_autotask_type_t type);

/**
 * @brief 设置空闲超时
 * @param scheduler 调度器句柄
 * @param timeout_ms 超时时间 (ms)
 * @return XY_AUTOTASK_OK 成功，其他值失败
 */
int xy_autotask_set_idle_timeout(xy_autotask_scheduler_t *scheduler, uint32_t timeout_ms);

/**
 * @brief 获取状态
 * @param scheduler 调度器句柄
 * @return 当前状态
 */
xy_autotask_state_t xy_autotask_get_state(const xy_autotask_scheduler_t *scheduler);

/**
 * @brief 获取运行统计
 * @param scheduler 调度器句柄
 * @param run_count 运行次数指针
 * @param stop_count 停止次数指针
 * @return XY_AUTOTASK_OK 成功，其他值失败
 */
int xy_autotask_get_stats(const xy_autotask_scheduler_t *scheduler, 
                          uint32_t *run_count, uint32_t *stop_count);

/**
 * @brief 注册任务回调
 * @param scheduler 调度器句柄
 * @param callback 任务回调
 * @param arg 用户参数
 * @return XY_AUTOTASK_OK 成功，其他值失败
 */
int xy_autotask_register_callback(xy_autotask_scheduler_t *scheduler,
                                  xy_autotask_callback_t callback, void *arg);

/**
 * @brief 注册完成回调
 * @param scheduler 调度器句柄
 * @param callback 完成回调
 * @return XY_AUTOTASK_OK 成功，其他值失败
 */
int xy_autotask_register_complete_callback(xy_autotask_scheduler_t *scheduler,
                                         xy_autotask_complete_cb_t callback);

/**
 * @brief 手动触发自主任务
 * @param scheduler 调度器句柄
 * @return XY_AUTOTASK_OK 成功，其他值失败
 */
int xy_autotask_trigger(xy_autotask_scheduler_t *scheduler);

/**
 * @brief 启用/禁用活动监控
 * @param scheduler 调度器句柄
 * @param enable 是否启用
 * @return XY_AUTOTASK_OK 成功，其他值失败
 */
int xy_autotask_enable_monitoring(xy_autotask_scheduler_t *scheduler, bool enable);

/**
 * @brief 获取空闲时间
 * @param scheduler 调度器句柄
 * @return 空闲时间 (ms)
 */
uint32_t xy_autotask_get_idle_time(const xy_autotask_scheduler_t *scheduler);

/**
 * @brief 获取剩余运行时间
 * @param scheduler 调度器句柄
 * @return 剩余时间 (ms)
 */
uint32_t xy_autotask_get_remaining_time(const xy_autotask_scheduler_t *scheduler);

#ifdef __cplusplus
}
#endif

#endif /* XY_AUTOTASK_H */
