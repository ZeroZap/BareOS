# XinYi System Components - 系统组件

**版本**: 1.0.0  
**日期**: 2026-03-18

---

## 📋 组件

- **xy_state_machine** - 状态机框架
- **xy_sys** - 系统管理
- **xy_timer** - 软件定时器

---

## 🔧 状态机

```c
xy_sm_t sm;
xy_sm_init(&sm, states, transitions, num_states);
xy_sm_run(&sm, event);
```

---

## ⏱️ 定时器

```c
xy_timer_t timer;
xy_timer_create(&timer, callback, period_ms, true);
xy_timer_start(&timer);
xy_timer_stop(&timer);
```

---

**完成度**: 80% 🟡
