# XinYi Kernel Components - 内核组件

**版本**: 1.0.0  
**日期**: 2026-03-18  
**状态**: 🟡 完善中

---

## 📋 概述

Kernel 层提供操作系统抽象层 (OSAL) 和基础服务，支持多 RTOS 后端。

---

## 🏗️ 组件结构

```
kernel/
├── osal/           # 操作系统抽象层
│   ├── backend/    # 多后端支持
│   │   ├── freertos/
│   │   ├── rtthread/
│   │   ├── rtx5/
│   │   └── baremetal/
│   ├── inc/        # 头文件
│   └── src/        # 源文件
├── misc/           # 杂项服务
│   └── xy_sysmon.c # 系统监控
└── service/        # 内核服务
    └── ...
```

---

## 🔧 OSAL 支持的后端

| 后端 | 状态 | 说明 |
|------|------|------|
| **FreeRTOS** | ✅ | 最常用的 RTOS |
| **RT-Thread** | ✅ | 国产 RTOS |
| **CMSIS-RTX5** | ✅ | ARM 官方 RTOS |
| **Bare-metal** | ✅ | 裸机模式 (无 OS) |

---

## 📁 核心 API

### 任务管理
```c
xy_os_task_t task;
xy_os_task_create(&task, "my_task", entry_func, arg, stack_size, priority);
xy_os_task_delete(&task);
xy_os_task_delay(100);  // 延迟 100ms
```

### 信号量
```c
xy_os_sem_t sem;
xy_os_sem_create(&sem, 1);  // 初始值 1
xy_os_sem_wait(&sem, 1000); // 等待 1s
xy_os_sem_post(&sem);       // 释放
```

### 互斥锁
```c
xy_os_mutex_t mutex;
xy_os_mutex_create(&mutex);
xy_os_mutex_lock(&mutex, 1000);
xy_os_mutex_unlock(&mutex);
```

### 消息队列
```c
xy_os_queue_t queue;
xy_os_queue_create(&queue, 10, sizeof(msg_t));
xy_os_queue_send(&queue, &msg, 100);
xy_os_queue_recv(&queue, &msg, 100);
```

---

## 🔨 构建配置

### CMake
```cmake
# 启用 OSAL
-DCONFIG_KERNEL_OSAL=y

# 选择后端
-DCONFIG_OSAL_BACKEND_FREERTOS=y
-DCONFIG_OSAL_BACKEND_RTTHREAD=y
-DCONFIG_OSAL_BACKEND_BAREMETAL=y
```

### Kconfig
```kconfig
config KERNEL_OSAL
    bool "Enable OSAL"
    default y

config OSAL_BACKEND_FREERTOS
    bool "FreeRTOS Backend"
    depends on KERNEL_OSAL
```

---

## 📊 完成度

| 模块 | 完成度 | 状态 |
|------|--------|------|
| **OSAL** | 98% | ✅ |
| **Misc** | 80% | 🟡 |
| **Service** | 60% | 🟡 |

**总体**: 85% 🟡

---

## 🚀 使用示例

### Bare-metal 模式
```c
#include "xy_os_baremetal.h"

int main(void) {
    xy_os_init();
    
    // 创建任务
    xy_os_task_t task;
    xy_os_task_create(&task, "blink", blink_entry, NULL, 512, 1);
    
    // 启动调度器
    xy_os_start();
    
    return 0;
}
```

### FreeRTOS 模式
```c
#include "xy_os_freertos.h"

void blink_entry(void *arg) {
    while (1) {
        xy_gpio_toggle(LED_PIN);
        xy_os_task_delay(100);
    }
}

int main(void) {
    xy_os_task_create(NULL, "blink", blink_entry, NULL, 512, 1);
    // FreeRTOS 调度器自动启动
    return 0;
}
```

---

## 📝 待完成任务

- [ ] 完善 CMakeLists.txt
- [ ] 完善 Kconfig
- [ ] 添加更多单元测试
- [ ] 补充文档示例

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
