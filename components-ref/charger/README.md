# XinYi Charger Component - 充电器管理组件

**版本**: 1.0.0
**日期**: 2026-03-17
**维护者**: XinYi Team

---

## 📖 概述

XinYi Charger 组件提供统一的充电器管理框架，支持多种充电管理芯片。

### 核心特性

- ✅ 统一的充电器 API
- ✅ 支持 I2C 接口充电芯片
- ✅ 充电状态监控
- ✅ 故障检测和报告
- ✅ 充电参数配置

---

## 🎯 支持的充电芯片

| 型号        | 厂商 | 类型             | 状态    |
| ----------- | ---- | ---------------- | ------- |
| **BQ25620** | TI   | 1 节 Li-Ion, I2C | ✅ 完成 |
| BQ2561x     | TI   | 1 节 Li-Ion, I2C | 📋 计划 |
| MP2615      | MPS  | 1-2 节 Li-Ion    | 📋 计划 |

---

## 🏗️ 架构设计

```
┌─────────────────────────────────┐
│      应用层 (Application)        │
│  xy_charger_start/stop/get_status│
└───────────────┬─────────────────┘
                │ 调用
┌───────────────▼─────────────────┐
│    Charger 组件 (统一 API)        │
│  xy_charger_t                   │
│  - xy_charger_init()            │
│  - xy_charger_start()           │
│  - xy_charger_stop()            │
│  - xy_charger_get_status()      │
└───────────────┬─────────────────┘
                │ 使用
┌───────────────▼─────────────────┐
│   芯片特定驱动 (BQ25620 等)       │
│  xy_bq25620_t                   │
│  - I2C 寄存器操作                │
│  - 状态解析                      │
│  - 参数配置                      │
└───────────────┬─────────────────┘
                │ 访问
┌───────────────▼─────────────────┐
│      HAL 层 (I2C)                │
│  xy_hal_i2c_master_transmit()   │
│  xy_hal_i2c_master_receive()    │
└─────────────────────────────────┘
```

---

## 🚀 快速开始

### 1. 初始化 BQ25620

```c
#include "xy_bq25620.h"
#include "xy_hal_i2c.h"

/* I2C 句柄 (需要先初始化) */
xy_hal_i2c_t i2c = xy_hal_i2c_bind("I2C1");

/* BQ25620 设备 */
xy_bq25620_t bq25620;

/* 初始化 */
int ret = xy_bq25620_init(&bq25620, i2c, 0x6A);
if (ret != XY_DEVICE_OK) {
    /* 初始化失败 */
    return ret;
}

/* 验证设备 ID */
uint8_t dev_id;
xy_bq25620_get_device_id(&bq25620, &dev_id);
printf("BQ25620 Device ID: 0x%02X\n", dev_id);
```

### 2. 配置充电参数

```c
/* 充电配置 */
xy_charger_config_t config = {
    .input_current_limit = 2000,    /* 输入电流限制 2A */
    .charge_current = 1000,         /* 充电电流 1A */
    .charge_voltage = 4200,         /* 充电电压 4.2V */
    .precharge_current = 128,       /* 预充电电流 128mA */
    .termination_current = 128,     /* 终止电流 128mA */
    .recharge_threshold = 100,      /* 再充电阈值 100mV */
    .auto_recharge = true,          /* 自动再充电使能 */
};

/* 应用配置 */
xy_charger_init(&bq25620.base, &config);
```

### 3. 启动充电

```c
/* 启动充电 */
xy_bq25620_start_charge(&bq25620);

/* 或者使用统一 API */
xy_charger_start(&bq25620.base);
```

### 4. 监控充电状态

```c
xy_charger_status_t status;

/* 定期读取状态 */
while (1) {
    xy_bq25620_get_status(&bq25620, &status);

    printf("充电状态：%d\n", status.state);
    printf("故障：%d\n", status.fault);
    printf("Power Good: %d\n", status.power_good);
    printf("充电中：%d\n", status.charging);
    printf("充电完成：%d\n", status.done);

    if (status.done) {
        printf("充电完成!\n");
        break;
    }

    if (status.fault != XY_CHARGER_FAULT_NONE) {
        printf("充电故障!\n");
        break;
    }

    xy_os_delay(1000);
}
```

### 5. 停止充电

```c
/* 停止充电 */
xy_bq25620_stop_charge(&bq25620);

/* 或者使用统一 API */
xy_charger_stop(&bq25620.base);
```

---

## 📋 API 参考

### 初始化/反初始化

```c
int xy_bq25620_init(xy_bq25620_t *dev, void *i2c_handle, uint8_t i2c_addr);
int xy_bq25620_deinit(xy_bq25620_t *dev);
```

### 充电控制

```c
int xy_bq25620_start_charge(xy_bq25620_t *dev);
int xy_bq25620_stop_charge(xy_bq25620_t *dev);
int xy_charger_start(xy_charger_t *charger);
int xy_charger_stop(xy_charger_t *charger);
```

### 参数配置

```c
int xy_bq25620_set_charge_current(xy_bq25620_t *dev, uint32_t current_mA);
int xy_bq25620_set_charge_voltage(xy_bq25620_t *dev, uint32_t voltage_mV);
int xy_bq25620_set_input_limit(xy_bq25620_t *dev, uint32_t current_mA);
```

### 状态读取

```c
int xy_bq25620_get_status(xy_bq25620_t *dev, xy_charger_status_t *status);
int xy_bq25620_get_device_id(xy_bq25620_t *dev, uint8_t *id);
```

### 寄存器操作

```c
int xy_bq25620_read_reg(xy_bq25620_t *dev, uint8_t reg, uint8_t *value);
int xy_bq25620_write_reg(xy_bq25620_t *dev, uint8_t reg, uint8_t value);
```

---

## 📊 BQ25620 参数范围

| 参数             | 最小值 | 最大值 | 步长  | 默认值 |
| ---------------- | ------ | ------ | ----- | ------ |
| **充电电流**     | 64mA   | 5056mA | 64mA  | 512mA  |
| **充电电压**     | 3500mV | 4470mV | 10mV  | 4200mV |
| **输入电流限制** | 100mA  | 6300mA | 100mA | 500mA  |
| **预充电电流**   | 64mA   | 960mA  | 64mA  | 128mA  |
| **终止电流**     | 64mA   | 960mA  | 64mA  | 128mA  |
| **再充电阈值**   | 100mV  | 300mV  | 100mV | 100mV  |

---

## ⚠️ 注意事项

### 硬件连接

- **I2C 地址**: 0x6A (固定)
- **I2C 速度**: 标准模式 (100kHz) 或快速模式 (400kHz)
- **TS 引脚**: 电池温度监测 (可选)
- **CE 引脚**: 充电使能 (可选，默认内部上拉)

### 安全提示

1. **电池类型**: 仅支持 1 节 Li-Ion/Li-Po 电池
2. **充电电压**: 默认 4.2V，不要超过电池规格
3. **充电电流**: 根据电池容量设置 (推荐 0.5C-1C)
4. **温度保护**: 启用 TS 引脚监测 (如果可用)
5. **散热**: 大电流充电时注意散热

### 故障处理

| 故障     | 可能原因         | 解决方法               |
| -------- | ---------------- | ---------------------- |
| 输入过压 | 输入电压 > 13.5V | 检查输入电源           |
| 过热     | 芯片温度过高     | 改善散热，降低充电电流 |
| 充电超时 | 充电时间过长     | 检查电池连接，更换电池 |
| 电池过压 | 电池电压异常     | 检查电池，停止充电     |

---

## 🔗 相关文档

- [充电器框架 API](inc/xy_charger.h)
- [BQ25620 数据手册](https://www.ti.com/product/BQ25620)
- [HAL I2C 使用指南](../hal/README.md)

---

## 📝 开发日志

### 2026-03-17

- ✅ 创建 Charger 组件目录
- ✅ 实现 BQ25620 驱动
- ✅ 统一 Charger API
- ✅ 添加文档和示例

---

**维护者**: XinYi Team
**许可证**: Apache License 2.0
**最后更新**: 2026-03-17
