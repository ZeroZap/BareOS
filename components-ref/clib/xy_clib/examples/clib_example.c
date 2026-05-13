/**
 * @file clib_example.c
 * @brief XY CLIB Usage Examples
 * @version 2.0
 * @date 2026-02-28
 */

#include "xy_clib.h"

/* 示例 1: 基础字符串操作 */
void string_example(void)
{
    char buffer[64];
    const char *src = "Hello World";
    
    // 安全字符串复制
    xy_strlcpy(buffer, src, sizeof(buffer));
    
    // 字符串比较
    if (xy_strcmp(buffer, "Hello World") == 0) {
        xy_log_i("String comparison passed\n");
    }
    
    // 字符串搜索
    char *pos = xy_strchr(buffer, 'W');
    if (pos) {
        xy_log_i("Found 'W' at position: %d\n", (int)(pos - buffer));
    }
    
    // 内存操作
    xy_memset(buffer, 0, sizeof(buffer));
    xy_memcpy(buffer, src, xy_strlen(src));
}

/* 示例 2: 数值转换和数学运算 */
void math_example(void)
{
    // 字符串转整数
    int value = xy_atoi("12345");
    xy_log_i("String to int: %d\n", value);
    
    // 整数转字符串
    char str[16];
    xy_itoa(value, str, 10);
    xy_log_i("Int to string: %s\n", str);
    
    // BCD 转换
    uint32_t dec_val = 1234;
    uint32_t bcd_val = xy_dec2bcd(dec_val);
    uint32_t back_val = xy_bcd2dec(bcd_val);
    
    xy_log_i("DEC: %u -> BCD: 0x%X -> DEC: %u\n", dec_val, bcd_val, back_val);
    
    // 软除法 (适用于无硬件除法的 MCU)
    uint8_t mod_val = xy_u8_mod10(237);
    xy_log_i("237 mod 10 = %u\n", mod_val);
    
    // 绝对值和数学运算
    int abs_val = xy_abs(-42);
    xy_log_i("Absolute value: %d\n", abs_val);
}

/* 示例 3: I/O 格式化 */
void io_example(void)
{
    char buffer[64];
    
    // 基本格式化
    xy_sprintf(buffer, "Value: %d, Hex: 0x%04X", 123, 0xABCD);
    xy_log_i("Formatted: %s\n", buffer);
    
    // 带长度限制的格式化
    xy_snprintf(buffer, sizeof(buffer), "Limited: %.*s", 10, "This is a long string");
    xy_log_i("Limited: %s\n", buffer);
    
    // 浮点格式化 (如果启用)
    #ifdef XY_PRINTF_FLOAT_ENABLE
    xy_sprintf(buffer, "Float: %.2f", 3.14159f);
    xy_log_i("Float: %s\n", buffer);
    #endif
}

/* 示例 4: 位操作 */
void bit_example(void)
{
    uint32_t reg = 0;
    
    // 设置位
    xy_set_bit(reg, 3);      // 设置第3位
    xy_set_bits(reg, 8, 0xF); // 设置第8-11位为0xF
    
    xy_log_i("After setting bits: 0x%08X\n", reg);
    
    // 检查位
    if (xy_get_bit(reg, 3)) {
        xy_log_i("Bit 3 is set\n");
    }
    
    // 清除位
    xy_clear_bit(reg, 3);
    xy_log_i("After clearing bit 3: 0x%08X\n", reg);
    
    // 翻转位
    xy_toggle_bit(reg, 4);
    xy_log_i("After toggling bit 4: 0x%08X\n", reg);
}

/* 示例 5: 环形缓冲区 */
void ringbuffer_example(void)
{
    // 使用镜像位环形缓冲 (RT-Thread 兼容)
    uint8_t rb_buffer[64];
    struct rt_ringbuffer rb;
    rt_ringbuffer_init(&rb, rb_buffer, sizeof(rb_buffer));
    
    // 写入数据
    const char *data = "Hello Ring Buffer";
    size_t written = rt_ringbuffer_put(&rb, (const uint8_t *)data, xy_strlen(data));
    xy_log_i("Written %zu bytes to ringbuffer\n", written);
    
    // 读取数据
    char read_buffer[64];
    size_t read = rt_ringbuffer_get(&rb, (uint8_t *)read_buffer, sizeof(read_buffer));
    read_buffer[read] = '\0';
    xy_log_i("Read from ringbuffer: %s\n", read_buffer);
    
    // 使用宏版环形缓冲 (超轻量)
    #define RB_SIZE 32
    uint8_t macro_buffer[RB_SIZE];
    xy_rbl_t macro_rb = { macro_buffer, 0, 0, 0, RB_SIZE };
    
    // 写入单字节
    for (int i = 0; i < 10; i++) {
        if (xy_rbl_put(&macro_rb, 'A' + i) == 0) {
            xy_log_i("Macro ringbuffer put failed\n");
            break;
        }
    }
    
    // 读取单字节
    for (int i = 0; i < 5; i++) {
        uint8_t c = xy_rbl_get(&macro_rb);
        if (c != 0) {
            xy_log_i("Macro ringbuffer read: %c\n", c);
        }
    }
}

/* 示例 6: 链表操作 */
void list_example(void)
{
    // 定义链表节点
    typedef struct {
        int value;
        xy_list_node node;
    } list_item_t;
    
    // 初始化链表头
    xy_list_node *head = NULL;
    
    // 创建节点并添加到链表
    for (int i = 0; i < 5; i++) {
        list_item_t *item = xy_malloc(sizeof(list_item_t));
        if (item) {
            item->value = i;
            xy_list_add_node(head, &item->node);
        }
    }
    
    // 遍历链表
    xy_list_node *node;
    xy_list_for_node(head, node) {
        list_item_t *item = xy_container_of(node, list_item_t, node);
        xy_log_i("List item: %d\n", item->value);
    }
    
    // 清理链表
    xy_list_for_node_safe(head, node, node) {
        list_item_t *item = xy_container_of(node, list_item_t, node);
        xy_list_del_node(head, node);
        xy_free(item);
    }
}

/* 示例 7: 安全操作 */
void safety_example(void)
{
    char buffer[16];
    const char *long_str = "This is a very long string that would overflow";
    
    // 使用安全字符串复制，防止溢出
    xy_strlcpy(buffer, long_str, sizeof(buffer));
    xy_log_i("Safe copy result: '%s' (len=%d)\n", buffer, xy_strlen(buffer));
    
    // 使用安全字符串连接
    xy_strlcpy(buffer, "Hello", sizeof(buffer));
    xy_strlcat(buffer, " World", sizeof(buffer));
    xy_log_i("Safe concat result: '%s'\n", buffer);
    
    // 参数有效性检查
    if (buffer != NULL && xy_strlen(buffer) > 0) {
        xy_log_i("Buffer is valid\n");
    }
}

/* 示例 8: 错误处理 */
void error_handling_example(void)
{
    xy_error_t ret;
    
    // 模拟可能失败的操作
    ret = some_potential_failure();
    if (XY_FAILED(ret)) {
        xy_log_e("Operation failed with error: %d (%s)\n", 
                 ret, xy_error_to_string(ret));
        return;
    }
    
    // 或者使用宏
    XY_RETURN_ON_ERROR(some_potential_failure());
    
    xy_log_i("Operation succeeded\n");
}

/* 示例 9: 性能优化 */
void performance_example(void)
{
    // 使用软除法优化 (适用于无硬件除法的 MCU)
    #ifdef XY_USE_SOFT_DIV
    uint32_t value = 123456789;
    uint32_t mod10 = xy_u32_mod10(value);
    uint32_t div10 = xy_u32_div10(value);
    
    xy_log_i("Value: %u, mod10: %u, div10: %u\n", value, mod10, div10);
    #endif
    
    // 选择合适的数据结构
    if (is_power_of_2(buffer_size)) {
        // 使用宏版环形缓冲 (最快)
        xy_rbl_t fast_rb = { buffer, 0, 0, 0, buffer_size };
        // ...
    } else {
        // 使用标准环形缓冲
        xy_rb_t std_rb;
        xy_rb_init(&std_rb, buffer, buffer_size);
        // ...
    }
}

/* 示例 10: 配置选项使用 */
void config_example(void)
{
    // 根据配置启用不同功能
    #if XY_PRINTF_FLOAT_ENABLE
    xy_log_i("Float support enabled\n");
    #else
    xy_log_i("Float support disabled\n");
    #endif
    
    #if XY_USE_SOFT_DIV
    xy_log_i("Soft division enabled\n");
    #else
    xy_log_i("Hardware division enabled\n");
    #endif
    
    // 使用配置的缓冲区大小
    char temp_buf[XY_PRINTF_BUFSIZE];
    xy_sprintf(temp_buf, "Using configured buffer size: %d\n", XY_PRINTF_BUFSIZE);
    xy_log_i("%s", temp_buf);
}

/* 主示例函数 */
void clib_examples_main(void)
{
    xy_log_i("=== XY CLIB Examples ===\n");
    
    string_example();
    xy_log_i("---\n");
    
    math_example();
    xy_log_i("---\n");
    
    io_example();
    xy_log_i("---\n");
    
    bit_example();
    xy_log_i("---\n");
    
    ringbuffer_example();
    xy_log_i("---\n");
    
    list_example();
    xy_log_i("---\n");
    
    safety_example();
    xy_log_i("---\n");
    
    error_handling_example();
    xy_log_i("---\n");
    
    performance_example();
    xy_log_i("---\n");
    
    config_example();
    xy_log_i("---\n");
    
    xy_log_i("=== Examples Complete ===\n");
}

/* 模拟可能失败的函数 */
xy_error_t some_potential_failure(void)
{
    // 模拟某种操作
    return XY_OK; // 或者返回错误码
}
