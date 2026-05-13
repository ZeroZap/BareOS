#include <stdio.h>
#include <string.h>
#include "xy_typedef.h"
#include "xy_sort.h"

// 打印数组
void print_array(xy_u16 *arr, xy_u16 len) {
    for (xy_u16 i = 0; i < len; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

// 验证数组是否已排序
xy_bool is_sorted(xy_u16 *arr, xy_u16 len) {
    for (xy_u16 i = 1; i < len; i++) {
        if (arr[i] < arr[i-1]) {
            return XY_FALSE;
        }
    }
    return XY_TRUE;
}

int main() {
    printf("Testing XY Sort Algorithms...\n");

    // 测试数据
    xy_u16 original[] = {64, 34, 25, 12, 22, 11, 90, 5};
    xy_u16 len = sizeof(original) / sizeof(original[0]);
    xy_u16 test_arr[len];

    // 测试冒泡排序
    printf("\n1. Bubble Sort Test:\n");
    memcpy(test_arr, original, len * sizeof(xy_u16));
    printf("Original: ");
    print_array(test_arr, len);
    xy_bubble_sort(test_arr, len);
    printf("Sorted:   ");
    print_array(test_arr, len);
    printf("Is sorted: %s\n", is_sorted(test_arr, len) ? "YES" : "NO");

    // 测试选择排序
    printf("\n2. Selection Sort Test:\n");
    memcpy(test_arr, original, len * sizeof(xy_u16));
    printf("Original: ");
    print_array(test_arr, len);
    xy_selection_sort(test_arr, len);
    printf("Sorted:   ");
    print_array(test_arr, len);
    printf("Is sorted: %s\n", is_sorted(test_arr, len) ? "YES" : "NO");

    // 测试插入排序
    printf("\n3. Insertion Sort Test:\n");
    memcpy(test_arr, original, len * sizeof(xy_u16));
    printf("Original: ");
    print_array(test_arr, len);
    xy_insertion_sort(test_arr, len);
    printf("Sorted:   ");
    print_array(test_arr, len);
    printf("Is sorted: %s\n", is_sorted(test_arr, len) ? "YES" : "NO");

    // 测试快速排序
    printf("\n4. Quick Sort Test:\n");
    memcpy(test_arr, original, len * sizeof(xy_u16));
    printf("Original: ");
    print_array(test_arr, len);
    xy_quick_sort(test_arr, len);
    printf("Sorted:   ");
    print_array(test_arr, len);
    printf("Is sorted: %s\n", is_sorted(test_arr, len) ? "YES" : "NO");

    // 测试希尔排序
    printf("\n5. Shell Sort Test:\n");
    memcpy(test_arr, original, len * sizeof(xy_u16));
    printf("Original: ");
    print_array(test_arr, len);
    xy_shell_sort(test_arr, len);
    printf("Sorted:   ");
    print_array(test_arr, len);
    printf("Is sorted: %s\n", is_sorted(test_arr, len) ? "YES" : "NO");

    // 测试堆排序
    printf("\n6. Heap Sort Test:\n");
    memcpy(test_arr, original, len * sizeof(xy_u16));
    printf("Original: ");
    print_array(test_arr, len);
    xy_heap_sort(test_arr, len);
    printf("Sorted:   ");
    print_array(test_arr, len);
    printf("Is sorted: %s\n", is_sorted(test_arr, len) ? "YES" : "NO");

    // 测试二分插入排序
    printf("\n7. Binary Insertion Sort Test:\n");
    memcpy(test_arr, original, len * sizeof(xy_u16));
    printf("Original: ");
    print_array(test_arr, len);
    xy_binary_insertion_sort(test_arr, len);
    printf("Sorted:   ");
    print_array(test_arr, len);
    printf("Is sorted: %s\n", is_sorted(test_arr, len) ? "YES" : "NO");

    // 测试计数排序
    printf("\n8. Counting Sort Test:\n");
    memcpy(test_arr, original, len * sizeof(xy_u16));
    printf("Original: ");
    print_array(test_arr, len);
    xy_counting_sort(test_arr, len, 100);  // 假设最大值不超过100
    printf("Sorted:   ");
    print_array(test_arr, len);
    printf("Is sorted: %s\n", is_sorted(test_arr, len) ? "YES" : "NO");

    // 测试二分查找
    printf("\n9. Binary Search Test:\n");
    xy_u16 search_arr[] = {5, 11, 12, 22, 25, 34, 64, 90};
    len = sizeof(search_arr) / sizeof(search_arr[0]);
    printf("Array: ");
    print_array(search_arr, len);
    
    xy_u16 target = 25;
    xy_u16 index = xy_binary_search(search_arr, len, target);
    printf("Searching for %d: ", target);
    if (index != XY_U16_MAX) {
        printf("Found at index %d\n", index);
    } else {
        printf("Not found\n");
    }

    target = 100;
    index = xy_binary_search(search_arr, len, target);
    printf("Searching for %d: ", target);
    if (index != XY_U16_MAX) {
        printf("Found at index %d\n", index);
    } else {
        printf("Not found\n");
    }

    printf("\nAll sort tests completed.\n");
    return 0;
}