# XinYi Actuator Component

**版本**: 1.0.0
**状态**: 🟡 完善中

---

## 📋 概述

执行器框架 (Actuator Framework)，统一管理舵机、继电器、直流电机、步进电机、PWM 输出等执行设备。提供标准化的设备注册、控制和状态监控接口。

---

## 🎯 特性

- ✅ 支持多种执行器类型（继电器、舵机、电机、PWM、LED、蜂鸣器等）
- ✅ 统一的设备注册和管理接口
- ✅ 标准化操作 API
- ✅ 紧急停止功能
- ✅ 批量操作支持

---

## 📁 文件结构

```
actuator/
├── xy_actuator.h    # 执行器框架头文件
├── xy_actuator.c    # 执行器框架实现
├── CMakeLists.txt
└── README.md        # 本文件
```

---

## 🔧 API 概述

### 设备注册

```c
#include "xy_actuator.h"

// 定义执行器设备
actuator_device_t my_servo = ACTUATOR_DEVICE_INIT(
    "servo_1",
    ACTUATOR_TYPE_SERVO,
    &servo_ops,
    bus_handle,
    NULL
);

// 注册设备
actuator_register(&my_servo);

// 查找设备
actuator_device_t *dev = actuator_find("servo_1");
actuator_device_t *dev = actuator_find_by_type(ACTUATOR_TYPE_SERVO);
```

### 通用操作

```c
// 初始化/反初始化
actuator_init(dev);
actuator_deinit(dev);

// 使能/禁用
actuator_enable(dev, true);

// 读取/写入值
actuator_value_t value;
value.servo.target_angle = 90.0f;
actuator_write(dev, &value);
actuator_read(dev, &value);

// 重置和急停
actuator_reset(dev);
actuator_emergency_stop(dev);

// 获取状态
actuator_status_t status = actuator_get_status(dev);
```

### 继电器操作

```c
#include "xy_actuator.h"

actuator_device_t *relay = actuator_find("relay_1");

// 继电器控制
relay_on(relay);           // 打开
relay_off(relay);          // 关闭
relay_toggle(relay);       // 翻转
relay_get(relay, &state);  // 获取状态
relay_pulse(relay, 100);   // 脉冲（100ms）
```

### 舵机操作

```c
#include "xy_actuator.h"

actuator_device_t *servo = actuator_find("servo_1");

// 初始化舵机
servo_init(servo);

// 设置角度 (-90° 到 +90°)
servo_set_angle(servo, 45.0f);

// 获取角度
float angle;
servo_get_angle(servo, &angle);

// 设置角度范围
servo_set_range(servo, -45.0f, 45.0f);

// 设置速度 (度/秒)
servo_set_speed(servo, 90);

// 往复摆动
servo_sweep(servo, -45.0f, 45.0f, 1000);  // 每步 1 秒

// 停止和回中
servo_stop(servo);
servo_center(servo);
```

### PWM 操作

```c
#include "xy_actuator.h"

actuator_device_t *pwm_dev = actuator_find("pwm_1");

// 设置占空比 (0-65535)
pwm_set_duty(pwm_dev, 32768);  // 50%

// 设置频率 (Hz)
pwm_set_frequency(pwm_dev, 1000);  // 1kHz
```

### 批量操作

```c
// 关闭所有执行器
actuator_all_off();

// 紧急停止所有
actuator_emergency_stop_all();
```

---

## 🚀 简单使用示例

```c
#include "xy_actuator.h"

// 定义舵机操作接口
static actuator_err_t my_servo_init(actuator_device_t *dev) {
    // 硬件初始化代码
    return ACTUATOR_EOK;
}

static actuator_err_t my_servo_write(actuator_device_t *dev, const actuator_value_t *value) {
    // 设置 PWM 占空比到目标角度
    uint16_t pwm = servo_angle_to_pwm(
        value->servo.target_angle,
        dev->config.servo_min_angle,
        dev->config.servo_max_angle,
        dev->config.servo_pwm_min,
        dev->config.servo_pwm_max
    );
    set_pwm_duty(dev->config.pwm_channel, pwm);
    return ACTUATOR_EOK;
}

static const actuator_ops_t servo_ops = {
    .init = my_servo_init,
    .write = my_servo_write,
    // ... 其他可选接口
};

int main(void)
{
    // 定义舵机设备
    actuator_device_t servo1 = SERVO_DEVICE_INIT(
        "servo_1",
        &servo_ops,
        1,              // PWM 通道
        -90.0f,         // 最小角度
        90.0f,          // 最大角度
        90              // 速度 (度/秒)
    );

    // 注册设备
    actuator_register(&servo1);

    // 使用舵机
    servo_device_t *servo = actuator_find("servo_1");
    servo_init(servo);
    servo_set_angle(servo, 45.0f);

    // 清理
    actuator_unregister(&servo1);

    return 0;
}
```

---

## 🏗️ 构建说明

### CMake 构建

```cmake
# 在您的 CMakeLists.txt 中
add_subdirectory(components/actuator)
target_link_libraries(your_target xy_actuator)
```

### 依赖

- 硬件 PWM 输出
- GPIO 引脚控制（用于继电器）
- `xy_log` 日志系统（可选）

---

## 📊 执行器类型

| 类型                          | 描述             |
| ----------------------------- | ---------------- |
| `ACTUATOR_TYPE_RELAY`         | 继电器（开关）   |
| `ACTUATOR_TYPE_SERVO`         | 舵机（角度控制） |
| `ACTUATOR_TYPE_MOTOR_DC`      | 直流电机         |
| `ACTUATOR_TYPE_MOTOR_STEPPER` | 步进电机         |
| `ACTUATOR_TYPE_SOLENOID`      | 电磁阀           |
| `ACTUATOR_TYPE_PWM`           | 通用 PWM 输出    |
| `ACTUATOR_TYPE_LED`           | LED 控制         |
| `ACTUATOR_TYPE_BUZZER`        | 蜂鸣器           |
| `ACTUATOR_TYPE_VALVE`         | 阀门             |

---

## 📊 执行器状态

| 状态                       | 描述       |
| -------------------------- | ---------- |
| `ACTUATOR_STATUS_IDLE`     | 空闲       |
| `ACTUATOR_STATUS_READY`    | 就绪       |
| `ACTUATOR_STATUS_BUSY`     | 动作执行中 |
| `ACTUATOR_STATUS_ERROR`    | 错误       |
| `ACTUATOR_STATUS_DISABLED` | 禁用       |

---

## 📊 错误码

| 错误码              | 描述       |
| ------------------- | ---------- |
| `ACTUATOR_EOK`      | 成功       |
| `ACTUATOR_ERROR`    | 一般错误   |
| `ACTUATOR_EINVAL`   | 无效参数   |
| `ACTUATOR_ENODEV`   | 设备未找到 |
| `ACTUATOR_EBUSY`    | 设备忙     |
| `ACTUATOR_ETIMEOUT` | 超时       |
| `ACTUATOR_ENOMEM`   | 内存不足   |
| `ACTUATOR_ENOSYS`   | 不支持     |
| `ACTUATOR_EIO`      | I/O 错误   |
| `ACTUATOR_ELIMIT`   | 超出限制   |
| `ACTUATOR_EHW`      | 硬件错误   |

---

## 🧪 测试

### 测试文件

```
tests/actuator/
├── CMakeLists.txt           # 测试构建配置
├── test_actuator_core.c     # 核心功能测试 (注册、查找、遍历)
├── test_actuator_relay.c    # 继电器测试 (开/关、翻转)
├── test_actuator_pwm.c      # PWM 测试 (占空比、频率)
└── test_actuator_servo.c    # 舵机测试 (角度控制)
```

### 构建测试

```bash
# 创建构建目录
mkdir -p build_actuator_test
cd build_actuator_test

# 配置项目 (从项目根目录)
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug

# 或者使用项目已有的 CMakeLists
# 需要在主 CMakeLists.txt 中启用 actutor_tests 子目录
```

### 运行测试

```bash
# 单独运行每个测试
./test_actuator_core
./test_actuator_relay
./test_actuator_pwm
./test_actuator_servo

# 或运行所有测试
make run_actuator_tests
```

### 测试覆盖

#### test_actuator_core.c

- 设备注册/注销 (单设备、多设备、重名检查)
- 设备查找 (按名称、按类型)
- 设备计数 (总数、按类型计数)
- 设备初始化/反初始化
- 使能/禁用控制
- 状态查询
- 字符串转换函数
- 通用读写操作
- 复位和急停

#### test_actuator_relay.c

- 继电器初始化
- 开/关控制 (`relay_on`, `relay_off`)
- 状态设置/读取
- 翻转操作 (`relay_toggle`)
- 脉冲操作 (`relay_pulse`)
- 状态枚举值验证

#### test_actuator_pwm.c

- PWM 初始化
- 占空比设置 (0%, 50%, 100%)
- 频率设置 (100Hz - 10MHz)
- PWM 值结构测试
- 配置测试

#### test_actuator_servo.c

- 舵机初始化
- 角度设置 (最小、最大、中心)
- 角度范围设置
- 速度设置
- 回中、停止、往复运动
- 角度与 PWM 转换 (包含边界 clamping)
- PWM 占空比转角度转换

### 预期输出

```
test_actuator_core.c
...
Tests [XX] Passed [XX] Failed [XX] Ignored [XX]

test_actuator_relay.c
...
Tests [XX] Passed [XX] Failed [XX] Ignored [XX]
```

---

**维护者**: XinYi Team
