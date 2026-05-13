# XY_CLIB API 参考手册

## 概述

XY_CLIB 是 XinYi 框架的嵌入式 C 标准库实现，提供了标准 C 库的轻量级替代方案。

### 统一头文件

推荐使用统一头文件 `xy_stdlib.h` 来包含所有功能:

```c
#include <stdint.h>      // 标准整型定义(允许)
#include "xy_stdlib.h"   // 包含所有 xy_clib 功能
```

---

## API 对照表

### 1️⃣ 内存操作函数 (string.h)

| XY API | 功能说明 | XY Header | 标准 API | 标准 Header | 备注 |
|--------|---------|-----------|----------|-------------|------|
| `xy_memset` | 填充内存块 | xy_string.h | `memset` | string.h | ✅ 已实现 |
| `xy_memcpy` | 复制内存块 | xy_string.h | `memcpy` | string.h | ✅ 已实现 |
| `xy_memcmp` | 比较内存块 | xy_string.h | `memcmp` | string.h | ✅ 已实现 |
| `xy_memmove` | 安全复制内存(处理重叠)| xy_string.h | `memmove` | string.h | ✅ 已实现 |
| `xy_memchr` | 在内存中查找字符 | xy_string.h | `memchr` | string.h | ✅ 已实现 |
| `xy_memrchr` | 反向查找字符 | xy_string.h | `memrchr` | string.h | ✅ 已实现(GNU扩展)|

### 2️⃣ 字符串操作函数 (string.h)

| XY API | 功能说明 | XY Header | 标准 API | 标准 Header | 备注 |
|--------|---------|-----------|----------|-------------|------|
| `xy_strlen` | 获取字符串长度 | xy_string.h | `strlen` | string.h | ✅ 已实现 |
| `xy_strnlen` | 安全获取字符串长度 | xy_string.h | `strnlen` | string.h | ✅ 已实现 |
| `xy_strcmp` | 比较字符串 | xy_string.h | `strcmp` | string.h | ✅ 已实现 |
| `xy_strncmp` | 限长比较字符串 | xy_string.h | `strncmp` | string.h | ✅ 已实现 |
| `xy_strcasecmp` | 忽略大小写比较 | xy_string.h | `strcasecmp` | string.h | ✅ 已实现 |
| `xy_strncasecmp` | 限长忽略大小写比较 | xy_string.h | `strncasecmp` | string.h | ✅ 已实现 |
| `xy_stricmp` | 忽略大小写比较(别名)| xy_string.h | `stricmp` | string.h | ✅ 已实现(Windows风格)|
| `xy_strcpy` | 复制字符串 | xy_string.h | `strcpy` | string.h | ✅ 已实现 |
| `xy_strncpy` | 限长复制字符串 | xy_string.h | `strncpy` | string.h | ✅ 已实现 |
| `xy_strcat` | 连接字符串 | xy_string.h | `strcat` | string.h | ✅ 已实现 |
| `xy_strncat` | 限长连接字符串 | xy_string.h | `strncat` | string.h | ✅ 已实现 |
| `xy_strchr` | 查找字符首次出现 | xy_string.h | `strchr` | string.h | ✅ 已实现 |
| `xy_strrchr` | 查找字符最后出现 | xy_string.h | `strrchr` | string.h | ✅ 已实现 |
| `xy_strstr` | 查找子串 | xy_string.h | `strstr` | string.h | ✅ 已实现 |
| `xy_strtok` | 分割字符串 | xy_string.h | `strtok` | string.h | ✅ 已实现 |
| `xy_strcspn` | 计算不匹配长度 | xy_string.h | `strcspn` | string.h | ✅ 已实现 |
| `xy_strpbrk` | 查找字符集 | xy_string.h | `strpbrk` | string.h | ✅ 已实现 |
| `xy_strspn` | 计算匹配长度 | xy_string.h | `strspn` | string.h | ✅ 已实现 |
| `xy_strdup` | 复制字符串(动态分配)| xy_string.h | `strdup` | string.h | ✅ 已实现 |
| `xy_strndup` | 限长复制字符串(动态分配)| xy_string.h | `strndup` | string.h | ✅ 已实现 |

### 3️⃣ 格式化输入输出 (stdio.h)

| XY API | 功能说明 | XY Header | 标准 API | 标准 Header | 备注 |
|--------|---------|-----------|----------|-------------|------|
| `xy_stdio_printf` | 格式化输出 | xy_stdio.h | `printf` | stdio.h | ✅ 已实现 |
| `xy_stdio_vprintf` | 可变参数格式化输出 | xy_stdio.h | `vprintf` | stdio.h | ✅ 已实现 |
| `xy_stdio_sprintf` | 格式化输出到字符串 | xy_stdio.h | `sprintf` | stdio.h | ✅ 已实现 |
| `xy_stdio_vsprintf` | 可变参数格式化到字符串 | xy_stdio.h | `vsprintf` | stdio.h | ✅ 已实现 |
| `xy_stdio_snprintf` | 限长格式化输出 | xy_stdio.h | `snprintf` | stdio.h | ✅ 已实现 |
| `xy_stdio_vsnprintf` | 限长可变参数格式化 | xy_stdio.h | `vsnprintf` | stdio.h | ✅ 已实现 |
| `xy_stdio_scanf` | 格式化输入 | xy_stdio.h | `scanf` | stdio.h | ✅ 已实现 |
| `xy_stdio_vscanf` | 可变参数格式化输入 | xy_stdio.h | `vscanf` | stdio.h | ✅ 已实现 |
| `xy_stdio_sscanf` | 从字符串格式化输入 | xy_stdio.h | `sscanf` | stdio.h | ✅ 已实现 |
| `xy_stdio_vsscanf` | 从字符串可变参数输入 | xy_stdio.h | `vsscanf` | stdio.h | ✅ 已实现 |

### 4️⃣ 字符串转数值 (stdlib.h)

| XY API | 功能说明 | XY Header | 标准 API | 标准 Header | 备注 |
|--------|---------|-----------|----------|-------------|------|
| `xy_atoi` | 字符串转整型 | xy_stdlib.h | `atoi` | stdlib.h | ✅ 已实现 |
| `xy_atol` | 字符串转长整型 | xy_stdlib.h | `atol` | stdlib.h | ✅ 已实现 |
| `xy_atoll` | 字符串转长长整型 | xy_stdlib.h | `atoll` | stdlib.h | ✅ 已实现 |
| `xy_atof` | 字符串转浮点数 | xy_stdlib.h | `atof` | stdlib.h | ✅ 已实现 |
| `xy_strtol` | 字符串转长整型(带进制)| xy_stdlib.h | `strtol` | stdlib.h | ✅ 已实现 |
| `xy_strtoul` | 字符串转无符号长整型 | xy_stdlib.h | `strtoul` | stdlib.h | ✅ 已实现 |
| `xy_strtoll` | 字符串转长长整型(带进制)| xy_stdlib.h | `strtoll` | stdlib.h | ✅ 已实现 |
| `xy_strtoull` | 字符串转无符号长长整型 | xy_stdlib.h | `strtoull` | stdlib.h | ✅ 已实现 |
| `xy_strtod` | 字符串转双精度浮点数 | xy_stdlib.h | `strtod` | stdlib.h | ✅ 已实现 |
| `xy_strtof` | 字符串转单精度浮点数 | xy_stdlib.h | `strtof` | stdlib.h | ✅ 已实现 |

### 5️⃣ 数值转字符串 (非标准/扩展)

| XY API | 功能说明 | XY Header | 标准 API | 标准 Header | 备注 |
|--------|---------|-----------|----------|-------------|------|
| `xy_itoa` | 整型转字符串 | xy_stdlib.h | `itoa` | stdlib.h | ✅ 已实现(非标准)|
| `xy_ltoa` | 长整型转字符串 | xy_stdlib.h | `ltoa` | stdlib.h | ✅ 已实现(非标准)|
| `xy_utoa` | 无符号整型转字符串 | xy_stdlib.h | `utoa` | stdlib.h | ✅ 已实现(非标准)|
| `xy_ultoa` | 无符号长整型转字符串 | xy_stdlib.h | `ultoa` | stdlib.h | ✅ 已实现(非标准)|

### 6️⃣ 内存管理 (stdlib.h)

| XY API | 功能说明 | XY Header | 标准 API | 标准 Header | 备注 |
|--------|---------|-----------|----------|-------------|------|
| `xy_malloc` | 分配内存 | xy_stdlib.h | `malloc` | stdlib.h | ⚠️ 占位实现 |
| `xy_calloc` | 分配并清零内存 | xy_stdlib.h | `calloc` | stdlib.h | ⚠️ 占位实现 |
| `xy_realloc` | 重新分配内存 | xy_stdlib.h | `realloc` | stdlib.h | ⚠️ 占位实现 |
| `xy_free` | 释放内存 | xy_stdlib.h | `free` | stdlib.h | ⚠️ 占位实现 |

**注意**: 内存管理函数需要根据平台(FreeRTOS/RT-Thread/裸机)进行适配。

### 7️⃣ 排序与搜索 (stdlib.h)

| XY API | 功能说明 | XY Header | 标准 API | 标准 Header | 备注 |
|--------|---------|-----------|----------|-------------|------|
| `xy_qsort` | 快速排序 | xy_stdlib.h | `qsort` | stdlib.h | ✅ 已实现 |
| `xy_bsearch` | 二分查找 | xy_stdlib.h | `bsearch` | stdlib.h | ✅ 已实现 |

### 8️⃣ 数学函数 (stdlib.h)

| XY API | 功能说明 | XY Header | 标准 API | 标准 Header | 备注 |
|--------|---------|-----------|----------|-------------|------|
| `xy_abs` | 整数绝对值 | xy_stdlib.h | `abs` | stdlib.h | ✅ 已实现 |
| `xy_labs` | 长整数绝对值 | xy_stdlib.h | `labs` | stdlib.h | ✅ 已实现 |
| `xy_llabs` | 长长整数绝对值 | xy_stdlib.h | `llabs` | stdlib.h | ✅ 已实现 |
| `xy_div` | 整数除法 | xy_stdlib.h | `div` | stdlib.h | ✅ 已实现 |
| `xy_ldiv` | 长整数除法 | xy_stdlib.h | `ldiv` | stdlib.h | ✅ 已实现 |
| `xy_lldiv` | 长长整数除法 | xy_stdlib.h | `lldiv` | stdlib.h | ✅ 已实现 |

### 9️⃣ 随机数生成 (stdlib.h)

| XY API | 功能说明 | XY Header | 标准 API | 标准 Header | 备注 |
|--------|---------|-----------|----------|-------------|------|
| `xy_rand` | 生成伪随机数 | xy_stdlib.h | `rand` | stdlib.h | ✅ 已实现 |
| `xy_srand` | 设置随机数种子 | xy_stdlib.h | `srand` | stdlib.h | ✅ 已实现 |
| `XY_RAND_MAX` | 随机数最大值 | xy_stdlib.h | `RAND_MAX` | stdlib.h | ✅ 已定义(32767)|

### 🔟 字符分类与转换 (ctype.h)

| XY API | 功能说明 | XY Header | 标准 API | 标准 Header | 备注 |
|--------|---------|-----------|----------|-------------|------|
| `xy_isalpha` | 是否为字母 | xy_ctype.h | `isalpha` | ctype.h | ✅ 已实现(宏)|
| `xy_isdigit` | 是否为数字 | xy_ctype.h | `isdigit` | ctype.h | ✅ 已实现(宏)|
| `xy_isalnum` | 是否为字母或数字 | xy_ctype.h | `isalnum` | ctype.h | ✅ 已实现(宏)|
| `xy_islower` | 是否为小写字母 | xy_ctype.h | `islower` | ctype.h | ✅ 已实现(宏)|
| `xy_isupper` | 是否为大写字母 | xy_ctype.h | `isupper` | ctype.h | ✅ 已实现(宏)|
| `xy_isspace` | 是否为空白字符 | xy_ctype.h | `isspace` | ctype.h | ✅ 已实现(宏)|
| `xy_isblank` | 是否为空格或制表符 | xy_ctype.h | `isblank` | ctype.h | ✅ 已实现(宏)|
| `xy_isxdigit` | 是否为十六进制数字 | xy_ctype.h | `isxdigit` | ctype.h | ✅ 已实现(宏)|
| `xy_isprint` | 是否为可打印字符 | xy_ctype.h | `isprint` | ctype.h | ✅ 已实现(宏)|
| `xy_isgraph` | 是否为图形字符 | xy_ctype.h | `isgraph` | ctype.h | ✅ 已实现(宏)|
| `xy_iscntrl` | 是否为控制字符 | xy_ctype.h | `iscntrl` | ctype.h | ✅ 已实现(宏)|
| `xy_ispunct` | 是否为标点符号 | xy_ctype.h | `ispunct` | ctype.h | ✅ 已实现(宏)|
| `xy_isascii` | 是否为 ASCII 字符 | xy_ctype.h | `isascii` | ctype.h | ✅ 已实现(宏)|
| `xy_tolower` | 转换为小写 | xy_ctype.h | `tolower` | ctype.h | ✅ 已实现(宏)|
| `xy_toupper` | 转换为大写 | xy_ctype.h | `toupper` | ctype.h | ✅ 已实现(宏)|

### 1️⃣1️⃣ 断言 (assert.h)

| XY API | 功能说明 | XY Header | 标准 API | 标准 Header | 备注 |
|--------|---------|-----------|----------|-------------|------|
| `xy_assert` | 断言宏 | xy_assert.h | `assert` | assert.h | ✅ 已实现 |
| `assert` | 断言宏(兼容)| xy_assert.h | `assert` | assert.h | ✅ 已定义(别名)|

### 1️⃣2️⃣ 类型定义 (stddef.h / stdint.h)

| XY 类型 | 功能说明 | XY Header | 标准类型 | 标准 Header | 备注 |
|--------|---------|-----------|----------|-------------|------|
| `size_t` | 无符号整型(大小)| xy_typedef.h | `size_t` | stddef.h | ✅ 已定义 |
| `NULL` | 空指针 | xy_typedef.h | `NULL` | stddef.h | ✅ 已定义 |
| `uint8_t` | 8位无符号整型 | stdint.h | `uint8_t` | stdint.h | ✅ 使用标准库 |
| `uint16_t` | 16位无符号整型 | stdint.h | `uint16_t` | stdint.h | ✅ 使用标准库 |
| `uint32_t` | 32位无符号整型 | stdint.h | `uint32_t` | stdint.h | ✅ 使用标准库 |
| `int8_t` | 8位有符号整型 | stdint.h | `int8_t` | stdint.h | ✅ 使用标准库 |
| `int16_t` | 16位有符号整型 | stdint.h | `int16_t` | stdint.h | ✅ 使用标准库 |
| `int32_t` | 32位有符号整型 | stdint.h | `int32_t` | stdint.h | ✅ 使用标准库 |

### 1️⃣3️⃣ 时间结构 (time.h)

| XY 类型 | 功能说明 | XY Header | 标准类型 | 标准 Header | 备注 |
|--------|---------|-----------|----------|-------------|------|
| `xy_time_t` | 时间结构体 | xy_time.h | `struct tm` | time.h | ✅ 已定义(仅结构)|

### 1️⃣4️⃣ 数学函数 (math.h) - 🆕 M0 MCU优化

#### 软件除法 (针对无硬件除法器的MCU)

| XY API | 功能说明 | XY Header | 标准 API | 标准 Header | 备注 |
|--------|---------|-----------|----------|-------------|------|
| `xy_udiv32` | 32位无符号除法 | xy_math.h | `/` | - | ✅ 已实现(M0优化)|
| `xy_sdiv32` | 32位有符号除法 | xy_math.h | `/` | - | ✅ 已实现 |
| `xy_udivmod32` | 32位除法带余数 | xy_math.h | `div` | stdlib.h | ✅ 已实现 |
| `xy_sdivmod32` | 32位有符号除法带余数 | xy_math.h | `div` | stdlib.h | ✅ 已实现 |
| `xy_udiv64` | 64位无符号除法 | xy_math.h | `/` | - | ✅ 已实现 |
| `xy_udivmod64` | 64位除法带余数 | xy_math.h | - | - | ✅ 已实现 |

#### 软件乘法

| XY API | 功能说明 | XY Header | 标准 API | 标准 Header | 备注 |
|--------|---------|-----------|----------|-------------|------|
| `xy_umul32` | 32位无符号乘法 | xy_math.h | `*` | - | ✅ 已实现 |
| `xy_umul32x32` | 32x32->64位乘法 | xy_math.h | - | - | ✅ 已实现 |

#### 基础数学运算

| XY API | 功能说明 | XY Header | 标准 API | 标准 Header | 备注 |
|--------|---------|-----------|----------|-------------|------|
| `xy_isqrt32` | 32位整数平方根 | xy_math.h | `sqrt` | math.h | ✅ 已实现 |
| `xy_isqrt64` | 64位整数平方根 | xy_math.h | `sqrt` | math.h | ✅ 已实现 |
| `xy_ipow` | 整数幂运算 | xy_math.h | `pow` | math.h | ✅ 已实现 |
| `xy_gcd` | 最大公约数 | xy_math.h | - | - | ✅ 已实现(欧几里得)|
| `xy_lcm` | 最小公倍数 | xy_math.h | - | - | ✅ 已实现 |
| `xy_avg` | 安全平均值(防溢出)| xy_math.h | - | - | ✅ 已实现 |

#### 位运算工具

| XY API | 功能说明 | XY Header | 标准 API | 标准 Header | 备注 |
|--------|---------|-----------|----------|-------------|------|
| `xy_is_power_of_2` | 判断2的幂 | xy_math.h | - | - | ✅ 已实现 |
| `xy_next_power_of_2` | 向上取2的幂 | xy_math.h | - | - | ✅ 已实现 |
| `xy_clz32` | 计数前导零 | xy_math.h | `__builtin_clz` | - | ✅ 已实现 |
| `xy_ctz32` | 计数后缀零 | xy_math.h | `__builtin_ctz` | - | ✅ 已实现 |
| `xy_popcount32` | 计数置位位数 | xy_math.h | `__builtin_popcount` | - | ✅ 已实现 |

#### 定点数运算 (Q16.16)

| XY API | 功能说明 | XY Header | 标准 API | 标准 Header | 备注 |
|--------|---------|-----------|----------|-------------|------|
| `xy_fixed_mul` | 定点数乘法 | xy_math.h | - | - | ✅ 已实现(Q16.16)|
| `xy_fixed_div` | 定点数除法 | xy_math.h | - | - | ✅ 已实现(Q16.16)|
| `xy_fixed_sqrt` | 定点数平方根 | xy_math.h | - | - | ✅ 已实现(Q16.16)|
| `xy_int_to_fixed` | 整数转定点数 | xy_math.h | - | - | ✅ 已实现(宏)|
| `xy_fixed_to_int` | 定点数转整数 | xy_math.h | - | - | ✅ 已实现(宏)|

#### 三角函数 (查表法)

| XY API | 功能说明 | XY Header | 标准 API | 标准 Header | 备注 |
|--------|---------|-----------|----------|-------------|------|
| `xy_sin_deg` | 正弦(整数度) | xy_math.h | `sin` | math.h | ✅ 已实现(Q0.15)|
| `xy_cos_deg` | 余弦(整数度) | xy_math.h | `cos` | math.h | ✅ 已实现(Q0.15)|
| `xy_tan_deg` | 正切(整数度) | xy_math.h | `tan` | math.h | ✅ 已实现(Q0.15)|

#### 数学宏

| XY 宏 | 功能说明 | XY Header | 标准宏 | 标准 Header | 备注 |
|-------|---------|-----------|--------|-------------|------|
| `XY_MIN` | 取最小值 | xy_math.h | - | - | ✅ 已定义 |
| `XY_MAX` | 取最大值 | xy_math.h | - | - | ✅ 已定义 |
| `XY_CLAMP` | 限制范围 | xy_math.h | - | - | ✅ 已定义 |
| `XY_ABS` | 绝对值 | xy_math.h | `abs` | stdlib.h | ✅ 已定义 |
| `XY_SIGN` | 符号函数 | xy_math.h | - | - | ✅ 已定义 |

---

## 📊 统计摘要

### 功能覆盖度

| 标准库头文件 | 实现状态 | 覆盖率 | 说明 |
|-------------|---------|-------|------|
| `string.h` | ✅ 完整 | ~95% | 26个函数，包含所有常用函数 |
| `stdio.h` | ✅ 主要功能 | ~80% | 10个格式化I/O函数 |
| `stdlib.h` | ✅ 主要功能 | ~75% | 29个函数(内存管理需适配)|
| `ctype.h` | ✅ 完整 | 100% | 15个字符分类宏 |
| `assert.h` | ✅ 完整 | 100% | 断言宏 |
| `stddef.h` | ✅ 完整 | 100% | 类型定义 |
| `time.h` | ⚠️ 部分 | 10% | 仅结构体定义 |
| `math.h` | ✅ 嵌入式优化 | ~60% | 30+函数(M0优化除法/定点数/三角) |

### API 总数统计

- **已实现函数**: 约 110+ 个
- **已实现宏**: 约 20+ 个
- **类型定义**: 约 10+ 个

---

## 🚀 使用建议

### 推荐方式

使用统一头文件 `xy_stdlib.h`，自动包含所有功能:

```c
#include <stdint.h>      // ✓ 允许(标准整型)
#include "xy_stdlib.h"   // ✓ 推荐(包含所有功能)
```

### 传统方式

也可以单独包含需要的头文件:

```c
#include "xy_string.h"   // 字符串操作
#include "xy_stdio.h"    // 格式化I/O
#include "xy_ctype.h"    // 字符分类
```

### 迁移指南

从标准库迁移到 xy_clib:

```c
// 旧代码
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
memcpy(dest, src, len);
printf("Hello\n");

// 新代码
#include <stdint.h>
#include "xy_stdlib.h"
xy_memcpy(dest, src, len);
xy_stdio_printf("Hello\n");
```

---

## ⚠️ 注意事项

1. **内存管理函数**(malloc/free等)目前是占位实现，需要根据具体平台适配
2. **strdup/strndup** 依赖动态内存分配
3. **数学库** (xy_math.h) 专为 Cortex-M0 等无硬件除法的 MCU 优化，提供软件实现
4. **时间函数** (time.h) 仅提供结构体定义，无实际时间函数
5. **定点数运算** 使用 Q16.16 格式，适合无浮点运算单元的 MCU
6. **三角函数** 使用查表法，精度为整数度(0-359)，返回 Q0.15 格式

---

## 📝 版本信息

- **版本**: 1.0.0
- **更新日期**: 2025-11-01
- **维护者**: XinYi Framework Team
