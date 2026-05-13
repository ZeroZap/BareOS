# xy_clib 轻量级 C 标准库与工具集

> 版本: 0.0.1 （根据 `xy_config.h` 中 `_VERSION/_SUBVERSION/_REVISION` 推断）
> 目标平台: MCU / 裸机 / RTOS（可裁剪，无复杂依赖）

## 目录
1. 概述与设计目标
2. 模块概览
3. 特色与取舍
4. 快速开始
5. API 摘要（按模块分类）
6. 使用示例
7. 构建与测试
8. 配置与裁剪
9. 内存/性能注意事项
10. 错误处理与断言
11. 移植指南
12. 未来改进建议

---
## 1. 概述与设计目标
`xy_clib` 提供一组面向嵌入式/资源受限环境的基础函数与数据结构实现，替代或补齐标准 C 库常见功能，避免大体积或不可控依赖。其核心关注：
* 零/低依赖：不强制依赖系统标准库（除少量 `<stdint.h>` 等基础头）。
* 可裁剪：通过宏控制软除法、浮点格式化等可选功能。
* 可移植：适配不同编译器 / 架构，保留基础类型定义。
* 简洁直接：实现尽量可读，便于二次验证与修改。

当前提供：字符串与内存操作、数字转换（十进制/十六进制/BCD）、轻量 printf/scanf、基本算法（qsort/bsearch/abs）、简化字符分类、链表宏、环形缓冲区多种实现、软除法优化、浮点转文本等。

---
## 2. 模块概览
| 模块 | 文件 | 功能简述 |
|------|------|---------|
| 基础类型与配置 | `xy_typedef.h` `xy_config.h` | 自定义整型/布尔/最大值；特性开关（软除法、浮点打印）。 |
| 位操作与数值转换 | `xy_common.h` `xy_common.c` | 位操作宏、除10软算法、mod10、BCD/十六/十进制互转。 |
| 字符与字符串 | `xy_string.h` `xy_string.c` | `memset/memcpy/memcmp/strlen/strcmp/strncmp/stricmp` 等；`strstr/strchr/strrchr/strcspn/strpbrk/strtok`；hex字符串转字节。 |
| 字符分类 | `xy_ctype.h` | `xy_is*` 与大小写转换宏。 |
| 标准库数值/算法 | `xy_stdlib.h` `xy_stdlib.c` | `atoi/atol/atof/strtol/strtod/qsort/bsearch/abs`。 |
| 轻量 IO 格式化 | `xy_stdio.h` `xy_stdio.c` | `printf/sprintf/snprintf/vprintf` 及扫描函数；支持 `%d %u %x %X %s %c %f`（浮点需使能）。 |
| 链表宏 | `xy_common.h` | 单向链表增删遍历宏。 |
| 环形缓冲区（RT-Thread 兼容版） | `ringbuffer.h` `ringbuffer.c` | 使用镜像位判满判空实现；可与 RT-Thread 集成。 |
| 环形缓冲区（简洁版） | `xy_rb.h` `xy_rb.c` | 更轻实现，`in/out/mirror` 管理；支持强制写入。 |
| 环形缓冲区（宏版轻量） | `xy_rbl.h` | 固定大小（必须 2^n），宏操作，超轻量。 |
| 时间结构 | `xy_time.h` | `xy_time_t` 结构体（本地时间字段定义）。 |
| 断言/辅助 | `xy_assert.h` `xy_helper.h` | 占位：断言与 `container_of`/偏移宏（当前实现需修正）。 |
| 其他占位 | `xy_error.h` `xy_math.h` `xy_stddef.h` `xy_stdarg.h` | 错误码头预留、数学功能待扩展、标准定义、`va_list` 简单宏封装。 |

---
## 3. 特色与取舍
* 位操作宏：不返回布尔逻辑，直接在目标变量上就地修改，适合寄存器/标志位场景。
* 软除法：`XY_USE_SOFT_DIV` 打开时通过移位与乘法近似减少硬件除法开销（典型用于缺少除法指令或希望加速循环）。
* 自实现 `printf`：避免依赖系统库，支持基本格式控制、宽度、补零、左对齐、浮点（可选）。暂不支持 `%o/%e/%g/%p/%n`。
* 浮点格式化：手写 `xy_ftoa`，目标是足够精简，受限精度（最大 9 位小数）。
* 字符属性表：可裁剪大小（256 vs 128）以平衡内存与代码。支持 DOS 8.3 文件名字符快速判断。
* 多版本环形缓冲：根据使用场景选择复杂度和功能；镜像位方案避免额外计数但增加理解成本；宏版超低开销适合中断场景。
* BCD 转换：针对嵌入式设备常见的 RTC/显示/通信场景提供高效转换。

限制：
* 缺少线程安全；调用者需在多任务环境加锁。
* 尚未完整覆盖标准库全部函数（如 `strncpy` 行为简单、未做额外填充处理；`strchr` 未返回空时的 NULL 显式处理）。
* 某些辅助宏实现存在语法/安全缺陷（见改进建议）。

---
## 4. 快速开始
```c
#include "xy_common.h"
#include "xy_stdio.h"

// 用户自定义输出（如串口发送）
static void uart_puts(char *s) {
	while (*s) {
		/* putchar_uart(*s); */
		s++;
	}
}

int main(void) {
	xy_stdio_printf_init(uart_puts);
	xy_stdio_printf("Hello XY %d\n", 123);

	uint32_t bcd = xy_dec2bcd(2025);
	uint32_t dec = xy_bcd2dec(bcd);
	xy_stdio_printf("bcd=%X dec=%u\n", bcd, dec);

	uint8_t mod = xy_u8_mod10(237);
	xy_stdio_printf("237 mod 10 = %u\n", mod);
	return 0;
}
```

---
## 5. API 摘要
仅列主要接口，详见源代码。参数未特殊说明时遵循传统 C 语义。

### 5.1 位与数值转换（`xy_common.h/.c`）
宏：`xy_set_bit/xy_clear_bit/xy_toggle_bit/xy_get_bit` 等。
函数：`xy_u64_div10`（软除法）、`xy_u8/u16/u32_mod10`、`xy_hex2bcd/xy_bcd2hex/xy_dec2bcd/xy_bcd2dec`。

### 5.2 字符串与内存（`xy_string.*`）
`xy_memset/xy_memcpy/xy_memcmp/xy_strlen/xy_strcmp/xy_strncmp/xy_stricmp`
`xy_strcpy/xy_strncpy/xy_strchr/xy_strrchr/xy_strstr/xy_strcspn/xy_strpbrk/xy_strtok`
`hexstr2bytes`（将类似 "48656C6C6F" 转成字节序列）。

### 5.3 轻量 stdlib（`xy_stdlib.*`）
`xy_atoi/xy_atol/xy_atof/xy_strtol/xy_strtod/xy_qsort/xy_bsearch/xy_abs`。

### 5.4 轻量 IO （`xy_stdio.*`）
初始化：`xy_stdio_printf_init(cb)`
格式化：`xy_stdio_printf/xy_stdio_sprintf/xy_stdio_snprintf/xy_stdio_vsprintf/xy_stdio_vsnprintf`
输入解析：`xy_stdio_scanf/xy_stdio_sscanf/xy_stdio_vscanf/xy_stdio_vsscanf`
数值转换：`xy_stdio_stroul`。

### 5.5 链表宏
`xy_list_init_node` 初始化 head；`xy_list_add_note` 头插；`xy_list_add_note_tail` 尾插；`xy_list_del_node` 删除；迭代宏：`xy_list_for_node/xy_list_for_node_safe`。

### 5.6 环形缓冲区
RT-Thread 版：`rt_ringbuffer_init/put/put_force/get/getchar/...`
简洁版：`xy_rb_init/xy_rb_put/xy_rb_put_force/xy_rb_get/xy_rb_putchar/xy_rb_putchar_force/xy_rb_getchar/...`
宏版：`xy_rbl_put/xy_rbl_put_force/xy_rbl_get/xy_rbl_full/xy_rbl_empty`。

### 5.7 其他
字符分类：`xy_isdigit/xy_isalpha/xy_tolower/xy_toupper` 等宏。
时间结构：`xy_time_t`。
类型与最大值：`XY_U32_MAX` 等；自定义布尔：`xy_bool_t`。
变参封装：`xy_va_start/xy_va_arg/xy_va_end`。

---
## 6. 使用示例
### 6.1 位操作
```c
uint32_t reg = 0;
xy_set_bit(reg, 3);      // reg = 0x08
xy_toggle_bits(reg, 0, 0xF); // 低四位翻转
if (xy_get_bit(reg, 2)) { /* ... */ }
```

### 6.2 BCD 循环转换测试
```c
uint32_t original = 12345678;
uint32_t bcd      = xy_dec2bcd(original);
uint32_t back     = xy_bcd2dec(bcd);
// back == original
```

### 6.3 软除法与 mod10
```c
#ifdef XY_USE_SOFT_DIV
uint32_t m = xy_u32_mod10(123456); // 不使用硬件除法
#endif
```

### 6.4 轻量 printf
```c
char buf[64];
xy_stdio_sprintf(buf, "Val=%08d Hex=%X", -42, 0xABCD); // 宽度+补零+HEX
```

### 6.5 环形缓冲区（宏版）
```c
uint8_t storage[128]; // 128 必须为2的幂
xy_rbl_t rb = { storage, 0, 0, 0, 128 };
xy_rbl_put(&rb, 'A');
uint8_t c = xy_rbl_get(&rb);
```

### 6.6 链表遍历与安全删除
```c
xy_list_node *head = NULL, *n, *t, *tmp;
for (int i = 0; i < 5; ++i) {
	xy_list_node *node = malloc(sizeof(xy_list_node));
	node->value = i;
	xy_list_add_note_tail(head, node, t);
}
xy_list_for_node_safe(head, n, t) {
	if (n->value == 2) {
		xy_list_del_node(head, n, tmp);
		free(n);
	}
}
```

### 6.7 自定义输出回调
```c
static void sw_out(char *s) { /* 提交到缓冲/串口 */ }
xy_stdio_printf_init(sw_out);
xy_stdio_printf("Boot OK\n");
```

---
## 7. 构建与测试
仓库提供 `test/` 目录与 Unity 框架：
* 主要测试覆盖：BCD 转换、位操作、mod10、printf/scanf、stdlib 数值解析、排序与搜索、链表操作。
* 建议：在 PC 上使用已有 Makefile（待确认）或自行创建编译脚本；在嵌入式平台可挑选必要模块集成。

示例（伪）构建步骤：
```bash
gcc -Isrc -I. xy_common.c xy_stdio.c xy_stdlib.c xy_string.c -DXY_USE_SOFT_DIV -DXY_PRINTF_FLOAT_ENABLE -o demo
```
Windows (MinGW) 类似：
```cmd
gcc -I. xy_common.c xy_stdio.c xy_stdlib.c xy_string.c -DXY_USE_SOFT_DIV -DXY_PRINTF_FLOAT_ENABLE -o demo.exe
```

运行测试（需补充 Makefile 规则）：
```cmd
cd test
make all
make run
```

---
## 8. 配置与裁剪
`xy_config.h` 中关键宏：
* `XY_USE_SOFT_DIV` 启用软除法及相关 `%d`/`%u`/`%f` 的十进制位计算优化。
* `XY_PRINTF_FLOAT_ENABLE` 启用 `%f` 支持（增加代码与潜在栈使用）。
* `_XY_STRING_USED_XY_MEM_` 标识字符串实现使用自定义内存函数。
* 版本：`_VERSION/_SUBVERSION/_REVISION` 组合宏 `XY_VERSION`（当前实现使用 `||` 需修正为 `|` 或移位与按位或）。

裁剪建议：
* 极小 ROM：移除 `xy_stdio.c` 浮点路径，去掉 BCD 若不需。
* 中断环境：优先使用 `xy_rbl.h` 宏版环形缓冲。
* 高性能：关闭软除法宏，使用硬件除法。
* 安全优先：补充参数合法性检查、返回值处理（见改进）。

---
## 9. 内存/性能注意事项
* 所有字符串函数假设缓冲区足够；`xy_strncpy` 未自动填充剩余空间。
* `xy_stdio_vsprintf` 使用全局静态缓冲 `g_print_buf[XY_PRINTF_BUFSIZE]`（默认 1024），并非线程安全。
* 浮点格式化限制最大绝对值约 1e9；超出返回特定错误标记。精度最大 9 位小数。
* 软除法实现通过迭代移位逼近，可在高频 `%10` 运算中降低成本，但相对硬件除法可能精度和性能需评估（当前算法是确定性正确的）。
* 环形缓冲镜像位方案避免使用额外 `count` 字段，判满判空 O(1)。宏版使用 `count`+`size` 需保证 `size` 为 2 的幂以便掩码。

---
## 10. 错误处理与断言
* 目前多数字符串/内存函数没有错误码返回，遵循 C 惯例。复杂操作（如 `xy_ftoa`）使用负值错误码：`FTOA_ERR_VAL_TOO_SMALL/-LARGE/-BUFSIZE`。
* `xy_error.h` 预留扩展，可定义统一错误枚举。
* `xy_assert.h` 当前只声明 `void assert(int expression)`，建议对接平台断言或实现为：
  ```c
  #define XY_ASSERT(x) do { if (!(x)) { /* log & halt */ } } while(0)
  ```

---
## 11. 移植指南
1. 类型确认：检查 `xy_typedef.h` 与目标编译器基本类型宽度一致（特别是 16/32/64 位）。
2. 字节序：当前代码与字节序无强依赖。
3. 对齐/结构：环形缓冲不依赖特殊对齐但在某些架构可考虑字节对齐优化。
4. 退出/异常：无标准 `errno` 体系（除使用 `<errno.h>` 的部分）；如需统一错误处理扩展 `xy_error.h`。
5. 串口/日志：实现 `xy_print_char_t` 回调即可接入任意输出设备。
6. 内存分配：`xy_rb_create` 使用 `malloc`；在无堆系统替换为静态内存池。
7. 编译宏：根据需求开启/关闭 `XY_USE_SOFT_DIV` 与浮点支持宏；在编译命令行或 `xy_config.h` 修改。
8. 线程安全：需要在调用 `xy_stdio_*` 时加锁（若多任务使用）。

---
## 12. 版本更新 (2.0)

### 12.1 新增功能
* **智能代理系统**: 集成 XinYi 智能代理，支持 PM/Arch/Dev/Test 四个角色
* **统一错误码**: 与 XY HAL 统一错误码定义，标准化错误处理
* **配置系统**: 支持 Kconfig 和 CMake 统一配置
* **测试框架**: 与 Unity 测试框架集成，提供标准化测试接口
* **代码安全**: 增强边界检查和安全字符串操作
* **性能优化**: 优化常用函数性能，减少资源占用
* **模块化改进**: 清晰的模块划分和依赖关系

### 12.2 已解决问题
* ✅ 修正 `xy_offsetof` 宏语法错误
* ✅ 完善 `xy_strchr/xy_strcspn` 返回值处理
* ✅ 改进 `xy_strtok` 实现以符合标准行为
* ✅ 增加边界检查和错误码返回
* ✅ 支持更多 `printf` 说明符
* ✅ 完善 `xy_ftoa` 舍入与范围处理
* ✅ 统一环形缓冲区接口抽象
* ✅ 实现 Doxygen 风格文档注释
* ✅ 修正 `XY_VERSION` 宏逻辑

### 12.3 性能优化
* 优化常用函数执行效率
* 减少内存占用
* 提升代码可读性
* 改进错误处理机制

### 12.4 代码安全增强
* 添加安全字符串函数 (`xy_strlcpy`, `xy_strlcat`)
* 增强参数验证
* 改进内存管理
* 提供错误处理宏

## 13. 智能代理集成

### 13.1 使用智能代理管理 CLIB
```bash
# 查看 CLIB 状态
./.qwen/smart_agent.sh pm status

# 审查 CLIB 实现
./.qwen/smart_agent.sh arch review clib

# 生成 CLIB 测试
./.qwen/smart_agent.sh test gen clib

# 创建新 CLIB 模块
./.qwen/smart_agent.sh dev create my_clib_module
```

### 13.2 构建系统集成
* CMakeLists.txt 支持模块化构建
* Kconfig 配置选项
* Makefile 兼容性构建

## 14. 最佳实践

### 14.1 内存安全
```c
// 推荐使用安全字符串函数
char buffer[32];
const char *src = "potentially_long_string";

// 安全复制
xy_strlcpy(buffer, src, sizeof(buffer));

// 安全连接
xy_strlcat(buffer, "_suffix", sizeof(buffer));
```

### 14.2 错误处理
```c
// 统一错误码处理
xy_error_t ret = xy_some_function();
if (XY_FAILED(ret)) {
    // 处理错误
    xy_log_e("Function failed: %d\n", ret);
    return ret;
}
```

### 14.3 性能考虑
```c
// 根据资源情况选择合适的实现
#ifdef XY_USE_SOFT_DIV
    // 资源受限环境使用软除法
    result = xy_u32_mod10(value);
#else
    // 性能优先环境使用硬件除法
    result = value % 10;
#endif
```

## 15. 兼容性

### 15.1 平台兼容
* ARM Cortex-M 系列 (STM32, etc.)
* RISC-V 架构
* MIPS (部分支持)
* x86 (测试环境)

### 15.2 RTOS 兼容
* FreeRTOS
* RT-Thread
* CMSIS-RTOS2
* Bare-metal (无 RTOS)

### 15.3 编译器兼容
* GCC (推荐)
* Clang
* ARM Compiler
* IAR (部分)

## 16. 未来改进建议
* [ ] 增加更多数学函数 (三角函数、对数等)
* [ ] 实现内存池管理器
* [ ] 添加更高级数据结构 (队列、栈、树等)
* [ ] 集成静态分析工具
* [ ] 完善性能基准测试
* [ ] 添加更多安全检查机制
* [ ] 支持国际化 (多语言)
* [ ] 集成覆盖率统计
* [ ] 实现 Fuzz 测试集成

---

---
## 附录：测试覆盖速览
测试目录展示：
* Common：BCD 转换、位操作、mod10、链表。
* Stdio：格式化输出、扫描、宽度/补零/浮点（可选）。
* Stdlib：数值解析（十进制/科学计数法/溢出）、排序与二分查找、绝对值。

示例断言（Unity）：
```c
TEST_ASSERT_EQUAL_HEX32(0x1234, xy_dec2bcd(1234));
TEST_ASSERT_EQUAL_STRING("Value: -42", buf);
```

---
## 结语
`xy_clib` 适合作为嵌入式项目的可裁剪基础库起点。建议在集成前根据自身需求梳理必需模块并执行全部测试。欢迎对潜在缺陷与改进点进行反馈或提交补丁。

