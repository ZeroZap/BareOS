/**
 * @file xy_st.h
 * @brief 状态机组件头文件
 *
 * 支持裸机（Bare-metal）和 RTOS 两种运行环境
 * 通过 XY_OS_BACKEND_* 宏选择后端
 *
 * =============================================================================
 * 状态机模型：
 *
 * 每个状态包含三个回调：
 *   - entry:   进入状态时调用（可选）
 *   - process: 状态运行中定期调用（必须，否则状态无法响应）
 *   - exit:    退出状态时调用（可选）
 *
 * 超时机制：
 *   - 可设置超时时间，超时后自动切换到指定的timeout状态
 *   - timeout状态也有entry/process/exit
 *
 * 示例状态图：
 *
 *   [IDLE] --event--> [RUNNING] --timeout 5s--> [TIMEOUT_STATE]
 *                |
 *                +--event--> [ERROR]
 *
 * =============================================================================
 * 后端选择（与 xy_os_cfg.h 保持一致）：
 *
 *   XY_OS_BACKEND_BAREMETAL  - 裸机，无RTOS
 *   XY_OS_BACKEND_FREERTOS   - FreeRTOS
 *   XY_OS_BACKEND_RTTHREAD   - RT-Thread
 *
 * =============================================================================
 * 调用约定（通用）：
 *
 * +----------------------------+---------------------------------------------+
 * | 函数                       | 调用上下文                                  |
 * +----------------------------+---------------------------------------------+
 * | xy_sm_init                 | 禁中断/持有锁                               |
 * | xy_sm_transition           | 禁中断/持有锁                               |
 * | xy_sm_transition_timeout   | 禁中断/持有锁                               |
 * | xy_sm_transition_delay     | 禁中断/持有锁                               |
 * | xy_sm_process_sample       | 禁中断/持有锁（主循环/任务中定期调用）       |
 * | xy_sm_process_sample_safe  | RTOS任务中（自动加锁）                      |
 * | xy_sm_process_sample_from_isr| RTOS中断中                                 |
 * | xy_sm_get_timeout_remain   | 禁中断/持有锁                               |
 * | xy_sm_cancel_timeout       | 禁中断/持有锁                               |
 * +----------------------------+---------------------------------------------+
 *
 * =============================================================================
 * 使用示例（裸机）：
 *
 * @code
 * // 定义包含状态机的结构体
 * typedef struct {
 *     xy_sm_t sm;         // 状态机必须作为第一个成员
 *     int my_data;
 * } my_obj_t;
 *
 * // 定义状态处理函数
 * void idle_entry(xy_sm_t *s)  { printf("-> IDLE\n"); }
 * void idle_process(xy_sm_t *s) { printf("IDLE processing\n"); }
 * void idle_exit(xy_sm_t *s)    { printf("IDLE exit\n"); }
 *
 * void running_entry(xy_sm_t *s) { printf("-> RUNNING\n"); }
 * void running_process(xy_sm_t *s) { printf("RUNNING processing\n"); }
 *
 * // 初始化并启动
 * my_obj_t obj;
 * xy_sm_init(&obj.sm);
 * xy_sm_transition(&obj.sm, idle_entry, idle_process, NULL);
 *
 * // 主循环
 * while (1) {
 *     xy_enter_critical();
 *     xy_sm_process_sample(&obj.sm, 10);  // 10ms驱动一次
 *     xy_exit_critical();
 * }
 * @endcode
 *
 * =============================================================================
 * 使用示例（RTOS - FreeRTOS）：
 *
 * @code
 * xy_os_mutex_id_t sm_mutex;
 *
 * void task_func(void *arg) {
 *     while (1) {
 *         xy_sm_process_sample_safe(&sm, 10, sm_mutex);
 *         xy_os_delay(10);
 *     }
 * }
 *
 * void timer_isr_example(void) {
 *     // 定时器中断中快速处理
 *     xy_sm_process_sample_from_isr(&sm, 1);
 * }
 * @endcode
 *
 * =============================================================================
 */

#ifndef _XY_STATE_MACHINE_H_
#define _XY_STATE_MACHINE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
 * 后端检测（与 xy_os_cfg.h 保持一致）
 *============================================================================*/

/**
 * @brief 裸机后端
 * @note 未定义时默认启用
 */
#ifndef XY_OS_BACKEND_BAREMETAL
    #define XY_OS_BACKEND_BAREMETAL  1
#endif

/**
 * @brief FreeRTOS后端
 * @note 由 xy_os_cfg.h 或用户定义
 */
#ifndef XY_OS_BACKEND_FREERTOS
    #define XY_OS_BACKEND_FREERTOS   0
#endif

/**
 * @brief RT-Thread后端
 * @note 由 xy_os_cfg.h 或用户定义
 */
#ifndef XY_OS_BACKEND_RTTHREAD
    #define XY_OS_BACKEND_RTTHREAD   0
#endif

/*==============================================================================
 * 平台适配层
 *============================================================================*/

/**
 * @brief 进入临界区（平台相关）
 *
 * 裸机：依赖调用者保证禁中断
 * RTOS：获取互斥锁或禁用调度器
 */
#if XY_OS_BACKEND_BAREMETAL
    /* 裸机下由调用者保证禁中断，此处为空 */
    #define SM_ENTER_CRITICAL()   do { } while (0)
    #define SM_EXIT_CRITICAL()    do { } while (0)
#elif XY_OS_BACKEND_FREERTOS
    #define SM_ENTER_CRITICAL()   taskENTER_CRITICAL()
    #define SM_EXIT_CRITICAL()    taskEXIT_CRITICAL()
#elif XY_OS_BACKEND_RTTHREAD
    #define SM_ENTER_CRITICAL()   rt_enter_critical()
    #define SM_EXIT_CRITICAL()     rt_exit_critical()
#else
    #define SM_ENTER_CRITICAL()   do { } while (0)
    #define SM_EXIT_CRITICAL()    do { } while (0)
#endif

/*==============================================================================
 * 类型定义
 *============================================================================*/

/**
 * @brief 状态机函数指针类型
 * @param self 指向拥有该状态的xy_sm_t结构体
 *
 * @note 在回调中不允许调用以下函数（会破坏状态机原子性）：
 *       - xy_sm_transition()
 *       - xy_sm_transition_timeout()
 *       - xy_sm_transition_delay()
 *       - xy_sm_process_sample()
 *       - xy_sm_cancel_timeout()
 */
typedef void xy_sm_fn(xy_sm_t *self);

/**
 * @brief 状态机结构体（透明类型，用户不应直接访问内部成员）
 *
 * 使用规则：
 * - 必须作为用户结构体的第一个成员
 * - 通过指针传递，不可值传递
 * - 初始化前所有成员默认为0/NULL
 *
 * @code
 * typedef struct {
 *     xy_sm_t sm;         // 状态机必须作为第一个成员
 *     int my_data;        // 用户数据
 * } my_obj_t;
 *
 * my_obj_t obj;
 * obj.sm.process = my_process;
 * xy_sm_init(&obj.sm);
 * @endcode
 */
typedef struct state_machine xy_sm_t;

/**
 * @cond INTERNAL
 * @{
 */

/**
 * @brief 状态机内部结构
 * @note 仅供内部使用，不应直接访问
 */
struct state_machine {
    xy_sm_fn *process;         /**< 当前状态的处理函数 */
    xy_sm_fn *exit;            /**< 当前状态的退出函数 */
    xy_sm_fn *entry;           /**< 目标状态的进入函数 */
    xy_sm_fn *timeout_entry;   /**< 超时时的进入函数 */
    xy_sm_fn *timeout_process; /**< 超时时的处理函数 */
    xy_sm_fn *timeout_exit;    /**< 超时时的退出函数 */
    size_t timeout_counter;    /**< 距离超时的剩余tick数 */
    size_t timeout_total;      /**< 超时总时长（用于恢复） */
};

/** @} @endcond */

/*==============================================================================
 * 核心接口
 *============================================================================*/

/**
 * @brief 初始化状态机
 * @param self 状态机指针
 *
 * @note 调用上下文：禁中断状态（裸机）/ 持有锁（RTOS）
 * @note 应在状态机使用前调用，建议在系统初始化时调用
 */
void xy_sm_init(xy_sm_t *self);

/**
 * @brief 同步切换到目标状态（无超时）
 * @param self    状态机指针
 * @param entry   目标状态的进入函数（可为NULL）
 * @param process 目标状态的处理函数（可为NULL）
 * @param exit    目标状态的退出函数（可为NULL）
 *
 * @note 调用此函数时必须处于禁中断/持有锁状态
 * @note 不允许在entry/process/exit回调中调用（会导致递归）
 *
 * 执行流程：
 *   1. 调用当前状态的exit回调（如果存在）
 *   2. 清空所有超时相关字段
 *   3. 切换process/exit/entry到目标状态
 *   4. 调用新状态的entry回调（如果存在）
 */
void xy_sm_transition(xy_sm_t *self, xy_sm_fn *entry,
                      xy_sm_fn *process, xy_sm_fn *exit);

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
 * @note 调用此函数时必须处于禁中断/持有锁状态
 * @note 超时后自动切换到timeout状态，会先调用当前状态的exit
 */
void xy_sm_transition_timeout(xy_sm_t *self, xy_sm_fn *entry,
                              xy_sm_fn *process, xy_sm_fn *exit,
                              xy_sm_fn *timeout_entry,
                              xy_sm_fn *timeout_process,
                              xy_sm_fn *timeout_exit,
                              size_t timeout);

/**
 * @brief 设置延迟切换（保持当前状态，仅设置超时跳转）
 * @param self            状态机指针
 * @param timeout_entry   超时时的进入函数（可为NULL）
 * @param timeout_process 超时时的处理函数（可为NULL）
 * @param timeout_exit    超时时的退出函数（可为NULL）
 * @param timeout         超时时长（tick数）
 *
 * @note 调用此函数时必须处于禁中断/持有锁状态
 * @note 与xy_sm_transition_timeout的区别：
 *       - 此函数不改变当前状态的entry/process/exit
 *       - 仅设置超时触发后的目标状态
 *       - 适用于"X秒后自动进入Y状态"的场景
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
                            size_t timeout);

/**
 * @brief 驱动状态机运行（轮询调度）
 * @param self    状态机指针
 * @param timeout 本次调度的间隔（tick数，通常为调度周期如10ms）
 *
 * @note 调用此函数时必须处于禁中断/持有锁状态
 * @note 应在主循环（裸机）或任务（RTOS）中定期调用，建议每1-100ms调用一次
 *
 * 执行流程：
 *   1. 调用当前状态的process回调
 *   2. 如果有超时设置，递减计数器
 *   3. 超时触发时，执行：exit -> 切换到timeout状态 -> 调用entry
 */
void xy_sm_process_sample(xy_sm_t *self, size_t timeout);

/*==============================================================================
 * 辅助接口
 *============================================================================*/

/**
 * @brief 获取状态机当前的超时剩余时间
 * @param self 状态机指针
 * @return 剩余tick数，无超时设置则返回0
 *
 * @note 调用此函数时必须处于禁中断/持有锁状态
 * @note 可用于判断距离下次超时还有多久
 */
size_t xy_sm_get_timeout_remain(xy_sm_t *self);

/**
 * @brief 取消当前的超时设置
 * @param self 状态机指针
 *
 * @note 调用此函数时必须处于禁中断/持有锁状态
 * @note 调用后不再触发超时切换，当前状态和处理函数保持不变
 * @note 幂等：重复调用无害
 */
void xy_sm_cancel_timeout(xy_sm_t *self);

/**
 * @brief 检查状态机是否处于超时计时状态
 * @param self 状态机指针
 * @return 1 表示正在计时中，0 表示无超时或已触发
 *
 * @note 调用此函数时必须处于禁中断/持有锁状态
 */
int xy_sm_is_timeout_active(xy_sm_t *self);

/**
 * @brief 重置超时计数器（恢复为初始超时值）
 * @param self 状态机指针
 *
 * @note 调用此函数时必须处于禁中断/持有锁状态
 * @note 如果没有设置过超时，无操作
 */
void xy_sm_reset_timeout(xy_sm_t *self);

/**
 * @brief 获取当前状态的处理函数
 * @param self 状态机指针
 * @return 当前process函数指针，状态机为NULL则返回NULL
 *
 * @note 调用此函数时必须处于禁中断/持有锁状态
 * @note 可用于判断当前状态
 */
xy_sm_fn *xy_sm_get_process(xy_sm_t *self);

/*==============================================================================
 * RTOS 专用接口（仅在非裸机模式下编译）
 *============================================================================*/

#if !XY_OS_BACKEND_BAREMETAL

/**
 * @brief 在 RTOS 任务中安全运行状态机（自动加锁）
 * @param self        状态机指针
 * @param timeout_ms  调度间隔（毫秒）
 * @param mutex       保护状态机的互斥锁（可为NULL表示无需保护）
 *
 * @note 仅在 RTOS 模式下有效
 * @note 推荐在任务循环中替代 xy_sm_process_sample
 *
 * @code
 * void task_func(void *arg) {
 *     while (1) {
 *         xy_sm_process_sample_safe(&sm, 10, sm_mutex);
 *         xy_os_delay(10);
 *     }
 * }
 * @endcode
 */
void xy_sm_process_sample_safe(xy_sm_t *self, uint32_t timeout_ms,
                               xy_os_mutex_id_t mutex);

/**
 * @brief 从中断安全上下文运行状态机（仅限RTOS）
 * @param self    状态机指针
 * @param timeout 调度间隔（tick数）
 *
 * @note 仅在 RTOS 模式下有效
 * @note 可在定时器中断或高优先级中断中调用
 * @note 不使用锁，假设中断不会与任务并发访问
 */
void xy_sm_process_sample_from_isr(xy_sm_t *self, size_t timeout);

#endif /* !XY_OS_BACKEND_BAREMETAL */

/*==============================================================================
 * 回调约束说明
 *============================================================================*/

/**
 * @page callback_constraints 状态机回调约束
 *
 * 在状态机的 entry/process/exit 回调中，禁止调用以下函数：
 *
 * - xy_sm_transition()       - 会导致状态机递归
 * - xy_sm_transition_timeout() - 会导致状态机递归
 * - xy_sm_transition_delay()  - 会导致状态机递归
 * - xy_sm_process_sample()    - 可能导致状态错乱
 * - xy_sm_cancel_timeout()    - 可能导致状态错乱
 * - xy_sm_reset_timeout()    - 可能导致状态错乱
 *
 * 允许调用的函数：
 * - xy_sm_get_timeout_remain() - 只读，安全
 * - xy_sm_is_timeout_active()  - 只读，安全
 * - xy_sm_get_process()        - 只读，安全
 *
 * 如果必须在回调中触发状态转换，建议：
 * 1. 设置一个标志位
 * 2. 在主循环/任务中检测标志位后调用 xy_sm_transition()
 * 3. 或者使用事件队列将转换请求发送到主线程处理
 */

#ifdef __cplusplus
}
#endif

#endif /* _XY_STATE_MACHINE_H_ */
