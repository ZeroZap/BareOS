# XinYi OSAL (OS Abstraction Layer)

## 概述

XinYi OSAL 是一个操作系统抽象层，为嵌入式系统提供统一的 RTOS 接口。它支持多种 RTOS 后端，包括 FreeRTOS、RT-Thread、CMSIS-RTX，以及裸机（无 RTOS）环境。

## 架构层次

```
┌─────────────────────────────────────────┐
│           应用层 (Projects)              │
│         使用统一的 xy_os API            │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│        OSAL 层 (components/osal)         │
│  xy_os.h - 统一的 CMSIS-RTOS2 兼容接口   │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│          后端适配层                      │
│  ┌──────────┬──────────┬──────────┐     │
│  │Bare-metal│ FreeRTOS │RT-Thread │     │
│  │  (裸机)  │          │          │     │
│  └──────────┴──────────┴──────────┘     │
└─────────────────────────────────────────┘
                    ▲
┌─────────────────────────────────────────┐
│      平台层 (MCU SDK / RTOS Kernel)      │
│  STM32 HAL / FreeRTOS / RT-Thread ...   │
└─────────────────────────────────────────┘
```

## 支持的后端

| 后端 | 描述 | 线程 | 同步 | 通信 | 定时器 |
|------|------|------|------|------|--------|
| **Bare-metal** | 裸机（无 RTOS） | ❌ | ❌ | ❌ | ✅ (软件) |
| **FreeRTOS** | FreeRTOS RTOS | ✅ | ✅ | ✅ | ✅ |
| **RT-Thread** | RT-Thread RTOS | ✅ | ✅ | ✅ | ✅ |
| **CMSIS-RTX** | ARM CMSIS-RTOS2 | ✅ | ✅ | ✅ | ✅ |

## 快速开始

### 1. 选择后端

在构建配置中选择一个后端：

**Makefile:**
```makefile
OSAL_BACKEND = baremetal  # 或 freertos, rt-thread, cmsis-rtx
```

**CMake:**
```cmake
set(OSAL_BACKEND "baremetal" CACHE STRING "OSAL backend")
add_subdirectory(components/kernel/osal)
```

### 2. 初始化 OS

```c
#include "xy_os.h"

int main(void)
{
    // 初始化 OS
    xy_os_kernel_init();
    
    // 创建线程（仅 RTOS 后端）
    #if !defined(XY_OS_BACKEND_BAREMETAL)
    xy_os_thread_new(my_thread_func, NULL, NULL);
    #endif
    
    // 启动 OS 调度器（仅 RTOS 后端）
    #if !defined(XY_OS_BACKEND_BAREMETAL)
    xy_os_kernel_start();
    #else
    // 裸机模式下直接进入主循环
    #endif
    
    // 主循环
    while (1) {
        xy_os_delay(1000);  // 延时 1 秒
    }
}
```

### 3. 使用 OS 服务

```c
// 创建互斥锁
xy_os_mutex_id_t mutex = xy_os_mutex_new(NULL);

// 获取信号量
xy_os_semaphore_id_t sem = xy_os_semaphore_new(10, 10, NULL);

// 创建消息队列
xy_os_msgqueue_id_t queue = xy_os_msgqueue_new(10, sizeof(int), NULL);

// 创建定时器
xy_os_timer_id_t timer = xy_os_timer_new(timer_callback, XY_OS_TIMER_PERIODIC, NULL, NULL);
xy_os_timer_start(timer, 1000);  // 1 秒周期
```

## 功能特性

### 内核控制

| 函数 | 描述 | Bare-metal | FreeRTOS | RT-Thread |
|------|------|------------|----------|-----------|
| `xy_os_kernel_init()` | 初始化内核 | ✅ | ✅ | ✅ |
| `xy_os_kernel_start()` | 启动调度器 | ✅ (无操作) | ✅ | ✅ |
| `xy_os_kernel_get_tick_count()` | 获取 tick 计数 | ✅ | ✅ | ✅ |
| `xy_os_kernel_lock()` | 锁定调度器 | ✅ | ✅ | ✅ |
| `xy_os_kernel_unlock()` | 解锁调度器 | ✅ | ✅ | ✅ |

### 线程管理

| 函数 | 描述 | Bare-metal | FreeRTOS | RT-Thread |
|------|------|------------|----------|-----------|
| `xy_os_thread_new()` | 创建线程 | ❌ | ✅ | ✅ |
| `xy_os_thread_get_id()` | 获取当前线程 ID | ✅ (stub) | ✅ | ✅ |
| `xy_os_thread_set_priority()` | 设置优先级 | ❌ | ✅ | ✅ |
| `xy_os_thread_suspend()` | 挂起线程 | ❌ | ✅ | ✅ |
| `xy_os_thread_resume()` | 恢复线程 | ❌ | ✅ | ✅ |
| `xy_os_thread_yield()` | 让出 CPU | ✅ (无操作) | ✅ | ✅ |

### 同步原语

| 函数 | 描述 | Bare-metal | FreeRTOS | RT-Thread |
|------|------|------------|----------|-----------|
| `xy_os_mutex_new()` | 创建互斥锁 | ❌ | ✅ | ✅ |
| `xy_os_mutex_acquire()` | 获取锁 | ❌ | ✅ | ✅ |
| `xy_os_mutex_release()` | 释放锁 | ❌ | ✅ | ✅ |
| `xy_os_semaphore_new()` | 创建信号量 | ❌ | ✅ | ✅ |
| `xy_os_semaphore_acquire()` | 获取信号量 | ❌ | ✅ | ✅ |
| `xy_os_event_flags_*()` | 事件标志 | ❌ | ✅ | ✅ |

### 通信

| 函数 | 描述 | Bare-metal | FreeRTOS | RT-Thread |
|------|------|------------|----------|-----------|
| `xy_os_msgqueue_new()` | 创建消息队列 | ❌ | ✅ | ✅ |
| `xy_os_msgqueue_put()` | 发送消息 | ❌ | ✅ | ✅ |
| `xy_os_msgqueue_get()` | 接收消息 | ❌ | ✅ | ✅ |
| `xy_os_mempool_*()` | 内存池 | ❌ | ⚠️ | ✅ |

### 定时器

| 函数 | 描述 | Bare-metal | FreeRTOS | RT-Thread |
|------|------|------------|----------|-----------|
| `xy_os_timer_new()` | 创建定时器 | ✅ | ✅ | ✅ |
| `xy_os_timer_start()` | 启动定时器 | ✅ | ✅ | ✅ |
| `xy_os_timer_stop()` | 停止定时器 | ✅ | ✅ | ✅ |
| `xy_os_timer_delete()` | 删除定时器 | ✅ | ✅ | ✅ |

### 延时

| 函数 | 描述 | Bare-metal | FreeRTOS | RT-Thread |
|------|------|------------|----------|-----------|
| `xy_os_delay()` | 相对延时 | ✅ (忙等) | ✅ (睡眠) | ✅ (睡眠) |
| `xy_os_delay_until()` | 绝对延时 | ✅ (忙等) | ✅ (睡眠) | ✅ (睡眠) |

图例：✅ 支持，❌ 不支持，⚠️ 部分支持

## 构建配置

### CMake 选项

| 选项 | 默认值 | 描述 |
|------|--------|------|
| `OSAL_BACKEND` | baremetal | 后端选择 |
| `XY_OS_FEATURE_THREAD` | ON | 线程管理 |
| `XY_OS_FEATURE_MUTEX` | ON | 互斥锁 |
| `XY_OS_FEATURE_SEMAPHORE` | ON | 信号量 |
| `XY_OS_FEATURE_TIMER` | ON | 定时器 |
| `XY_OS_PARAM_CHECK` | ON | 参数检查 |

### 使用示例

```bash
# 裸机构建
cmake .. -DOSAL_BACKEND=baremetal

# FreeRTOS 构建
cmake .. -DOSAL_BACKEND=freertos

# RT-Thread 构建
cmake .. -DOSAL_BACKEND=rt-thread
```

## 优先级映射

### FreeRTOS
- XY OSAL: 0 (最低) → 56 (最高)
- FreeRTOS: 0 (最低) → configMAX_PRIORITIES-1 (最高)
- **方向相同**，直接映射

### RT-Thread
- XY OSAL: 0 (最低) → 56 (最高)
- RT-Thread: RT_THREAD_PRIORITY_MAX-1 (最低) → 0 (最高)
- **方向相反**，需要反转映射

### CMSIS-RTX
- XY OSAL: 0 (最低) → 56 (最高)
- CMSIS-RTX: osPriorityLow (0) → osPriorityISR (56)
- **方向相同**，直接映射

## 移植指南

### 添加新后端

1. 创建后端目录：`components/osal/<backend>/`
2. 实现 `xy_os_<backend>.c`，包含所有 `xy_os_*` 函数
3. 在 `CMakeLists.txt` 中添加后端配置
4. 在 `Kconfig.osal` 中添加后端选项

### 最小实现要求

```c
// 必须实现的函数
xy_os_status_t xy_os_kernel_init(void);
xy_os_status_t xy_os_kernel_start(void);
uint32_t xy_os_kernel_get_tick_count(void);
xy_os_status_t xy_os_delay(uint32_t ticks);

// 可选实现的函数（返回 stub）
xy_os_thread_id_t xy_os_thread_new(...);
xy_os_mutex_id_t xy_os_mutex_new(...);
// ... 其他函数
```

## 使用示例

### 线程创建

```c
#include "xy_os.h"

static void led_thread(void *arg)
{
    (void)arg;
    while (1) {
        // 翻转 LED
        toggle_led();
        xy_os_delay(500);  // 500ms
    }
}

void app_main(void)
{
    xy_os_kernel_init();
    
    // 创建 LED 线程
    xy_os_thread_attr_t attr = {
        .name = "led_thread",
        .priority = XY_OS_PRIORITY_NORMAL,
        .stack_size = 1024,
    };
    
    xy_os_thread_new(led_thread, NULL, &attr);
    
    xy_os_kernel_start();
}
```

### 互斥锁保护

```c
static xy_os_mutex_id_t g_mutex;

void shared_resource_init(void)
{
    g_mutex = xy_os_mutex_new(NULL);
}

void shared_resource_access(void)
{
    // 获取锁
    if (xy_os_mutex_acquire(g_mutex, XY_OS_WAIT_FOREVER) == XY_OS_OK) {
        // 访问共享资源
        // ...
        
        // 释放锁
        xy_os_mutex_release(g_mutex);
    }
}
```

### 软件定时器

```c
static void timer_callback(void *arg)
{
    (void)arg;
    // 定时器到期处理
    printf("Timer expired!\n");
}

void timer_example(void)
{
    xy_os_timer_attr_t attr = {
        .name = "my_timer",
    };
    
    xy_os_timer_id_t timer = xy_os_timer_new(
        timer_callback,
        XY_OS_TIMER_PERIODIC,  // 周期定时器
        NULL,
        &attr
    );
    
    xy_os_timer_start(timer, 1000);  // 1 秒周期
}
```

## 注意事项

### 裸机后端限制

1. **无多线程**: 所有线程函数返回 stub 值
2. **忙等延时**: `xy_os_delay()` 使用忙等，会阻塞 CPU
3. **无同步原语**: 互斥锁、信号量等返回错误
4. **软件定时器**: 需要在主循环中调用 `xy_timer_sw_poll()`

### 中断安全

- 所有 `xy_os_*` 函数**不保证**在中断上下文中安全
- 在中断中请使用 RTOS 提供的 ISR 安全 API
- FreeRTOS: 使用 `FromISR` 后缀的函数
- RT-Thread: 使用 `rt_xxx_from_isr` 函数

### 内存管理

- 动态创建的 OS 对象（线程、互斥锁等）需要手动删除
- 避免内存泄漏：确保 `xy_os_*_new()` 和 `xy_os_*_delete()` 成对调用

## 许可证

本项目采用 Apache License 2.0 许可证。

## 相关文档

- [实现指南](IMPLEMENTATION_GUIDE.md)
- [实现状态](IMPLEMENTATION_STATUS.md)
- [后端对比](BACKEND_COMPARISON.md)
- [快速开始](QUICK_START.md)
