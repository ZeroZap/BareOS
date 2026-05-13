# Sensor 组件优化方案

**日期**: 2026-03-05  
**参考**: Zephyr Sensor Framework

---

## 📊 当前问题分析

### 现有架构

```
sensor/
├── inc/                    # 驱动头文件 (40+ 个)
├── sensors/                # 驱动实现 (40+ 个)
├── sensor.h                # 主接口
├── sensor_core.h/c         # 核心实现
├── sensor_type.h           # 类型定义
└── 杂项文件 (15+ 个)
    ├── sensor_calibration.c
    ├── sensor_dma.c
    ├── sensor_fifo.c
    ├── sensor_filter.c
    ├── sensor_fusion.c
    ├── sensor_interrupt.c
    ├── sensor_power.c
    └── ...
```

### 问题点

1. **接口不统一** - 每个驱动有自己的 API
2. **缺少设备模型** - 没有统一的 device 抽象
3. **通道抽象缺失** - 没有标准化的数据通道
4. **触发机制不完善** - 中断/阈值处理分散
5. **配置分散** - 没有统一的配置管理

---

## ✅ 优化方案 (自主设计)

### 新架构设计

```
sensor/
├── inc/
│   ├── xy_sensor.h         # 统一 Sensor API ⭐
│   ├── xy_sensor_device.h  # 设备模型
│   ├── xy_sensor_channel.h # 通道抽象
│   ├── xy_sensor_trigger.h # 触发机制
│   └── xy_sensor_attr.h    # 属性配置
│
├── core/
│   ├── sensor_core.c       # 核心实现
│   ├── sensor_bus.c        # 总线抽象 (I2C/SPI)
│   ├── sensor_trigger.c    # 触发子系统
│   └── sensor_power.c      # 电源管理
│
├── drivers/                # 驱动层
│   ├── temperature/
│   │   ├── sensor_aht20.c
│   │   ├── sensor_sht30.c
│   │   └── sensor_sht40.c
│   ├── pressure/
│   │   ├── sensor_bmp280.c
│   │   └── sensor_bme280.c
│   ├── motion/
│   │   ├── sensor_mpu6050.c
│   │   └── sensor_adxl362.c
│   └── light/
│       ├── sensor_bh1750.c
│       └── sensor_tsl2561.c
│
└── examples/
    └── sensor_demo.c
```

---

## 🎯 核心设计

### 1. 统一 Sensor API

```c
/**
 * @brief 传感器设备句柄
 */
typedef struct xy_sensor_device {
    const char *name;                   // 设备名称
    xy_sensor_type_t type;              // 传感器类型
    const xy_sensor_driver_api *api;    // 驱动接口
    void *data;                         // 私有数据
    xy_sensor_bus_t bus;                // 总线信息
} xy_sensor_device_t;

/**
 * @brief 传感器驱动 API (统一接口)
 */
typedef struct {
    int (*init)(xy_sensor_device_t *dev);
    int (*sample_fetch)(xy_sensor_device_t *dev, xy_sensor_channel_t channel);
    int (*channel_get)(xy_sensor_device_t *dev, 
                       xy_sensor_channel_t channel, 
                       xy_sensor_value_t *val);
    int (*attr_set)(xy_sensor_device_t *dev,
                    xy_sensor_channel_t channel,
                    xy_sensor_attr_t attr,
                    const xy_sensor_value_t *val);
    int (*trigger_set)(xy_sensor_device_t *dev,
                       const xy_sensor_trigger_t *trigger);
} xy_sensor_driver_api_t;
```

### 2. 通道抽象

```c
/**
 * @brief 传感器通道类型 (标准化)
 */
typedef enum {
    /* 温度 */
    XY_SENSOR_CHAN_AMBIENT_TEMP,    // 环境温度
    XY_SENSOR_CHAN_DIE_TEMP,        // 芯片温度
    
    /* 湿度 */
    XY_SENSOR_CHAN_HUMIDITY,        // 相对湿度
    
    /* 压力 */
    XY_SENSOR_CHAN_PRESSURE,        // 气压
    
    /* 加速度 */
    XY_SENSOR_CHAN_ACCEL_X,         // X 轴加速度
    XY_SENSOR_CHAN_ACCEL_Y,         // Y 轴加速度
    XY_SENSOR_CHAN_ACCEL_Z,         // Z 轴加速度
    
    /* 角速度 */
    XY_SENSOR_CHAN_GYRO_X,          // X 轴角速度
    XY_SENSOR_CHAN_GYRO_Y,          // Y 轴角速度
    XY_SENSOR_CHAN_GYRO_Z,          // Z 轴角速度
    
    /* 磁场 */
    XY_SENSOR_CHAN_MAGN_X,          // X 轴磁场
    XY_SENSOR_CHAN_MAGN_Y,          // Y 轴磁场
    XY_SENSOR_CHAN_MAGN_Z,          // Z 轴磁场
    
    /* 光线 */
    XY_SENSOR_CHAN_LIGHT,           // 环境光强度
    XY_SENSOR_CHAN_PROX,            // 接近感应
    
    /* 气体 */
    XY_SENSOR_CHAN_CO2,             // CO2 浓度
    XY_SENSOR_CHAN_VOC,             // VOC 浓度
    
    /* 电源 */
    XY_SENSOR_CHAN_VOLTAGE,         // 电压
    XY_SENSOR_CHAN_CURRENT,         // 电流
    
    /* 特殊通道 */
    XY_SENSOR_CHAN_ALL,             // 所有通道
} xy_sensor_channel_t;
```

### 3. 数据值表示

```c
/**
 * @brief 传感器值 (定点数表示，参考 Zephyr)
 * 
 * 值 = val1 + val2 / 1000000
 * 例如：25.5°C = {25, 500000}
 */
typedef struct {
    int32_t val1;   // 整数部分
    int32_t val2;   // 小数部分 (微单位)
} xy_sensor_value_t;

/* 便捷转换宏 */
#define XY_SENSOR_VALUE_SET(val, v1, v2)  \
    do {                                  \
        (val)->val1 = (v1);               \
        (val)->val2 = (v2);               \
    } while(0)

#define XY_SENSOR_VALUE_TO_FLOAT(val)     \
    ((float)((val).val1 + (val).val2 / 1000000.0f))
```

### 4. 触发机制

```c
/**
 * @brief 触发器类型
 */
typedef enum {
    XY_SENSOR_TRIG_DATA_READY,      // 数据就绪
    XY_SENSOR_TRIG_THRESHOLD,       // 阈值触发
    XY_SENSOR_TRIG_TAP,             // 敲击检测
    XY_SENSOR_TRIG_MOTION,          // 运动检测
    XY_SENSOR_TRIG_FIFO_FULL,       // FIFO 满
    XY_SENSOR_TRIG_FIFO_WATERMARK,  // FIFO 水位
} xy_sensor_trigger_type_t;

/**
 * @brief 触发器配置
 */
typedef struct {
    xy_sensor_trigger_type_t type;  // 触发类型
    xy_sensor_channel_t channel;    // 关联通道
    void (*trigger_handler)(xy_sensor_device_t *dev, 
                            const xy_sensor_trigger_t *trigger);
} xy_sensor_trigger_t;
```

### 5. 属性配置

```c
/**
 * @brief 传感器属性
 */
typedef enum {
    XY_SENSOR_ATTR_SAMPLING_FREQUENCY,  // 采样频率
    XY_SENSOR_ATTR_LOWER_THRESH,        // 下阈值
    XY_SENSOR_ATTR_UPPER_THRESH,        // 上阈值
    XY_SENSOR_ATTR_SLOPE_THRESH,        // 斜率阈值
    XY_SENSOR_ATTR_DURATION,            // 持续时间
    XY_SENSOR_ATTR_FULL_SCALE,          // 满量程
    XY_SENSOR_ATTR_OFFSET,              // 偏移量
    XY_SENSOR_ATTR_POWER_MODE,          // 电源模式
} xy_sensor_attr_t;
```

---

## 🔧 使用示例

### 示例 1: 轮询模式

```c
#include "xy_sensor.h"

// 初始化传感器
xy_sensor_device_t *sensor = xy_sensor_device_get("AHT20");
xy_sensor_init(sensor);

struct sensor_value temp, humidity;

while (1) {
    // 获取最新采样
    xy_sensor_sample_fetch(sensor, XY_SENSOR_CHAN_ALL);
    
    // 读取数据
    xy_sensor_channel_get(sensor, XY_SENSOR_CHAN_AMBIENT_TEMP, &temp);
    xy_sensor_channel_get(sensor, XY_SENSOR_CHAN_HUMIDITY, &humidity);
    
    // 转换为单位值
    float temp_c = XY_SENSOR_VALUE_TO_FLOAT(temp);
    float humidity_pct = XY_SENSOR_VALUE_TO_FLOAT(humidity);
    
    printf("T: %.2f°C, H: %.2f%%\n", temp_c, humidity_pct);
    
    xy_os_delay(1000);
}
```

### 示例 2: 触发模式

```c
#include "xy_sensor.h"

// 触发回调
void motion_trigger(xy_sensor_device_t *dev, 
                    const xy_sensor_trigger_t *trigger) {
    xy_sensor_value_t val;
    xy_sensor_channel_get(dev, XY_SENSOR_CHAN_ACCEL_X, &val);
    
    float accel = XY_SENSOR_VALUE_TO_FLOAT(val);
    xy_log_i("Motion detected! Accel: %.2f\n", accel);
}

// 设置触发器
xy_sensor_trigger_t trigger = {
    .type = XY_SENSOR_TRIG_MOTION,
    .channel = XY_SENSOR_CHAN_ACCEL_X,
    .trigger_handler = motion_trigger,
};

xy_sensor_trigger_set(mpu6050_dev, &trigger);
```

### 示例 3: 属性配置

```c
#include "xy_sensor.h"

// 设置采样频率
xy_sensor_value_t freq = {100, 0};  // 100Hz
xy_sensor_attr_set(bmp280_dev, 
                   XY_SENSOR_CHAN_PRESSURE,
                   XY_SENSOR_ATTR_SAMPLING_FREQUENCY,
                   &freq);

// 设置阈值
xy_sensor_value_t thresh = {30, 0};  // 30°C
xy_sensor_attr_set(sht30_dev,
                   XY_SENSOR_CHAN_AMBIENT_TEMP,
                   XY_SENSOR_ATTR_UPPER_THRESH,
                   &thresh);
```

---

## 📊 迁移计划

### 阶段 1: 核心框架 (4 小时)

- [ ] xy_sensor.h - 统一 API
- [ ] xy_sensor_device.h - 设备模型
- [ ] xy_sensor_channel.h - 通道抽象
- [ ] sensor_core.c - 核心实现

### 阶段 2: 驱动迁移 (8 小时)

- [ ] AHT20/SHT30/SHT40 迁移
- [ ] BMP280/BME280 迁移
- [ ] MPU6050/ADXL362 迁移
- [ ] BH1750/TSL2561 迁移

### 阶段 3: 触发子系统 (4 小时)

- [ ] sensor_trigger.c - 触发核心
- [ ] 中断处理
- [ ] 阈值检测

### 阶段 4: 电源管理 (2 小时)

- [ ] sensor_power.c - 电源管理
- [ ] 低功耗模式
- [ ] 自动唤醒

---

## ✅ 优势对比

| 特性 | 当前架构 | 优化后 | 改善 |
|------|---------|--------|------|
| **统一 API** | ❌ | ✅ | ✅ |
| **设备模型** | ❌ | ✅ | ✅ |
| **通道抽象** | ❌ | ✅ | ✅ |
| **触发机制** | ⚠️ | ✅ | ✅ |
| **电源管理** | ⚠️ | ✅ | ✅ |
| **驱动数量** | 40+ | 40+ | 兼容 |
| **代码复用** | 低 | 高 | ✅ |

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
