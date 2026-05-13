#include "xy_sort.h"
#include "xy_typedef.h"
#include <string.h>

/**
 * @brief 交换两个元素
 * @param a 第一个元素指针
 * @param b 第二个元素指针
 */
static void xy_swap(xy_u16 *a, xy_u16 *b)
{
    if (a != b) {
        xy_u16 temp = *a;
        *a = *b;
        *b = temp;
    }
}

/**
 * @brief 冒泡排序 (Bubble Sort)
 * @param arr 待排序数组
 * @param len 数组长度
 * @note 时间复杂度: O(n²)，空间复杂度: O(1)
 */
void xy_bubble_sort(xy_u16 *arr, xy_u16 len)
{
    if (arr == XY_NULL || len <= 1) {
        return;
    }

    for (xy_u16 i = 0; i < len - 1; i++) {
        xy_bool swapped = XY_FALSE;
        for (xy_u16 j = 0; j < len - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                xy_swap(&arr[j], &arr[j + 1]);
                swapped = XY_TRUE;
            }
        }
        // 如果没有发生交换，说明数组已经有序
        if (!swapped) {
            break;
        }
    }
}

/**
 * @brief 选择排序 (Selection Sort)
 * @param arr 待排序数组
 * @param len 数组长度
 * @note 时间复杂度: O(n²)，空间复杂度: O(1)
 */
void xy_selection_sort(xy_u16 *arr, xy_u16 len)
{
    if (arr == XY_NULL || len <= 1) {
        return;
    }

    for (xy_u16 i = 0; i < len - 1; i++) {
        xy_u16 min_index = i;
        for (xy_u16 j = i + 1; j < len; j++) {
            if (arr[j] < arr[min_index]) {
                min_index = j;
            }
        }
        if (min_index != i) {
            xy_swap(&arr[i], &arr[min_index]);
        }
    }
}

/**
 * @brief 插入排序 (Insertion Sort)
 * @param arr 待排序数组
 * @param len 数组长度
 * @note 时间复杂度: O(n²)，空间复杂度: O(1)
 */
void xy_insertion_sort(xy_u16 *arr, xy_u16 len)
{
    if (arr == XY_NULL || len <= 1) {
        return;
    }

    for (xy_u16 i = 1; i < len; i++) {
        xy_u16 key = arr[i];
        xy_i16 j = i - 1;
        
        // 将大于key的元素向后移动
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

/**
 * @brief 快速排序的递归实现
 * @param arr 待排序数组
 * @param low 起始索引
 * @param high 结束索引
 */
static void quick_sort_recursive(xy_u16 *arr, xy_i16 low, xy_i16 high)
{
    if (low < high) {
        xy_u16 pivot = arr[high];  // 选择最后一个元素作为基准
        xy_i16 i = low - 1;        // 较小元素的索引

        for (xy_i16 j = low; j < high; j++) {
            if (arr[j] <= pivot) {
                i++;
                xy_swap(&arr[i], &arr[j]);
            }
        }
        xy_swap(&arr[i + 1], &arr[high]);
        xy_i16 pi = i + 1;

        quick_sort_recursive(arr, low, pi - 1);
        quick_sort_recursive(arr, pi + 1, high);
    }
}

/**
 * @brief 快速排序 (Quick Sort)
 * @param arr 待排序数组
 * @param len 数组长度
 * @note 平均时间复杂度: O(n log n)，空间复杂度: O(log n)
 */
void xy_quick_sort(xy_u16 *arr, xy_u16 len)
{
    if (arr == XY_NULL || len <= 1) {
        return;
    }

    quick_sort_recursive(arr, 0, len - 1);
}

/**
 * @brief 合并两个已排序的子数组
 * @param arr 待排序数组
 * @param temp 临时数组，用于归并排序，大小至少为原数组长度
 * @param left 左边界
 * @param mid 中间位置
 * @param right 右边界
 */
static void merge(xy_u16 *arr, xy_u16 *temp, xy_u16 left, xy_u16 mid, xy_u16 right)
{
    xy_u16 n1 = mid - left + 1;
    xy_u16 n2 = right - mid;

    // 复制数据到临时数组
    for (xy_u16 i = 0; i < n1; i++) {
        temp[i] = arr[left + i];
    }
    for (xy_u16 j = 0; j < n2; j++) {
        temp[n1 + j] = arr[mid + 1 + j];
    }

    // 合并临时数组回原数组
    xy_u16 i = 0, j = n1, k = left;
    while (i < n1 && j < n1 + n2) {
        if (temp[i] <= temp[j]) {
            arr[k] = temp[i];
            i++;
        } else {
            arr[k] = temp[j];
            j++;
        }
        k++;
    }

    // 复制剩余元素
    while (i < n1) {
        arr[k] = temp[i];
        i++;
        k++;
    }
    while (j < n1 + n2) {
        arr[k] = temp[j];
        j++;
        k++;
    }
}

/**
 * @brief 归并排序的递归实现
 * @param arr 待排序数组
 * @param temp 临时数组，用于归并排序
 * @param left 左边界
 * @param right 右边界
 */
static void merge_sort_recursive(xy_u16 *arr, xy_u16 *temp, xy_i16 left, xy_i16 right)
{
    if (left < right) {
        xy_i16 mid = left + (right - left) / 2;
        merge_sort_recursive(arr, temp, left, mid);
        merge_sort_recursive(arr, temp, mid + 1, right);
        merge(arr, temp, left, mid, right);
    }
}

/**
 * @brief 归并排序 (Merge Sort)
 * @param arr 待排序数组
 * @param len 数组长度
 * @note 时间复杂度: O(n log n)，空间复杂度: O(n)，需要额外的临时数组
 */
void xy_merge_sort(xy_u16 *arr, xy_u16 len)
{
    if (arr == XY_NULL || len <= 1) {
        return;
    }

    // 创建临时数组用于归并操作
    // 注意：在资源受限的嵌入式系统中，应使用静态或栈分配的临时数组
    xy_u16 temp[len];
    merge_sort_recursive(arr, temp, 0, len - 1);
}

/**
 * @brief 希尔排序 (Shell Sort)
 * @param arr 待排序数组
 * @param len 数组长度
 * @note 时间复杂度: O(n^1.3) ~ O(n²)，空间复杂度: O(1)
 */
void xy_shell_sort(xy_u16 *arr, xy_u16 len)
{
    if (arr == XY_NULL || len <= 1) {
        return;
    }

    // 使用Knuth序列: h = 3*h + 1
    xy_u16 gap = 1;
    while (gap < len / 3) {
        gap = gap * 3 + 1;
    }

    while (gap >= 1) {
        for (xy_u16 i = gap; i < len; i++) {
            xy_u16 temp = arr[i];
            xy_i16 j = i;
            while (j >= gap && arr[j - gap] > temp) {
                arr[j] = arr[j - gap];
                j -= gap;
            }
            arr[j] = temp;
        }
        gap = gap / 3;
    }
}

/**
 * @brief 计数排序 (Counting Sort)
 * @param arr 待排序数组
 * @param len 数组长度
 * @param max_value 数组中的最大值
 * @note 时间复杂度: O(n+k)，空间复杂度: O(k)，适用于整数且范围较小的情况
 */
void xy_counting_sort(xy_u16 *arr, xy_u16 len, xy_u16 max_value)
{
    if (arr == XY_NULL || len <= 1) {
        return;
    }

    // 检查max_value是否过大，以避免栈溢出
    if (max_value > 1000) {
        return; // 对于嵌入式系统，限制计数排序的最大范围
    }

    // 创建计数数组（使用栈分配）
    xy_u16 count[max_value + 1];
    xy_u16 i;
    
    // 初始化计数数组
    for (i = 0; i <= max_value; i++) {
        count[i] = 0;
    }

    // 统计每个元素的出现次数
    for (i = 0; i < len; i++) {
        count[arr[i]]++;
    }

    // 根据计数数组重构原数组
    xy_u16 index = 0;
    for (i = 0; i <= max_value; i++) {
        while (count[i] > 0) {
            arr[index] = i;
            index++;
            count[i]--;
        }
    }
}

/**
 * @brief 调整堆，使其满足最大堆性质
 * @param arr 待排序数组
 * @param len 数组长度
 * @param i 当前节点索引
 */
static void heapify(xy_u16 *arr, xy_u16 len, xy_u16 i)
{
    xy_u16 largest = i;        // 初始化最大值为根节点
    xy_u16 left = 2 * i + 1;   // 左子节点
    xy_u16 right = 2 * i + 2;  // 右子节点

    // 如果左子节点存在且大于根节点
    if (left < len && arr[left] > arr[largest]) {
        largest = left;
    }

    // 如果右子节点存在且大于当前最大值
    if (right < len && arr[right] > arr[largest]) {
        largest = right;
    }

    // 如果最大值不是根节点
    if (largest != i) {
        xy_swap(&arr[i], &arr[largest]);
        heapify(arr, len, largest);
    }
}

/**
 * @brief 堆排序 (Heap Sort)
 * @param arr 待排序数组
 * @param len 数组长度
 * @note 时间复杂度: O(n log n)，空间复杂度: O(1)
 */
void xy_heap_sort(xy_u16 *arr, xy_u16 len)
{
    if (arr == XY_NULL || len <= 1) {
        return;
    }

    // 构建最大堆
    for (xy_i16 i = len / 2 - 1; i >= 0; i--) {
        heapify(arr, len, i);
    }

    // 逐个提取元素
    for (xy_i16 i = len - 1; i > 0; i--) {
        xy_swap(&arr[0], &arr[i]);
        heapify(arr, i, 0);
    }
}

/**
 * @brief 二分查找 (Binary Search)
 * @param arr 已排序数组
 * @param len 数组长度
 * @param target 目标值
 * @return 目标值在数组中的索引，如果未找到则返回 XY_U16_MAX
 * @note 时间复杂度: O(log n)
 */
xy_u16 xy_binary_search(xy_u16 *arr, xy_u16 len, xy_u16 target)
{
    if (arr == XY_NULL || len == 0) {
        return XY_U16_MAX;
    }

    xy_i16 left = 0;
    xy_i16 right = len - 1;

    while (left <= right) {
        xy_i16 mid = left + (right - left) / 2;

        if (arr[mid] == target) {
            return mid;
        } else if (arr[mid] < target) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return XY_U16_MAX;  // 未找到
}

/**
 * @brief 二分插入排序 (Binary Insertion Sort)
 * @param arr 待排序数组
 * @param len 数组长度
 * @note 时间复杂度: O(n²)，但减少了比较次数，空间复杂度: O(1)
 */
void xy_binary_insertion_sort(xy_u16 *arr, xy_u16 len)
{
    if (arr == XY_NULL || len <= 1) {
        return;
    }

    for (xy_u16 i = 1; i < len; i++) {
        xy_u16 key = arr[i];
        xy_i16 left = 0;
        xy_i16 right = i - 1;

        // 二分查找插入位置
        while (left <= right) {
            xy_i16 mid = left + (right - left) / 2;
            if (arr[mid] > key) {
                right = mid - 1;
            } else {
                left = mid + 1;
            }
        }

        // 移动元素为key腾出位置
        for (xy_i16 j = i - 1; j >= left; j--) {
            arr[j + 1] = arr[j];
        }
        arr[left] = key;
    }
}

// 由于项目中可能没有标准库的malloc/free函数，我们提供一个简化版本的归并排序
// 使用原地排序的快速排序作为主要排序方法
void xy_sort(void *arr, xy_u16 len, xy_u16 size, int (*compare)(const void *a, const void *b))
{
    // 这里简单地使用快速排序的变体
    // 对于xy_u16类型的数组，使用快速排序
    xy_quick_sort((xy_u16*)arr, len);
}

xy_u16 xy_search(void *arr, xy_u16 len, xy_u16 size, const void *target, 
                int (*compare)(const void *a, const void *b))
{
    // 简单的线性查找
    xy_u8 *byte_arr = (xy_u8*)arr;
    for (xy_u16 i = 0; i < len; i++) {
        if (compare(&byte_arr[i * size], target) == 0) {
            return i;
        }
    }
    return XY_U16_MAX;
}