# 设备模型 API 详解

**版本**: 1.0.0  
**日期**: 2026-03-16  
**维护者**: XinYi Team

---

## 📖 概述

XinYi 设备模型提供统一的设备管理框架，参考 RT-Thread 和 Zephyr 设备模型设计。

### 核心特性
- ✅ 统一设备结构
- ✅ 设备注册/查找机制
- ✅ 电源管理
- ✅ 异步操作支持
- ✅ 设备 API 虚表

---

## 🔌 电源管理 API

### 数据结构

```c
/* 电源状态 */
typedef enum {
    XY_DEVICE_PM_STATE_ACTIVE = 0,     /* 全功率运行 */
    XY_DEVICE_PM_STATE_SLEEP,          /* 睡眠模式 */
    XY_DEVICE_PM_STATE_DEEP_SLEEP,     /* 深度睡眠 */
    XY_DEVICE_PM_STATE_OFF,            /* 关闭 */
} xy_device_pm_state_t;

/* 管理策略 */
typedef enum {
    XY_DEVICE_PM_POLICY_ALWAYS_ON = 0, /* 始终开启 */
    XY_DEVICE_PM_POLICY_AUTO,          /* 自动管理 */
    XY_DEVICE_PM_POLICY_MANUAL,        /* 手动控制 */
} xy_device_pm_policy_t;
```

### 初始化

```c
#include "inc/xy_device_pm.h"

/* 定义电源管理操作集 */
static const xy_device_pm_ops_t pm_ops = {
    .set_state = my_pm_set_state,
    .get_state = my_pm_get_state,
    .set_wakeup = my_pm_set_wakeup,
    .get_power_consumption = my_pm_get_consumption,
};

/* 初始化电源管理 */
xy_device_pm_init(dev, &pm_ops);
```

### 电源状态控制

```c
/* 设置电源状态 */
xy_device_pm_set_state(dev, XY_DEVICE_PM_STATE_SLEEP);

/* 获取电源状态 */
xy_device_pm_state_t state;
xy_device_pm_get_state(dev, &state);

/* 便捷 API */
xy_device_pm_sleep(dev);    /* 进入睡眠 */
xy_device_pm_wakeup(dev);   /* 唤醒 */
xy_device_pm_off(dev);      /* 关闭 */
xy_device_pm_on(dev);       /* 开启 */
```

### 自动睡眠示例

```c
/* 设置自动管理策略 */
xy_device_pm_set_policy(dev, XY_DEVICE_PM_POLICY_AUTO);

/* 设置空闲超时 10 秒 */
xy_device_pm_set_idle_timeout(dev, 10000);

/* 记录设备活动 (自动唤醒) */
xy_device_pm_record_activity(dev);

/* 检查空闲超时 (自动睡眠) */
xy_device_pm_check_idle(dev);
```

### 功耗查询

```c
uint32_t power_uw;
xy_device_pm_get_consumption(dev, &power_uw);
printf("Current power: %d uW\n", power_uw);
```

**典型功耗**:
| 状态 | 功耗 |
|------|------|
| ACTIVE | 10mW |
| SLEEP | 100uW |
| DEEP_SLEEP | 10uW |
| OFF | 1uW |

---

## ⚡ 异步操作 API

### 数据结构

```c
/* 异步操作类型 */
typedef enum {
    XY_DEVICE_ASYNC_OP_NONE = 0,
    XY_DEVICE_ASYNC_OP_READ,
    XY_DEVICE_ASYNC_OP_WRITE,
    XY_DEVICE_ASYNC_OP_IOCTL,
} xy_device_async_op_t;

/* 异步操作状态 */
typedef enum {
    XY_DEVICE_ASYNC_STATE_IDLE = 0,
    XY_DEVICE_ASYNC_STATE_PENDING,
    XY_DEVICE_ASYNC_STATE_COMPLETED,
    XY_DEVICE_ASYNC_STATE_ERROR,
} xy_device_async_state_t;

/* 回调函数 */
typedef void (*xy_device_async_callback_t)(
    xy_device_t *dev, 
    xy_device_async_op_t op,
    int result,
    void *user_data
);
```

### 初始化

```c
#include "inc/xy_device_async.h"

/* 初始化异步操作 */
xy_device_async_init(dev);
```

### 异步读取

```c
/* 定义回调函数 */
void read_callback(xy_device_t *dev, xy_device_async_op_t op,
                   int result, void *user_data)
{
    if (result >= 0) {
        printf("Read completed: %d bytes\n", result);
    } else {
        printf("Read failed: %d\n", result);
    }
}

/* 异步读取 */
uint8_t buffer[256];
xy_device_async_read(dev, buffer, sizeof(buffer),
                     read_callback, NULL, 1000);
```

### 异步写入

```c
void write_callback(xy_device_t *dev, xy_device_async_op_t op,
                    int result, void *user_data)
{
    if (result >= 0) {
        printf("Write completed: %d bytes\n", result);
    }
}

const char *data = "Hello Device!";
xy_device_async_write(dev, data, strlen(data),
                      write_callback, NULL, 1000);
```

### 轮询模式

```c
/* 启动异步读取 */
xy_device_async_read(dev, buffer, sizeof(buffer), NULL, NULL, 1000);

/* 轮询检查完成 */
while (xy_device_async_poll(dev) == 0) {
    /* 等待完成 */
}

/* 获取已传输字节数 */
size_t transferred;
xy_device_async_get_transferred(dev, &transferred);
```

### 等待模式

```c
/* 启动异步操作 */
xy_device_async_read(dev, buffer, sizeof(buffer), NULL, NULL, 1000);

/* 等待完成 (阻塞) */
xy_error_t ret = xy_device_async_wait(dev, 1000);
if (ret == XY_DEVICE_OK) {
    printf("Operation completed\n");
} else if (ret == XY_DEVICE_TIMEOUT) {
    printf("Operation timeout\n");
}
```

### 取消操作

```c
/* 取消异步操作 */
xy_device_async_cancel(dev);
```

### 就绪检查

```c
/* 检查是否可写 */
if (xy_device_async_ready(dev, true)) {
    /* 可以写入 */
}

/* 检查是否可读 */
if (xy_device_async_ready(dev, false)) {
    /* 可以读取 */
}
```

---

## 🔧 设备注册 API

### 设备结构

```c
typedef struct xy_device {
    const char *name;              /* 设备名称 */
    xy_dev_type_t type;            /* 设备类型 */
    uint32_t flags;                /* 设备标志 */
    xy_dev_state_t state;          /* 设备状态 */
    const xy_dev_api_t *api;       /* API 虚表 */
    const void *config;            /* 配置数据 */
    void *data;                    /* 运行时数据 */
    uint8_t ref_count;             /* 引用计数 */
} xy_device_t;
```

### 设备注册

```c
/* 静态注册宏 */
#define XY_DEVICE_DEFINE(name, type, init_fn, api_ptr, config_ptr) \
    static xy_device_t name = { \
        .name = #name, \
        .type = type, \
        .api = api_ptr, \
        .config = config_ptr, \
    }

/* 注册设备 */
XY_DEVICE_DEFINE(my_device, XY_DEV_TYPE_SENSOR, 
                 my_device_init, &my_device_api, &my_config);
```

### 设备查找

```c
/* 按名称查找 */
xy_device_t *dev = xy_device_find("I2C1");

/* 按类型查找 */
xy_device_t *dev = xy_device_find_by_type(XY_DEV_TYPE_I2C);

/* 遍历设备 */
xy_device_foreach(dev) {
    printf("Device: %s\n", dev->name);
}
```

---

## 📝 使用示例

### 完整示例：I2C 传感器

```c
#include "xy_device.h"
#include "inc/xy_device_pm.h"
#include "inc/xy_device_async.h"

/* 1. 查找设备 */
xy_device_t *dev = xy_device_find("I2C1");
if (!dev) {
    /* 错误处理 */
}

/* 2. 打开设备 */
xy_device_open(dev, XY_DEV_FLAG_RDWR);

/* 3. 配置电源管理 */
xy_device_pm_set_policy(dev, XY_DEVICE_PM_POLICY_AUTO);
xy_device_pm_set_idle_timeout(dev, 5000);

/* 4. 异步读取数据 */
uint8_t buffer[32];
xy_device_async_read(dev, buffer, sizeof(buffer),
                     read_callback, NULL, 100);

/* 5. 使用设备 */
xy_device_read(dev, 0, buffer, sizeof(buffer));

/* 6. 关闭设备 */
xy_device_close(dev);
```

---

## 🔗 相关文档

- [设备架构设计](DEVICE_ARCHITECTURE.md)
- [设计规范](DESIGN_SPEC.md)
- [电源管理实现报告](DEVICE_MODEL_PM_ASYNC_REPORT_2026-03-15.md)

---

**最后更新**: 2026-03-16  
**许可证**: Apache License 2.0
