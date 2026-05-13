/**
 * @file xy_st.c
 * @brief 状态机组件实现
 *
 * 支持裸机（Bare-metal）和 RTOS 两种运行环境
 * 通过 XY_OS_BACKEND_* 宏选择后端
 *
 * =============================================================================
 * 调用约定（通用）：
 * - 所有 API 调用必须处于禁中断或持有锁的状态
 * - 不允许在 entry/process/exit 回调中调用 xy_sm_transition 系列函数
 * - 不允许在回调中调用 xy_sm_process_sample
 *
 * 使用示例（裸机）：
 * @code
 * while (1) {
 *     xy_enter_critical();
 *     xy_sm_process_sample(&my_sm, 10);  // 每10ms驱动一次
 *     xy_exit_critical();
 * }
 * @endcode
 *
 * 使用示例（RTOS）：
 * @code
 * void task_func(void *arg) {
 *     while (1) {
 *         xy_os_mutex_lock(&mutex);
 *         xy_sm_process_sample(&my_sm, 10);
 *         xy_os_mutex_unlock(&mutex);
 *         xy_os_delay(10);
 *     }
 * }
 * @endcode
 * =============================================================================
 */

#include "sm.h"
#include "xy_os.h"

/*==============================================================================
 * 后端检测（与 xy_os_cfg.h 保持一致）
 *============================================================================*/

#ifndef XY_OS_BACKEND_BAREMETAL
    #define XY_OS_BACKEND_BAREMETAL  1
    #define XY_OS_BACKEND_FREERTOS   0
    #define XY_OS_BACKEND_RTTHREAD   0
#elif !defined(XY_OS_BACKEND_FREERTOS)
    #define XY_OS_BACKEND_FREERTOS   0
#endif

#if !defined(XY_OS_BACKEND_FREERTOS) || !defined(XY_OS_BACKEND_RTTHREAD)
    #ifndef XY_OS_BACKEND_FREERTOS
        #define XY_OS_BACKEND_FREERTOS   0
    #endif
    #ifndef XY_OS_BACKEND_RTTHREAD
        #define XY_OS_BACKEND_RTTHREAD   0
    #endif
#endif

/*==============================================================================
 * 平台适配层
 *============================================================================*/

/**
 * @brief 进入临界区（平台相关）
 *
 * 裸机：禁用全局中断
 * RTOS：获取互斥锁或禁用调度器
 */
#if XY_OS_BACKEND_BAREMETAL
    #define SM_ENTER_CRITICAL()    do { } while (0)  /* 由调用者保证 */
#elif XY_OS_BACKEND_FREERTOS
    #define SM_ENTER_CRITICAL()    taskENTER_CRITICAL()
    #define SM_EXIT_CRITICAL()     taskEXIT_CRITICAL()
#elif XY_OS_BACKEND_RTTHREAD
    #define SM_ENTER_CRITICAL()    rt_enter_critical()
    #define SM_EXIT_CRITICAL()      rt_exit_critical()
#else
    #define SM_ENTER_CRITICAL()    do { } while (0)
    #define SM_EXIT_CRITICAL()     do { } while (0)
#endif

/*==============================================================================
 * 类型定义
 *============================================================================*/

/**
 * @brief 状态机结构体
 *
 * 使用规则：
 * - 必须作为用户结构体的第一个成员
 * - 通过指针传递，不可值传递
 * - 初始化前所有成员默认为0/NULL
 *
 * @code
 * typedef struct {
 *     xy_sm_t sm;       // 状态机必须作为第一个成员
 *     int my_data;
 * } my_obj_t;
 * @endcode
 */
struct state_machine {
    xy_sm_fn *process;        /**< 当前状态的处理函数 */
    xy_sm_fn *exit;           /**< 当前状态的退出函数 */
    xy_sm_fn *entry;          /**< 目标状态的进入函数 */
    xy_sm_fn *timeout_entry;  /**< 超时时的进入函数 */
    xy_sm_fn *timeout_process;/**< 超时时的处理函数 */
    xy_sm_fn *timeout_exit;   /**< 超时时的退出函数 */
    size_t timeout_counter;   /**< 距离超时的剩余tick数 */
    size_t timeout_total;      /**< 超时总时长（用于恢复） */
};

/*==============================================================================
 * 接口实现
 *============================================================================*/

/**
 * @brief 初始化状态机
 * @param self 状态机指针
 * @note 调用上下文：禁中断状态
 */
void xy_sm_init(xy_sm_t *self)
{
    if (self == NULL) {
        return;
    }

    self->process         = NULL;
    self->exit            = NULL;
    self->entry           = NULL;
    self->timeout_entry   = NULL;
    self->timeout_process = NULL;
    self->timeout_exit    = NULL;
    self->timeout_counter = 0;
    self->timeout_total   = 0;
}

/**
 * @brief 切换到目标状态（无超时）
 * @param self    状态机指针
 * @param entry   目标状态的进入函数（可为NULL）
 * @param process 目标状态的处理函数（可为NULL）
 * @param exit    目标状态的退出函数（可为NULL）
 *
 * @note 调用此函数时必须处于禁中断状态
 * @note 不允许在entry/process/exit回调中调用此函数（会导致递归）
 *
 * 执行流程：
 *   1. 调用当前状态的exit回调（如果存在）
 *   2. 清空所有超时相关字段
 *   3. 切换process/exit/entry到目标状态
 *   4. 调用新状态的entry回调（如果存在）
 */
void xy_sm_transition(xy_sm_t *self, xy_sm_fn *entry,
                      xy_sm_fn *process, xy_sm_fn *exit)
{
    if (self == NULL) {
        return;
    }

    /* 调用当前状态的退出函数 */
    if (self->exit) {
        self->exit(self);
    }

    /* 切换到新状态 */
    self->process         = process;
    self->exit            = exit;
    self->entry           = entry;

    /* 清空超时相关字段 */
    self->timeout_entry   = NULL;
    self->timeout_process = NULL;
    self->timeout_exit    = NULL;
    self->timeout_counter = 0;
    self->timeout_total   = 0;

    /* 调用新状态的进入函数 */
    if (self->entry) {
        self->entry(self);
    }
}

/**
 * @brief 切换到目标状态（带超时检测）
 * @param self            状态机指针
 * @param entry           目标状态的进入函数（可为NULL）
 * @param process         目标状态的处理函数（可为NULL）
 * @param exit            目标状态的退出函数（可为NULL）
 * @param timeout_entry   超时时的进入函数（可为NULL）
 * @param timeout_process 超时时的处理函数（可为NULL）
 * @param timeout_exit    超时时的退出函数（可为NULL）
 * @param timeout         超时时长（tick数）
 *
 * @note 调用此函数时必须处于禁中断状态
 * @note 超时后自动切换到timeout状态，会先调用当前状态的exit
 */
void xy_sm_transition_timeout(xy_sm_t *self, xy_sm_fn *entry,
                              xy_sm_fn *process, xy_sm_fn *exit,
                              xy_sm_fn *timeout_entry,
                              xy_sm_fn *timeout_process,
                              xy_sm_fn *timeout_exit,
                              size_t timeout)
{
    if (self == NULL) {
        return;
    }

    xy_sm_transition(self, entry, process, exit);

    self->timeout_entry   = timeout_entry;
    self->timeout_process = timeout_process;
    self->timeout_exit    = timeout_exit;
    self->timeout_counter = timeout;
    self->timeout_total   = timeout;
}

/**
 * @brief 延迟切换（仅设置超时，到期后切换到timeout状态）
 * @param self            状态机指针
 * @param timeout_entry   超时时的进入函数（可为NULL）
 * @param timeout_process 超时时的处理函数（可为NULL）
 * @param timeout_exit    超时时的退出函数（可为NULL）
 * @param timeout         超时时长（tick数）
 *
 * @note 调用此函数时必须处于禁中断状态
 * @note 当前状态的处理函数和进入/退出函数保持不变，仅设置超时跳转
 *
 * 示例：
 * @code
 * // 保持在WAIT状态，3秒后自动进入TIMEOUT状态
 * xy_sm_transition_delay(&sm, timeout_entry, timeout_process, timeout_exit, 3000);
 * @endcode
 */
void xy_sm_transition_delay(xy_sm_t *self, xy_sm_fn *timeout_entry,
                            xy_sm_fn *timeout_process,
                            xy_sm_fn *timeout_exit,
                            size_t timeout)
{
    if (self == NULL) {
        return;
    }

    self->timeout_entry   = timeout_entry;
    self->timeout_process = timeout_process;
    self->timeout_exit    = timeout_exit;
    self->timeout_counter = timeout;
    self->timeout_total   = timeout;
}

/**
 * @brief 驱动状态机运行（轮询调度）
 * @param self    状态机指针
 * @param timeout 本次调度的间隔（tick数，通常为调度周期如10ms）
 *
 * @note 调用此函数时必须处于禁中断状态
 * @note 应在主循环（裸机）或任务（RTOS）中定期调用，建议每1-100ms调用一次
 *
 * 执行流程：
 *   1. 调用当前状态的process回调
 *   2. 如果有超时设置，递减计数器
 *   3. 超时触发时，执行：exit -> 切换到timeout状态 -> 调用entry
 */
void xy_sm_process_sample(xy_sm_t *self, size_t timeout)
{
    if (self == NULL) {
        return;
    }

    /* 调用当前状态的process函数 */
    if (self->process) {
        self->process(self);
    }

    /* 处理超时逻辑 */
    if (self->timeout_counter > 0) {
        if (timeout >= self->timeout_counter) {
            /* 超时触发：执行exit -> timeout状态切换 */
            if (self->exit) {
                self->exit(self);
            }

            /* 保存目标状态的回调（避免在切换过程中被修改） */
            xy_sm_fn *te = self->timeout_entry;
            xy_sm_fn *tp = self->timeout_process;
            xy_sm_fn *tx = self->timeout_exit;

            /* 清空当前超时相关的字段 */
            self->timeout_entry   = NULL;
            self->timeout_process = NULL;
            self->timeout_exit    = NULL;
            self->timeout_counter = 0;

            /* 切换到timeout状态（不调用entry，保持原子性） */
            self->entry           = te;
            self->process         = tp;
            self->exit            = tx;

            /* 调用timeout状态的entry回调 */
            if (self->entry) {
                self->entry(self);
            }
        } else {
            self->timeout_counter -= timeout;
        }
    }
}

/**
 * @brief 获取状态机当前的超时剩余时间
 * @param self 状态机指针
 * @return 剩余tick数，无超时则返回0
 *
 * @note 调用此函数时必须处于禁中断状态
 * @note 可用于判断距离下次超时还有多久
 */
size_t xy_sm_get_timeout_remain(xy_sm_t *self)
{
    if (self == NULL) {
        return 0;
    }
    return self->timeout_counter;
}

/**
 * @brief 取消当前的超时设置
 * @param self 状态机指针
 *
 * @note 调用此函数时必须处于禁中断状态
 * @note 只取消超时，状态和处理函数保持不变
 * @note 幂等：重复调用无害
 */
void xy_sm_cancel_timeout(xy_sm_t *self)
{
    if (self == NULL) {
        return;
    }

    self->timeout_entry   = NULL;
    self->timeout_process = NULL;
    self->timeout_exit    = NULL;
    self->timeout_counter = 0;
    self->timeout_total   = 0;
}

/*==============================================================================
 * 可选：RTOS 专用接口（仅在非裸机模式下编译）
 *============================================================================*/

#if !XY_OS_BACKEND_BAREMETAL

/**
 * @brief 在 RTOS 任务中安全运行状态机
 * @param self    状态机指针
 * @param timeout 调度间隔（ms）
 * @param mutex   保护状态机的互斥锁（可为NULL表示无需保护）
 *
 * @note 仅在 RTOS 模式下有效
 * @note 推荐在任务循环中替代 xy_sm_process_sample
 *
 * @code
 * void task_func(void *arg) {
 *     while (1) {
 *         xy_sm_process_sample_safe(&sm, 10, &sm_mutex);
 *         xy_os_delay(10);
 *     }
 * }
 * @endcode
 */
void xy_sm_process_sample_safe(xy_sm_t *self, uint32_t timeout_ms,
                               xy_os_mutex_id_t mutex)
{
    if (self == NULL) {
        return;
    }

    if (mutex != NULL) {
        xy_os_mutex_acquire(mutex, XY_OS_WAIT_FOREVER);
    }

    /* 将 ms 转换为 tick，假设 tick 频率为 1000Hz */
    xy_sm_process_sample(self, timeout_ms);

    if (mutex != NULL) {
        xy_os_mutex_release(mutex);
    }
}

/**
 * @brief 从中断安全上下文运行状态机（仅限RTOS）
 * @param self    状态机指针
 * @param timeout 调度间隔（tick数）
 *
 * @note 仅在 RTOS 模式下有效
 * @note 可在定时器中断或高优先级中断中调用
 * @note 不使用锁，假设中断不会与任务并发访问
 */
void xy_sm_process_sample_from_isr(xy_sm_t *self, size_t timeout)
{
    if (self == NULL) {
        return;
    }

    /* ISR 中直接调用，减少栈空间使用 */
    xy_sm_process_sample(self, timeout);
}

#endif /* !XY_OS_BACKEND_BAREMETAL */

/*==============================================================================
 * 辅助函数
 *============================================================================*/

/**
 * @brief 检查状态机是否处于超时状态
 * @param self 状态机指针
 * @return 1 表示正在超时计数中，0 表示无超时或已超时触发
 *
 * @note 调用此函数时必须处于禁中断状态
 */
int xy_sm_is_timeout_active(xy_sm_t *self)
{
    if (self == NULL) {
        return 0;
    }
    return (self->timeout_counter > 0) ? 1 : 0;
}

/**
 * @brief 重置超时计数器（恢复为初始超时值）
 * @param self 状态机指针
 *
 * @note 调用此函数时必须处于禁中断状态
 * @note 如果没有设置过超时，无操作
 */
void xy_sm_reset_timeout(xy_sm_t *self)
{
    if (self == NULL || self->timeout_total == 0) {
        return;
    }

    self->timeout_counter = self->timeout_total;
}

/**
 * @brief 获取当前状态的处理函数
 * @param self 状态机指针
 * @return 当前process函数指针
 *
 * @note 调用此函数时必须处于禁中断状态
 */
xy_sm_fn *xy_sm_get_process(xy_sm_t *self)
{
    if (self == NULL) {
        return NULL;
    }
    return self->process;
}
