# XinYi CLIB (C Library) 组件详细说明

## 概述

xy_clib 是 XinYi 框架的核心 C 标准库实现，专为嵌入式系统设计。它提供了轻量级、可裁剪的标准 C 库功能，包括字符串操作、内存管理、数学计算、数据结构等，特别适合资源受限的微控制器环境。

### 特性

- ✅ **轻量级实现**: 适用于嵌入式系统
- ✅ **模块化设计**: 按需选择功能模块
- ✅ **可裁剪配置**: 通过宏定义控制功能
- ✅ **统一接口**: 跨平台兼容
- ✅ **内存安全**: 边界检查和错误处理
- ✅ **性能优化**: 针对嵌入式优化

---

## 模块组成

### 1. 核心类型定义 (xy_typedef.h)

```c
/* 基本类型定义 */
typedef uint8_t     xy_u8;      // 8位无符号整数
typedef uint16_t    xy_u16;     // 16位无符号整数
typedef uint32_t    xy_u32;     // 32位无符号整数
typedef uint64_t    xy_u64;     // 64位无符号整数
typedef int8_t      xy_i8;      // 8位有符号整数
typedef int16_t     xy_i16;     // 16位有符号整数
typedef int32_t     xy_i32;     // 32位有符号整数
typedef int64_t     xy_i64;     // 64位有符号整数
typedef size_t      xy_size_t;  // 大小类型
typedef ssize_t     xy_ssize_t; // 有符号大小类型
```

### 2. 字符串操作 (xy_string.h/c)

**功能**:
- 内存操作: `xy_memset`, `xy_memcpy`, `xy_memcmp`
- 字符串操作: `xy_strlen`, `xy_strcmp`, `xy_strcpy`, `xy_strcat`
- 字符串搜索: `xy_strchr`, `xy_strrchr`, `xy_strstr`
- 字符串分割: `xy_strtok`, `xy_strspn`, `xy_strcspn`

**示例**:
```c
#include "xy_string.h"

void string_example(void)
{
    char src[] = "Hello World";
    char dst[32];
    
    // 复制字符串
    xy_strcpy(dst, src);
    
    // 比较字符串
    if (xy_strcmp(src, dst) == 0) {
        // 字符串相等
    }
    
    // 查找字符
    char *pos = xy_strchr(src, 'W');
    if (pos) {
        // 找到了 'W'
    }
}
```

### 3. 标准 I/O (xy_stdio.h/c)

**功能**:
- 格式化输出: `xy_printf`, `xy_sprintf`, `xy_snprintf`
- 格式化输入: `xy_scanf`, `xy_sscanf` (基础支持)
- 浮点支持: 可选编译

**配置选项**:
```c
#define XY_PRINTF_FLOAT_ENABLE  1   // 启用浮点支持
#define XY_PRINTF_BUFSIZE      1024 // 缓冲区大小
```

**示例**:
```c
#include "xy_stdio.h"

void io_example(void)
{
    char buffer[64];
    
    // 格式化到缓冲区
    xy_sprintf(buffer, "Value: %d, Hex: 0x%X", 123, 0xABCD);
    
    // 格式化到固定长度缓冲区
    xy_snprintf(buffer, sizeof(buffer), "Limited: %.*s", 10, "This is a long string");
}
```

### 4. 标准库函数 (xy_stdlib.h/c)

**功能**:
- 数值转换: `xy_atoi`, `xy_atof`, `xy_strtol`
- 数值格式化: `xy_itoa`, `xy_ltoa`
- 排序搜索: `xy_qsort`, `xy_bsearch`
- 数学函数: `xy_abs`, `xy_div`

**示例**:
```c
#include "xy_stdlib.h"

void stdlib_example(void)
{
    // 字符串转整数
    int value = xy_atoi("1234");
    
    // 整数转字符串
    char str[16];
    xy_itoa(1234, str, 10);
    
    // 排序
    int arr[] = {5, 2, 8, 1, 9};
    xy_qsort(arr, 5, sizeof(int), compare_ints);
}
```

### 5. 字符分类 (xy_ctype.h)

**功能**:
- 字符类型检查: `xy_isdigit`, `xy_isalpha`, `xy_isspace`
- 字符转换: `xy_tolower`, `xy_toupper`

### 6. 位操作工具 (xy_common.h/c)

**功能**:
- 位操作宏: `xy_set_bit`, `xy_clear_bit`, `xy_toggle_bit`
- 数值转换: `xy_dec2bcd`, `xy_bcd2dec`, `xy_hex2bcd`
- 软除法: `xy_u32_div10`, `xy_u32_mod10`

**示例**:
```c
#include "xy_common.h"

void bit_operation_example(void)
{
    uint32_t value = 0;
    
    // 设置位
    xy_set_bit(value, 3);     // 设置第3位
    xy_set_bits(value, 8, 0xF); // 设置第8-11位为0xF
    
    // 获取位
    uint8_t bit = xy_get_bit(value, 3);
    
    // BCD转换
    uint32_t bcd = xy_dec2bcd(1234);
    uint32_t dec = xy_bcd2dec(bcd);
    
    // 软除法 (节省硬件除法资源)
    uint8_t mod = xy_u8_mod10(127);
}
```

### 7. 链表工具 (xy_common.h)

**功能**:
- 双向链表宏操作
- 安全的链表遍历和修改

**示例**:
```c
// 定义链表节点
typedef struct {
    int data;
    xy_list_node node;
} my_node_t;

void list_example(void)
{
    xy_list_head head;
    xy_list_init_head(&head);
    
    // 添加节点
    my_node_t *n1 = malloc(sizeof(my_node_t));
    n1->data = 1;
    xy_list_add_node(&head, &n1->node);
    
    // 遍历链表
    xy_list_node *pos;
    xy_list_for_each(pos, &head) {
        my_node_t *item = xy_container_of(pos, my_node_t, node);
        printf("Data: %d\n", item->data);
    }
}
```

### 8. 环形缓冲区 (xy_rb.h/c, ringbuffer.h/c)

**功能**:
- 三种实现: RT-Thread兼容版、简洁版、宏版
- 支持阻塞/非阻塞操作
- 镜像位算法避免额外计数器

**示例**:
```c
#include "xy_rb.h"

void ringbuffer_example(void)
{
    xy_rb_t rb;
    uint8_t buffer[256];
    
    // 初始化环形缓冲
    xy_rb_init(&rb, buffer, sizeof(buffer));
    
    // 写入数据
    const char *data = "Hello Ring Buffer";
    xy_rb_write(&rb, data, strlen(data));
    
    // 读取数据
    char read_buf[64];
    size_t read_size = xy_rb_read(&rb, read_buf, sizeof(read_buf));
}
```

---

## 配置选项

### xy_config.h 配置

```c
/* 软除法支持 */
#define XY_USE_SOFT_DIV        0    // 0=硬件除法, 1=软件除法

/* 浮点打印支持 */
#define XY_PRINTF_FLOAT_ENABLE 1    // 0=禁用, 1=启用

/* printf 缓冲区大小 */
#define XY_PRINTF_BUFSIZE      1024 // 字节

/* 字符属性表大小 */
#define MINIMIZE_CATTR_TABLE   0    // 0=256字节, 1=128字节
```

### Kconfig 配置

```
menu "CLIB Configuration"

config XY_CLIB_ENABLED
    bool "Enable XY CLIB"
    default y

config XY_CLIB_USE_SOFT_DIV
    bool "Use Software Division"
    default n
    depends on XY_CLIB_ENABLED

config XY_CLIB_PRINTF_FLOAT
    bool "Enable Float in printf"
    default y
    depends on XY_CLIB_ENABLED

config XY_CLIB_PRINTF_BUFSIZE
    int "Printf Buffer Size"
    default 1024
    range 256 8192
    depends on XY_CLIB_ENABLED

endmenu
```

---

## 性能特性

### 资源占用 (估算)

| 功能 | Flash (bytes) | RAM (bytes) | 说明 |
|------|---------------|-------------|------|
| **基本功能** | ~2KB | ~0 | 类型定义 + 基本宏 |
| **字符串操作** | ~3KB | ~0 | 所有字符串函数 |
| **标准 I/O** | ~8KB | 1KB | 启用浮点支持 |
| **标准库** | ~4KB | ~0 | 数值转换 + 排序 |
| **环形缓冲** | ~1KB | ~0 | 所有版本 |
| **完整库** | ~15KB | ~1KB | 所有功能启用 |

### 优化特性

1. **软除法优化**: 在没有硬件除法的 MCU 上使用位移和乘法近似
2. **内存效率**: 避免动态内存分配，使用栈和静态内存
3. **代码密度**: 高度优化的嵌入式算法
4. **可裁剪性**: 通过编译宏控制功能包含

---

## 使用指南

### 1. 基本使用

```c
#include "xy_clib.h"  // 统一包含所有功能

int main(void)
{
    // 使用 CLIB 功能
    char buffer[64];
    xy_sprintf(buffer, "Hello from XinYi CLIB v%d", XY_VERSION);
    
    return 0;
}
```

### 2. 按需包含

```c
// 只包含需要的功能
#include "xy_string.h"  // 字符串操作
#include "xy_stdio.h"   // 标准 I/O
#include "xy_common.h"  // 通用工具
```

### 3. 配置裁剪

```c
// 在项目配置中
#define XY_USE_SOFT_DIV 1              // 启用软除法
#define XY_PRINTF_FLOAT_ENABLE 0       // 禁用浮点支持以节省空间
#define XY_PRINTF_BUFSIZE 512          // 减小缓冲区
```

### 4. 错误处理

```c
#include "xy_error.h"

void safe_operation(void)
{
    xy_error_t ret = xy_some_function();
    if (ret != XY_OK) {
        // 处理错误
        xy_log_e("Function failed: %d\n", ret);
    }
}
```

---

## 兼容性

### 支持的架构
- ARM Cortex-M0/M3/M4/M7/M33
- RISC-V
- MIPS (部分)
- x86 (测试环境)

### 编译器兼容性
- GCC (推荐)
- Clang
- ARM Compiler
- IAR (部分)

### RTOS 兼容性
- FreeRTOS
- RT-Thread
- CMSIS-RTOS2
- Bare-metal

---

## 最佳实践

### 1. 内存安全

```c
// 使用安全的字符串操作
char dst[32];
const char *src = "potentially_long_string";
xy_strlcpy(dst, src, sizeof(dst));  // 自动截断
```

### 2. 性能优化

```c
// 选择合适的数据结构
// 小数据量: 直接操作
// 大数据量: 使用指针和索引

// 选择合适的缓冲区大小
#define SMALL_BUF_SIZE  64   // 小缓冲区用于短字符串
#define LARGE_BUF_SIZE  512  // 大缓冲区用于复杂格式化
```

### 3. 配置建议

```c
// 资源受限环境
#define XY_USE_SOFT_DIV 1
#define XY_PRINTF_FLOAT_ENABLE 0
#define XY_PRINTF_BUFSIZE 256

// 功能丰富环境
#define XY_USE_SOFT_DIV 0
#define XY_PRINTF_FLOAT_ENABLE 1
#define XY_PRINTF_BUFSIZE 1024
```

---

## 与其他组件集成

### 与 HAL 集成
```c
#include "xy_clib.h"
#include "xy_hal.h"

void integrated_example(void)
{
    // 使用 CLIB 处理 HAL 返回的数据
    xy_hal_uart_config_t config = { 0 };
    xy_hal_error_t ret = xy_hal_uart_init(&huart1, &config);
    
    if (ret != XY_HAL_OK) {
        char error_msg[64];
        xy_sprintf(error_msg, "UART init failed: %d", ret);
        xy_log_e(error_msg);
    }
}
```

### 与 OSAL 集成
```c
#include "xy_clib.h"
#include "xy_os.h"

void os_integration_example(void)
{
    // 使用 CLIB 处理线程间数据
    char thread_name[16];
    xy_sprintf(thread_name, "thread_%d", get_unique_id());
    
    xy_os_thread_attr_t attr = {
        .name = thread_name,
        .stack_size = 1024,
    };
    
    xy_os_thread_new(worker_func, NULL, &attr);
}
```

---

## 维护和扩展

### 添加新功能

1. 在 `xy_clib/` 目录创建新模块文件
2. 更新 `xy_clib.h` 包含新头文件
3. 添加到构建系统 (CMakeLists.txt/Makefile)
4. 编写单元测试
5. 更新文档

### 测试验证

```bash
# 运行 CLIB 测试
bash .qwen/smart_agent.sh test run clib
```

---

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| **1.0** | 2025-06 | 初始版本，基本功能 |
| **1.5** | 2025-10 | 性能优化，添加环形缓冲 |
| **2.0** | 2026-02 | 智能代理集成，统一接口 |

---

## 许可证

Apache License 2.0

## 联系

- **维护者**: XinYi Team
- **邮箱**: zerozap2020@gmail.com
- **主页**: https://github.com/zerozap
