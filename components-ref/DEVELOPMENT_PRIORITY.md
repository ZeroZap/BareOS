# XinYi 开发优先级

**更新时间**: 2026-03-11  
**维护者**: ese

---

## 🎯 核心原则

**XinYi 是一个参考 RT-Thread 和 Zephyr 设计的嵌入式框架**
- ✅ 支持主流 RTOS (FreeRTOS/RT-Thread/CMSIS-RTX/RTX5/Bare-metal)
- ✅ 不绑定特定 RTOS，保持灵活性
- ✅ 优先完善 C 版本 xinyi，Rust 版本延后

---

## 📊 优先级矩阵

| 优先级 | 组件 | 状态 | 工时 | 说明 |
|--------|------|------|------|------|
| **P0** | OSAL 完善 | 🟡 中 | 8h | 支持更多 RTOS |
| **P0** | HAL 统一 | 🟡 中 | 12h | 统一多平台 API |
| **P0** | 设备模型 | 🟡 中 | 10h | 参考 RT-Thread/Zephyr |
| **P1** | Crypto 优化 | 🟡 中 | 11h | 汇编优化 |
| **P1** | Net 完善 | 🟡 中 | 4h | CAN/LTE |
| **P2** | FOTA 增强 | ✅ 完成 | - | 已完成 |
| **P3** | XinYi_rs | ⏸️ 搁置 | - | 等 xinyi 成熟 |

---

## P0: 核心架构任务

### 1. OSAL 完善 (8h)

**目标**: 让 OSAL 更灵活，支持更多 RTOS

**TODO**:
- [ ] 添加 RTX5 后端支持
- [ ] 改进 Bare-metal 模式的中断处理
- [ ] 添加 OSAL 性能测试
- [ ] 完善文档和示例

**文件**:
- `components/kernel/osal/backend/`
- `components/kernel/osal/inc/xy_os.h`

---

### 2. HAL 统一 (12h)

**目标**: 统一多平台 HAL API，参考 Zephyr 设备模型

**TODO**:
- [ ] 统一 GPIO API (STM32/WCH/HC32)
- [ ] 统一 UART API
- [ ] 统一 SPI/I2C API
- [ ] 添加 HAL 测试套件

**设计原则**:
```c
/* 统一 HAL API 示例 */
typedef struct xy_gpio_config {
    uint32_t pin;
    xy_gpio_mode_t mode;
    xy_gpio_pull_t pull;
    xy_gpio_speed_t speed;
} xy_gpio_config_t;

xy_error_t xy_gpio_init(xy_gpio_t *gpio, const xy_gpio_config_t *config);
xy_error_t xy_gpio_write(xy_gpio_t *gpio, uint8_t value);
uint8_t xy_gpio_read(xy_gpio_t *gpio);
```

---

### 3. 设备模型 (10h)

**目标**: 实现统一的设备管理框架

**设计**:
```c
/* 统一设备结构 */
typedef struct xy_device {
    const char *name;
    xy_dev_type_t type;
    uint32_t flags;
    xy_dev_state_t state;
    const xy_dev_api_t *api;
    const void *config;
    void *data;
    uint8_t ref_count;
    struct xy_device *next;
} xy_device_t;

/* 设备注册 (静态) */
#define XY_DEVICE_DEFINE(name, type, init_fn, api_ptr, config_ptr) \
    static xy_device_t name = { \
        .name = #name, \
        .type = type, \
        .api = api_ptr, \
        .config = config_ptr, \
    }
```

**TODO**:
- [ ] 实现设备注册/查找 API
- [ ] 实现设备电源管理
- [ ] 实现设备异步操作
- [ ] 添加设备管理示例

---

## P1: 中优先级任务

### Crypto 汇编优化 (11h)

**TODO**:
- [ ] CRYPTO-001: 64-bit multiply (3h)
- [ ] CRYPTO-002: High 32 bits square (2h)
- [ ] CRYPTO-003: High32 reduce (2h)
- [ ] CRYPTO-004: High32 mul256 (2h)
- [ ] CRYPTO-005: Shift-and-add (2h)

**文件**: `components/crypto/xy_25519/asm/`

---

### Net 网络协议 (4h)

**TODO**:
- [ ] NET-001: CAN 停止硬件控制器 (1h)
- [ ] NET-002: CAN 回调注册 (1h)
- [ ] NET-003: AT 协议检查 (2h)

**文件**: 
- `components/net/src/xy_can.c`
- `components/net/src/xy_at_socket.c`

---

## P2: 低优先级任务

### Kernel 系统监控 (2h)

- [ ] SYSMON-001: 任务列表打印

### GUI 字体优化 (2h)

- [ ] GUI-001: 字符缓存机制

### Sensor 传感器 (1h)

- [ ] SENSOR-001: MLX90614 EEPROM 发射率

### Clib 标准库 (标记为不支持)

- [ ] CLIB-001~007: scanf/atof 等 (已标记不支持)

---

## P3: 延后任务

### XinYi_rs (Rust 版本)

**状态**: ⏸️ 搁置

**原因**:
- xinyi (C 版本) 架构还在完善中
- 等核心框架稳定后再 port 到 Rust
- 避免重复工作

**启动条件**:
- [ ] OSAL 架构稳定
- [ ] HAL API 统一完成
- [ ] 设备模型成熟
- [ ] 有实际 Rust 项目需求

---

## 📈 进度追踪

```
P0 核心架构：▓▓▓▓▓▓▓▓▓▓░░░░░░░░ 40%
P1 中优先级：▓▓▓▓▓▓▓▓▓▓▓▓░░░░░░ 60%
P2 低优先级：▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░ 90%
P3 延后任务：░░░░░░░░░░░░░░░░░░ 0%

总体进度：████████████████▓▓▓▓ 60%
```

---

## 🎯 本周目标

- [ ] 完成设备模型设计
- [ ] 实现 OSAL RTX5 后端
- [ ] 统一 GPIO HAL API

---

**最后更新**: 2026-03-11 by ese
