#include <stdio.h>
#include "xy_typedef.h"
#include "xy_filter.h"

// 简单的测试函数
void test_filter(void) {
    printf("Testing XY Filter Algorithms...\n");

    // 测试限幅滤波
    printf("\n1. Amplitude Limiting Filter Test:\n");
    xy_u16 last_val = 100;
    xy_u16 result = xy_filter_amplitude_limiting(105, last_val, 10);  // 正常变化
    printf("  Input: %d, Last: %d, Limit: 10 -> Output: %d\n", 105, last_val, result);
    
    result = xy_filter_amplitude_limiting(120, last_val, 10);  // 超过限幅
    printf("  Input: %d, Last: %d, Limit: 10 -> Output: %d\n", 120, last_val, result);

    // 测试中位值滤波
    printf("\n2. Median Filter Test:\n");
    xy_u16 median_buffer[5];
    xy_median_filter_t median_filter;
    xy_filter_median_init(&median_filter, median_buffer, 5);
    
    // 输入包含噪声的数据 [100, 102, 500(噪声), 101, 99]
    printf("  Input sequence with noise: 100, 102, 500, 101, 99\n");
    printf("  Filtered outputs: ");
    printf("%d, ", xy_filter_median(&median_filter, 100));
    printf("%d, ", xy_filter_median(&median_filter, 102));
    printf("%d, ", xy_filter_median(&median_filter, 500));  // 噪声
    printf("%d, ", xy_filter_median(&median_filter, 101));
    printf("%d\n", xy_filter_median(&median_filter, 99));

    // 测试滑动平均滤波
    printf("\n3. Recursive Average Filter Test:\n");
    xy_u16 avg_buffer[4];
    xy_recursive_average_filter_t avg_filter;
    xy_filter_recursive_average_init(&avg_filter, avg_buffer, 4);
    
    printf("  Input sequence: 100, 102, 98, 100\n");
    printf("  Filtered outputs: ");
    printf("%d, ", xy_filter_recursive_average(&avg_filter, 100));
    printf("%d, ", xy_filter_recursive_average(&avg_filter, 102));
    printf("%d, ", xy_filter_recursive_average(&avg_filter, 98));
    printf("%d\n", xy_filter_recursive_average(&avg_filter, 100));

    // 测试一阶滞后滤波
    printf("\n4. First Order Lag Filter Test:\n");
    xy_first_order_lag_filter_t lag_filter;
    xy_filter_first_order_lag_init(&lag_filter, 3);  // 使用1/8的系数
    
    printf("  Input sequence: 100, 102, 98, 100\n");
    printf("  Filtered outputs: ");
    printf("%d, ", xy_filter_first_order_lag(&lag_filter, 100));
    printf("%d, ", xy_filter_first_order_lag(&lag_filter, 102));
    printf("%d, ", xy_filter_first_order_lag(&lag_filter, 98));
    printf("%d\n", xy_filter_first_order_lag(&lag_filter, 100));

    printf("\nFilter test completed.\n");
}

int main() {
    test_filter();
    return 0;
}