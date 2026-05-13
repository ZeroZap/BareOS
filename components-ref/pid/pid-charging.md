# 便携式加热设备智能充电温控系统 V2.0

## 📋 温度区间与回滞设计

### 温度区间定义

```text
温度(°C)
    │
 70 ├─────────────────────────────────────────── 禁止区
    │         ████████████████████████████████
 60 ├─────────████████████████████████████████── 过热阈值 (回滞55°C)
    │    █████████████████████████████████████
 55 ├────█████████████████████████████████████── 过热回滞点
    │    █████████████████████████████████████   高温降流区
 46 ├────█████████████████████████████████████── 高温阈值 (回滞40°C)
 45 ├─────────────────────────────────────────
    │    ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
 40 ├────▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓── 高温回滞点
    │    ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓   正常充电区
 11 ├────▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
 10 ├─────────────────────────────────────────
    │    ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   低温预热区
  5 ├────░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░── 低温回滞点
    │    ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
  0 ├─────────────────────────────────────────── 过冷阈值 (回滞5°C)
    │         ████████████████████████████████   禁止区
-10 ├─────────████████████████████████████████
    │
    └─────────────────────────────────────────────────────────►
```

### 回滞逻辑说明

| 边界 | 进入条件 | 退出条件 | 回滞值 |        说明         |
| :--: | :------: | :------: | :----: | :-----------------: |
| 低温 | T < 0°C  | T > 5°C  |  5°C   | 防止0°C附近频繁切换 |
| 高温 | T > 45°C | T < 40°C |  5°C   | 进入降流模式的回滞  |
| 过热 | T > 60°C | T < 55°C |  5°C   |   停止充电的回滞    |

------

## 📁 文件结构

```text
charge_thermal_v2/
├── charge_thermal_ctrl.h   # 头文件
├── charge_thermal_ctrl.c   # 实现文件
└── charge_thermal_test.c   # 测试与仿真
```

------

## 📄 charge_thermal_ctrl.h

```c
/**
 * @file charge_thermal_ctrl.h
 * @brief 便携式加热设备充电温控管理系统 V2.0
 * @version 2.0
 * 
 * 温度区间设计:
 * - 过冷区:   T < 0°C,  回滞到 5°C
 * - 低温区:   0°C ~ 10°C, 小电流充电
 * - 正常区:   11°C ~ 45°C, 正常充电
 * - 高温区:   46°C ~ 60°C, 降流充电, 回滞到 40°C
 * - 过热区:   T > 60°C, 停止充电, 回滞到 55°C
 */

#ifndef __CHARGE_THERMAL_CTRL_H__
#define __CHARGE_THERMAL_CTRL_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================
 *                              温度阈值配置
 *===========================================================================*/

/* 温度单位: 0.1°C (例如 450 = 45.0°C) */
#define TEMP_SCALE                  10

/* 温度区间阈值 */
#define TEMP_COLD_THRESHOLD         (0   * TEMP_SCALE)   /* 0°C: 过冷阈值 */
#define TEMP_COLD_HYSTERESIS        (5   * TEMP_SCALE)   /* 5°C: 过冷回滞点 */

#define TEMP_LOW_THRESHOLD          (10  * TEMP_SCALE)   /* 10°C: 低温区上限 */

#define TEMP_NORMAL_MIN             (11  * TEMP_SCALE)   /* 11°C: 正常区下限 */
#define TEMP_NORMAL_MAX             (45  * TEMP_SCALE)   /* 45°C: 正常区上限 */

#define TEMP_HIGH_THRESHOLD         (46  * TEMP_SCALE)   /* 46°C: 高温区下限 */
#define TEMP_HIGH_HYSTERESIS        (40  * TEMP_SCALE)   /* 40°C: 高温回滞点 */

#define TEMP_OVERHEAT_THRESHOLD     (60  * TEMP_SCALE)   /* 60°C: 过热阈值 */
#define TEMP_OVERHEAT_HYSTERESIS    (55  * TEMP_SCALE)   /* 55°C: 过热回滞点 */

/* PID控制目标温度 */
#define TEMP_PID_TARGET             (42  * TEMP_SCALE)   /* 42°C: PID目标温度 */

/* 环境温度范围 */
#define TEMP_ENV_MIN                (-10 * TEMP_SCALE)   /* -10°C */
#define TEMP_ENV_MAX                (80  * TEMP_SCALE)   /* 80°C */

/*============================================================================
 *                              电流配置
 *===========================================================================*/

/* 电流单位: mA */
#define CURRENT_MAX                 2000    /* 最大充电电流 */
#define CURRENT_NORMAL              1500    /* 正常充电电流 */
#define CURRENT_LOW_TEMP            800     /* 低温充电电流 */
#define CURRENT_HIGH_TEMP           500     /* 高温降流电流 */
#define CURRENT_MIN                 200     /* 最小充电电流 */
#define CURRENT_TRICKLE             100     /* 涓流充电电流 */
#define CURRENT_OFF                 0       /* 停止充电 */

/* 电流变化步进 (每次调整最大值) */
#define CURRENT_STEP_MAX            100     /* 100mA/次 */

/*============================================================================
 *                              时间配置
 *===========================================================================*/

#define CTRL_PERIOD_MS              1000    /* 控制周期 1秒 */
#define TEMP_FILTER_PERIOD_MS       100     /* 温度滤波周期 */
#define STATE_DEBOUNCE_MS           3000    /* 状态切换去抖时间 */

/*============================================================================
 *                              PID配置
 *===========================================================================*/

#define PID_Q_BITS                  8
#define PID_Q_SCALE                 (1 << PID_Q_BITS)

/*============================================================================
 *                              类型定义
 *===========================================================================*/

/**
 * @brief 温度区间枚举
 */
typedef enum {
    TEMP_ZONE_OVERCOLD = 0,     /* 过冷区: T < 0°C */
    TEMP_ZONE_LOW,              /* 低温区: 0°C ~ 10°C */
    TEMP_ZONE_NORMAL,           /* 正常区: 11°C ~ 45°C */
    TEMP_ZONE_HIGH,             /* 高温区: 46°C ~ 60°C */
    TEMP_ZONE_OVERHEAT,         /* 过热区: T > 60°C */
    TEMP_ZONE_MAX
} temp_zone_t;

/**
 * @brief 充电状态枚举
 */
typedef enum {
    CHARGE_STATE_IDLE = 0,          /* 空闲 */
    CHARGE_STATE_WAIT_TEMP,         /* 等待温度恢复 */
    CHARGE_STATE_PRECHECK,          /* 预检查 */
    CHARGE_STATE_LOW_TEMP,          /* 低温充电 */
    CHARGE_STATE_NORMAL,            /* 正常充电 */
    CHARGE_STATE_HIGH_TEMP,         /* 高温降流充电 */
    CHARGE_STATE_THERMAL_PROTECT,   /* 热保护 */
    CHARGE_STATE_COMPLETE,          /* 充电完成 */
    CHARGE_STATE_FAULT,             /* 故障 */
    CHARGE_STATE_MAX
} charge_state_t;

/**
 * @brief 温度趋势枚举
 */
typedef enum {
    TEMP_TREND_FALLING_FAST = -2,   /* 快速下降 (< -0.5°C/s) */
    TEMP_TREND_FALLING = -1,        /* 缓慢下降 */
    TEMP_TREND_STABLE = 0,          /* 稳定 */
    TEMP_TREND_RISING = 1,          /* 缓慢上升 */
    TEMP_TREND_RISING_FAST = 2,     /* 快速上升 (> 0.5°C/s) */
} temp_trend_t;

/**
 * @brief 回滞状态结构体
 */
typedef struct {
    bool in_overcold;       /* 是否处于过冷状态 */
    bool in_high_temp;      /* 是否处于高温状态 */
    bool in_overheat;       /* 是否处于过热状态 */
} hysteresis_state_t;

/**
 * @brief 温度信息结构体
 */
typedef struct {
    int16_t current;            /* 当前温度 (0.1°C) */
    int16_t filtered;           /* 滤波后温度 */
    int16_t previous;           /* 上次温度 */
    int16_t delta;              /* 温度变化率 (0.1°C/s) */
    int16_t ambient_estimate;   /* 估算环境温度 */
    temp_zone_t zone;           /* 当前温度区间 */
    temp_zone_t zone_prev;      /* 上次温度区间 */
    temp_trend_t trend;         /* 温度趋势 */
    hysteresis_state_t hyst;    /* 回滞状态 */
} temp_info_t;

/**
 * @brief PID控制器结构体
 */
typedef struct {
    int32_t kp;                 /* 比例系数 (Q8) */
    int32_t ki;                 /* 积分系数 (Q8) */
    int32_t kd;                 /* 微分系数 (Q8) */
    
    int32_t setpoint;           /* 目标值 */
    int32_t error;              /* 当前误差 */
    int32_t error_prev;         /* 上次误差 */
    int32_t integral;           /* 积分累积 */
    int32_t output;             /* 输出值 */
    
    int32_t integral_max;       /* 积分上限 */
    int32_t integral_min;       /* 积分下限 */
    int32_t output_max;         /* 输出上限 */
    int32_t output_min;         /* 输出下限 */
    
    bool enabled;               /* 使能标志 */
} thermal_pid_t;

/**
 * @brief 充电控制器主结构体
 */
typedef struct {
    /* 状态 */
    charge_state_t state;
    charge_state_t state_prev;
    uint32_t state_time_ms;
    uint32_t state_debounce_ms;
    
    /* 温度信息 */
    temp_info_t temp;
    
    /* 电流控制 */
    int16_t current_target;     /* 目标电流 (mA) */
    int16_t current_actual;     /* 实际电流 (mA) */
    int16_t current_limit;      /* 电流上限 (mA) */
    
    /* PID控制器 */
    thermal_pid_t pid;
    
    /* 统计信息 */
    uint32_t total_charge_time_ms;
    uint32_t high_temp_time_ms;
    uint32_t overcold_time_ms;
    
    /* 标志位 */
    struct {
        uint8_t initialized     : 1;
        uint8_t enabled         : 1;
        uint8_t charger_present : 1;
        uint8_t battery_full    : 1;
        uint8_t temp_fault      : 1;
        uint8_t heating_stopped : 1;
    } flags;
    
    /* 回调函数 */
    void (*cb_set_current)(int16_t current_ma);
    int16_t (*cb_get_temperature)(void);
    int16_t (*cb_get_voltage)(void);
    uint8_t (*cb_get_soc)(void);
    bool (*cb_is_charger_connected)(void);
    void (*cb_on_state_change)(charge_state_t old_state, charge_state_t new_state);
    
} charge_ctrl_t;

/*============================================================================
 *                              API函数
 *===========================================================================*/

/**
 * @brief 初始化充电控制器
 */
int charge_ctrl_init(charge_ctrl_t *ctrl);

/**
 * @brief 注册回调函数
 */
void charge_ctrl_set_callbacks(
    charge_ctrl_t *ctrl,
    void (*set_current)(int16_t),
    int16_t (*get_temp)(void),
    int16_t (*get_voltage)(void),
    uint8_t (*get_soc)(void),
    bool (*is_charger)(void),
    void (*on_state_change)(charge_state_t, charge_state_t)
);

/**
 * @brief 启动充电
 */
int charge_ctrl_start(charge_ctrl_t *ctrl);

/**
 * @brief 停止充电
 */
void charge_ctrl_stop(charge_ctrl_t *ctrl);

/**
 * @brief 通知加热已停止
 */
void charge_ctrl_heating_stopped(charge_ctrl_t *ctrl);

/**
 * @brief 周期处理函数 (需定时调用)
 */
void charge_ctrl_process(charge_ctrl_t *ctrl, uint32_t elapsed_ms);

/**
 * @brief 获取当前状态
 */
charge_state_t charge_ctrl_get_state(charge_ctrl_t *ctrl);

/**
 * @brief 获取温度区间
 */
temp_zone_t charge_ctrl_get_temp_zone(charge_ctrl_t *ctrl);

/**
 * @brief 获取状态字符串
 */
const char* charge_ctrl_state_str(charge_state_t state);

/**
 * @brief 获取温度区间字符串
 */
const char* charge_ctrl_zone_str(temp_zone_t zone);

/**
 * @brief 获取诊断信息
 */
void charge_ctrl_get_info(charge_ctrl_t *ctrl, char *buf, int size);

#ifdef __cplusplus
}
#endif

#endif /* __CHARGE_THERMAL_CTRL_H__ */
```

------

## 📄 charge_thermal_ctrl.c

```c
/**
 * @file charge_thermal_ctrl.c
 * @brief 便携式加热设备充电温控管理系统实现 V2.0
 */

#include "charge_thermal_ctrl.h"
#include <string.h>
#include <stdio.h>

/*============================================================================
 *                              内部宏定义
 *===========================================================================*/

#define ABS(x)              (((x) >= 0) ? (x) : -(x))
#define CLAMP(x, lo, hi)    (((x) < (lo)) ? (lo) : (((x) > (hi)) ? (hi) : (x)))
#define MIN(a, b)           (((a) < (b)) ? (a) : (b))
#define MAX(a, b)           (((a) > (b)) ? (a) : (b))

#define Q8_MUL(a, b)        (((int32_t)(a) * (b)) >> PID_Q_BITS)
#define Q8_FROM_FLOAT(x)    ((int32_t)((x) * PID_Q_SCALE))
#define Q8_TO_INT(x)        ((x) >> PID_Q_BITS)

/* 温度变化率阈值 (0.1°C/s) */
#define DELTA_FAST_THRESHOLD    5   /* 0.5°C/s */
#define DELTA_SLOW_THRESHOLD    1   /* 0.1°C/s */

/*============================================================================
 *                              温度滤波器
 *===========================================================================*/

#define FILTER_SIZE     8

typedef struct {
    int16_t buf[FILTER_SIZE];
    uint8_t idx;
    uint8_t cnt;
    int32_t sum;
} filter_t;

static filter_t g_filter;

static void filter_init(void)
{
    memset(&g_filter, 0, sizeof(g_filter));
}

static int16_t filter_update(int16_t val)
{
    if (g_filter.cnt >= FILTER_SIZE) {
        g_filter.sum -= g_filter.buf[g_filter.idx];
    }
    
    g_filter.buf[g_filter.idx] = val;
    g_filter.sum += val;
    g_filter.idx = (g_filter.idx + 1) % FILTER_SIZE;
    
    if (g_filter.cnt < FILTER_SIZE) {
        g_filter.cnt++;
    }
    
    return (int16_t)(g_filter.sum / g_filter.cnt);
}

/*============================================================================
 *                              回滞处理核心
 *===========================================================================*/

/**
 * @brief 带回滞的温度区间判断
 * 
 * 回滞逻辑:
 * 1. 过冷: T < 0°C 进入, T > 5°C 退出
 * 2. 高温: T > 45°C 进入, T < 40°C 退出  
 * 3. 过热: T > 60°C 进入, T < 55°C 退出
 */
static temp_zone_t determine_temp_zone_with_hysteresis(temp_info_t *temp)
{
    int16_t t = temp->filtered;
    hysteresis_state_t *hyst = &temp->hyst;
    
    /*
     * 过热判断 (最高优先级)
     * ─────────────────────────────────────────
     * 进入条件: T > 60°C
     * 退出条件: T < 55°C
     */
    if (!hyst->in_overheat) {
        /* 当前不在过热状态 */
        if (t > TEMP_OVERHEAT_THRESHOLD) {
            hyst->in_overheat = true;   /* 进入过热 */
        }
    } else {
        /* 当前在过热状态 */
        if (t < TEMP_OVERHEAT_HYSTERESIS) {
            hyst->in_overheat = false;  /* 退出过热 */
        }
    }
    
    if (hyst->in_overheat) {
        return TEMP_ZONE_OVERHEAT;
    }
    
    /*
     * 过冷判断
     * ─────────────────────────────────────────
     * 进入条件: T < 0°C
     * 退出条件: T > 5°C
     */
    if (!hyst->in_overcold) {
        /* 当前不在过冷状态 */
        if (t < TEMP_COLD_THRESHOLD) {
            hyst->in_overcold = true;   /* 进入过冷 */
        }
    } else {
        /* 当前在过冷状态 */
        if (t > TEMP_COLD_HYSTERESIS) {
            hyst->in_overcold = false;  /* 退出过冷 */
        }
    }
    
    if (hyst->in_overcold) {
        return TEMP_ZONE_OVERCOLD;
    }
    
    /*
     * 高温判断
     * ─────────────────────────────────────────
     * 进入条件: T > 45°C (即 >= 46°C)
     * 退出条件: T < 40°C
     */
    if (!hyst->in_high_temp) {
        /* 当前不在高温状态 */
        if (t >= TEMP_HIGH_THRESHOLD) {
            hyst->in_high_temp = true;  /* 进入高温 */
        }
    } else {
        /* 当前在高温状态 */
        if (t < TEMP_HIGH_HYSTERESIS) {
            hyst->in_high_temp = false; /* 退出高温 */
        }
    }
    
    if (hyst->in_high_temp) {
        return TEMP_ZONE_HIGH;
    }
    
    /*
     * 低温区判断 (0°C ~ 10°C)
     * ─────────────────────────────────────────
     * 注意: 已经排除了过冷状态，所以这里 T >= 0°C
     */
    if (t <= TEMP_LOW_THRESHOLD) {
        return TEMP_ZONE_LOW;
    }
    
    /*
     * 正常区 (11°C ~ 45°C)
     * ─────────────────────────────────────────
     */
    return TEMP_ZONE_NORMAL;
}

/**
 * @brief 分析温度趋势
 */
static temp_trend_t analyze_trend(int16_t delta)
{
    if (delta < -DELTA_FAST_THRESHOLD) {
        return TEMP_TREND_FALLING_FAST;
    } else if (delta < -DELTA_SLOW_THRESHOLD) {
        return TEMP_TREND_FALLING;
    } else if (delta > DELTA_FAST_THRESHOLD) {
        return TEMP_TREND_RISING_FAST;
    } else if (delta > DELTA_SLOW_THRESHOLD) {
        return TEMP_TREND_RISING;
    }
    return TEMP_TREND_STABLE;
}

/**
 * @brief 更新温度信息
 */
static void update_temp_info(charge_ctrl_t *ctrl)
{
    int16_t raw_temp;
    
    /* 获取原始温度 */
    if (ctrl->cb_get_temperature) {
        raw_temp = ctrl->cb_get_temperature();
    } else {
        raw_temp = 250;  /* 默认25°C */
    }
    
    /* 保存上次温度 */
    ctrl->temp.previous = ctrl->temp.filtered;
    
    /* 滤波 */
    ctrl->temp.current = raw_temp;
    ctrl->temp.filtered = filter_update(raw_temp);
    
    /* 计算变化率 (每秒) */
    ctrl->temp.delta = ctrl->temp.filtered - ctrl->temp.previous;
    
    /* 分析趋势 */
    ctrl->temp.trend = analyze_trend(ctrl->temp.delta);
    
    /* 保存上次区间 */
    ctrl->temp.zone_prev = ctrl->temp.zone;
    
    /* 带回滞的区间判断 */
    ctrl->temp.zone = determine_temp_zone_with_hysteresis(&ctrl->temp);
    
    /* 估算环境温度 (取历史最低值) */
    if (ctrl->temp.filtered < ctrl->temp.ambient_estimate || 
        ctrl->temp.ambient_estimate == 0) {
        ctrl->temp.ambient_estimate = ctrl->temp.filtered;
    }
}

/*============================================================================
 *                              PID控制器
 *===========================================================================*/

static void pid_init(thermal_pid_t *pid)
{
    memset(pid, 0, sizeof(thermal_pid_t));
    
    /*
     * PID参数设计:
     * - 目标: 控制温度在42°C附近
     * - 输出: 充电电流
     * - 误差 = 目标温度 - 当前温度
     * - 温度高时误差为负，减小电流
     */
    pid->kp = Q8_FROM_FLOAT(40.0f);     /* 每1°C调整40mA */
    pid->ki = Q8_FROM_FLOAT(3.0f);      /* 积分系数 */
    pid->kd = Q8_FROM_FLOAT(15.0f);     /* 微分系数 */
    
    pid->setpoint = TEMP_PID_TARGET;
    
    pid->integral_max = Q8_FROM_FLOAT(300.0f);
    pid->integral_min = Q8_FROM_FLOAT(-300.0f);
    pid->output_max = CURRENT_NORMAL;
    pid->output_min = CURRENT_MIN;
    
    pid->enabled = false;
}

static void pid_reset(thermal_pid_t *pid)
{
    pid->error = 0;
    pid->error_prev = 0;
    pid->integral = 0;
    pid->output = CURRENT_NORMAL;
}

static void pid_enable(thermal_pid_t *pid, bool enable)
{
    if (enable && !pid->enabled) {
        pid_reset(pid);
    }
    pid->enabled = enable;
}

/**
 * @brief PID计算
 * 
 * 反向控制: 温度越高，电流越小
 */
static int16_t pid_compute(thermal_pid_t *pid, int16_t current_temp)
{
    if (!pid->enabled) {
        return (int16_t)pid->output;
    }
    
    int32_t p_term, i_term, d_term;
    
    /* 保存上次误差 */
    pid->error_prev = pid->error;
    
    /* 计算误差 */
    pid->error = pid->setpoint - current_temp;
    
    /* 比例项 */
    p_term = Q8_MUL(pid->kp, pid->error);
    
    /* 积分项 - 积分分离 */
    if (ABS(pid->error) < (10 * TEMP_SCALE)) {  /* 10°C以内 */
        pid->integral += pid->error;
        pid->integral = CLAMP(pid->integral, pid->integral_min, pid->integral_max);
    }
    i_term = Q8_MUL(pid->ki, pid->integral);
    
    /* 微分项 */
    d_term = Q8_MUL(pid->kd, pid->error - pid->error_prev);
    
    /* 计算输出: 基础电流 + PID调节 */
    int32_t output = CURRENT_NORMAL + Q8_TO_INT(p_term + i_term + d_term);
    
    /* 限幅 */
    output = CLAMP(output, pid->output_min, pid->output_max);
    
    pid->output = output;
    
    return (int16_t)output;
}

/*============================================================================
 *                              电流计算
 *===========================================================================*/

/**
 * @brief 根据温度区间计算基础电流
 */
static int16_t calc_zone_base_current(temp_zone_t zone, int16_t temp)
{
    switch (zone) {
        case TEMP_ZONE_OVERCOLD:
        case TEMP_ZONE_OVERHEAT:
            /* 禁止充电 */
            return CURRENT_OFF;
            
        case TEMP_ZONE_LOW:
            /* 低温区: 小电流充电 (0~10°C) */
            /* 温度越低电流越小 */
            {
                int32_t ratio = (temp - TEMP_COLD_THRESHOLD) * 100 / 
                               (TEMP_LOW_THRESHOLD - TEMP_COLD_THRESHOLD);
                ratio = CLAMP(ratio, 0, 100);
                return CURRENT_MIN + (CURRENT_LOW_TEMP - CURRENT_MIN) * ratio / 100;
            }
            
        case TEMP_ZONE_NORMAL:
            /* 正常区: 全速充电 (11~45°C) */
            return CURRENT_MAX;
            
        case TEMP_ZONE_HIGH:
            /* 高温区: 降流充电 (46~60°C) */
            /* 使用PID控制，这里返回上限 */
            return CURRENT_HIGH_TEMP;
            
        default:
            return CURRENT_OFF;
    }
}

/**
 * @brief 根据温度趋势调整电流
 */
static int16_t adjust_by_trend(int16_t base, temp_trend_t trend, temp_zone_t zone)
{
    int16_t adjusted = base;
    
    switch (trend) {
        case TEMP_TREND_RISING_FAST:
            /* 温度快速上升，大幅降流 */
            adjusted = base * 50 / 100;
            break;
            
        case TEMP_TREND_RISING:
            /* 温度上升，适度降流 */
            adjusted = base * 80 / 100;
            break;
            
        case TEMP_TREND_STABLE:
            /* 稳定 */
            break;
            
        case TEMP_TREND_FALLING:
            /* 温度下降，可增加电流 */
            if (zone == TEMP_ZONE_NORMAL || zone == TEMP_ZONE_LOW) {
                adjusted = MIN(base * 110 / 100, CURRENT_MAX);
            }
            break;
            
        case TEMP_TREND_FALLING_FAST:
            /* 温度快速下降 */
            if (zone == TEMP_ZONE_NORMAL) {
                adjusted = MIN(base * 120 / 100, CURRENT_MAX);
            }
            break;
    }
    
    return CLAMP(adjusted, CURRENT_MIN, CURRENT_MAX);
}

/**
 * @brief 综合计算目标电流
 */
static int16_t calc_target_current(charge_ctrl_t *ctrl)
{
    int16_t base_current;
    int16_t pid_current;
    int16_t final_current;
    
    /* 1. 获取区间基础电流 */
    base_current = calc_zone_base_current(ctrl->temp.zone, ctrl->temp.filtered);
    
    /* 禁止充电的区间直接返回 */
    if (base_current == CURRENT_OFF) {
        return CURRENT_OFF;
    }
    
    /* 2. 高温区使用PID控制 */
    if (ctrl->temp.zone == TEMP_ZONE_HIGH) {
        pid_enable(&ctrl->pid, true);
        pid_current = pid_compute(&ctrl->pid, ctrl->temp.filtered);
        /* PID输出不超过区间限制 */
        base_current = MIN(pid_current, base_current);
    } else {
        pid_enable(&ctrl->pid, false);
    }
    
    /* 3. 根据趋势调整 */
    final_current = adjust_by_trend(base_current, ctrl->temp.trend, ctrl->temp.zone);
    
    /* 4. 应用电流上限 */
    final_current = MIN(final_current, ctrl->current_limit);
    
    return final_current;
}

/*============================================================================
 *                              状态机
 *===========================================================================*/

/**
 * @brief 切换状态
 */
static void change_state(charge_ctrl_t *ctrl, charge_state_t new_state)
{
    if (ctrl->state != new_state) {
        charge_state_t old_state = ctrl->state;
        ctrl->state_prev = old_state;
        ctrl->state = new_state;
        ctrl->state_time_ms = 0;
        ctrl->state_debounce_ms = 0;
        
        /* 状态变化回调 */
        if (ctrl->cb_on_state_change) {
            ctrl->cb_on_state_change(old_state, new_state);
        }
    }
}

/**
 * @brief 空闲状态处理
 */
static void handle_idle(charge_ctrl_t *ctrl)
{
    ctrl->current_target = CURRENT_OFF;
    
    /* 检查是否可以开始充电 */
    if (ctrl->flags.enabled && ctrl->flags.charger_present) {
        change_state(ctrl, CHARGE_STATE_PRECHECK);
    }
}

/**
 * @brief 等待温度恢复状态
 */
static void handle_wait_temp(charge_ctrl_t *ctrl)
{
    ctrl->current_target = CURRENT_OFF;
    
    /* 根据之前的温度问题决定等待条件 */
    switch (ctrl->temp.zone) {
        case TEMP_ZONE_OVERCOLD:
            /* 等待温度升高到5°C以上 (回滞) */
            ctrl->overcold_time_ms += CTRL_PERIOD_MS;
            break;
            
        case TEMP_ZONE_OVERHEAT:
            /* 等待温度降低到55°C以下 (回滞) */
            ctrl->high_temp_time_ms += CTRL_PERIOD_MS;
            break;
            
        case TEMP_ZONE_LOW:
        case TEMP_ZONE_NORMAL:
        case TEMP_ZONE_HIGH:
            /* 温度已恢复到可充电范围 */
            change_state(ctrl, CHARGE_STATE_PRECHECK);
            return;
            
        default:
            break;
    }
    
    /* 超时处理 */
    if (ctrl->state_time_ms > 600000) {  /* 10分钟 */
        /* 长时间无法恢复，报故障 */
        change_state(ctrl, CHARGE_STATE_FAULT);
    }
}

/**
 * @brief 预检查状态
 */
static void handle_precheck(charge_ctrl_t *ctrl)
{
    ctrl->current_target = CURRENT_OFF;
    
    /* 检查充电器 */
    if (!ctrl->flags.charger_present) {
        change_state(ctrl, CHARGE_STATE_IDLE);
        return;
    }
    
    /* 检查电池是否已满 */
    if (ctrl->cb_get_soc && ctrl->cb_get_soc() >= 100) {
        change_state(ctrl, CHARGE_STATE_COMPLETE);
        return;
    }
    
    /* 检查温度区间 */
    switch (ctrl->temp.zone) {
        case TEMP_ZONE_OVERCOLD:
        case TEMP_ZONE_OVERHEAT:
            change_state(ctrl, CHARGE_STATE_WAIT_TEMP);
            break;
            
        case TEMP_ZONE_LOW:
            change_state(ctrl, CHARGE_STATE_LOW_TEMP);
            break;
            
        case TEMP_ZONE_NORMAL:
            change_state(ctrl, CHARGE_STATE_NORMAL);
            break;
            
        case TEMP_ZONE_HIGH:
            change_state(ctrl, CHARGE_STATE_HIGH_TEMP);
            break;
            
        default:
            change_state(ctrl, CHARGE_STATE_FAULT);
            break;
    }
}

/**
 * @brief 低温充电状态 (0~10°C)
 */
static void handle_low_temp(charge_ctrl_t *ctrl)
{
    /* 检查温度区间变化 */
    switch (ctrl->temp.zone) {
        case TEMP_ZONE_OVERCOLD:
            /* 温度过低，停止充电 */
            change_state(ctrl, CHARGE_STATE_WAIT_TEMP);
            return;
            
        case TEMP_ZONE_NORMAL:
            /* 温度升高到正常区 */
            change_state(ctrl, CHARGE_STATE_NORMAL);
            return;
            
        case TEMP_ZONE_HIGH:
        case TEMP_ZONE_OVERHEAT:
            /* 不应该从低温直接跳到高温，可能是传感器故障 */
            change_state(ctrl, CHARGE_STATE_FAULT);
            return;
            
        default:
            break;
    }
    
    /* 检查是否充满 */
    if (ctrl->cb_get_soc && ctrl->cb_get_soc() >= 100) {
        change_state(ctrl, CHARGE_STATE_COMPLETE);
        return;
    }
    
    /* 计算低温充电电流 */
    ctrl->current_target = calc_target_current(ctrl);
}

/**
 * @brief 正常充电状态 (11~45°C)
 */
static void handle_normal(charge_ctrl_t *ctrl)
{
    /* 检查温度区间变化 */
    switch (ctrl->temp.zone) {
        case TEMP_ZONE_OVERCOLD:
            change_state(ctrl, CHARGE_STATE_WAIT_TEMP);
            return;
            
        case TEMP_ZONE_LOW:
            change_state(ctrl, CHARGE_STATE_LOW_TEMP);
            return;
            
        case TEMP_ZONE_HIGH:
            /* 进入高温区 (T > 45°C) */
            /* 使用去抖动，避免边界抖动 */
            ctrl->state_debounce_ms += CTRL_PERIOD_MS;
            if (ctrl->state_debounce_ms >= STATE_DEBOUNCE_MS) {
                change_state(ctrl, CHARGE_STATE_HIGH_TEMP);
            }
            return;
            
        case TEMP_ZONE_OVERHEAT:
            change_state(ctrl, CHARGE_STATE_THERMAL_PROTECT);
            return;
            
        default:
            ctrl->state_debounce_ms = 0;
            break;
    }
    
    /* 检查是否充满 */
    if (ctrl->cb_get_soc && ctrl->cb_get_soc() >= 100) {
        change_state(ctrl, CHARGE_STATE_COMPLETE);
        return;
    }
    
    /* 正常充电，最大电流 */
    ctrl->current_target = calc_target_current(ctrl);
    ctrl->high_temp_time_ms = 0;
}

/**
 * @brief 高温降流充电状态 (46~60°C)
 */
static void handle_high_temp(charge_ctrl_t *ctrl)
{
    ctrl->high_temp_time_ms += CTRL_PERIOD_MS;
    
    /* 检查温度区间变化 */
    switch (ctrl->temp.zone) {
        case TEMP_ZONE_OVERHEAT:
            /* 温度过高，进入热保护 */
            change_state(ctrl, CHARGE_STATE_THERMAL_PROTECT);
            return;
            
        case TEMP_ZONE_NORMAL:
            /* 
             * 温度降到正常区 
             * 注意: 由于回滞，需要 T < 40°C 才会退出高温状态
             */
            ctrl->state_debounce_ms += CTRL_PERIOD_MS;
            if (ctrl->state_debounce_ms >= STATE_DEBOUNCE_MS) {
                change_state(ctrl, CHARGE_STATE_NORMAL);
            }
            return;
            
        case TEMP_ZONE_LOW:
        case TEMP_ZONE_OVERCOLD:
            /* 异常情况 */
            change_state(ctrl, CHARGE_STATE_PRECHECK);
            return;
            
        default:
            ctrl->state_debounce_ms = 0;
            break;
    }
    
    /* 检查是否充满 */
    if (ctrl->cb_get_soc && ctrl->cb_get_soc() >= 100) {
        change_state(ctrl, CHARGE_STATE_COMPLETE);
        return;
    }
    
    /* 使用PID控制电流 */
    ctrl->current_target = calc_target_current(ctrl);
}

/**
 * @brief 热保护状态 (T > 60°C)
 */
static void handle_thermal_protect(charge_ctrl_t *ctrl)
{
    ctrl->current_target = CURRENT_OFF;
    ctrl->high_temp_time_ms += CTRL_PERIOD_MS;
    
    /* 检查温度是否降低 */
    /* 由于回滞，需要 T < 55°C 才会退出过热状态 */
    if (ctrl->temp.zone != TEMP_ZONE_OVERHEAT) {
        /* 温度已降低，重新检查 */
        change_state(ctrl, CHARGE_STATE_PRECHECK);
    }
}

/**
 * @brief 充电完成状态
 */
static void handle_complete(charge_ctrl_t *ctrl)
{
    ctrl->current_target = CURRENT_OFF;
    ctrl->flags.battery_full = 1;
    
    /* 检查是否需要补充充电 */
    if (ctrl->cb_get_soc && ctrl->cb_get_soc() < 95) {
        ctrl->flags.battery_full = 0;
        change_state(ctrl, CHARGE_STATE_PRECHECK);
    }
}

/**
 * @brief 故障状态
 */
static void handle_fault(charge_ctrl_t *ctrl)
{
    ctrl->current_target = CURRENT_OFF;
    ctrl->flags.temp_fault = 1;
    
    /* 等待手动复位或条件恢复 */
}

/*============================================================================
 *                              API实现
 *===========================================================================*/

int charge_ctrl_init(charge_ctrl_t *ctrl)
{
    if (ctrl == NULL) {
        return -1;
    }
    
    memset(ctrl, 0, sizeof(charge_ctrl_t));
    
    /* 初始化滤波器 */
    filter_init();
    
    /* 初始化PID */
    pid_init(&ctrl->pid);
    
    /* 默认参数 */
    ctrl->state = CHARGE_STATE_IDLE;
    ctrl->current_limit = CURRENT_MAX;
    ctrl->flags.initialized = 1;
    
    /* 初始化回滞状态 */
    ctrl->temp.hyst.in_overcold = false;
    ctrl->temp.hyst.in_high_temp = false;
    ctrl->temp.hyst.in_overheat = false;
    
    return 0;
}

void charge_ctrl_set_callbacks(
    charge_ctrl_t *ctrl,
    void (*set_current)(int16_t),
    int16_t (*get_temp)(void),
    int16_t (*get_voltage)(void),
    uint8_t (*get_soc)(void),
    bool (*is_charger)(void),
    void (*on_state_change)(charge_state_t, charge_state_t))
{
    if (ctrl == NULL) return;
    
    ctrl->cb_set_current = set_current;
    ctrl->cb_get_temperature = get_temp;
    ctrl->cb_get_voltage = get_voltage;
    ctrl->cb_get_soc = get_soc;
    ctrl->cb_is_charger_connected = is_charger;
    ctrl->cb_on_state_change = on_state_change;
}

int charge_ctrl_start(charge_ctrl_t *ctrl)
{
    if (ctrl == NULL || !ctrl->flags.initialized) {
        return -1;
    }
    
    ctrl->flags.enabled = 1;
    ctrl->flags.battery_full = 0;
    ctrl->flags.temp_fault = 0;
    
    /* 重置统计 */
    ctrl->total_charge_time_ms = 0;
    ctrl->high_temp_time_ms = 0;
    ctrl->overcold_time_ms = 0;
    
    /* 重置PID */
    pid_reset(&ctrl->pid);
    
    /* 触发状态转换 */
    if (ctrl->state == CHARGE_STATE_IDLE || 
        ctrl->state == CHARGE_STATE_FAULT) {
        change_state(ctrl, CHARGE_STATE_PRECHECK);
    }
    
    return 0;
}

void charge_ctrl_stop(charge_ctrl_t *ctrl)
{
    if (ctrl == NULL) return;
    
    ctrl->flags.enabled = 0;
    ctrl->current_target = CURRENT_OFF;
    
    /* 应用电流 */
    if (ctrl->cb_set_current) {
        ctrl->cb_set_current(CURRENT_OFF);
    }
    
    change_state(ctrl, CHARGE_STATE_IDLE);
}

void charge_ctrl_heating_stopped(charge_ctrl_t *ctrl)
{
    if (ctrl == NULL) return;
    
    ctrl->flags.heating_stopped = 1;
    ctrl->temp.ambient_estimate = 0;  /* 重新估算环境温度 */
    
    /* 重置回滞状态，让系统重新评估 */
    ctrl->temp.hyst.in_overcold = false;
    ctrl->temp.hyst.in_high_temp = false;
    ctrl->temp.hyst.in_overheat = false;
}

void charge_ctrl_process(charge_ctrl_t *ctrl, uint32_t elapsed_ms)
{
    if (ctrl == NULL || !ctrl->flags.initialized) {
        return;
    }
    
    /* 更新时间 */
    ctrl->state_time_ms += elapsed_ms;
    if (ctrl->flags.enabled && ctrl->current_actual > 0) {
        ctrl->total_charge_time_ms += elapsed_ms;
    }
    
    /* 更新充电器状态 */
    if (ctrl->cb_is_charger_connected) {
        ctrl->flags.charger_present = ctrl->cb_is_charger_connected() ? 1 : 0;
    }
    
    /* 充电器断开时返回空闲 */
    if (!ctrl->flags.charger_present && 
        ctrl->state != CHARGE_STATE_IDLE &&
        ctrl->state != CHARGE_STATE_FAULT) {
        change_state(ctrl, CHARGE_STATE_IDLE);
    }
    
    /* 更新温度信息 */
    update_temp_info(ctrl);
    
    /* 状态机处理 */
    switch (ctrl->state) {
        case CHARGE_STATE_IDLE:
            handle_idle(ctrl);
            break;
        case CHARGE_STATE_WAIT_TEMP:
            handle_wait_temp(ctrl);
            break;
        case CHARGE_STATE_PRECHECK:
            handle_precheck(ctrl);
            break;
        case CHARGE_STATE_LOW_TEMP:
            handle_low_temp(ctrl);
            break;
        case CHARGE_STATE_NORMAL:
            handle_normal(ctrl);
            break;
        case CHARGE_STATE_HIGH_TEMP:
            handle_high_temp(ctrl);
            break;
        case CHARGE_STATE_THERMAL_PROTECT:
            handle_thermal_protect(ctrl);
            break;
        case CHARGE_STATE_COMPLETE:
            handle_complete(ctrl);
            break;
        case CHARGE_STATE_FAULT:
            handle_fault(ctrl);
            break;
        default:
            change_state(ctrl, CHARGE_STATE_IDLE);
            break;
    }
    
    /* 电流渐变处理 */
    if (ctrl->cb_set_current) {
        int16_t diff = ctrl->current_target - ctrl->current_actual;
        
        if (diff > CURRENT_STEP_MAX) {
            ctrl->current_actual += CURRENT_STEP_MAX;
        } else if (diff < -CURRENT_STEP_MAX) {
            ctrl->current_actual -= CURRENT_STEP_MAX;
        } else {
            ctrl->current_actual = ctrl->current_target;
        }
        
        ctrl->cb_set_current(ctrl->current_actual);
    }
}

charge_state_t charge_ctrl_get_state(charge_ctrl_t *ctrl)
{
    return ctrl ? ctrl->state : CHARGE_STATE_IDLE;
}

temp_zone_t charge_ctrl_get_temp_zone(charge_ctrl_t *ctrl)
{
    return ctrl ? ctrl->temp.zone : TEMP_ZONE_NORMAL;
}

const char* charge_ctrl_state_str(charge_state_t state)
{
    static const char* names[] = {
        "IDLE", "WAIT_TEMP", "PRECHECK", "LOW_TEMP",
        "NORMAL", "HIGH_TEMP", "THERMAL_PROTECT", 
        "COMPLETE", "FAULT", "UNKNOWN"
    };
    return (state < CHARGE_STATE_MAX) ? names[state] : names[CHARGE_STATE_MAX];
}

const char* charge_ctrl_zone_str(temp_zone_t zone)
{
    static const char* names[] = {
        "OVERCOLD", "LOW", "NORMAL", "HIGH", "OVERHEAT", "UNKNOWN"
    };
    return (zone < TEMP_ZONE_MAX) ? names[zone] : names[TEMP_ZONE_MAX];
}

void charge_ctrl_get_info(charge_ctrl_t *ctrl, char *buf, int size)
{
    if (ctrl == NULL || buf == NULL || size < 200) return;
    
    snprintf(buf, size,
        "State: %s (%.1fs)\n"
        "Temp: %.1fC [%s] Trend:%+d\n"
        "Hyst: Cold=%d High=%d Hot=%d\n"
        "Current: %dmA -> %dmA\n"
        "PID: En=%d Err=%d Int=%ld Out=%ld\n"
        "Time: Total=%.1fs HiTemp=%.1fs\n",
        charge_ctrl_state_str(ctrl->state),
        ctrl->state_time_ms / 1000.0f,
        ctrl->temp.filtered / 10.0f,
        charge_ctrl_zone_str(ctrl->temp.zone),
        ctrl->temp.trend,
        ctrl->temp.hyst.in_overcold,
        ctrl->temp.hyst.in_high_temp,
        ctrl->temp.hyst.in_overheat,
        ctrl->current_actual,
        ctrl->current_target,
        ctrl->pid.enabled,
        (int)ctrl->pid.error,
        (long)ctrl->pid.integral,
        (long)ctrl->pid.output,
        ctrl->total_charge_time_ms / 1000.0f,
        ctrl->high_temp_time_ms / 1000.0f
    );
}
```

------

## 📄 charge_thermal_test.c - 测试与仿真

```c
/**
 * @file charge_thermal_test.c
 * @brief 充电温控系统测试与仿真
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "charge_thermal_ctrl.h"

/*============================================================================
 *                              仿真环境
 *===========================================================================*/

typedef struct {
    float temp;             /* 电池温度 (°C) */
    float ambient;          /* 环境温度 (°C) */
    float current;          /* 充电电流 (mA) */
    float soc;              /* 电量 (%) */
    bool charger;           /* 充电器连接 */
    
    /* 热模型参数 */
    float r_th;             /* 热阻 */
    float c_th;             /* 热容 */
} sim_t;

static sim_t g_sim;
static charge_ctrl_t g_ctrl;

/*============================================================================
 *                              仿真模型
 *===========================================================================*/

void sim_init(float ambient, float init_temp, float init_soc)
{
    memset(&g_sim, 0, sizeof(g_sim));
    g_sim.ambient = ambient;
    g_sim.temp = init_temp;
    g_sim.soc = init_soc;
    g_sim.charger = true;
    g_sim.r_th = 5.0f;
    g_sim.c_th = 50.0f;
}

void sim_step(float dt)
{
    /* 充电产热 */
    float i_amp = g_sim.current / 1000.0f;
    float p_heat = i_amp * i_amp * 0.5f;  /* P = I²R */
    
    /* 散热 */
    float p_cool = (g_sim.temp - g_sim.ambient) / g_sim.r_th;
    
    /* 温度变化 */
    float dt_temp = (p_heat - p_cool) * dt / g_sim.c_th * 100;
    g_sim.temp += dt_temp;
    g_sim.temp = (g_sim.temp < g_sim.ambient) ? g_sim.ambient : g_sim.temp;
    
    /* SOC变化 */
    float cap = 3000.0f;  /* 3000mAh */
    g_sim.soc += (g_sim.current * dt / 3600.0f) / cap * 100.0f;
    if (g_sim.soc > 100.0f) g_sim.soc = 100.0f;
}

/*============================================================================
 *                              回调函数
 *===========================================================================*/

void cb_set_current(int16_t ma)
{
    g_sim.current = (float)ma;
}

int16_t cb_get_temp(void)
{
    return (int16_t)(g_sim.temp * 10);
}

int16_t cb_get_voltage(void)
{
    return 3700;
}

uint8_t cb_get_soc(void)
{
    return (uint8_t)g_sim.soc;
}

bool cb_is_charger(void)
{
    return g_sim.charger;
}

void cb_state_change(charge_state_t old_s, charge_state_t new_s)
{
    printf("  >>> 状态变化: %s -> %s\n", 
           charge_ctrl_state_str(old_s),
           charge_ctrl_state_str(new_s));
}

/*============================================================================
 *                              测试辅助
 *===========================================================================*/

void print_header(const char* title)
{
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════════╗\n");
    printf("║ %-66s ║\n", title);
    printf("╚════════════════════════════════════════════════════════════════════╝\n\n");
}

void print_table_header(void)
{
    printf("┌────────┬────────────────┬────────┬──────────┬────────┬────────┬───────┐\n");
    printf("│ Time   │ State          │ Temp   │ Zone     │ Current│ Target │ SOC   │\n");
    printf("├────────┼────────────────┼────────┼──────────┼────────┼────────┼───────┤\n");
}

void print_table_row(float time)
{
    printf("│ %5.1fs │ %-14s │ %5.1f°C│ %-8s │ %4.0fmA │ %4dmA │ %4.1f%% │\n",
           time,
           charge_ctrl_state_str(g_ctrl.state),
           g_sim.temp,
           charge_ctrl_zone_str(g_ctrl.temp.zone),
           g_sim.current,
           g_ctrl.current_target,
           g_sim.soc);
}

void print_table_footer(void)
{
    printf("└────────┴────────────────┴────────┴──────────┴────────┴────────┴───────┘\n");
}

void print_hysteresis_status(void)
{
    printf("回滞状态: 过冷=%d 高温=%d 过热=%d\n",
           g_ctrl.temp.hyst.in_overcold,
           g_ctrl.temp.hyst.in_high_temp,
           g_ctrl.temp.hyst.in_overheat);
}

/*============================================================================
 *                              测试场景
 *===========================================================================*/

/**
 * @brief 测试1: 高温回滞测试 (45°C/40°C边界)
 */
void test_high_temp_hysteresis(void)
{
    print_header("测试1: 高温回滞测试 (进入>45°C, 退出<40°C)");
    
    printf("场景: 温度从35°C升到50°C，再降回35°C\n");
    printf("预期: 超过45°C进入高温区，必须降到40°C以下才退出\n\n");
    
    sim_init(25.0f, 35.0f, 20.0f);
    
    charge_ctrl_init(&g_ctrl);
    charge_ctrl_set_callbacks(&g_ctrl, cb_set_current, cb_get_temp,
                              cb_get_voltage, cb_get_soc, cb_is_charger,
                              cb_state_change);
    charge_ctrl_start(&g_ctrl);
    
    print_table_header();
    
    float dt = 1.0f;
    float temp_target = 35.0f;
    int phase = 0;
    
    for (float t = 0; t < 300; t += dt) {
        /* 温度控制: 模拟温度变化 */
        if (phase == 0 && t > 10) {
            temp_target = 50.0f;  /* 升温 */
            phase = 1;
            printf("│        │ >>> 开始升温到50°C                                      │\n");
        }
        if (phase == 1 && g_sim.temp > 49) {
            temp_target = 35.0f;  /* 降温 */
            phase = 2;
            printf("│        │ >>> 开始降温到35°C                                      │\n");
        }
        
        /* 简单温度模拟 */
        if (g_sim.temp < temp_target) {
            g_sim.temp += 0.5f;
        } else if (g_sim.temp > temp_target) {
            g_sim.temp -= 0.3f;
        }
        
        charge_ctrl_process(&g_ctrl, (uint32_t)(dt * 1000));
        sim_step(dt);
        
        if ((int)t % 5 == 0) {
            print_table_row(t);
        }
    }
    
    print_table_footer();
    print_hysteresis_status();
}

/**
 * @brief 测试2: 过热回滞测试 (60°C/55°C边界)
 */
void test_overheat_hysteresis(void)
{
    print_header("测试2: 过热回滞测试 (进入>60°C, 退出<55°C)");
    
    printf("场景: 温度从50°C升到65°C，再降回50°C\n");
    printf("预期: 超过60°C停止充电，必须降到55°C以下才恢复\n\n");
    
    sim_init(30.0f, 50.0f, 30.0f);
    
    charge_ctrl_init(&g_ctrl);
    charge_ctrl_set_callbacks(&g_ctrl, cb_set_current, cb_
```





```
继续完成未完成代码
```

![img](https://ai.eaglelab.tcl.com/images/models/Claude.jpeg)

Claude Claude-4.5-Opus

已完成生成

```c
    charge_ctrl_set_callbacks(&g_ctrl, cb_set_current, cb_get_temp,
                              cb_get_voltage, cb_get_soc, cb_is_charger,
                              cb_state_change);
    charge_ctrl_start(&g_ctrl);
    
    print_table_header();
    
    float dt = 1.0f;
    float temp_target = 50.0f;
    int phase = 0;
    
    for (float t = 0; t < 400; t += dt) {
        /* 温度控制 */
        if (phase == 0 && t > 10) {
            temp_target = 65.0f;  /* 升温到过热 */
            phase = 1;
            printf("│        │ >>> 开始升温到65°C (触发过热保护)                       │\n");
        }
        if (phase == 1 && g_sim.temp > 64) {
            temp_target = 50.0f;  /* 降温 */
            phase = 2;
            printf("│        │ >>> 开始降温到50°C                                      │\n");
        }
        
        /* 温度模拟 */
        if (g_sim.temp < temp_target) {
            g_sim.temp += 0.4f;
        } else if (g_sim.temp > temp_target) {
            g_sim.temp -= 0.25f;
        }
        
        charge_ctrl_process(&g_ctrl, (uint32_t)(dt * 1000));
        sim_step(dt);
        
        if ((int)t % 5 == 0) {
            print_table_row(t);
        }
    }
    
    print_table_footer();
    print_hysteresis_status();
}

/**
 * @brief 测试3: 低温回滞测试 (0°C/5°C边界)
 */
void test_cold_hysteresis(void)
{
    print_header("测试3: 低温回滞测试 (进入<0°C, 退出>5°C)");
    
    printf("场景: 温度从10°C降到-5°C，再升回10°C\n");
    printf("预期: 低于0°C停止充电，必须升到5°C以上才恢复\n\n");
    
    sim_init(-10.0f, 10.0f, 40.0f);  /* 环境温度-10°C */
    
    charge_ctrl_init(&g_ctrl);
    charge_ctrl_set_callbacks(&g_ctrl, cb_set_current, cb_get_temp,
                              cb_get_voltage, cb_get_soc, cb_is_charger,
                              cb_state_change);
    charge_ctrl_start(&g_ctrl);
    
    print_table_header();
    
    float dt = 1.0f;
    float temp_target = 10.0f;
    int phase = 0;
    
    for (float t = 0; t < 400; t += dt) {
        /* 温度控制 */
        if (phase == 0 && t > 10) {
            temp_target = -5.0f;  /* 降温到过冷 */
            phase = 1;
            printf("│        │ >>> 开始降温到-5°C (触发过冷保护)                       │\n");
        }
        if (phase == 1 && g_sim.temp < -4) {
            temp_target = 10.0f;  /* 升温 */
            phase = 2;
            printf("│        │ >>> 开始升温到10°C                                      │\n");
        }
        
        /* 温度模拟 */
        if (g_sim.temp < temp_target) {
            g_sim.temp += 0.2f;
        } else if (g_sim.temp > temp_target) {
            g_sim.temp -= 0.3f;
        }
        
        charge_ctrl_process(&g_ctrl, (uint32_t)(dt * 1000));
        sim_step(dt);
        
        if ((int)t % 5 == 0) {
            print_table_row(t);
        }
    }
    
    print_table_footer();
    print_hysteresis_status();
}

/**
 * @brief 测试4: 边界抖动测试
 */
void test_boundary_jitter(void)
{
    print_header("测试4: 边界抖动测试 (温度在45°C附近波动)");
    
    printf("场景: 温度在43°C~47°C之间反复波动\n");
    printf("预期: 回滞机制防止状态频繁切换\n\n");
    
    sim_init(25.0f, 43.0f, 50.0f);
    
    charge_ctrl_init(&g_ctrl);
    charge_ctrl_set_callbacks(&g_ctrl, cb_set_current, cb_get_temp,
                              cb_get_voltage, cb_get_soc, cb_is_charger,
                              cb_state_change);
    charge_ctrl_start(&g_ctrl);
    
    print_table_header();
    
    float dt = 1.0f;
    int state_changes = 0;
    charge_state_t last_state = g_ctrl.state;
    
    for (float t = 0; t < 200; t += dt) {
        /* 模拟温度在45°C附近波动 */
        g_sim.temp = 45.0f + 2.0f * sinf(t * 0.1f);
        
        charge_ctrl_process(&g_ctrl, (uint32_t)(dt * 1000));
        sim_step(dt);
        
        /* 统计状态变化次数 */
        if (g_ctrl.state != last_state) {
            state_changes++;
            last_state = g_ctrl.state;
        }
        
        if ((int)t % 10 == 0) {
            print_table_row(t);
        }
    }
    
    print_table_footer();
    printf("\n状态切换次数: %d (回滞机制应该减少切换)\n", state_changes);
    print_hysteresis_status();
}

/**
 * @brief 测试5: 完整充电周期 (加热后充电)
 */
void test_full_charge_cycle(void)
{
    print_header("测试5: 完整充电周期 (加热后高温启动)");
    
    printf("场景: 加热结束后电池温度55°C，环境25°C，充电至满\n");
    printf("预期: 系统自动降流，温度降低后恢复正常充电\n\n");
    
    sim_init(25.0f, 55.0f, 10.0f);
    
    charge_ctrl_init(&g_ctrl);
    charge_ctrl_set_callbacks(&g_ctrl, cb_set_current, cb_get_temp,
                              cb_get_voltage, cb_get_soc, cb_is_charger,
                              cb_state_change);
    
    /* 通知加热已停止 */
    charge_ctrl_heating_stopped(&g_ctrl);
    charge_ctrl_start(&g_ctrl);
    
    print_table_header();
    
    float dt = 1.0f;
    float max_temp = 0;
    float time_above_45 = 0;
    
    for (float t = 0; t < 1800 && g_sim.soc < 100; t += dt) {
        charge_ctrl_process(&g_ctrl, (uint32_t)(dt * 1000));
        sim_step(dt);
        
        /* 统计 */
        if (g_sim.temp > max_temp) max_temp = g_sim.temp;
        if (g_sim.temp > 45.0f) time_above_45 += dt;
        
        if ((int)t % 30 == 0) {
            print_table_row(t);
        }
    }
    
    print_table_footer();
    
    printf("\n充电统计:\n");
    printf("  最终SOC: %.1f%%\n", g_sim.soc);
    printf("  最高温度: %.1f°C\n", max_temp);
    printf("  高温时间(>45°C): %.0f秒\n", time_above_45);
    printf("  总充电时间: %.1f秒\n", g_ctrl.total_charge_time_ms / 1000.0f);
}

/**
 * @brief 测试6: 各温度区间电流验证
 */
void test_zone_currents(void)
{
    print_header("测试6: 各温度区间电流验证");
    
    printf("验证不同温度区间的充电电流设置\n\n");
    
    typedef struct {
        float temp;
        const char* desc;
    } test_point_t;
    
    test_point_t points[] = {
        {-5.0f,  "过冷区 (T<0°C)"},
        {3.0f,   "低温区 (0~10°C) 下部"},
        {8.0f,   "低温区 (0~10°C) 上部"},
        {25.0f,  "正常区 (11~45°C) 中部"},
        {43.0f,  "正常区 (11~45°C) 上部"},
        {48.0f,  "高温区 (46~60°C) 下部"},
        {55.0f,  "高温区 (46~60°C) 上部"},
        {62.0f,  "过热区 (T>60°C)"},
    };
    
    int n = sizeof(points) / sizeof(points[0]);
    
    printf("┌────────┬──────────────────────────────┬──────────┬────────┐\n");
    printf("│ 温度   │ 区间描述                     │ 温度区间 │ 电流   │\n");
    printf("├────────┼──────────────────────────────┼──────────┼────────┤\n");
    
    for (int i = 0; i < n; i++) {
        sim_init(25.0f, points[i].temp, 50.0f);
        
        charge_ctrl_init(&g_ctrl);
        charge_ctrl_set_callbacks(&g_ctrl, cb_set_current, cb_get_temp,
                                  cb_get_voltage, cb_get_soc, cb_is_charger,
                                  NULL);
        charge_ctrl_start(&g_ctrl);
        
        /* 运行几个周期让状态稳定 */
        for (int j = 0; j < 10; j++) {
            charge_ctrl_process(&g_ctrl, 1000);
        }
        
        printf("│ %5.1f°C│ %-28s │ %-8s │ %4dmA │\n",
               points[i].temp,
               points[i].desc,
               charge_ctrl_zone_str(g_ctrl.temp.zone),
               g_ctrl.current_target);
    }
    
    printf("└────────┴──────────────────────────────┴──────────┴────────┘\n");
}

/**
 * @brief 测试7: 回滞边界精确测试
 */
void test_hysteresis_boundaries(void)
{
    print_header("测试7: 回滞边界精确测试");
    
    printf("测试各回滞边界的精确触发点\n\n");
    
    /* 测试高温回滞 */
    printf("【高温回滞测试】进入>45°C, 退出<40°C\n");
    printf("─────────────────────────────────────────────────\n");
    
    sim_init(25.0f, 44.0f, 50.0f);
    charge_ctrl_init(&g_ctrl);
    charge_ctrl_set_callbacks(&g_ctrl, cb_set_current, cb_get_temp,
                              cb_get_voltage, cb_get_soc, cb_is_charger, NULL);
    charge_ctrl_start(&g_ctrl);
    
    /* 稳定在44°C */
    for (int i = 0; i < 5; i++) {
        charge_ctrl_process(&g_ctrl, 1000);
    }
    printf("温度44°C: Zone=%s, in_high=%d\n", 
           charge_ctrl_zone_str(g_ctrl.temp.zone),
           g_ctrl.temp.hyst.in_high_temp);
    
    /* 升到45°C */
    g_sim.temp = 45.0f;
    for (int i = 0; i < 5; i++) {
        charge_ctrl_process(&g_ctrl, 1000);
    }
    printf("温度45°C: Zone=%s, in_high=%d\n", 
           charge_ctrl_zone_str(g_ctrl.temp.zone),
           g_ctrl.temp.hyst.in_high_temp);
    
    /* 升到46°C (应该进入高温) */
    g_sim.temp = 46.0f;
    for (int i = 0; i < 5; i++) {
        charge_ctrl_process(&g_ctrl, 1000);
    }
    printf("温度46°C: Zone=%s, in_high=%d ← 应进入高温区\n", 
           charge_ctrl_zone_str(g_ctrl.temp.zone),
           g_ctrl.temp.hyst.in_high_temp);
    
    /* 降到41°C (应该还在高温) */
    g_sim.temp = 41.0f;
    for (int i = 0; i < 5; i++) {
        charge_ctrl_process(&g_ctrl, 1000);
    }
    printf("温度41°C: Zone=%s, in_high=%d ← 回滞中，仍在高温区\n", 
           charge_ctrl_zone_str(g_ctrl.temp.zone),
           g_ctrl.temp.hyst.in_high_temp);
    
    /* 降到39°C (应该退出高温) */
    g_sim.temp = 39.0f;
    for (int i = 0; i < 5; i++) {
        charge_ctrl_process(&g_ctrl, 1000);
    }
    printf("温度39°C: Zone=%s, in_high=%d ← 应退出高温区\n", 
           charge_ctrl_zone_str(g_ctrl.temp.zone),
           g_ctrl.temp.hyst.in_high_temp);
    
    /* 测试过热回滞 */
    printf("\n【过热回滞测试】进入>60°C, 退出<55°C\n");
    printf("─────────────────────────────────────────────────\n");
    
    sim_init(25.0f, 59.0f, 50.0f);
    charge_ctrl_init(&g_ctrl);
    charge_ctrl_set_callbacks(&g_ctrl, cb_set_current, cb_get_temp,
                              cb_get_voltage, cb_get_soc, cb_is_charger, NULL);
    charge_ctrl_start(&g_ctrl);
    
    for (int i = 0; i < 5; i++) {
        charge_ctrl_process(&g_ctrl, 1000);
    }
    printf("温度59°C: Zone=%s, in_overheat=%d\n", 
           charge_ctrl_zone_str(g_ctrl.temp.zone),
           g_ctrl.temp.hyst.in_overheat);
    
    g_sim.temp = 61.0f;
    for (int i = 0; i < 5; i++) {
        charge_ctrl_process(&g_ctrl, 1000);
    }
    printf("温度61°C: Zone=%s, in_overheat=%d ← 应进入过热区\n", 
           charge_ctrl_zone_str(g_ctrl.temp.zone),
           g_ctrl.temp.hyst.in_overheat);
    
    g_sim.temp = 56.0f;
    for (int i = 0; i < 5; i++) {
        charge_ctrl_process(&g_ctrl, 1000);
    }
    printf("温度56°C: Zone=%s, in_overheat=%d ← 回滞中，仍在过热区\n", 
           charge_ctrl_zone_str(g_ctrl.temp.zone),
           g_ctrl.temp.hyst.in_overheat);
    
    g_sim.temp = 54.0f;
    for (int i = 0; i < 5; i++) {
        charge_ctrl_process(&g_ctrl, 1000);
    }
    printf("温度54°C: Zone=%s, in_overheat=%d ← 应退出过热区\n", 
           charge_ctrl_zone_str(g_ctrl.temp.zone),
           g_ctrl.temp.hyst.in_overheat);
    
    /* 测试过冷回滞 */
    printf("\n【过冷回滞测试】进入<0°C, 退出>5°C\n");
    printf("─────────────────────────────────────────────────\n");
    
    sim_init(-10.0f, 2.0f, 50.0f);
    charge_ctrl_init(&g_ctrl);
    charge_ctrl_set_callbacks(&g_ctrl, cb_set_current, cb_get_temp,
                              cb_get_voltage, cb_get_soc, cb_is_charger, NULL);
    charge_ctrl_start(&g_ctrl);
    
    for (int i = 0; i < 5; i++) {
        charge_ctrl_process(&g_ctrl, 1000);
    }
    printf("温度 2°C: Zone=%s, in_overcold=%d\n", 
           charge_ctrl_zone_str(g_ctrl.temp.zone),
           g_ctrl.temp.hyst.in_overcold);
    
    g_sim.temp = -1.0f;
    for (int i = 0; i < 5; i++) {
        charge_ctrl_process(&g_ctrl, 1000);
    }
    printf("温度-1°C: Zone=%s, in_overcold=%d ← 应进入过冷区\n", 
           charge_ctrl_zone_str(g_ctrl.temp.zone),
           g_ctrl.temp.hyst.in_overcold);
    
    g_sim.temp = 3.0f;
    for (int i = 0; i < 5; i++) {
        charge_ctrl_process(&g_ctrl, 1000);
    }
    printf("温度 3°C: Zone=%s, in_overcold=%d ← 回滞中，仍在过冷区\n", 
           charge_ctrl_zone_str(g_ctrl.temp.zone),
           g_ctrl.temp.hyst.in_overcold);
    
    g_sim.temp = 6.0f;
    for (int i = 0; i < 5; i++) {
        charge_ctrl_process(&g_ctrl, 1000);
    }
    printf("温度 6°C: Zone=%s, in_overcold=%d ← 应退出过冷区\n", 
           charge_ctrl_zone_str(g_ctrl.temp.zone),
           g_ctrl.temp.hyst.in_overcold);
}

/**
 * @brief 测试8: 温度曲线可视化
 */
void test_temp_curve_visual(void)
{
    print_header("测试8: 温度与电流曲线可视化");
    
    printf("场景: 加热后55°C启动充电，观察温度和电流变化\n\n");
    
    sim_init(25.0f, 55.0f, 20.0f);
    
    charge_ctrl_init(&g_ctrl);
    charge_ctrl_set_callbacks(&g_ctrl, cb_set_current, cb_get_temp,
                              cb_get_voltage, cb_get_soc, cb_is_charger, NULL);
    charge_ctrl_heating_stopped(&g_ctrl);
    charge_ctrl_start(&g_ctrl);
    
    /* 记录数据 */
    #define DATA_POINTS 60
    float temp_data[DATA_POINTS];
    float curr_data[DATA_POINTS];
    int idx = 0;
    
    float dt = 1.0f;
    for (float t = 0; t < 600 && idx < DATA_POINTS; t += dt) {
        charge_ctrl_process(&g_ctrl, (uint32_t)(dt * 1000));
        sim_step(dt);
        
        if ((int)t % 10 == 0) {
            temp_data[idx] = g_sim.temp;
            curr_data[idx] = g_sim.current;
            idx++;
        }
    }
    
    /* 绘制温度曲线 */
    printf("温度曲线 (°C):\n");
    printf("     │\n");
    
    for (int row = 6; row >= 0; row--) {
        float t_low = 20.0f + row * 7.0f;
        float t_high = t_low + 7.0f;
        
        printf("%4.0f │", t_low + 3.5f);
        
        for (int col = 0; col < idx; col++) {
            if (temp_data[col] >= t_low && temp_data[col] < t_high) {
                printf("●");
            } else if (temp_data[col] >= t_high) {
                printf("│");
            } else {
                printf(" ");
            }
        }
        
        /* 标注关键温度线 */
        if (row == 4) printf(" ← 45°C (高温阈值)");
        if (row == 3) printf(" ← 40°C (高温回滞)");
        
        printf("\n");
    }
    
    printf("     └");
    for (int i = 0; i < idx; i++) printf("─");
    printf("\n");
    printf("      0        100       200       300       400       500  时间(s)\n");
    
    /* 绘制电流曲线 */
    printf("\n电流曲线 (mA):\n");
    printf("     │\n");
    
    for (int row = 4; row >= 0; row--) {
        float c_low = row * 500.0f;
        float c_high = c_low + 500.0f;
        
        printf("%4.0f │", c_low + 250.0f);
        
        for (int col = 0; col < idx; col++) {
            if (curr_data[col] >= c_low && curr_data[col] < c_high) {
                printf("■");
            } else if (curr_data[col] >= c_high) {
                printf("│");
            } else {
                printf(" ");
            }
        }
        printf("\n");
    }
    
    printf("     └");
    for (int i = 0; i < idx; i++) printf("─");
    printf("\n");
    printf("      0        100       200       300       400       500  时间(s)\n");
}

/**
 * @brief 测试9: 诊断信息输出
 */
void test_diagnostics(void)
{
    print_header("测试9: 诊断信息输出");
    
    sim_init(25.0f, 48.0f, 60.0f);
    
    charge_ctrl_init(&g_ctrl);
    charge_ctrl_set_callbacks(&g_ctrl, cb_set_current, cb_get_temp,
                              cb_get_voltage, cb_get_soc, cb_is_charger, NULL);
    charge_ctrl_start(&g_ctrl);
    
    /* 运行一段时间 */
    for (int i = 0; i < 30; i++) {
        charge_ctrl_process(&g_ctrl, 1000);
        sim_step(1.0f);
    }
    
    char buf[512];
    charge_ctrl_get_info(&g_ctrl, buf, sizeof(buf));
    
    printf("系统诊断信息:\n");
    printf("─────────────────────────────────────────────────\n");
    printf("%s", buf);
    printf("─────────────────────────────────────────────────\n");
}

/*============================================================================
 *                              主函数
 *===========================================================================*/

/* 需要math.h中的sinf函数 */
#include <math.h>

int main(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════╗\n");
    printf("║           便携式加热设备 - 智能充电温控系统 V2.0                     ║\n");
    printf("║                                                                      ║\n");
    printf("║  温度区间:                                                           ║\n");
    printf("║    • 过冷区: T < 0°C   (回滞: 5°C)   - 禁止充电                      ║\n");
    printf("║    • 低温区: 0~10°C                  - 小电流充电                    ║\n");
    printf("║    • 正常区: 11~45°C                 - 正常充电                      ║\n");
    printf("║    • 高温区: 46~60°C  (回滞: 40°C)  - 降流充电                      ║\n");
    printf("║    • 过热区: T > 60°C (回滞: 55°C)  - 禁止充电                      ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════╝\n");
    
    /* 运行所有测试 */
    test_high_temp_hysteresis();
    test_overheat_hysteresis();
    test_cold_hysteresis();
    test_boundary_jitter();
    test_full_charge_cycle();
    test_zone_currents();
    test_hysteresis_boundaries();
    test_temp_curve_visual();
    test_diagnostics();
    
    printf("\n");
    printf("══════════════════════════════════════════════════════════════════════\n");
    printf("                           所有测试完成\n");
    printf("══════════════════════════════════════════════════════════════════════\n\n");
    
    return 0;
}
```

------

## 📊 回滞逻辑详解

### 状态转换图

```text
                            温度上升 →
    ════════════════════════════════════════════════════════════════
    
         过冷区          低温区         正常区         高温区        过热区
        (禁止)         (小电流)       (正常)        (降流)        (禁止)
    ┌──────────┐    ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐
    │  T < 0°C │    │ 0~10°C   │   │ 11~45°C  │   │ 46~60°C  │   │ T > 60°C │
    │          │    │          │   │          │   │          │   │          │
    │ 电流: 0  │    │电流:小   │   │电流:正常 │   │电流:降流 │   │ 电流: 0  │
    └──────────┘    └──────────┘   └──────────┘   └──────────┘   └──────────┘
         │              │              │              │              │
         │              │              │              │              │
         │    进入      │              │    进入      │    进入      │
         │◄───T<0°C─────┤              │◄──T>45°C────┤◄──T>60°C────┤
         │              │              │              │              │
         ├───T>5°C─────►│              ├───T<40°C───►│───T<55°C───►│
         │    退出      │              │    退出      │    退出      │
         │              │              │              │              │
    
    ════════════════════════════════════════════════════════════════
                            ← 温度下降
```

### 回滞工作示例

```text
温度变化: 35°C → 50°C → 38°C → 50°C → 35°C

时间轴:
─────────────────────────────────────────────────────────────────────►

温度:    35    40    45    50    45    40    38    45    50    40    35
         │     │     │     │     │     │     │     │     │     │     │
状态:  正常  正常  正常  高温  高温  高温  正常  正常  高温  高温  正常
                      ↑                 ↑                       ↑
                   T>45°C            T<40°C                   T<40°C
                   进入高温          退出高温                 退出高温

关键点:
1. 温度升到46°C时进入高温区
2. 温度降到41°C时仍在高温区 (回滞保护)
3. 温度降到39°C时才退出高温区
4. 温度再次升到46°C时重新进入高温区
```

------

## 🔧 编译与运行

```bash
# 编译 (需要math库)
gcc -o charge_test charge_thermal_ctrl.c charge_thermal_test.c -lm -Wall -O2

# 运行
./charge_test
```

### Makefile

```makefile
CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lm

TARGET = charge_test
SRCS = charge_thermal_ctrl.c charge_thermal_test.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
```

------

## 📋 系统参数总结

|     参数     |   值    |           说明           |
| :----------: | :-----: | :----------------------: |
| **过冷阈值** |   0°C   |    低于此温度禁止充电    |
| **过冷回滞** |   5°C   |   需升至5°C以上才恢复    |
|  **低温区**  | 0~10°C  |  小电流充电 (200~800mA)  |
|  **正常区**  | 11~45°C |  正常充电 (最大2000mA)   |
| **高温阈值** |  46°C   |    高于此温度开始降流    |
| **高温回滞** |  40°C   | 需降至40°C以下才恢复正常 |
| **过热阈值** |  60°C   |    高于此温度停止充电    |
| **过热回滞** |  55°C   |   需降至55°C以下才恢复   |
| **PID目标**  |  42°C   |    高温区PID控制目标     |