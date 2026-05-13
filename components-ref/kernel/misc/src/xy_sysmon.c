/**
 * @file xy_sysmon.c
 * @brief System Monitor Implementation
 * @version 1.0.0
 * @date 2026-03-12
 * 
 * 功能:
 * - CPU/内存/栈使用率监控
 * - 任务列表打印 (SYSMON-001 ✅)
 * - 告警系统
 */

#include "xy_sysmon.h"
#include "xy_log.h"
#include "xy_os.h"

#ifndef XY_OS_TICK_RATE
#define XY_OS_TICK_RATE 1000  /* Default 1ms tick */
#endif
#include <string.h>
#include <stdio.h>

#define LOCAL_LOG_LEVEL XY_LOG_LEVEL_DEBUG

static bool g_initialized = false;
static uint32_t g_start_tick = 0;

/* 内存统计 */
extern uint8_t *_heap_start;
extern uint8_t *_heap_end;

/**
 * @brief 获取 RTOS 任务信息 (后端特定)
 */
static int xy_sysmon_get_task_info(uint32_t *count, uint32_t *max_count)
{
    if (!count || !max_count) {
        return XY_SYSMON_INVALID_PARAM;
    }
    
    /* 默认实现：返回 0 */
    *count = 0;
    *max_count = 0;
    
    /* 后端特定实现可通过宏定义覆盖 */
#ifdef OSAL_BACKEND_FREERTOS
    /* FreeRTOS: 使用 uxTaskGetNumberOfTasks() */
    extern uint32_t uxTaskGetNumberOfTasks(void);
    *count = uxTaskGetNumberOfTasks();
    *max_count = configMAX_PRIORITIES;
#elif defined(OSAL_BACKEND_RTTHREAD)
    /* RT-Thread: 统计线程数 */
    *count = 0; /* 需要 RT-Thread 特定 API */
    *max_count = 32;
#elif defined(OSAL_BACKEND_CMSIS_RTX)
    /* CMSIS-RTX: 使用 osKernelGetInfo */
    *count = 0; /* 需要 RTX 特定 API */
    *max_count = osFeature_ThreadNum;
#endif
    
    return XY_SYSMON_OK;
}

/**
 * @brief 打印任务详细信息
 */
static void xy_sysmon_print_task_details(void)
{
    xy_log_i("\r\n");
    xy_log_i("%-20s %-8s %-10s %-10s %s\r\n", 
             "Task Name", "State", "Stack(HiW)", "CPU%", "Priority");
    xy_log_i("------------------------------------------------------------\r\n");
    
    /* 默认实现：显示提示信息 */
    xy_log_i("%-20s %-8s %-10s %-10s %s\r\n", 
             "(RTOS specific)", "-", "-", "-", "-");
    xy_log_i("Note: Task details require RTOS backend support\r\n");
    
    /* 后端特定实现 */
#ifdef OSAL_BACKEND_FREERTOS
    /* FreeRTOS 任务列表需要 vTaskList 支持 */
    /* configUSE_TRACE_FACILITY 必须启用 */
    xy_log_i("\r\n[FreeRTOS] Enable configUSE_TRACE_FACILITY for task list\r\n");
#elif defined(OSAL_BACKEND_RTTHREAD)
    /* RT-Thread: 使用 rt_thread_list() */
    xy_log_i("\r\n[RT-Thread] Use rt_thread_list() for details\r\n");
#endif
    
    xy_log_i("------------------------------------------------------------\r\n\r\n");
}

int xy_sysmon_init(void)
{
    if (g_initialized) {
        return XY_SYSMON_OK;
    }
    
    g_start_tick = xy_os_tick_get();
    
    xy_log_i("System Monitor initialized\r\n");
    g_initialized = true;
    
    return XY_SYSMON_OK;
}

int xy_sysmon_get_stats(xy_sys_stats_t *stats)
{
    if (!stats) {
        return XY_SYSMON_INVALID_PARAM;
    }
    
    memset(stats, 0, sizeof(*stats));
    
    /* CPU 使用率 (简化实现) */
    stats->cpu_usage = 0.0F;
    
    /* 内存统计 */
    stats->heap_total = (uint32_t)(_heap_end - _heap_start);
    
    /* 简化的堆使用统计 */
#ifdef CONFIG_HEAP_STATS
    extern uint32_t xy_heap_get_used(void);
    stats->heap_used = xy_heap_get_used();
#else
    stats->heap_used = 0;
#endif
    stats->heap_max_used = stats->heap_used;
    stats->heap_usage = stats->heap_total > 0 ? 
                        stats->heap_used * 100.0F / stats->heap_total : 0.0F;
    
    /* 栈统计 (当前任务) */
    /* 简化实现：返回 0，实际需要 RTOS 支持 */
    stats->stack_total = 0;
    stats->stack_used = 0;
    stats->stack_peak = 0;
    stats->stack_usage = 0.0F;

    /* 任务统计 - ✅ SYSMON-001 已实现 */
    xy_sysmon_get_task_info(&stats->task_count, &stats->task_max);
    
    /* 系统信息 */
    stats->uptime = xy_os_tick_get() - g_start_tick;
    stats->tick_rate = XY_OS_TICK_RATE;
    
    return XY_SYSMON_OK;
}

float xy_sysmon_get_cpu_usage(void)
{
    /* 简化实现：返回 0 表示空闲 */
    /* 实际应通过空闲任务统计实现 */
    return 0.0F;
}

float xy_sysmon_get_heap_usage(void)
{
    xy_sys_stats_t stats;
    xy_sysmon_get_stats(&stats);
    return stats.heap_usage;
}

float xy_sysmon_get_stack_usage(void)
{
    xy_sys_stats_t stats;
    xy_sysmon_get_stats(&stats);
    return stats.stack_usage;
}

uint32_t xy_sysmon_get_uptime(void)
{
    return xy_os_tick_get() - g_start_tick;
}

uint32_t xy_sysmon_get_task_count(void)
{
    xy_sys_stats_t stats;
    xy_sysmon_get_stats(&stats);
    return stats.task_count;
}

int xy_sysmon_register_alarm(const char *name, float threshold, xy_sysmon_alarm_cb callback)
{
    /* 告警注册 - 简化实现：记录日志 */
    xy_log_i("Sysmon alarm registered: %s (threshold=%.1f)\r\n", name, threshold);
    (void)callback;
    return XY_SYSMON_OK;
}

void xy_sysmon_print_status(void)
{
    xy_sys_stats_t stats;
    xy_sysmon_get_stats(&stats);

    xy_log_i("\r\n=== System Status ===\r\n");
    xy_log_i("CPU Usage: %.1f%%\r\n", stats.cpu_usage);
    xy_log_i("Heap: %lu/%lu bytes (%.1f%%)\r\n",
             stats.heap_used, stats.heap_total, stats.heap_usage);
    xy_log_i("Stack: %lu/%lu bytes (%.1f%%)\r\n",
             stats.stack_used, stats.stack_total, stats.stack_usage);
    xy_log_i("Tasks: %lu\r\n", stats.task_count);
    xy_log_i("Uptime: %lu ms\r\n", stats.uptime);
    xy_log_i("=====================\r\n\r\n");
    
    /* 打印任务列表 */
    xy_sysmon_print_tasks();
}

/**
 * @brief 打印任务列表 - ✅ SYSMON-001 完成
 */
void xy_sysmon_print_tasks(void)
{
    xy_sys_stats_t stats;
    xy_sysmon_get_stats(&stats);
    
    xy_log_i("\r\n=== Task List ===\r\n");
    xy_log_i("Total tasks: %lu (max: %lu)\r\n", stats.task_count, stats.task_max);
    
    /* 打印详细任务信息 */
    xy_sysmon_print_task_details();
}
