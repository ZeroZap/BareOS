# Sensor 框架使用指南

**版本**: 1.0.0  
**日期**: 2026-03-05

---

## 📋 概述

XY_Sensor 是一个参考 Zephyr Sensor 框架设计的统一传感器抽象层，提供标准化的传感器 API。

### 特性

- ✅ 统一 API (所有传感器使用相同接口)
- ✅ 通道抽象 (80+ 种标准化通道)
- ✅ 设备模型 (基于设备句柄)
- ✅ 触发机制 (中断/阈值/FIFO)
- ✅ 电源管理 (睡眠/唤醒/低功耗)
- ✅ 总线抽象 (I2C/SPI 统一接口)

---

## 🚀 快速开始

### 1. 基础使用 (轮询模式)

```c
#include "xy_sensor.h"

int main(void)
{
    /* 获取传感器 */
    xy_sensor_device_t *sensor = xy_sensor_device_get("AHT20");
    
    /* 初始化 */
    xy_sensor_init(sensor);
    
    xy_sensor_value_t temp, humidity;
    
    while (1) {
        /* 获取采样 */
        xy_sensor_sample_fetch(sensor, XY_SENSOR_CHAN_ALL);
        
        /* 读取数据 */
        xy_sensor_channel_get(sensor, XY_SENSOR_CHAN_AMBIENT_TEMP, &temp);
        xy_sensor_channel_get(sensor, XY_SENSOR_CHAN_HUMIDITY, &humidity);
        
        /* 转换为单位值 */
        float temp_c = XY_SENSOR_VALUE_TO_FLOAT(temp);
        float humidity_pct = XY_SENSOR_VALUE_TO_FLOAT(humidity);
        
        printf("T: %.2f°C, H: %.2f%%\n", temp_c, humidity_pct);
        
        xy_os_delay(1000);
    }
}
```

---

### 2. 触发模式

```c
#include "xy_sensor.h"

/* 触发回调 */
void motion_callback(xy_sensor_device_t *dev, 
                     const xy_sensor_trigger_t *trigger)
{
    xy_sensor_value_t val;
    xy_sensor_channel_get(dev, XY_SENSOR_CHAN_ACCEL_X, &val);
    
    float accel = XY_SENSOR_VALUE_TO_FLOAT(val);
    printf("Motion! Accel: %.2f\n", accel);
}

int main(void)
{
    xy_sensor_device_t *mpu = xy_sensor_device_get("MPU6050");
    xy_sensor_init(mpu);
    
    /* 配置触发器 */
    xy_sensor_trigger_t trigger = {
        .type = XY_SENSOR_TRIG_MOTION,
        .channel = XY_SENSOR_CHAN_ACCEL_X,
        .trigger_handler = motion_callback,
    };
    
    xy_sensor_trigger_set(mpu, &trigger);
    
    /* 主循环 */
    while (1) {
        xy_os_delay(1000);
    }
}
```

---

### 3. 阈值告警

```c
#include "xy_sensor.h"

void temp_alarm(xy_sensor_device_t *dev, 
                const xy_sensor_trigger_t *trigger)
{
    xy_log_w("Temperature alarm!\n");
}

int main(void)
{
    xy_sensor_device_t *sensor = xy_sensor_device_get("SHT30");
    xy_sensor_init(sensor);
    
    /* 配置阈值 */
    xy_sensor_thresh_config_t thresh = {
        .lower = {20, 0},       /* 20°C */
        .upper = {35, 0},       /* 35°C */
        .duration = 1000,       /* 1 秒 */
    };
    
    xy_sensor_trigger_set_thresh(sensor, 
                                 XY_SENSOR_CHAN_AMBIENT_TEMP, 
                                 &thresh);
    
    /* 配置触发器 */
    xy_sensor_trigger_t trigger = {
        .type = XY_SENSOR_TRIG_THRESHOLD,
        .trigger_handler = temp_alarm,
    };
    
    xy_sensor_trigger_set(sensor, &trigger);
    
    while (1) {
        xy_os_delay(1000);
    }
}
```

---

### 4. 低功耗模式

```c
#include "xy_sensor.h"

int main(void)
{
    xy_sensor_device_t *sensor = xy_sensor_device_get("AHT20");
    xy_sensor_init(sensor);
    
    while (1) {
        /* 唤醒 */
        xy_sensor_wakeup(sensor);
        
        /* 读取数据 */
        xy_sensor_sample_fetch(sensor, XY_SENSOR_CHAN_ALL);
        
        xy_sensor_value_t temp;
        xy_sensor_channel_get(sensor, XY_SENSOR_CHAN_AMBIENT_TEMP, &temp);
        
        float temp_c = XY_SENSOR_VALUE_TO_FLOAT(temp);
        printf("T: %.2f°C\n", temp_c);
        
        /* 睡眠 */
        xy_sensor_sleep(sensor);
        
        /* 等待 10 秒 */
        xy_os_delay(10000);
    }
}
```

---

## 📖 API 参考

### 设备管理

| 函数 | 说明 |
|------|------|
| `xy_sensor_device_get(name)` | 根据名称获取设备 |
| `xy_sensor_device_get_by_type(type, index)` | 根据类型获取设备 |
| `xy_sensor_device_count()` | 获取传感器数量 |
| `xy_sensor_device_foreach(cb, ud)` | 遍历所有设备 |

### 核心 API

| 函数 | 说明 |
|------|------|
| `xy_sensor_init(dev)` | 初始化传感器 |
| `xy_sensor_deinit(dev)` | 反初始化 |
| `xy_sensor_sample_fetch(dev, channel)` | 获取采样 |
| `xy_sensor_channel_get(dev, channel, val)` | 读取数据 |
| `xy_sensor_attr_set(dev, channel, attr, val)` | 设置属性 |

### 触发器

| 函数 | 说明 |
|------|------|
| `xy_sensor_trigger_set(dev, trigger)` | 设置触发器 |
| `xy_sensor_trigger_unset(dev, type)` | 禁用触发器 |
| `xy_sensor_trigger_set_thresh(dev, ch, cfg)` | 设置阈值 |
| `xy_sensor_trigger_set_fifo(dev, cfg)` | 设置 FIFO |

### 电源管理

| 函数 | 说明 |
|------|------|
| `xy_sensor_set_power_mode(dev, mode)` | 设置电源模式 |
| `xy_sensor_sleep(dev)` | 进入睡眠 |
| `xy_sensor_wakeup(dev)` | 唤醒 |
| `xy_sensor_low_power(dev)` | 低功耗模式 |

---

## 📊 通道类型

### 环境传感器

| 通道 | 说明 | 单位 |
|------|------|------|
| `XY_SENSOR_CHAN_AMBIENT_TEMP` | 环境温度 | °C |
| `XY_SENSOR_CHAN_HUMIDITY` | 相对湿度 | % |
| `XY_SENSOR_CHAN_PRESSURE` | 气压 | kPa |

### 运动传感器

| 通道 | 说明 | 单位 |
|------|------|------|
| `XY_SENSOR_CHAN_ACCEL_X/Y/Z` | 加速度 | mG |
| `XY_SENSOR_CHAN_GYRO_X/Y/Z` | 角速度 | mdps |
| `XY_SENSOR_CHAN_MAGN_X/Y/Z` | 磁场 | mGauss |

### 光线传感器

| 通道 | 说明 | 单位 |
|------|------|------|
| `XY_SENSOR_CHAN_LIGHT` | 环境光 | lux |
| `XY_SENSOR_CHAN_PROX` | 接近感应 | cm |
| `XY_SENSOR_CHAN_COLOR_RED/GREEN/BLUE` | RGB 颜色 | - |

---

## 🔧 数据值表示

```c
/* 定点数：val1 + val2/1000000 */
typedef struct {
    int32_t val1;   /* 整数部分 */
    int32_t val2;   /* 小数部分 (微单位) */
} xy_sensor_value_t;

/* 示例：25.5°C = {25, 500000} */
xy_sensor_value_t temp = {25, 500000};
float temp_c = XY_SENSOR_VALUE_TO_FLOAT(temp);  /* 25.5 */

/* 从浮点数设置 */
XY_SENSOR_VALUE_FROM_FLOAT(&val, 25.5);  /* val = {25, 500000} */
```

---

## ⚠️ 注意事项

### 线程安全

- 本库**不是**线程安全的
- 多线程环境下需要自行加锁

### 初始化顺序

```c
/* 1. 注册设备 (驱动层完成) */
xy_sensor_device_register(dev);

/* 2. 获取设备 */
xy_sensor_device_t *sensor = xy_sensor_device_get("AHT20");

/* 3. 初始化 */
xy_sensor_init(sensor);

/* 4. 使用 */
xy_sensor_sample_fetch(sensor, XY_SENSOR_CHAN_ALL);
```

### 错误处理

```c
xy_sensor_status_t ret = xy_sensor_init(sensor);
if (ret != XY_SENSOR_OK) {
    xy_log_e("Init failed: %d\n", ret);
    return;
}
```

---

## 📚 相关文档

- [Sensor 优化计划](SENSOR_OPTIMIZATION_PLAN.md)
- [Zephyr Sensor API](https://docs.zephyrproject.org/latest/reference/peripherals/sensor.html)

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
