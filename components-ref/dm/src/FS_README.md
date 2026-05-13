# XY_FS - 文件系统抽象层

**版本**: 1.0.0  
**日期**: 2026-03-05

---

## 📋 概述

XY_FS 是一个文件系统抽象层，提供统一的文件操作接口，支持多种底层文件系统 (FatFS/LittleFS/SPIFFS 等)。

### 特性

- ✅ 统一文件操作接口
- ✅ 支持多驱动器
- ✅ 路径解析自动
- ✅ 便捷 API (read_file/write_file)
- ✅ 零外部依赖

### 性能

| 指标 | 数值 |
|------|------|
| **代码大小** | ~400 行 |
| **最大驱动器** | 4 个 |
| **最大路径** | 256 字符 |
| **RAM 占用** | 动态分配 |

---

## 🚀 快速开始

### 1. 注册文件系统

```c
#include "xy_fs.h"

// 定义底层操作
static int flash_init(void) { return 0; }
static int flash_open(xy_fs_file_t *f, const char *p, uint8_t m) { return 0; }
static int flash_read(xy_fs_file_t *f, void *b, size_t l) { return 0; }
static int flash_write(xy_fs_file_t *f, const void *b, size_t l) { return 0; }
static int flash_close(xy_fs_file_t *f) { return 0; }

static xy_fs_ops_t flash_ops = {
    .init = flash_init,
    .open = flash_open,
    .read = flash_read,
    .write = flash_write,
    .close = flash_close,
};

// 注册驱动器
xy_fs_t flash_fs;
xy_fs_register(&flash_fs, "flash", &flash_ops);
```

---

### 2. 挂载文件系统

```c
// 挂载到 "0:"
xy_fs_mount(&flash_fs, "0:");

// 现在可以使用 "0:/path/to/file" 访问文件
```

---

### 3. 文件操作

```c
// 打开文件
xy_fs_file_t *file = xy_fs_open("0:/test.txt", XY_FS_MODE_READ);
if (!file) {
    // 打开失败
    return;
}

// 读取文件
char buf[256];
int len = xy_fs_read(file, buf, sizeof(buf));

// 关闭文件
xy_fs_close(file);
```

---

### 4. 便捷 API

```c
// 读取整个文件
char buf[1024];
size_t actual;
xy_fs_read_file("0:/config.json", buf, sizeof(buf), &actual);

// 写入整个文件
const char *data = "Hello, World!";
xy_fs_write_file("0:/hello.txt", data, strlen(data));

// 检查文件是否存在
if (xy_fs_exists("0:/test.txt")) {
    // 文件存在
}

// 删除文件
xy_fs_remove("0:/old.txt");

// 重命名文件
xy_fs_rename("0:/old.txt", "0:/new.txt");
```

---

## 📖 API 参考

### 基础 API

| 函数 | 说明 |
|------|------|
| `xy_fs_register(fs, name, ops)` | 注册文件系统 |
| `xy_fs_mount(fs, mount_point)` | 挂载文件系统 |
| `xy_fs_unmount(fs)` | 卸载文件系统 |

### 文件操作

| 函数 | 说明 |
|------|------|
| `xy_fs_open(path, mode)` | 打开文件 |
| `xy_fs_close(file)` | 关闭文件 |
| `xy_fs_read(file, buf, len)` | 读取文件 |
| `xy_fs_write(file, buf, len)` | 写入文件 |
| `xy_fs_seek(file, offset, whence)` | 定位文件指针 |
| `xy_fs_tell(file)` | 获取当前位置 |

### 文件属性

| 函数 | 说明 |
|------|------|
| `xy_fs_size(path, size)` | 获取文件大小 |
| `xy_fs_exists(path)` | 检查文件是否存在 |
| `xy_fs_remove(path)` | 删除文件 |
| `xy_fs_rename(old, new)` | 重命名文件 |

### 便捷 API

| 函数 | 说明 |
|------|------|
| `xy_fs_read_file(path, buf, len, actual)` | 读取整个文件 |
| `xy_fs_write_file(path, buf, len)` | 写入整个文件 |

---

## 🔧 使用示例

### 示例 1: 读取配置文件

```c
// 读取 JSON 配置
char config_buf[1024];
size_t actual;

if (xy_fs_read_file("0:/config.json", config_buf, sizeof(config_buf), &actual) == XY_FS_OK) {
    // 解析 JSON
    xy_json_t *config = xy_json_parse(config_buf);
    
    // 使用配置...
    
    xy_json_free(config);
}
```

---

### 示例 2: 日志记录

```c
// 追加日志
xy_fs_file_t *log_file = xy_fs_open("0:/system.log", 
                                      XY_FS_MODE_WRITE | XY_FS_MODE_APPEND);
if (log_file) {
    char log_line[128];
    snprintf(log_line, sizeof(log_line), "[%lu] System started\n", xy_os_tick_get());
    xy_fs_write(log_file, log_line, strlen(log_line));
    xy_fs_close(log_file);
}
```

---

### 示例 3: 数据持久化

```c
typedef struct {
    uint32_t counter;
    uint32_t last_value;
    char timestamp[32];
} system_state_t;

// 保存状态
system_state_t state = {
    .counter = 12345,
    .last_value = 67890,
};

xy_fs_write_file("0:/state.dat", &state, sizeof(state));

// 恢复状态
system_state_t loaded_state;
size_t actual;
if (xy_fs_read_file("0:/state.dat", &loaded_state, sizeof(loaded_state), &actual) == XY_FS_OK) {
    // 使用加载的状态
}
```

---

### 示例 4: 多驱动器支持

```c
// 注册多个驱动器
xy_fs_register(&flash_fs, "flash", &flash_ops);
xy_fs_register(&sd_fs, "sd", &sd_ops);

// 挂载
xy_fs_mount(&flash_fs, "0:");  // 内部 Flash
xy_fs_mount(&sd_fs, "1:");     // SD 卡

// 访问不同驱动器
xy_fs_read_file("0:/config.json", buf, len, &actual);  // 从 Flash 读取
xy_fs_read_file("1:/data.bin", buf, len, &actual);     // 从 SD 卡读取
```

---

## 📊 文件模式

| 模式 | 标志 | 说明 |
|------|------|------|
| **Read** | `XY_FS_MODE_READ` | 只读 |
| **Write** | `XY_FS_MODE_WRITE` | 只写 |
| **Append** | `XY_FS_MODE_APPEND` | 追加 |
| **Create** | `XY_FS_MODE_CREATE` | 不存在则创建 |
| **Trunc** | `XY_FS_MODE_TRUNC` | 截断已有文件 |

---

## ⚠️ 注意事项

### 路径格式

```
格式："<drive>:<path>"
示例：
  "0:/config.json"    - 驱动器 0 的 config.json
  "1:/data/log.txt"   - 驱动器 1 的 data/log.txt
```

### 线程安全

- 本库**不是**线程安全的
- 多线程环境下需要自行加锁

### 缓冲区大小

- 读取大文件时注意缓冲区大小
- 建议使用 `xy_fs_read_file()` 分块读取

---

## 📚 相关文档

- [DM 组件优化计划](../DM_OPTIMIZATION_PLAN.md)
- [DM TODO 进度](../DM_TODO_PROGRESS.md)
- [JSON 解析器](JSON_README.md)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
