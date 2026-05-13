# 驱动迁移指南

**版本**: 2.0.0  
**日期**: 2026-03-05

---

## 📋 概述

本文档说明如何将现有传感器驱动迁移到新的 Sensor 框架。

---

## 🏗️ 新架构

### 旧驱动结构

```c
/* 旧方式：每个驱动有自己的 API */
xy_aht20_t aht20;
xy_aht20_init(&aht20, i2c_handle);
xy_aht20_read(&aht20);
xy_aht20_get_temperature(&aht20, &temp);
```

### 新驱动结构

```c
/* 新方式：统一 Sensor API */
xy_sensor_device_t *sensor = xy_sensor_device_get("AHT20");
xy_sensor_init(sensor);
xy_sensor_sample_fetch(sensor, XY_SENSOR_CHAN_ALL);
xy_sensor_channel_get(sensor, XY_SENSOR_CHAN_AMBIENT_TEMP, &val);
```

---

## 🔧 迁移步骤

### 步骤 1: 创建驱动结构

```c
/* 1. 定义私有数据 */
typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    xy_sensor_value_t temp_data;
    xy_sensor_value_t humidity_data;
} aht20_private_data_t;

/* 2. 定义设备实例 */
static aht20_private_data_t aht20_priv_data;
static xy_sensor_device_t aht20_device = {
    .name = "AHT20",
    .type = XY_SENSOR_TYPE_COMBO,
    .data = &aht20_priv_data,
    .bus = {0},
};
```

---

### 步骤 2: 实现驱动 API

```c
/* 1. 初始化 */
static int aht20_init(xy_sensor_device_t *dev)
{
    aht20_private_data_t *priv = (aht20_private_data_t *)dev->data;
    /* 硬件初始化代码 */
    priv->initialized = true;
    return 0;
}

/* 2. 采样获取 */
static int aht20_sample_fetch(xy_sensor_device_t *dev, xy_sensor_channel_t channel)
{
    aht20_private_data_t *priv = (aht20_private_data_t *)dev->data;
    /* 读取传感器数据 */
    /* 存储到 priv->temp_data, priv->humidity_data */
    return 0;
}

/* 3. 通道数据获取 */
static int aht20_channel_get(xy_sensor_device_t *dev, 
                             xy_sensor_channel_t channel, 
                             xy_sensor_value_t *val)
{
    aht20_private_data_t *priv = (aht20_private_data_t *)dev->data;
    switch (channel) {
        case XY_SENSOR_CHAN_AMBIENT_TEMP:
            *val = priv->temp_data;
            break;
        case XY_SENSOR_CHAN_HUMIDITY:
            *val = priv->humidity_data;
            break;
    }
    return 0;
}

/* 4. 定义驱动 API */
static const xy_sensor_driver_api_t aht20_driver_api = {
    .init = aht20_init,
    .sample_fetch = aht20_sample_fetch,
    .channel_get = aht20_channel_get,
    .attr_set = NULL,
    .trigger_set = NULL,
};
```

---

### 步骤 3: 注册设备

```c
int xy_sensor_aht20_register(void *i2c_handle, uint8_t addr)
{
    aht20_private_data_t *priv = &aht20_priv_data;
    
    /* 配置总线 */
    xy_sensor_bus_config_i2c(&priv->bus, i2c_handle, addr);
    
    /* 注册设备 */
    return xy_sensor_device_register(&aht20_device);
}
```

---

## 📊 已迁移驱动

### 温湿度传感器 (4 个)

| 驱动 | 状态 | 文件 |
|------|------|------|
| **AHT20** | ✅ | `drivers/temperature/xy_sensor_aht20.c` |
| **SHT30** | ✅ | `drivers/temperature/xy_sensor_sht30.c` |
| **SHT40** | ⏳ | 待迁移 |
| **HDC1080** | ⏳ | 待迁移 |

### 压力传感器 (2 个)

| 驱动 | 状态 | 文件 |
|------|------|------|
| **BMP280** | ✅ | `drivers/pressure/xy_sensor_bmp280.c` |
| **BME280** | ⏳ | 待迁移 |

### 运动传感器 (3 个)

| 驱动 | 状态 | 文件 |
|------|------|------|
| **MPU6050** | ✅ | `drivers/motion/xy_sensor_mpu6050.c` |
| **ADXL362** | ⏳ | 待迁移 |
| **ICM20608** | ⏳ | 待迁移 |

---

## 🎯 迁移优先级

### 高优先级 (🔴)

1. **AHT20/SHT30/SHT40** - 温湿度传感器 (最常用)
2. **BMP280/BME280** - 压力传感器
3. **MPU6050** - 6 轴运动传感器

### 中优先级 (🟡)

1. **BH1750/TSL2561** - 光线传感器
2. **ADXL362/LIS2DH12** - 加速度计
3. **INA226/BQ25620** - 电源传感器

### 低优先级 (🟢)

1. **CCS811** - 气体传感器
2. **VL53L0X** - 距离传感器
3. **APDS9960** - 手势传感器

---

## 📚 参考示例

### 完整驱动模板

```c
#include "xy_sensor.h"
#include "xy_log.h"

/* 私有数据 */
typedef struct {
    xy_sensor_bus_t bus;
    bool initialized;
    xy_sensor_value_t data[XY_SENSOR_CHAN_COUNT];
} xxx_private_data_t;

/* 初始化 */
static int xxx_init(xy_sensor_device_t *dev)
{
    xxx_private_data_t *priv = dev->data;
    /* 硬件初始化 */
    priv->initialized = true;
    return 0;
}

/* 采样获取 */
static int xxx_sample_fetch(xy_sensor_device_t *dev, xy_sensor_channel_t channel)
{
    xxx_private_data_t *priv = dev->data;
    /* 读取数据 */
    return 0;
}

/* 通道数据获取 */
static int xxx_channel_get(xy_sensor_device_t *dev, 
                           xy_sensor_channel_t channel, 
                           xy_sensor_value_t *val)
{
    xxx_private_data_t *priv = dev->data;
    *val = priv->data[channel];
    return 0;
}

/* 驱动 API */
static const xy_sensor_driver_api_t xxx_driver_api = {
    .init = xxx_init,
    .sample_fetch = xxx_sample_fetch,
    .channel_get = xxx_channel_get,
};

/* 设备实例 */
static xxx_private_data_t xxx_priv;
static xy_sensor_device_t xxx_device = {
    .name = "XXX",
    .type = XY_SENSOR_TYPE_XXX,
    .api = &xxx_driver_api,
    .data = &xxx_priv,
};

/* 注册函数 */
int xy_sensor_xxx_register(void *i2c_handle, uint8_t addr)
{
    xy_sensor_bus_config_i2c(&xxx_priv.bus, i2c_handle, addr);
    return xy_sensor_device_register(&xxx_device);
}
```

---

## ✅ 迁移检查清单

- [ ] 定义私有数据结构
- [ ] 实现 init() 函数
- [ ] 实现 sample_fetch() 函数
- [ ] 实现 channel_get() 函数
- [ ] 定义驱动 API 结构
- [ ] 创建设备实例
- [ ] 实现注册函数
- [ ] 测试轮询模式
- [ ] 测试触发模式 (可选)
- [ ] 更新文档

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
