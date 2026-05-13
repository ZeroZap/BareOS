#ifndef _XY_FILTER_H_
#define _XY_FILTER_H_

#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

// 默认的中位值滤波器大小 (必须为奇数)
#define FILTER_MEDIAN_SIZE  11

/**
 * @brief 限幅滤波法 (程序判断滤波法)
 * 最大优点是程序简单，对脉冲性干扰有很好的抑制作用。
 * 缺点是无法抑制周期性干扰和平滑度差。
 * 
 * @param value 当前采样值
 * @param last_value 上次采样值
 * @param limit 限幅值
 * @return 滤波后的值
 */
xy_u16 xy_filter_amplitude_limiting(xy_u16 value, xy_u16 last_value, xy_u16 limit);

/**
 * @brief 中位值滤波法
 * 对脉冲干扰有很好的抑制作用，但对周期性干扰无抑制效果。
 * 适用于变化较慢的参数，如温度、液位等。
 */
typedef struct {
    xy_u16 *buffer;           // 数据缓冲区
    xy_u8 size;              // 缓冲区大小
    xy_u8 index;             // 当前索引
    xy_u8 count;             // 当前数据个数
} xy_median_filter_t;

xy_u16 xy_filter_median(xy_median_filter_t *filter, xy_u16 value);
void xy_filter_median_init(xy_median_filter_t *filter, xy_u16 *buffer, xy_u8 size);

/**
 * @brief 算术平均滤波法
 * 适用于高频震荡的系统，对周期性干扰有良好抑制作用。
 * 不适用于脉冲干扰的系统，对测量速度影响大。
 */
typedef struct {
    xy_u16 *buffer;           // 数据缓冲区
    xy_u8 size;              // 缓冲区大小
    xy_u8 index;             // 当前索引
    xy_u8 count;             // 当前数据个数
    xy_u32 sum;              // 数据总和
} xy_arithmetic_average_filter_t;

xy_u16 xy_filter_arithmetic_average(xy_arithmetic_average_filter_t *filter, xy_u16 value);
void xy_filter_arithmetic_average_init(xy_arithmetic_average_filter_t *filter, xy_u16 *buffer, xy_u8 size);

/**
 * @brief 递推平均滤波法 (滑动平均滤波法)
 * 对周期性干扰有良好抑制作用，平滑度高。
 * 适用于变化较快的系统，但灵敏度低。
 */
typedef struct {
    xy_u16 *buffer;           // 数据缓冲区
    xy_u8 size;              // 缓冲区大小
    xy_u8 index;             // 当前索引
    xy_u8 count;             // 当前数据个数
    xy_u32 sum;              // 数据总和
} xy_recursive_average_filter_t;

xy_u16 xy_filter_recursive_average(xy_recursive_average_filter_t *filter, xy_u16 value);
void xy_filter_recursive_average_init(xy_recursive_average_filter_t *filter, xy_u16 *buffer, xy_u8 size);

/**
 * @brief 中位值平均滤波法
 * 结合了中位值滤波和算术平均滤波的优点，抗干扰能力强。
 * 适用于变化较慢的参数，如温度、液位等。
 */
typedef struct {
    xy_median_filter_t median_filter;
    xy_arithmetic_average_filter_t arithmetic_filter;
} xy_median_average_filter_t;

xy_u16 xy_filter_median_average(xy_median_average_filter_t *filter, xy_u16 value);

/**
 * @brief 一阶滞后滤波法
 * 对周期性和脉冲性干扰都有一定抑制作用。
 * 适用于变化缓慢的系统，平滑度和平滑性都较好。
 */
typedef struct {
    xy_u32 accumulator;       // 累加器 (高8位是整数部分，低8位是小数部分)
    xy_u8 shift_factor;       // 转换因子 (对应滤波系数)
} xy_first_order_lag_filter_t;

xy_u16 xy_filter_first_order_lag(xy_first_order_lag_filter_t *filter, xy_u16 value);
void xy_filter_first_order_lag_init(xy_first_order_lag_filter_t *filter, xy_u8 shift_factor);

/**
 * @brief 加权递推平均滤波法
 * 对周期性和脉冲性干扰都有一定抑制作用。
 * 重点突出新值的影响，提高系统灵敏度。
 */
typedef struct {
    xy_u16 *buffer;           // 数据缓冲区
    xy_u8 *weights;           // 权重缓冲区
    xy_u8 size;              // 缓冲区大小
    xy_u8 count;             // 当前数据个数
} xy_weighted_recursive_average_filter_t;

xy_u16 xy_filter_weighted_recursive_average(xy_weighted_recursive_average_filter_t *filter, xy_u16 value);
void xy_filter_weighted_recursive_average_init(xy_weighted_recursive_average_filter_t *filter,
                                               xy_u16 *buffer, xy_u8 *weights, xy_u8 size);

/**
 * @brief 消抖滤波法
 * 适用于变化缓慢的参数，如按键检测等。
 * 可以有效消除数据抖动现象。
 */
typedef struct {
    xy_u16 last_value;        // 上次值
    xy_u16 output_value;      // 输出值
    xy_u8 threshold;          // 消抖阈值
    xy_u8 counter;            // 计数器
} xy_debounce_filter_t;

xy_u16 xy_filter_debounce(xy_debounce_filter_t *filter, xy_u16 value);
void xy_filter_debounce_init(xy_debounce_filter_t *filter, xy_u8 threshold);

/**
 * @brief 限幅消抖滤波法
 * 结合限幅和消抖两种滤波方法，抗干扰能力强。
 */
typedef struct {
    xy_u16 limit_value;                       // 限幅值
    xy_u16 last_value;                        // 上次原始值
    xy_u16 last_filtered_value;              // 上次限幅后值
    xy_u16 output_value;                      // 输出值
    xy_u8 debounce_threshold;                 // 消抖阈值
    xy_u8 debounce_counter;                   // 消抖计数器
} xy_amplitude_limiting_debounce_filter_t;

xy_u16 xy_filter_amplitude_limiting_debounce(xy_amplitude_limiting_debounce_filter_t *filter, 
                                            xy_u16 current_value);
void xy_filter_amplitude_limiting_debounce_init(xy_amplitude_limiting_debounce_filter_t *filter,
                                               xy_u16 limit_value, xy_u8 debounce_threshold);

#ifdef __cplusplus
}
#endif

#endif /* _XY_FILTER_H_ */