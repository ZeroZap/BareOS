/**
 * @file xy_sysmon.h
 * @brief System Monitor - CPU, Memory, Stack Monitoring
 * @version 1.0.0
 * @date 2026-03-01 上午
 */

#ifndef XY_SYSMON_H
#define XY_SYSMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 错误码
 */
#define XY_SYSMON_OK            0
#define XY_SYSMON_ERROR         (-1)
#define XY_SYSMON_INVALID_PARAM (-2)

/**
 * @brief 系统统计信息
 */
typedef struct {
    /* CPU */
    float cpu_usage;                /**< CPU 使用率 (%) */
    uint32_t cpu_idle;              /**< 空闲时间 (ms) */
    uint32_t cpu_total;             /**< 总时间 (ms) */
    
    /* Memory */
    uint32_t heap_total;            /**< 堆总大小 (bytes) */
    uint32_t heap_used;             /**< 堆已使用 (bytes) */
    uint32_t heap_max_used;         /**< 堆最大使用 (bytes) */
    float heap_usage;               /**< 堆使用率 (%) */
    
    /* Stack */
    uint32_t stack_total;           /**< 栈总大小 (bytes) */
    uint32_t stack_used;            /**< 栈已使用 (bytes) */
    uint32_t stack_peak;            /**< 栈峰值使用 (bytes) */
    float stack_usage;              /**< 栈使用率 (%) */
    
    /* Tasks */
    uint32_t task_count;            /**< 任务数量 */
    uint32_t task_max;              /**< 最大任务数 */
    
    /* System */
    uint32_t uptime;                /**< 运行时间 (ms) */
    uint32_t tick_rate;             /**< 系统节拍 (Hz) */
} xy_sys_stats_t;

/**
 * @brief 告警回调
 */
typedef void (*xy_sysmon_alarm_cb)(const char *name, float value, float threshold);

/**
 * @brief 初始化系统监控
 * @return XY_SYSMON_OK 成功，其他值失败
 */
int xy_sysmon_init(void);

/**
 * @brief 获取系统统计信息
 * @param stats 统计信息指针
 * @return XY_SYSMON_OK 成功，其他值失败
 */
int xy_sysmon_get_stats(xy_sys_stats_t *stats);

/**
 * @brief 获取 CPU 使用率
 * @return CPU 使用率 (%)
 */
float xy_sysmon_get_cpu_usage(void);

/**
 * @brief 获取堆使用率
 * @return 堆使用率 (%)
 */
float xy_sysmon_get_heap_usage(void);

/**
 * @brief 获取栈使用率
 * @return 栈使用率 (%)
 */
float xy_sysmon_get_stack_usage(void);

/**
 * @brief 获取系统运行时间
 * @return 运行时间 (ms)
 */
uint32_t xy_sysmon_get_uptime(void);

/**
 * @brief 获取任务数量
 * @return 任务数量
 */
uint32_t xy_sysmon_get_task_count(void);

/**
 * @brief 注册告警回调
 * @param name 告警名称
 * @param threshold 阈值
 * @param callback 回调函数
 * @return XY_SYSMON_OK 成功，其他值失败
 */
int xy_sysmon_register_alarm(const char *name, float threshold, xy_sysmon_alarm_cb callback);

/**
 * @brief 打印系统状态
 */
void xy_sysmon_print_status(void);

/**
 * @brief 打印任务列表
 */
void xy_sysmon_print_tasks(void);

#ifdef __cplusplus
}
#endif

#endif /* XY_SYSMON_H */
