#ifndef _XY_SORT_H_
#define _XY_SORT_H_

#include "xy_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 冒泡排序 (Bubble Sort)
 * @param arr 待排序数组
 * @param len 数组长度
 * @note 时间复杂度: O(n²)，空间复杂度: O(1)
 */
void xy_bubble_sort(xy_u16 *arr, xy_u16 len);

/**
 * @brief 选择排序 (Selection Sort)
 * @param arr 待排序数组
 * @param len 数组长度
 * @note 时间复杂度: O(n²)，空间复杂度: O(1)
 */
void xy_selection_sort(xy_u16 *arr, xy_u16 len);

/**
 * @brief 插入排序 (Insertion Sort)
 * @param arr 待排序数组
 * @param len 数组长度
 * @note 时间复杂度: O(n²)，空间复杂度: O(1)
 */
void xy_insertion_sort(xy_u16 *arr, xy_u16 len);

/**
 * @brief 快速排序 (Quick Sort)
 * @param arr 待排序数组
 * @param len 数组长度
 * @note 平均时间复杂度: O(n log n)，空间复杂度: O(log n)
 */
void xy_quick_sort(xy_u16 *arr, xy_u16 len);

/**
 * @brief 归并排序 (Merge Sort)
 * @param arr 待排序数组
 * @param len 数组长度
 * @note 时间复杂度: O(n log n)，空间复杂度: O(n)
 */
void xy_merge_sort(xy_u16 *arr, xy_u16 len);

/**
 * @brief 希尔排序 (Shell Sort)
 * @param arr 待排序数组
 * @param len 数组长度
 * @note 时间复杂度: O(n^1.3) ~ O(n²)，空间复杂度: O(1)
 */
void xy_shell_sort(xy_u16 *arr, xy_u16 len);

/**
 * @brief 计数排序 (Counting Sort)
 * @param arr 待排序数组
 * @param len 数组长度
 * @param max_value 数组中的最大值
 * @note 时间复杂度: O(n+k)，空间复杂度: O(k)，适用于整数且范围较小的情况
 */
void xy_counting_sort(xy_u16 *arr, xy_u16 len, xy_u16 max_value);

/**
 * @brief 堆排序 (Heap Sort)
 * @param arr 待排序数组
 * @param len 数组长度
 * @note 时间复杂度: O(n log n)，空间复杂度: O(1)
 */
void xy_heap_sort(xy_u16 *arr, xy_u16 len);

/**
 * @brief 二分查找 (Binary Search)
 * @param arr 已排序数组
 * @param len 数组长度
 * @param target 目标值
 * @return 目标值在数组中的索引，如果未找到则返回 XY_U16_MAX
 * @note 时间复杂度: O(log n)
 */
xy_u16 xy_binary_search(xy_u16 *arr, xy_u16 len, xy_u16 target);

/**
 * @brief 二分插入排序 (Binary Insertion Sort)
 * @param arr 待排序数组
 * @param len 数组长度
 * @note 时间复杂度: O(n²)，但减少了比较次数，空间复杂度: O(1)
 */
void xy_binary_insertion_sort(xy_u16 *arr, xy_u16 len);

/**
 * @brief 通用排序函数，使用快速排序
 * @param arr 待排序数组
 * @param len 数组长度
 * @param compare 比较函数指针，返回值小于0表示a<b，等于0表示a==b，大于0表示a>b
 * @note 可用于不同数据类型的排序
 */
void xy_sort(void *arr, xy_u16 len, xy_u16 size, int (*compare)(const void *a, const void *b));

/**
 * @brief 通用查找函数，使用线性查找
 * @param arr 数组
 * @param len 数组长度
 * @param target 目标值
 * @param size 元素大小
 * @param compare 比较函数指针
 * @return 目标值在数组中的索引，如果未找到则返回 XY_U16_MAX
 */
xy_u16 xy_search(void *arr, xy_u16 len, xy_u16 size, const void *target, 
                int (*compare)(const void *a, const void *b));

#ifdef __cplusplus
}
#endif

#endif /* _XY_SORT_H_ */