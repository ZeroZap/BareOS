# xy_st 状态机使用指南

**版本**: 1.0.0  
**日期**: 2026-03-05

---

## 📋 概述

xy_st 是一个轻量级状态机库，支持：
- ✅ 状态转换
- ✅ 超时处理
- ✅ 延迟处理
- ✅ 状态回调

---

## 📁 文件结构

```
sys/xy_state_machine/
├── xy_st.h          # 状态机接口
├── xy_st.c          # 状态机实现
├── CMakeLists.txt   # 构建配置
└── Kconfig          # 配置选项
```

---

## 🚀 快速开始

### 1. 定义状态机

```c
#include "xy_st.h"

/* 定义状态 */
static void state_idle_entry(xy_sm_t *self);
static void state_idle_process(xy_sm_t *self);
static void state_idle_exit(xy_sm_t *self);

static void state_run_entry(xy_sm_t *self);
static void state_run_process(xy_sm_t *self);
static void state_run_exit(xy_sm_t *self);

/* 创建状态机实例 */
static xy_sm_t g_state_machine;
```

### 2. 实现状态回调

```c
/* 空闲状态 */
static void state_idle_entry(xy_sm_t *self)
{
    xy_log_i("Enter idle state\n");
}

static void state_idle_process(xy_sm_t *self)
{
    /* 处理空闲状态逻辑 */
    if (should_run()) {
        /* 转换到运行状态 */
        xy_sm_transition(self, state_run_entry, state_run_process, state_run_exit);
    }
}

static void state_idle_exit(xy_sm_t *self)
{
    xy_log_i("Exit idle state\n");
}

/* 运行状态 */
static void state_run_entry(xy_sm_t *self)
{
    xy_log_i("Enter run state\n");
}

static void state_run_process(xy_sm_t *self)
{
    /* 处理运行状态逻辑 */
    if (should_stop()) {
        /* 转换到空闲状态 */
        xy_sm_transition(self, state_idle_entry, state_idle_process, state_idle_exit);
    }
}

static void state_run_exit(xy_sm_t *self)
{
    xy_log_i("Exit run state\n");
}
```

### 3. 初始化和运行

```c
/* 初始化状态机 */
void state_machine_init(void)
{
    xy_sm_init(&g_state_machine);
    
    /* 设置初始状态为空闲 */
    xy_sm_transition(&g_state_machine, 
                     state_idle_entry, 
                     state_idle_process, 
                     state_idle_exit);
}

/* 运行状态机 */
void state_machine_run(void)
{
    xy_sm_process_sample(&g_state_machine);
}
```

---

## ⏱️ 超时处理

### 带超时的状态转换

```c
static void state_wait_entry(xy_sm_t *self)
{
    xy_log_i("Enter wait state\n");
}

static void state_wait_process(xy_sm_t *self)
{
    xy_log_i("Waiting...\n");
}

static void state_wait_exit(xy_sm_t *self)
{
    xy_log_i("Exit wait state\n");
}

static void state_wait_timeout_entry(xy_sm_t *self)
{
    xy_log_i("Wait timeout - enter next state\n");
}

static void state_wait_timeout_process(xy_sm_t *self)
{
    xy_log_i("Handling timeout\n");
}

static void state_wait_timeout_exit(xy_sm_t *self)
{
    xy_log_i("Exit timeout state\n");
}

/* 设置超时状态转换 (超时 1000ms) */
xy_sm_transition_timeout(&g_state_machine,
                         state_wait_entry,
                         state_wait_process,
                         state_wait_exit,
                         state_wait_timeout_entry,
                         state_wait_timeout_process,
                         state_wait_timeout_exit,
                         1000);  /* 超时时间 (ms) */
```

---

## 🕐 延迟处理

### 延迟状态转换

```c
/* 延迟转换到下一个状态 (延迟 500ms) */
xy_sm_transition_delay(&g_state_machine,
                       state_delay_entry,
                       state_delay_process,
                       state_delay_exit,
                       500);  /* 延迟时间 (ms) */
```

---

## 📊 状态机生命周期

```
初始化 → 设置初始状态 → 循环处理 → 状态转换 → 销毁

xy_sm_init()
    ↓
xy_sm_transition()
    ↓
xy_sm_process_sample() ← 循环调用
    ↓
xy_sm_transition() (可选)
```

---

## 🎯 使用场景

### 1. 充电状态管理

```c
typedef enum {
    CHG_STATE_IDLE,
    CHG_STATE_PRECHARGE,
    CHG_STATE_FAST_CHARGE,
    CHG_STATE_CONSTANT_VOLT,
    CHG_STATE_FULL,
} charging_state_t;

static void chg_idle_process(xy_sm_t *self)
{
    if (charger_connected()) {
        xy_sm_transition(self, chg_precharge_entry, chg_precharge_process, chg_precharge_exit);
    }
}

static void chg_precharge_process(xy_sm_t *self)
{
    if (voltage >= PRECHARGE_THRESHOLD) {
        xy_sm_transition(self, chg_fast_entry, chg_fast_process, chg_fast_exit);
    }
}

static void chg_fast_process(xy_sm_t *self)
{
    if (voltage >= FAST_CHARGE_THRESHOLD) {
        xy_sm_transition(self, chg_cv_entry, chg_cv_process, chg_cv_exit);
    }
}
```

### 2. 电池保护状态

```c
typedef enum {
    PROT_STATE_NORMAL,
    PROT_STATE_OVER_VOLT,
    PROT_STATE_UNDER_VOLT,
    PROT_STATE_OVER_TEMP,
} protection_state_t;

static void prot_normal_process(xy_sm_t *self)
{
    if (voltage > OV_THRESHOLD) {
        xy_sm_transition(self, prot_ov_entry, prot_ov_process, prot_ov_exit);
    } else if (voltage < UV_THRESHOLD) {
        xy_sm_transition(self, prot_uv_entry, prot_uv_process, prot_uv_exit);
    }
}

static void prot_ov_process(xy_sm_t *self)
{
    /* 过压保护逻辑 */
    disable_charging();
    
    if (voltage <= OV_RECOVERY) {
        xy_sm_transition(self, prot_normal_entry, prot_normal_process, prot_normal_exit);
    }
}
```

### 3. 通信协议状态

```c
typedef enum {
    COMM_STATE_IDLE,
    COMM_STATE_RX,
    COMM_STATE_PROCESS,
    COMM_STATE_TX,
} comm_state_t;

static void comm_idle_process(xy_sm_t *self)
{
    if (data_received()) {
        xy_sm_transition(self, comm_rx_entry, comm_rx_process, comm_rx_exit);
    }
}

static void comm_rx_process(xy_sm_t *self)
{
    /* 接收数据 */
    receive_data();
    
    if (rx_complete()) {
        xy_sm_transition(self, comm_proc_entry, comm_proc_process, comm_proc_exit);
    }
}
```

---

## ⚠️ 注意事项

### 1. 状态转换

- 状态转换会调用 exit 回调
- 然后调用 entry 回调
- 最后设置 process 回调

### 2. 超时处理

- 超时时间单位：毫秒
- 超时后自动转换到超时状态
- 超时状态也可以再次转换

### 3. 重入问题

- 避免在状态回调中递归调用状态转换
- 使用标志位防止重入

---

## 📚 API 参考

| 函数 | 说明 |
|------|------|
| `xy_sm_init(self)` | 初始化状态机 |
| `xy_sm_transition(self, entry, process, exit)` | 状态转换 |
| `xy_sm_transition_timeout(self, entry, process, exit, timeout_entry, timeout_process, timeout_exit, timeout)` | 带超时的状态转换 |
| `xy_sm_transition_delay(self, timeout_entry, timeout_process, timeout_exit, timeout)` | 延迟状态转换 |
| `xy_sm_process_sample(self)` | 处理状态机 |

---

## 🎊 示例代码

完整示例请参考：
- `components/fuel_gauge/core/xy_fuel_gauge_status.c` - 充电状态管理
- `components/fota/src/xy_fota_secure.c` - FOTA 状态管理

---

**维护者**: XinYi Team  
**许可证**: Apache License 2.0
