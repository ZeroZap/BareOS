# XY CLIB (XinYi C Library)

**版本**: 2.0  
**日期**: 2026-02-28  
**许可证**: Apache License 2.0

## 概述

XY CLIB 是 XinYi 框架的轻量级 C 标准库实现，专为嵌入式系统设计。它提供了可裁剪、可移植的标准 C 库功能，特别适合资源受限的微控制器环境。

## 特性

### ✅ 核心功能

- **轻量级实现**: 针对嵌入式系统优化，最小化资源占用
- **模块化设计**: 按需选择功能模块，减少代码体积
- **可裁剪配置**: 通过宏定义控制功能启用/禁用
- **跨平台兼容**: 支持多种 MCU 架构和 RTOS
- **安全操作**: 提供防溢出字符串函数和边界检查
- **性能优化**: 针对嵌入式场景优化的算法实现

### 🚀 高级特性

- **软除法优化**: 为无硬件除法指令的 MCU 提供快速除法
- **BCD 转换**: 高效 BCD/十进制/十六进制转换
- **镜像位环形缓冲**: 无额外计数器的高效缓冲区实现
- **RT-Thread 兼容链表**: 熟悉的链表操作宏
- **统一错误码**: 与 XY HAL 兼容的错误处理系统
- **智能代理集成**: 支持 PM/Arch/Dev/Test 自动化管理

---

## 模块列表

### 1. 基础类型 (xy_typedef.h)

```c
#include "xy_typedef.h"

// 统一基础类型定义
typedef uint8_t     xy_u8;      // 8位无符号整数
typedef uint16_t    xy_u16;     // 16位无符号整数
typedef uint32_t    xy_u32;     // 32位无符号整数
typedef uint64_t    xy_u64;     // 64位无符号整数
typedef int8_t      xy_i8;      // 8位有符号整数
typedef int16_t     xy_i16;     // 16位有符号整数
typedef int32_t     xy_i32;     // 32位有符号整数
typedef int64_t     xy_i64;     // 64位有符号整数
typedef size_t      xy_size_t;  // 大小类型
```

### 2. 字符串操作 (xy_string.h)

```c
#include "xy_string.h"

void string_example(void)
{
    char buffer[64];
    const char *src = "Hello World";
    
    // 安全字符串复制
    xy_strlcpy(buffer, src, sizeof(buffer));
    
    // 安全字符串连接
    xy_strlcat(buffer, "!", sizeof(buffer));
    
    // 字符串比较
    if (xy_strcmp(buffer, "Hello World!") == 0) {
        // 字符串相等
    }
    
    // 字符串搜索
    char *pos = xy_strchr(buffer, 'W');
    if (pos) {
        // 找到字符
    }
}
```

### 3. 标准 I/O (xy_stdio.h)

```c
#include "xy_stdio.h"

void io_example(void)
{
    char buffer[64];
    
    // 格式化输出
    xy_sprintf(buffer, "Value: %d, Hex: 0x%04X", 123, 0xABCD);
    
    // 带长度限制的格式化
    xy_snprintf(buffer, sizeof(buffer), "Limited: %.*s", 10, "Very long string");
    
    // 自定义输出回调
    xy_stdio_printf_init(my_uart_output);
    xy_stdio_printf("Hello from custom output\n");
}
```

### 4. 位操作工具 (xy_common.h)

```c
#include "xy_common.h"

void bit_example(void)
{
    uint32_t reg = 0;
    
    // 位操作宏
    xy_set_bit(reg, 3);        // 设置第3位
    xy_clear_bit(reg, 3);      // 清除第3位
    xy_toggle_bit(reg, 3);     // 翻转第3位
    uint8_t bit_val = xy_get_bit(reg, 3); // 获取第3位
    
    // 多位操作
    xy_set_bits(reg, 8, 0xF);  // 设置第8-11位为0xF
    
    // BCD 转换
    uint32_t bcd = xy_dec2bcd(1234);
    uint32_t dec = xy_bcd2dec(bcd);
    
    // 软除法 (适用于无硬件除法的 MCU)
    uint8_t mod = xy_u8_mod10(237);
}
```

### 5. 环形缓冲区 (xy_rb.h)

```c
#include "xy_rb.h"

void ringbuffer_example(void)
{
    // 标准环形缓冲
    xy_rb_t rb;
    uint8_t buffer[256];
    xy_rb_init(&rb, buffer, sizeof(buffer));
    
    // 写入数据
    const char *data = "Hello Ring Buffer";
    xy_rb_put(&rb, (const uint8_t *)data, strlen(data));
    
    // 读取数据
    char read_buf[64];
    size_t read_size = xy_rb_get(&rb, (uint8_t *)read_buf, sizeof(read_buf));
    
    // 超轻量宏版 (适用于中断场景)
    #define RB_SIZE 64
    uint8_t macro_buffer[RB_SIZE];
    xy_rbl_t macro_rb = { macro_buffer, 0, 0, 0, RB_SIZE };
    
    xy_rbl_put(&macro_rb, 'A');
    uint8_t c = xy_rbl_get(&macro_rb);
}
```

### 6. 链表操作 (xy_list.h)

```c
#include "xy_list.h"

void list_example(void)
{
    // 定义链表节点
    typedef struct {
        int value;
        xy_list_node node;
    } list_item_t;
    
    // 初始化链表头
    xy_list_head head;
    xy_list_init_head(&head);
    
    // 添加节点
    list_item_t *item = malloc(sizeof(list_item_t));
    item->value = 42;
    xy_list_add_node_tail(&head, &item->node);
    
    // 遍历链表
    xy_list_node *node;
    xy_list_for_node_safe(&head, node) {
        list_item_t *item = xy_container_of(node, list_item_t, node);
        printf("Value: %d\n", item->value);
    }
}
```

---

## 配置选项

### 编译时配置 (xy_config.h)

```c
// 软除法支持 (节省硬件除法资源)
#define XY_USE_SOFT_DIV 1

// 浮点格式化支持 (增加代码大小)
#define XY_PRINTF_FLOAT_ENABLE 0

// printf 缓冲区大小
#define XY_PRINTF_BUFSIZE 512

// 字符属性表大小 (内存 vs 代码权衡)
#define MINIMIZE_CATTR_TABLE 0
```

### Kconfig 配置

```
menu "CLIB Configuration"

config XY_CLIB_ENABLED
    bool "Enable XY CLIB"
    default y

config XY_CLIB_USE_SOFT_DIV
    bool "Use Software Division"
    default y
    depends on XY_CLIB_ENABLED

config XY_CLIB_PRINTF_FLOAT
    bool "Enable Float in printf"
    default n
    depends on XY_CLIB_ENABLED

config XY_CLIB_PRINTF_BUFSIZE
    int "Printf Buffer Size"
    default 512
    range 128 4096
    depends on XY_CLIB_ENABLED

endmenu
```

---

## 构建系统

### CMakeLists.txt

```cmake
# CLIB 组件 CMakeLists.txt
cmake_minimum_required(VERSION 3.12)
project(xy_clib C)

set(CLIB_SOURCES
    xy_string.c
    xy_stdio.c
    xy_stdlib.c
    xy_common.c
    xy_rb.c
    xy_math.c
)

add_library(xy_clib STATIC ${CLIB_SOURCES})

target_include_directories(xy_clib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(xy_clib PUBLIC
    XY_CLIB_ENABLED
    $<$<BOOL:${XY_USE_SOFT_DIV}>:XY_USE_SOFT_DIV>
    $<$<BOOL:${XY_PRINTF_FLOAT_ENABLE}>:XY_PRINTF_FLOAT_ENABLE>
)
```

### Makefile

```makefile
# CLIB Makefile
CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c99 -O2

# Source files
SRCS = xy_string.c xy_stdio.c xy_stdlib.c xy_common.c xy_rb.c xy_math.c
OBJS = $(SRCS:.c=.o)

# Build options
ifeq ($(SOFT_DIV),1)
    CFLAGS += -DXY_USE_SOFT_DIV
endif

ifeq ($(FLOAT_PRINTF),1)
    CFLAGS += -DXY_PRINTF_FLOAT_ENABLE
endif

libxy_clib.a: $(OBJS)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o libxy_clib.a
```

---

## 使用示例

### 1. 基础使用

```c
#include "xy_clib.h"  // 统一包含所有功能

int main(void)
{
    // 使用字符串功能
    char buffer[64];
    xy_strlcpy(buffer, "Hello", sizeof(buffer));
    xy_strlcat(buffer, " CLIB", sizeof(buffer));
    
    // 使用位操作
    uint32_t reg = 0;
    xy_set_bit(reg, 5);
    
    // 使用格式化输出
    xy_sprintf(buffer, "Reg: 0x%08X\n", reg);
    
    return 0;
}
```

### 2. 高级使用

```c
// 在 RTOS 环境中使用
void thread_function(void *arg)
{
    // 使用环形缓冲进行线程间通信
    xy_rb_t *comm_buffer = (xy_rb_t *)arg;
    
    while (1) {
        char data = xy_rb_getchar(comm_buffer);
        if (data != 0) {
            process_data(data);
        }
        xy_os_delay(10);
    }
}
```

### 3. 配置裁剪

```c
// 资源受限环境
#define XY_USE_SOFT_DIV 1           // 启用软除法
#define XY_PRINTF_FLOAT_ENABLE 0    // 禁用浮点
#define XY_PRINTF_BUFSIZE 256       // 小缓冲区

// 功能丰富环境
#define XY_USE_SOFT_DIV 0           // 硬件除法
#define XY_PRINTF_FLOAT_ENABLE 1    // 启用浮点
#define XY_PRINTF_BUFSIZE 1024      // 大缓冲区
```

---

## 性能特性

### 资源占用

| 功能模块 | Flash (bytes) | RAM (bytes) | 说明 |
|----------|---------------|-------------|------|
| **基础类型** | ~100 | 0 | 类型定义 |
| **字符串操作** | ~2000 | 0 | 安全字符串函数 |
| **标准 I/O** | ~4000 | 1024 | 格式化 I/O |
| **环形缓冲** | ~500 | 0 | 3 种实现 |
| **链表宏** | ~200 | 0 | RT-Thread 兼容 |
| **完整库** | ~8000 | 1024 | 所有功能 |

### 优化特性

- **软除法**: 在无硬件除法 MCU 上提供快速除法运算
- **内存效率**: 避免动态内存分配，使用栈和静态内存
- **代码密度**: 高度优化的嵌入式算法
- **可裁剪性**: 通过编译宏控制功能包含

---

## 智能代理集成

```bash
# 使用智能代理管理 CLIB
./.qwen/smart_agent.sh pm status
./.qwen/smart_agent.sh arch review clib
./.qwen/smart_agent.sh dev docs clib
./.qwen/smart_agent.sh test gen clib
```

---

## 兼容性

### 支持的架构
- ARM Cortex-M0/M3/M4/M7/M33
- RISC-V
- MIPS (部分)
- x86 (测试环境)

### 支持的 RTOS
- FreeRTOS
- RT-Thread
- CMSIS-RTOS2
- Bare-metal (无 RTOS)

### 编译器兼容
- GCC (推荐)
- Clang
- ARM Compiler
- IAR (部分)

---

## 最佳实践

### 1. 内存安全

```c
// 使用安全字符串操作
char buffer[32];
const char *src = "potentially_long_string";

// 安全复制 - 自动截断
xy_strlcpy(buffer, src, sizeof(buffer));

// 安全连接 - 考虑剩余空间
xy_strlcat(buffer, "_suffix", sizeof(buffer));
```

### 2. 错误处理

```c
// 统一错误码处理
xy_error_t ret = xy_some_function();
if (XY_FAILED(ret)) {
    xy_log_e("Function failed: %d\n", ret);
    return ret;
}
```

### 3. 性能优化

```c
// 根据资源情况选择实现
#ifdef XY_USE_SOFT_DIV
    // 无硬件除法时使用软除法
    result = xy_u32_mod10(value);
#else
    // 有硬件除法时直接使用
    result = value % 10;
#endif
```

---

## API 参考

### 统一错误码

| 错误码 | 值 | 含义 |
|--------|----|------|
| XY_OK | 0 | 成功 |
| XY_ERROR | -1 | 通用错误 |
| XY_ERROR_INVALID_PARAM | -2 | 无效参数 |
| XY_ERROR_NOT_SUPPORT | -3 | 不支持 |
| ... | ... | ... |

### 常用宏

| 宏 | 功能 |
|----|------|
| `XY_MIN(a, b)` | 最小值 |
| `XY_MAX(a, b)` | 最大值 |
| `XY_CLAMP(x, min, max)` | 限制范围 |
| `XY_ARRAY_SIZE(arr)` | 数组大小 |
| `XY_CONTAINER_OF(ptr, type, member)` | 包含结构体指针 |

---

## 维护者

- **团队**: XinYi Team
- **邮箱**: zerozap2020@gmail.com
- **主页**: https://github.com/zerozap

## 许可证

Apache License 2.0
