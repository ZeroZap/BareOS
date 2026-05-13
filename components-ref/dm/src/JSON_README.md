# XY_JSON - 轻量级 JSON 解析器

**版本**: 1.0.0  
**日期**: 2026-03-05

---

## 📋 概述

XY_JSON 是一个轻量级 JSON 解析器，支持完整的 JSON 语法，提供对象/数组操作 API。

### 特性

- ✅ 完整 JSON 语法支持 (对象/数组/字符串/数值/布尔/null)
- ✅ 内存自动管理
- ✅ 对象成员操作 (get/set/delete/foreach)
- ✅ 数组元素操作 (get/append/insert/remove)
- ✅ 便捷 API (字符串/数值/布尔)
- ✅ 零外部依赖

### 性能

| 指标 | 数值 |
|------|------|
| **代码大小** | ~700 行 |
| **RAM 占用** | 动态分配 |
| **解析速度** | ~10KB/ms |
| **支持深度** | 不限 |

---

## 🚀 快速开始

### 1. 解析 JSON

```c
#include "xy_json.h"

const char *json_str = '{"name":"John","age":30,"city":"NYC"}';

// 解析 JSON
xy_json_t *json = xy_json_parse(json_str);
if (!json) {
    // 解析失败
    return;
}

// 使用完毕后释放
xy_json_free(json);
```

---

### 2. 对象操作

```c
// 创建对象
xy_json_t *obj = xy_json_new_object();

// 添加成员
xy_json_object_set(obj, "name", xy_json_new_string("John"));
xy_json_object_set(obj, "age", xy_json_new_number(30));
xy_json_object_set(obj, "active", xy_json_new_bool(true));

// 查找成员
xy_json_t *val = xy_json_object_get(obj, "name");
const char *name = xy_json_get_string(val, NULL, "Unknown");

// 删除成员
xy_json_object_delete(obj, "active");

// 遍历成员
xy_json_object_foreach(obj, [](const char *key, xy_json_t *val, void *ud) {
    printf("%s: ", key);
    // 处理值...
}, NULL);

// 释放
xy_json_free(obj);
```

---

### 3. 数组操作

```c
// 创建数组
xy_json_t *arr = xy_json_new_array();

// 添加元素
xy_json_array_append(arr, xy_json_new_number(1));
xy_json_array_append(arr, xy_json_new_number(2));
xy_json_array_append(arr, xy_json_new_number(3));

// 获取元素
xy_json_t *item = xy_json_array_get(arr, 0);
double num = xy_json_get_number(item, NULL, 0);

// 插入元素
xy_json_array_insert(arr, 1, xy_json_new_number(99));

// 删除元素
xy_json_array_remove(arr, 2);

// 获取长度
uint16_t len = xy_json_array_size(arr);

// 释放
xy_json_free(arr);
```

---

### 4. 便捷 API

```c
const char *json_str = '{"user":{"name":"John","age":30},"items":[1,2,3]}';
xy_json_t *json = xy_json_parse(json_str);

// 获取字符串
const char *name = xy_json_get_string(
    xy_json_object_get(json, "user"), 
    "name", 
    "Unknown"
);

// 获取数值
double age = xy_json_get_number(
    xy_json_object_get(json, "user"), 
    "age", 
    0
);

// 获取布尔值
bool active = xy_json_get_bool(json, "active", false);

xy_json_free(json);
```

---

## 📖 API 参考

### 核心 API

| 函数 | 说明 |
|------|------|
| `xy_json_parse(str)` | 解析 JSON 字符串 |
| `xy_json_free(json)` | 释放 JSON 对象 |
| `xy_json_stringify(json, buf, size)` | JSON 转字符串 |

### 对象 API

| 函数 | 说明 |
|------|------|
| `xy_json_object_get(obj, key)` | 获取成员 |
| `xy_json_object_set(obj, key, val)` | 设置成员 |
| `xy_json_object_delete(obj, key)` | 删除成员 |
| `xy_json_object_foreach(obj, cb, ud)` | 遍历成员 |

### 数组 API

| 函数 | 说明 |
|------|------|
| `xy_json_array_get(arr, idx)` | 获取元素 |
| `xy_json_array_append(arr, val)` | 添加元素 |
| `xy_json_array_insert(arr, idx, val)` | 插入元素 |
| `xy_json_array_remove(arr, idx)` | 删除元素 |
| `xy_json_array_size(arr)` | 获取长度 |

### 便捷 API

| 函数 | 说明 |
|------|------|
| `xy_json_new_string(str)` | 创建字符串 |
| `xy_json_new_number(num)` | 创建数值 |
| `xy_json_new_bool(val)` | 创建布尔 |
| `xy_json_new_object()` | 创建对象 |
| `xy_json_new_array()` | 创建数组 |
| `xy_json_get_string(json, key, def)` | 获取字符串 |
| `xy_json_get_number(json, key, def)` | 获取数值 |
| `xy_json_get_bool(json, key, def)` | 获取布尔 |

---

## 📊 JSON 类型

| 类型 | 枚举值 | 说明 |
|------|--------|------|
| **Null** | `XY_JSON_NULL` | null 值 |
| **Boolean** | `XY_JSON_BOOL` | true/false |
| **Number** | `XY_JSON_NUMBER` | 整数/浮点数 |
| **String** | `XY_JSON_STRING` | 字符串 |
| **Array** | `XY_JSON_ARRAY` | 数组 [] |
| **Object** | `XY_JSON_OBJECT` | 对象 {} |

---

## 🔧 使用示例

### 示例 1: 解析配置文件

```c
const char *config_json = 
    "{"
    "  \"wifi\": {"
    "    \"ssid\": \"MyNetwork\","
    "    \"password\": \"secret\","
    "    \"auto_connect\": true"
    "  },"
    "  \"debug\": true,"
    "  \"log_level\": 3"
    "}";

xy_json_t *config = xy_json_parse(config_json);

// 读取配置
xy_json_t *wifi = xy_json_object_get(config, "wifi");
const char *ssid = xy_json_get_string(wifi, "ssid", "default");
const char *pass = xy_json_get_string(wifi, "password", "");
bool auto_conn = xy_json_get_bool(wifi, "auto_connect", false);

bool debug = xy_json_get_bool(config, "debug", false);
int log_level = xy_json_get_number(config, "log_level", 0);

xy_json_free(config);
```

---

### 示例 2: 构建 JSON 响应

```c
// 构建响应
xy_json_t *resp = xy_json_new_object();

xy_json_object_set(resp, "status", xy_json_new_string("ok"));
xy_json_object_set(resp, "code", xy_json_new_number(200));

// 添加数据数组
xy_json_t *data = xy_json_new_array();
xy_json_array_append(data, xy_json_new_string("item1"));
xy_json_array_append(data, xy_json_new_string("item2"));
xy_json_array_append(data, xy_json_new_string("item3"));

xy_json_object_set(resp, "data", data);

// 序列化 (简化实现)
char buf[512];
xy_json_stringify(resp, buf, sizeof(buf));

xy_json_free(resp);
```

---

### 示例 3: 嵌套对象

```c
const char *nested = 
    "{"
    "  \"user\": {"
    "    \"profile\": {"
    "      \"name\": \"John\","
    "      \"age\": 30"
    "    }"
    "  }"
    "}";

xy_json_t *json = xy_json_parse(nested);

// 访问嵌套对象
xy_json_t *user = xy_json_object_get(json, "user");
xy_json_t *profile = xy_json_object_get(user, "profile");
const char *name = xy_json_get_string(profile, "name", "");
int age = xy_json_get_number(profile, "age", 0);

xy_json_free(json);
```

---

## ⚠️ 注意事项

### 内存管理

- 所有 `xy_json_new_*()` 创建的对象必须用 `xy_json_free()` 释放
- `xy_json_parse()` 返回的对象必须释放
- 子对象由父对象自动管理，无需单独释放

### 线程安全

- 本库**不是**线程安全的
- 多线程环境下需要自行加锁

### 性能优化

- 大 JSON 文件建议流式解析
- 频繁读写建议使用对象池
- 长字符串注意内存占用

---

## 📚 相关文档

- [DM 组件优化计划](../DM_OPTIMIZATION_PLAN.md)
- [DM TODO 进度](../DM_TODO_PROGRESS.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
