# XinYi PID Controller Component

**版本**: 2.0.0
**日期**: 2026-03-01
**状态**: 🟢 稳定

---

## 📋 概述

PID (Proportional-Integral-Derivative) 控制器组件，用于闭环控制系统。支持高级特性如积分抗饱和、微分滤波、参数在线调整。

---

## 🎯 特性

- ✅ 标准 PID 算法（位置式/增量式）
- ✅ 积分抗饱和 (Anti-windup)
- ✅ 微分滤波
- ✅ 参数在线调整
- ✅ 自动整定功能 (Ziegler-Nichols)
- ✅ 全面的状态监控

---

## 📁 文件结构

```
pid/
├── inc/
│   ├── xy_pid.h           # PID 控制器主头文件
│   └── xy_pid_auto.h      # 自动整定头文件
├── src/
│   ├── xy_pid.c           # PID 控制器实现
│   └── xy_pid_auto.c      # 自动整定实现
├── examples/              # 示例程序
│   ├── CMakeLists.txt     # 示例构建配置
│   ├── example_basic.c    # 基础 PID 用法
│   ├── example_incremental.c  # 增量式 PID
│   ├── example_temperature.c  # 温度控制
│   ├── example_charging.c     # 充电控制
│   └── example_auto_tune.c    # 自动整定
├── README.md              # 本文件
├── pid-basic.md           # 基础 PID 文档
├── pid-charging.md        # 充电应用文档
└── pid.md                 # 详细技术文档
```

---

## 🔧 API 概述

### PID 控制器初始化

```c
#include "xy_pid.h"

xy_pid_t pid;
xy_pid_config_t config = {
    .kp = 1.0f,
    .ki = 0.5f,
    .kd = 0.2f,
    .output_min = 0.0f,
    .output_max = 100.0f,
    .integral_min = -50.0f,
    .integral_max = 50.0f,
    .derivative_filter = 0.1f
};

xy_pid_init(&pid, &config);
xy_pid_set_mode(&pid, XY_PID_MODE_AUTO);
```

### 计算 PID 输出

```c
float setpoint = 50.0f;  // 目标值
float measurement = 45.0f;  // 测量值
float output;

xy_pid_set_setpoint(&pid, setpoint);
xy_pid_compute(&pid, measurement, &output);
// output 包含计算后的控制量
```

### 参数调整

```c
// 在线调整 PID 参数
xy_pid_set_tuning(&pid, new_kp, new_ki, new_kd);

// 设置输出限制
xy_pid_set_output_limits(&pid, 0.0f, 100.0f);

// 重置 PID（清除积分项）
xy_pid_reset(&pid);
```

### 高级特性

```c
// 启用抗积分饱和
xy_pid_enable_anti_windup(&pid, true);

// 启用微分滤波
xy_pid_enable_derivative_filter(&pid, true, 0.1f);

// 获取内部状态
float error = xy_pid_get_error(&pid);
float integral = xy_pid_get_integral(&pid);
float derivative = xy_pid_get_derivative(&pid);
```

---

## 🚀 简单使用示例

```c
#include "xy_pid.h"

int main(void)
{
    xy_pid_t temperature_ctrl;
    xy_pid_config_t config = {
        .kp = 10.0f,
        .ki = 0.5f,
        .kd = 2.0f,
        .output_min = 0.0f,
        .output_max = 100.0f
    };

    // 初始化
    xy_pid_init(&temperature_ctrl, &config);
    xy_pid_set_mode(&temperature_ctrl, XY_PID_MODE_AUTO);
    xy_pid_set_setpoint(&temperature_ctrl, 25.0f);  // 目标温度 25°C

    // 控制循环
    while (1) {
        float temp = read_temperature_sensor();  // 读取温度
        float output;

        xy_pid_compute(&temperature_ctrl, temp, &output);

        apply_heater_power(output);  // 应用控制输出
    }

    return 0;
}
```

---

## 🔄 自动整定示例

```c
#include "xy_pid.h"
#include "xy_pid_auto.h"

int main(void)
{
    xy_pid_t pid;
    xy_pid_auto_tuner_t tuner;
    xy_pid_auto_config_t auto_config = {
        .method = XY_PID_AUTO_METHOD_ZN,
        .step_amplitude = 50.0f,
        .sample_interval_ms = 100,
        .num_samples = 100
    };

    // 初始化 PID
    xy_pid_config_t pid_config = {
        .kp = 1.0f, .ki = 0.0f, .kd = 0.0f,
        .output_min = 0.0f, .output_max = 100.0f
    };
    xy_pid_init(&pid, &pid_config);

    // 初始化自动整定器
    xy_pid_auto_init(&tuner, &pid, &auto_config);
    xy_pid_auto_start(&tuner);

    // 整定循环
    while (xy_pid_auto_get_state(&tuner) != XY_PID_AUTO_STATE_COMPLETE) {
        float pv = read_process_variable();
        xy_pid_auto_loop(&tuner, pv);
        delay_ms(100);
    }

    // 应用整定结果
    xy_pid_auto_apply(&tuner);

    return 0;
}
```

---

## 🏗️ 构建说明

### CMake 构建

```cmake
# 在您的 CMakeLists.txt 中
add_subdirectory(components/pid)
target_link_libraries(your_target xy_pid)
```

### Kconfig 配置

```
CONFIG_XY_PID=y
CONFIG_XY_PID_AUTO=y
```

### 依赖

- `xy_log` - 日志系统
- `xy_os` - 操作系统抽象（用于延时）

---

## 📊 状态码

| 状态码               | 描述     |
| -------------------- | -------- |
| XY_PID_OK            | 成功     |
| XY_PID_ERROR         | 一般错误 |
| XY_PID_INVALID_PARAM | 无效参数 |

自动整定状态：

| 状态                          | 描述   |
| ----------------------------- | ------ |
| XY_PID_AUTO_STATE_IDLE        | 空闲   |
| XY_PID_AUTO_STATE_MEASURING   | 测量中 |
| XY_PID_AUTO_STATE_CALCULATING | 计算中 |
| XY_PID_AUTO_STATE_COMPLETE    | 完成   |
| XY_PID_AUTO_STATE_ERROR       | 错误   |

## 📖 示例程序

### 构建示例

```bash
cd components/pid/examples
mkdir build && cd build
cmake .. && make
```

### 示例列表

| 示例文件                | 描述                                                                                         |
| ----------------------- | -------------------------------------------------------------------------------------------- |
| `example_basic.c`       | 基础 PID 控制器用法（位置式） - 包含温度控制、在线调参、输出限幅、手动/自动模式切换          |
| `example_incremental.c` | 增量式 PID 控制器 - 位置控制、抗扰动、参考跟踪、速度控制                                     |
| `example_temperature.c` | 温度 PID 控制示例 - 加热/冷却、双极性输出、设定点跟踪、热扰动、抗过冲                        |
| `example_charging.c`    | 充电 PID 控制示例 - CC-CV 充电、充电宝输出、充电速率优化、温度感知充电                       |
| `example_auto_tune.c`   | 自动整定示例 - 手动 Ziegler-Nichols 整定、自动整定库使用、手动 vs 自动调参对比、温度自动整定 |

### 运行示例

```bash
# 运行基础示例
./example_basic

# 运行增量式 PID 示例
./example_incremental

# 运行温度控制示例
./example_temperature

# 运行充电控制示例
./example_charging

# 运行自动整定示例
./example_auto_tune
```

---

**维护者**: XinYi Team
**许可**: MIT
