#ifndef _XY_TIMER_H_
#define _XY_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

//==============================================================================
// 类型定义
//==============================================================================

typedef void *xy_timer_ref;

/**
 * @brief 定时器回调函数类型
 * @param timer_handler 定时器句柄，用于在回调中访问或操作定时器
 * @param params 用户传入的自定义参数
 *
 * @note 回调函数中不允许调用以下函数（会导致链表损坏或double-free）：
 *        - xy_timer_create()
 *        - xy_timer_kill() (操作同一定时器)
 *        - xy_timer_change_cnt()
 *        - xy_timer_change_reload()
 *        - xy_timer_change_func()
 */
#if (PLATFORM == PLATFORM_C51)
typedef void (*timer_proc)(xy_timer_ref xdata timer_handler,
                           void xdata *params) reentrant;
#else
typedef void (*timer_proc)(xy_timer_ref timer_handler, void *params) reentrant;
#endif

//==============================================================================
// 函数接口
//==============================================================================

/**
 * @brief 初始化定时器模块
 * @note 应在系统初始化时调用，建议在硬件定时器中断配置之前调用
 * @note 调用上下文：禁中断状态
 */
void xy_timer_init(void);

/**
 * @brief 获取当前系统tick值
 * @return 当前tick计数值
 * @note 调用上下文：任意（已内置临界区保护）
 */
xy_uint32_t xy_timer_get_tick(void);

/**
 * @brief 从中断中获取当前tick值（更快，但不保护）
 * @return 当前tick计数值
 * @note 调用上下文：仅限定时器硬件中断内
 */
xy_uint32_t xy_timer_get_tick_from_isr(void);

/**
 * @brief 手动设置系统tick值
 * @param tick 新的tick值
 * @note 调用上下文：禁中断状态（慎用，可能导致定时器计时错乱）
 * @warning 通常不需要手动设置，仅在特殊场景下使用
 */
void xy_timer_set_tick(xy_uint32_t tick);

/**
 * @brief 获取最近一个定时器到期还需的tick数
 * @return 下一个到期定时器还需的tick数，无定时器则返回0
 * @note 调用上下文：任意
 */
xy_uint32_t xy_timer_get_nexttick(void);

/**
 * @brief 创建定时器
 * @param cnt    初始计数值（tick数，0表示立即触发一次）
 * @param reload 重载值（0表示单次定时器，非0表示自动重载周期）
 * @param pfunc  回调函数，不能为NULL
 * @param params 回调参数（可为NULL）
 * @return 定时器句柄，失败返回NULL
 * @note 调用上下文：禁中断状态
 * @note cnt 为相对时间，会自动加上系统运行至今的偏移量
 */
xy_timer_ref xy_timer_create(xy_uint32_t cnt, xy_uint32_t reload,
                             timer_proc pfunc, void *params);

/**
 * @brief 定时器Tick更新（驱动定时器运行）
 * @note 必须且仅在硬件定时器中断中调用，通常由1ms硬件定时器触发
 * @note 中断内不可调用其他xy_timer_*函数（除xy_timer_get_tick_from_isr外）
 *
 * 调用示例（伪代码）：
 * @code
 * void TIM1_IRQHandler(void) {
 *     if (xy_timer_get_nexttick() == 0) {
 *         return; // 无活跃定时器，可跳过处理
 *     }
 *     xy_timer_ticks();
 * }
 * @endcode
 */
void xy_timer_ticks(void);

/**
 * @brief 删除定时器
 * @param timer_handler 定时器句柄
 * @note 调用上下文：禁中断状态
 * @note 在定时器回调中调用此函数是安全的（会被延迟到回调结束后删除）
 * @note 允许对同一定时器句柄重复调用（幂等）
 */
void xy_timer_kill(xy_timer_ref timer_handler);

/**
 * @brief 修改定时器当前计数值
 * @param timer_handler 定时器句柄
 * @param cnt           新的计数值（tick数）
 * @note 调用上下文：禁中断状态
 * @note 不允许在定时器回调中调用（会被忽略）
 */
void xy_timer_change_cnt(xy_timer_ref timer_handler, xy_uint32_t cnt);

/**
 * @brief 修改定时器重载值
 * @param timer_handler 定时器句柄
 * @param reload        新的重载值（0=单次，非0=自动重载）
 * @note 调用上下文：禁中断状态
 */
void xy_timer_change_reload(xy_timer_ref timer_handler, xy_uint32_t reload);

/**
 * @brief 修改定时器回调函数
 * @param timer_handler 定时器句柄
 * @param pfunc         新的回调函数
 * @note 调用上下文：禁中断状态
 */
void xy_timer_change_func(xy_timer_ref timer_handler, timer_proc pfunc);

/**
 * @brief 获取定时器当前回调函数
 * @param timer_handler 定时器句柄
 * @return 当前回调函数指针，无则返回NULL
 * @note 调用上下文：任意
 */
timer_proc xy_timer_get_func(xy_timer_ref timer_handler);

//==============================================================================
// 附录：裸机调用约定速查表
//==============================================================================
/*

+-------------------+----------------------------+----------------------------+
|      函数         |        ISR (中断中)        |       Main/Task            |
+-------------------+----------------------------+----------------------------+
| xy_timer_ticks    |  ✅ 必须（仅此上下文）      |  ❌ 禁止                   |
| xy_timer_create   |  ❌                        |  ✅ 禁中断                  |
| xy_timer_kill     |  ❌                        |  ✅ 禁中断                  |
| xy_timer_change_* |  ❌                        |  ✅ 禁中断                  |
| xy_timer_get_tick |  ✅                       |  ✅                         |
| xy_timer_get_nexttick |  ✅                   |  ✅                         |
| xy_timer_get_func |  ✅                       |  ✅                         |
| xy_timer_set_tick |  ❌                        |  ✅ 禁中断（慎用）          |
+-------------------+----------------------------+----------------------------+

*/

#ifdef __cplusplus
}
#endif

#endif
