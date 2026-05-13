# 组件架构优化方案

**日期**: 2026-03-05  
**参考**: Zephyr Project 架构

---

## 📊 Zephyr 架构分析

### Zephyr 驱动组织方式

```
zephyr/drivers/
├── sensor/              # 传感器驱动
│   ├── bosch/          # 按厂商组织
│   ├── st/
│   ├── tdk/
│   └── ... (85+ 厂商)
├── fuel_gauge/          # 电量计驱动 (独立!)
│   ├── max17043.c
│   ├── max17048.c
│   ├── bq27z561.c
│   └── ...
├── adc/                 # ADC 驱动
├── dac/                 # DAC 驱动
└── ...
```

### 关键发现

1. **电量计独立** - fuel_gauge 与 sensor 平级
2. **按厂商组织** - 驱动按厂商分类 (bosch/st/tdk 等)
3. **统一 API** - 所有传感器使用相同 API
4. **触发机制** - 支持中断/轮询模式

---

## ✅ 优化方案

### 新架构设计

```
components/
├── sensor/              # 传感器组件
│   ├── inc/
│   │   └── xy_sensor.h  # 统一 Sensor API
│   ├── core/
│   │   ├── sensor_core.c
│   │   ├── sensor_bus.c
│   │   └── ...
│   └── drivers/
│       ├── temperature/  # 温湿度驱动
│       ├── pressure/     # 压力驱动
│       ├── motion/       # 运动驱动
│       ├── light/        # 光线驱动
│       └── ...
│
├── fuel_gauge/          # 电量计组件 (新!)
│   ├── inc/
│   │   └── xy_fuel_gauge.h
│   ├── core/
│   │   └── fuel_gauge_core.c
│   └── drivers/
│       ├── max17043.c
│       ├── bq27z561.c
│       └── ...
│
└── pm/                  # 电源管理
    ├── battery/         # 电池管理
    └── charger/         # 充电管理
```

---

## 🎯 分离原因

### 电量计 vs 传感器

| 特性 | 传感器 | 电量计 |
|------|--------|--------|
| **数据类型** | 环境数据 (温/湿/压) | 电池数据 (电压/电流/SOC) |
| **采样频率** | 定期采样 (秒级) | 持续监测 (毫秒级) |
| **精度要求** | 中等 | 高精度 |
| **安全关键** | 低 | 高 (电池安全) |
| **校准需求** | 简单 | 复杂 (库仑计数) |
| **电源管理** | 被动 | 主动 (充放电控制) |

### Zephyr 设计理由

1. **安全隔离** - 电量计涉及电池安全，需要独立管理
2. **精度要求** - 电量计需要高精度库仑计数
3. **复杂算法** - SOC 计算需要复杂模型
4. **实时性** - 电池监测需要更高实时性

---

## 📋 执行计划

### 阶段 1: 创建 fuel_gauge 组件

```bash
components/fuel_gauge/
├── inc/
│   └── xy_fuel_gauge.h
├── core/
│   └── fuel_gauge_core.c
└── drivers/
    ├── xy_fg_max17043.c
    ├── xy_fg_bq27z561.c
    └── ...
```

### 阶段 2: 迁移电量计驱动

从 sensor 迁移到 fuel_gauge:
- MAX17043 → fuel_gauge/drivers/
- BQ27z561 → fuel_gauge/drivers/
- ...

### 阶段 3: 优化 sensor 组织

参考 Zephyr 按厂商组织:
```
sensor/drivers/
├── bosch/
│   ├── bme280.c
│   └── bmp280.c
├── sensirion/
│   ├── sht30.c
│   └── sht40.c
├── aosong/
│   └── aht20.c
└── ...
```

---

## 🔧 fuel_gauge API 设计

### 核心 API

```c
/* 初始化 */
int xy_fuel_gauge_init(xy_fuel_gauge_t *fg);

/* 读取电池电压 */
int xy_fuel_gauge_get_voltage(xy_fuel_gauge_t *fg, uint16_t *voltage_mv);

/* 读取电流 */
int xy_fuel_gauge_get_current(xy_fuel_gauge_t *fg, int16_t *current_ma);

/* 读取 SOC (State of Charge) */
int xy_fuel_gauge_get_soc(xy_fuel_gauge_t *fg, uint8_t *soc_pct);

/* 读取 SOH (State of Health) */
int xy_fuel_gauge_get_soh(xy_fuel_gauge_t *fg, uint8_t *soh_pct);

/* 读取温度 */
int xy_fuel_gauge_get_temperature(xy_fuel_gauge_t *fg, int16_t *temp_c);

/* 库仑计数 */
int xy_fuel_gauge_get_coulomb_counter(xy_fuel_gauge_t *fg, int32_t *counter);

/* 设置告警阈值 */
int xy_fuel_gauge_set_alert(xy_fuel_gauge_t *fg, uint8_t low_soc, uint8_t high_soc);
```

### 数据结构

```c
typedef struct {
    uint16_t voltage_mv;      /* 电池电压 (mV) */
    int16_t  current_ma;      /* 电流 (mA, 正=充电，负=放电) */
    uint8_t  soc;             /* 电量百分比 (0-100%) */
    uint8_t  soh;             /* 健康度 (0-100%) */
    int16_t  temperature_c;   /* 温度 (0.1°C) */
    uint32_t cycle_count;     /* 循环次数 */
    uint32_t timestamp;       /* 时间戳 */
} xy_fuel_gauge_data_t;

typedef struct {
    const char *name;
    const xy_fuel_gauge_api_t *api;
    void *data;
    xy_fuel_gauge_data_t latest;
} xy_fuel_gauge_t;
```

---

## 📊 Sensor 优化

### 参考 Zephyr 组织方式

**当前结构**:
```
sensor/drivers/
├── temperature/
├── pressure/
├── motion/
└── ...
```

**优化后**:
```
sensor/drivers/
├── bosch/           # 按厂商
│   ├── bme280.c
│   ├── bmp280.c
│   └── bmi160.c
├── sensirion/
│   ├── sht30.c
│   ├── sht40.c
│   └── scd30.c
├── aosong/
│   ├── aht20.c
│   └── dht11.c
├── tdk/
│   ├── icm20608.c
│   └── icm42688.c
└── ...
```

### 优势

1. **易于查找** - 知道芯片厂商就能找到驱动
2. **便于维护** - 同一厂商的驱动放在一起
3. **支持扩展** - 新增厂商驱动不影响现有结构
4. **符合业界** - Zephyr/Linux 都采用此方式

---

## ✅ 实施步骤

### 步骤 1: 创建 fuel_gauge 组件 (2 小时)
- [ ] 创建目录结构
- [ ] 定义核心 API
- [ ] 迁移 MAX17043 驱动
- [ ] 迁移 BQ27 系列驱动

### 步骤 2: 优化 sensor 组织 (2 小时)
- [ ] 按厂商重组驱动目录
- [ ] 更新 Kconfig/CMakeLists
- [ ] 更新文档

### 步骤 3: 完善 fuel_gauge (2 小时)
- [ ] 添加更多电量计驱动
- [ ] 实现库仑计数
- [ ] 实现 SOC/SOH 算法
- [ ] 添加电池安全监测

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
