#include "xy_typedef.h"
#include "xy_filter.h"
#include <string.h>

/**
 * @brief 限幅滤波法 (程序判断滤波法)
 * @param value 当前采样值
 * @param last_value 上次采样值
 * @param limit 限幅值
 * @return 滤波后的值
 */
xy_u16 xy_filter_amplitude_limiting(xy_u16 value, xy_u16 last_value, xy_u16 limit)
{
    if ((value > last_value + limit) || (value < last_value - limit)) {
        return last_value;
    }
    return value;
}

/**
 * @brief 中位值滤波法
 * @param filter 滤波器上下文
 * @param value 当前采样值
 * @return 滤波后的值
 */
xy_u16 xy_filter_median(xy_median_filter_t *filter, xy_u16 value)
{
    if (filter == XY_NULL) {
        return value;
    }

    // 将新值加入缓冲区
    filter->buffer[filter->index] = value;
    filter->index++;
    
    if (filter->index >= filter->size) {
        filter->index = 0;
    }

    // 如果缓冲区未满，返回当前值
    if (filter->count < filter->size) {
        filter->count++;
        return value;
    }

    // 复制缓冲区以进行排序
    xy_u16 temp_buffer[FILTER_MEDIAN_SIZE];
    memcpy(temp_buffer, filter->buffer, filter->size * sizeof(xy_u16));

    // 简单冒泡排序
    for (xy_u8 i = 0; i < filter->size - 1; i++) {
        for (xy_u8 j = 0; j < filter->size - 1 - i; j++) {
            if (temp_buffer[j] > temp_buffer[j + 1]) {
                xy_u16 temp = temp_buffer[j];
                temp_buffer[j] = temp_buffer[j + 1];
                temp_buffer[j + 1] = temp;
            }
        }
    }

    // 返回中位值
    return temp_buffer[filter->size / 2];
}

/**
 * @brief 算术平均滤波法
 * @param filter 滤波器上下文
 * @param value 当前采样值
 * @return 滤波后的值
 */
xy_u16 xy_filter_arithmetic_average(xy_arithmetic_average_filter_t *filter, xy_u16 value)
{
    if (filter == XY_NULL) {
        return value;
    }

    filter->sum -= filter->buffer[filter->index];
    filter->buffer[filter->index] = value;
    filter->sum += value;
    filter->index++;
    
    if (filter->index >= filter->size) {
        filter->index = 0;
    }

    if (filter->count < filter->size) {
        filter->count++;
        return (xy_u16)(filter->sum / filter->count);
    }

    return (xy_u16)(filter->sum / filter->size);
}

/**
 * @brief 递推平均滤波法 (滑动平均滤波法)
 * @param filter 滤波器上下文
 * @param value 当前采样值
 * @return 滤波后的值
 */
xy_u16 xy_filter_recursive_average(xy_recursive_average_filter_t *filter, xy_u16 value)
{
    if (filter == XY_NULL) {
        return value;
    }

    filter->sum -= filter->buffer[filter->index];
    filter->buffer[filter->index] = value;
    filter->sum += value;
    filter->index++;
    
    if (filter->index >= filter->size) {
        filter->index = 0;
    }

    if (filter->count < filter->size) {
        filter->count++;
        return (xy_u16)(filter->sum / filter->count);
    }

    return (xy_u16)(filter->sum / filter->size);
}

/**
 * @brief 中位值平均滤波法 (先用中位值滤波，再算平均值)
 * @param filter 滤波器上下文
 * @param value 当前采样值
 * @return 滤波后的值
 */
xy_u16 xy_filter_median_average(xy_median_average_filter_t *filter, xy_u16 value)
{
    if (filter == XY_NULL) {
        return value;
    }

    // 先进行中位值滤波
    xy_u16 median_value = xy_filter_median(&filter->median_filter, value);

    // 再进行平均值滤波
    return xy_filter_arithmetic_average(&filter->arithmetic_filter, median_value);
}

/**
 * @brief 一阶滞后滤波法
 * @param filter 滤波器上下文
 * @param value 当前采样值
 * @return 滤波后的值
 */
xy_u16 xy_filter_first_order_lag(xy_first_order_lag_filter_t *filter, xy_u16 value)
{
    if (filter == XY_NULL) {
        return value;
    }

    // 使用定点运算避免浮点运算
    // output = (alpha * currentValue + (1 - alpha) * lastValue)
    // 为简化计算，使用系数为 factor/256
    filter->accumulator = filter->accumulator - (filter->accumulator >> filter->shift_factor) + 
                         (value << (8 - filter->shift_factor));
    return (xy_u16)(filter->accumulator >> 8);
}

/**
 * @brief 加权递推平均滤波法
 * @param filter 滤波器上下文
 * @param value 当前采样值
 * @return 滤波后的值
 */
xy_u16 xy_filter_weighted_recursive_average(xy_weighted_recursive_average_filter_t *filter, xy_u16 value)
{
    if (filter == XY_NULL) {
        return value;
    }

    // 移动缓冲区数据，新值放在最后
    for (xy_u8 i = 0; i < filter->size - 1; i++) {
        filter->buffer[i] = filter->buffer[i + 1];
    }
    filter->buffer[filter->size - 1] = value;

    if (filter->count < filter->size) {
        filter->count++;
    }

    // 计算加权平均
    xy_u32 weighted_sum = 0;
    xy_u8 count = (filter->count < filter->size) ? filter->count : filter->size;
    
    for (xy_u8 i = 0; i < count; i++) {
        weighted_sum += filter->buffer[i] * filter->weights[i];
    }
    
    xy_u16 total_weight = 0;
    for (xy_u8 i = 0; i < count; i++) {
        total_weight += filter->weights[i];
    }

    if (total_weight == 0) {
        return value;
    }
    
    return (xy_u16)(weighted_sum / total_weight);
}

/**
 * @brief 消抖滤波法
 * @param filter 滤波器上下文
 * @param value 当前采样值
 * @return 滤波后的值
 */
xy_u16 xy_filter_debounce(xy_debounce_filter_t *filter, xy_u16 value)
{
    if (filter == XY_NULL) {
        return value;
    }

    if (filter->last_value != value) {
        filter->counter = 0;
        filter->last_value = value;
        return filter->output_value;
    }

    if (filter->counter < filter->threshold) {
        filter->counter++;
        return filter->output_value;
    }

    filter->output_value = value;
    return value;
}

/**
 * @brief 限幅消抖滤波法 (结合限幅和消抖)
 * @param filter 滤波器上下文
 * @param value 当前采样值
 * @return 滤波后的值
 */
xy_u16 xy_filter_amplitude_limiting_debounce(xy_amplitude_limiting_debounce_filter_t *filter, 
                                            xy_u16 current_value)
{
    if (filter == XY_NULL) {
        return current_value;
    }

    // 首先进行限幅处理
    xy_u16 limited_value = xy_filter_amplitude_limiting(current_value, filter->last_value, 
                                                        filter->limit_value);

    // 然后进行消抖处理
    if (filter->last_filtered_value != limited_value) {
        filter->debounce_counter = 0;
        filter->last_filtered_value = limited_value;
        return filter->output_value;
    }

    if (filter->debounce_counter < filter->debounce_threshold) {
        filter->debounce_counter++;
        return filter->output_value;
    }

    filter->output_value = limited_value;
    return limited_value;
}

/**
 * @brief 初始化中位值滤波器
 * @param filter 滤波器上下文
 * @param buffer 缓冲区
 * @param size 缓冲区大小 (必须为奇数以获得明确的中位值)
 */
void xy_filter_median_init(xy_median_filter_t *filter, xy_u16 *buffer, xy_u8 size)
{
    if (filter == XY_NULL || buffer == XY_NULL || size == 0) {
        return;
    }

    filter->buffer = buffer;
    filter->size = size;
    filter->index = 0;
    filter->count = 0;
    
    for (xy_u8 i = 0; i < size; i++) {
        filter->buffer[i] = 0;
    }
}

/**
 * @brief 初始化算术平均滤波器
 * @param filter 滤波器上下文
 * @param buffer 缓冲区
 * @param size 缓冲区大小
 */
void xy_filter_arithmetic_average_init(xy_arithmetic_average_filter_t *filter, 
                                      xy_u16 *buffer, xy_u8 size)
{
    if (filter == XY_NULL || buffer == XY_NULL || size == 0) {
        return;
    }

    filter->buffer = buffer;
    filter->size = size;
    filter->index = 0;
    filter->count = 0;
    filter->sum = 0;
    
    for (xy_u8 i = 0; i < size; i++) {
        filter->buffer[i] = 0;
    }
}

/**
 * @brief 初始化递推平均滤波器
 * @param filter 滤波器上下文
 * @param buffer 缓冲区
 * @param size 缓冲区大小
 */
void xy_filter_recursive_average_init(xy_recursive_average_filter_t *filter, 
                                     xy_u16 *buffer, xy_u8 size)
{
    if (filter == XY_NULL || buffer == XY_NULL || size == 0) {
        return;
    }

    filter->buffer = buffer;
    filter->size = size;
    filter->index = 0;
    filter->count = 0;
    filter->sum = 0;
    
    for (xy_u8 i = 0; i < size; i++) {
        filter->buffer[i] = 0;
    }
}

/**
 * @brief 初始化一阶滞后滤波器
 * @param filter 滤波器上下文
 * @param shift_factor 转换因子 (1-7, 对应滤波系数 1/2 - 1/128)
 */
void xy_filter_first_order_lag_init(xy_first_order_lag_filter_t *filter, xy_u8 shift_factor)
{
    if (filter == XY_NULL) {
        return;
    }

    filter->shift_factor = (shift_factor > 7) ? 7 : ((shift_factor < 1) ? 1 : shift_factor);
    filter->accumulator = 0;
}

/**
 * @brief 初始化加权递推平均滤波器
 * @param filter 滤波器上下文
 * @param buffer 数据缓冲区
 * @param weights 权重缓冲区
 * @param size 缓冲区大小
 */
void xy_filter_weighted_recursive_average_init(xy_weighted_recursive_average_filter_t *filter,
                                              xy_u16 *buffer, xy_u8 *weights, xy_u8 size)
{
    if (filter == XY_NULL || buffer == XY_NULL || weights == XY_NULL || size == 0) {
        return;
    }

    filter->buffer = buffer;
    filter->weights = weights;
    filter->size = size;
    filter->count = 0;
    
    for (xy_u8 i = 0; i < size; i++) {
        filter->buffer[i] = 0;
        filter->weights[i] = 1;  // 默认权重为1
    }
}

/**
 * @brief 初始化消抖滤波器
 * @param filter 滤波器上下文
 * @param threshold 消抖阈值
 */
void xy_filter_debounce_init(xy_debounce_filter_t *filter, xy_u8 threshold)
{
    if (filter == XY_NULL) {
        return;
    }

    filter->threshold = threshold;
    filter->counter = 0;
    filter->last_value = 0;
    filter->output_value = 0;
}

/**
 * @brief 初始化限幅消抖滤波器
 * @param filter 滤波器上下文
 * @param limit_value 限幅值
 * @param debounce_threshold 消抖阈值
 */
void xy_filter_amplitude_limiting_debounce_init(xy_amplitude_limiting_debounce_filter_t *filter,
                                               xy_u16 limit_value, xy_u8 debounce_threshold)
{
    if (filter == XY_NULL) {
        return;
    }

    filter->limit_value = limit_value;
    filter->debounce_threshold = debounce_threshold;
    filter->debounce_counter = 0;
    filter->last_value = 0;
    filter->last_filtered_value = 0;
    filter->output_value = 0;
}